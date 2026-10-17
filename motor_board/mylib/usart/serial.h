#ifndef SERIAL
#define SERIAL
#include "usart.h"
#include "pwm.h"
#define serial_rx_buffer_size  64
typedef struct {
    uint8_t  data[serial_rx_buffer_size];
    uint16_t len;
} serial_Msg_t;
void UART1_IT(void);
void data_swap(void*data,int len,int width);
void serial_init();
void serial_parse_data(uint8_t*data,int len);
extern serial_Msg_t uart1_msg;
extern int uart1_it_flag;
#endif