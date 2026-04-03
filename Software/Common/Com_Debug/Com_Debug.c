#include "Com_Debug.h"

void Com_Debug_Init(void)
{
	uart_init(UART1_id, 115200);
    Debug_sendln("***********************************************************************\r\n \
                The STM32 health detector program has been activated and can now be debugged,\r\n \
                Please wait for the initialization to complete.\r\n \
            ***********************************************************************\r\n");
}

int fputc(int ch,FILE *file)
{
    uart_send(USART1, (uint8_t)ch);
    return ch;
}
