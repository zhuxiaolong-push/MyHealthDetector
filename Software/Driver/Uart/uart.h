#ifndef __UART_H
#define __UART_H

#include "stm32f4xx.h"

// 串口号枚举
typedef enum {
    UART1_id = 1,
    UART3_id = 3,
    UART4_id = 4,
} UART_ID;

// 初始化指定串口，波特率由参数指定
void uart_init(UART_ID uart, uint32_t baudrate);

// 向指定串口发送一个字节
void uart_send(USART_TypeDef* USARTx, uint8_t data);

// 向指定串口发送一个字节
void uart_send(USART_TypeDef* USARTx, uint8_t data);

// 发送字符串（以 '\0' 结尾）
void uart_send_string(USART_TypeDef* USARTx, const char* str);

// 接收一个字节（阻塞，直到收到数据）
uint8_t uart_receive_byte(USART_TypeDef* USARTx);

// 接收一行字符串，直到遇到换行符 '\n' 或达到最大长度
// 返回实际接收到的字符数（不包括结尾的 '\0'）
uint16_t uart_receive_line(USART_TypeDef* USARTx, char* buffer, uint16_t max_len);

#endif
