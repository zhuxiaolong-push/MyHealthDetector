#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f4xx.h"

// 错误码定义
typedef enum {
    ESP_OK = 0,
    ESP_ERROR = 1,
    ESP_TIMEOUT = 2,
    ESP_NO_AP = 3,
    ESP_NO_IP = 4,
    ESP_BUSY = 5
} ESP_Status;

// 初始化 ESP8266（必须事先初始化好 USART 外设）
ESP_Status ESP8266_Init(USART_TypeDef* USARTx, uint32_t baudrate);

// 连接 WiFi（阻塞，直到成功或超时）
ESP_Status ESP8266_ConnectWiFi(char *ssid, char *password);

// 检查当前 WiFi 连接状态
ESP_Status ESP8266_CheckWiFiStatus(void);

// 获取本地 IP 地址（字符串，需提供足够缓冲区，至少 16 字节）
ESP_Status ESP8266_GetLocalIP(char *ip_buffer);

// 断开当前连接
ESP_Status ESP8266_DisconnectWiFi(void);

// 发送 AT 指令并等待预期响应（内部使用）
ESP_Status ESP8266_SendCmd(char *cmd, char *expected_response, uint32_t timeout_ms);

#endif
