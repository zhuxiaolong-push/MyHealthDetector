#include "esp8266.h"
#include <string.h>
#include <stdio.h>

// 全局变量：当前使用的 USART 外设
static USART_TypeDef* g_usart = NULL;

// 简单延时函数（基于循环，非精确，但够用）
static void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms * 1000; i++) {
        __NOP();
    }
}

// 发送单个字符（阻塞）
static void UART_SendChar(uint8_t ch) {
    while (USART_GetFlagStatus(g_usart, USART_FLAG_TXE) == RESET);
    USART_SendData(g_usart, ch);
}

// 发送字符串
static void UART_SendString(char *str) {
    while (*str) {
        UART_SendChar(*str++);
    }
}

// 接收单个字符（带超时，返回0表示超时）
static uint8_t UART_ReceiveChar(uint8_t *ch, uint32_t timeout_ms) {
    uint32_t start = 0;
    // 简单循环计数作为超时（取决于系统主频，粗略）
    for (start = 0; start < timeout_ms * 1000; start++) {
        if (USART_GetFlagStatus(g_usart, USART_FLAG_RXNE) != RESET) {
            *ch = USART_ReceiveData(g_usart);
            return 1;
        }
    }
    return 0;
}

// 接收一行数据（以 \r\n 结尾），存入 buffer，返回接收字节数（不含\r\n）
static uint16_t UART_ReceiveLine(char *buffer, uint16_t max_len, uint32_t timeout_ms) {
    uint16_t index = 0;
    uint32_t start = 0;
    // 超时计数：简单循环
    for (start = 0; start < timeout_ms * 1000; start++) {
        uint8_t ch;
        if (UART_ReceiveChar(&ch, 1)) {
            if (ch == '\n') {
                if (index > 0 && buffer[index-1] == '\r') {
                    buffer[index-1] = '\0';
                    return index-1;
                }
            } else if (index < max_len - 1) {
                buffer[index++] = ch;
            }
        }
    }
    return 0; // 超时
}

// 发送 AT 指令并等待预期响应
ESP_Status ESP8266_SendCmd(char *cmd, char *expected_response, uint32_t timeout_ms) {
    char buf[128];
    // 发送指令
    UART_SendString(cmd);
    UART_SendString("\r\n");
    
    uint32_t start = 0;
    for (start = 0; start < timeout_ms * 1000; start++) {
        if (UART_ReceiveLine(buf, sizeof(buf), 100) > 0) {
            // 检查是否收到期望的响应
            if (strstr(buf, expected_response) != NULL) {
                return ESP_OK;
            }
            // 检查错误响应
            if (strstr(buf, "ERROR") != NULL) {
                return ESP_ERROR;
            }
            // 检查忙状态
            if (strstr(buf, "busy") != NULL || strstr(buf, "BUSY") != NULL) {
                return ESP_BUSY;
            }
        }
    }
    return ESP_TIMEOUT;
}

// 初始化 ESP8266
ESP_Status ESP8266_Init(USART_TypeDef* USARTx, uint32_t baudrate) {
    g_usart = USARTx;
    // 假设 USART 已在外部初始化，这里只进行通信测试
    // 发送 AT 测试指令
    if (ESP8266_SendCmd("AT", "OK", 2000) != ESP_OK) {
        return ESP_ERROR;
    }
    // 关闭回显
    ESP8266_SendCmd("ATE0", "OK", 1000);
    // 设置为 STA 模式
    if (ESP8266_SendCmd("AT+CWMODE=1", "OK", 1000) != ESP_OK) {
        return ESP_ERROR;
    }
    return ESP_OK;
}

// 连接 WiFi
ESP_Status ESP8266_ConnectWiFi(char *ssid, char *password) {
    char cmd[128];
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", ssid, password);
    
    // 尝试连接，超时 15 秒（WiFi 连接可能较慢）
    if (ESP8266_SendCmd(cmd, "OK", 15000) != ESP_OK) {
        // 检查是否是因为已经连接
        if (ESP8266_CheckWiFiStatus() == ESP_OK) {
            return ESP_OK;
        }
        return ESP_NO_AP;
    }
    
    // 等待获取 IP（最多 10 秒）
    for (uint32_t i = 0; i < 20; i++) {
        char ip[16];
        if (ESP8266_GetLocalIP(ip) == ESP_OK) {
            return ESP_OK;
        }
        delay_ms(500);
    }
    return ESP_NO_IP;
}

// 检查当前连接状态
ESP_Status ESP8266_CheckWiFiStatus(void) {
    // 发送查询指令
    char buf[64];
    UART_SendString("AT+CWJAP?\r\n");
    
    for (uint32_t i = 0; i < 5000; i++) {
        if (UART_ReceiveLine(buf, sizeof(buf), 100) > 0) {
            if (strstr(buf, "+CWJAP:") != NULL) {
                // 已连接
                return ESP_OK;
            }
            if (strstr(buf, "No AP") != NULL) {
                return ESP_NO_AP;
            }
        }
    }
    return ESP_TIMEOUT;
}

// 获取本地 IP
ESP_Status ESP8266_GetLocalIP(char *ip_buffer) {
    char buf[64];
    UART_SendString("AT+CIFSR\r\n");
    
    for (uint32_t i = 0; i < 3000; i++) {
        if (UART_ReceiveLine(buf, sizeof(buf), 200) > 0) {
            // 解析 +CIFSR:STAIP,"192.168.1.100"
            char *p = strstr(buf, "+CIFSR:STAIP,\"");
            if (p) {
                p += strlen("+CIFSR:STAIP,\"");
                char *end = strchr(p, '\"');
                if (end) {
                    int len = end - p;
                    if (len < 16) {
                        memcpy(ip_buffer, p, len);
                        ip_buffer[len] = '\0';
                        return ESP_OK;
                    }
                }
            }
        }
    }
    return ESP_TIMEOUT;
}

// 断开 WiFi
ESP_Status ESP8266_DisconnectWiFi(void) {
    return ESP8266_SendCmd("AT+CWQAP", "OK", 3000);
}
