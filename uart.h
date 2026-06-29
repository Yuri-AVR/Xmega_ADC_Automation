/*
 * uart.h
 *
 * Created: 6/25/2026 9:32:37 AM
 *  Author: Yuri
 */ 


#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include <stdint.h>

// Config -----------------------------------------

//Change these after import to match baud rate needed

#define F_CPU	32000000UL
#define BSEL	207 //From (FCPU/(16*Baud)-1)

// Function Prototypes

void uart_init(void); //set the initial mode and controls
void uart_send_char(char c); //single character
void uart_send_string(const char *str); //send a string, using a pointer 
void uart_send_int(int16_t val); //Send 16 bit signed value, not string
void uart_send_uint(uint16_t val); //Send unsigned 16 bit value 
void uart_send_float(float val, uint8_t decimals); //Send Float as cast float and decimals afterwards



#endif /* UART_H_ */