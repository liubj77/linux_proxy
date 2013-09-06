/***************************/
#ifndef UART_H
#define UART_H

#include <stdint.h>

int32_t 			open_uart(char *Dev);
int8_t 			config_uart(int32_t fd,int32_t baud_rate,int32_t data_bits,int32_t stop_bits,uint8_t parity);
//static int8_t   hdlc_q_byte(struct hdlc_buff_t* lm, uint8_t byte, uint16_t max_size);

#endif
