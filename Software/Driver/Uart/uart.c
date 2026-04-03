#include "uart.h"
#include <stddef.h>

// 串口配置结构体
typedef struct {
    USART_TypeDef *USARTx;          // 串口寄存器基地址
    uint32_t       USART_Clock;     // 串口时钟宏（如 RCC_APB2Periph_USART1）
    uint8_t        USART_Bus;       // 总线标识：1=APB1, 2=APB2
    GPIO_TypeDef  *GPIOx;            // GPIO 端口
    uint16_t       TX_Pin;           // TX 引脚号
    uint16_t       RX_Pin;           // RX 引脚号
    uint8_t        TX_Source;        // TX 引脚源（如 GPIO_PinSource9）
    uint8_t        RX_Source;        // RX 引脚源（如 GPIO_PinSource10）
    uint8_t        AF_Value;         // 复用功能值（如 GPIO_AF_USART1）
    uint32_t       GPIO_Clock;       // GPIO 时钟宏（如 RCC_AHB1Periph_GPIOA）
} UART_Config;

static const UART_Config uart_configs[] = {
    [UART1_id] = {      // 用于开发版上串口调试
        .USARTx       = USART1,
        .USART_Clock  = RCC_APB2Periph_USART1,
        .USART_Bus    = 2,
        .GPIOx        = GPIOA,
        .TX_Pin       = GPIO_Pin_9,
        .RX_Pin       = GPIO_Pin_10,
        .TX_Source    = GPIO_PinSource9,
        .RX_Source    = GPIO_PinSource10,
        .AF_Value     = GPIO_AF_USART1,
        .GPIO_Clock   = RCC_AHB1Periph_GPIOA
    },
    [UART3_id] = {     //用于底板上串口调试
        .USARTx       = USART3,
        .USART_Clock  = RCC_APB1Periph_USART3,
        .USART_Bus    = 1,
        .GPIOx        = GPIOD,
        .TX_Pin       = GPIO_Pin_8,
        .RX_Pin       = GPIO_Pin_9,
        .TX_Source    = GPIO_PinSource8,
        .RX_Source    = GPIO_PinSource9,
        .AF_Value     = GPIO_AF_USART3,
        .GPIO_Clock   = RCC_AHB1Periph_GPIOD
    },
    [UART4_id] = {    //用于ESP8266控制
        .USARTx       = UART4,
        .USART_Clock  = RCC_APB1Periph_UART4,
        .USART_Bus    = 1,
        .GPIOx        = GPIOA,
        .TX_Pin       = GPIO_Pin_0,
        .RX_Pin       = GPIO_Pin_1,
        .TX_Source    = GPIO_PinSource0,
        .RX_Source    = GPIO_PinSource1,
        .AF_Value     = GPIO_AF_UART4,
        .GPIO_Clock   = RCC_AHB1Periph_GPIOA
    }
};

// 获取数组元素个数（用于边界检查）
#define UART_CONFIG_COUNT  (sizeof(uart_configs) / sizeof(uart_configs[0]))

void uart_init(UART_ID uart, uint32_t baudrate)
{
    // 检查串口号是否有效
    if (uart < UART1_id || uart >= UART_CONFIG_COUNT) {
        return;  // 不支持的串口
    }

    const UART_Config *cfg = &uart_configs[uart];  // 获取对应配置

    // 1. 使能 GPIO 时钟
    RCC_AHB1PeriphClockCmd(cfg->GPIO_Clock, ENABLE);

    // 2. 使能 USART 时钟（根据总线类型调用不同函数）
    if (cfg->USART_Bus == 1) {
        RCC_APB1PeriphClockCmd(cfg->USART_Clock, ENABLE);
    } else {
        RCC_APB2PeriphClockCmd(cfg->USART_Clock, ENABLE);
    }

    // 3. 配置 TX 引脚
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin   = cfg->TX_Pin;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_UP;      // 上拉
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;     // 推挽输出
    GPIO_InitStruct.GPIO_Speed = GPIO_Fast_Speed;   // 速度 50MHz
    GPIO_Init(cfg->GPIOx, &GPIO_InitStruct);

    // 4. 配置 RX 引脚
    GPIO_InitStruct.GPIO_Pin = cfg->RX_Pin;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_UP;      // 上拉
    GPIO_InitStruct.GPIO_Speed = GPIO_Fast_Speed;   // 速度 50MHz
    GPIO_Init(cfg->GPIOx, &GPIO_InitStruct);   // 复用模式等与 TX 相同

    // 5. 引脚复用映射
    GPIO_PinAFConfig(cfg->GPIOx, cfg->TX_Source, cfg->AF_Value);
    GPIO_PinAFConfig(cfg->GPIOx, cfg->RX_Source, cfg->AF_Value);

    // 6. USART 参数配置
    USART_InitTypeDef USART_InitStruct;
    USART_InitStruct.USART_BaudRate   = baudrate;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits   = USART_StopBits_1;
    USART_InitStruct.USART_Parity     = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode       = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(cfg->USARTx, &USART_InitStruct);

    // 7. 使能 USART
    USART_Cmd(cfg->USARTx, ENABLE);
}

// 通用发送函数
void uart_send(USART_TypeDef* USARTx, uint8_t data)
{
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);
    // 等待发送数据寄存器空
    USART_SendData(USARTx, (uint16_t)data);

}

// 发送字符串
void uart_send_string(USART_TypeDef* USARTx, const char* str)
{
    while (*str)
    {
        uart_send(USARTx, (uint8_t)(*str++));
    }
}

// 接收一个字节
uint8_t uart_receive_byte(USART_TypeDef* USARTx)
{
    while (USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) == RESET);
    return (uint8_t)USART_ReceiveData(USARTx);
}
uint16_t uart_receive_line(USART_TypeDef* USARTx, char* buffer, uint16_t max_len)
{
    uint16_t count = 0;
    char c;

    if (max_len == 0) return 0;

    while (count < max_len - 1)
    {
        c = (char)uart_receive_byte(USARTx);
        if (c == '\n' || c == '\r')
        {
            break;
        }
        buffer[count++] = c;
    }
    buffer[count] = '\0';
    return count;
}
