/*
 * UART.c
 *
 * Created: 6/25/2026 9:30:41 AM
 * Author : Yuri
 */ 

#include "uart.h"
#include <stdio.h> //for the sprint float values
#include <avr/io.h>


void uart_init(void)
{
	PORTC.DIRSET = PIN3_bm;
	//TXD on PC3
	
	//Baud rate
	USARTC0.BAUDCTRLA = BSEL & 0XFF;
	USARTC0.BAUDCTRLB = (BSEL >> 8) & 0X0F;
	
	//8 Data bits, no parity, 1 stop bit, asynch
	USARTC0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc;
	
	//Enable TX
	USARTC0.CTRLB = USART_TXEN_bm; 	
}

// SEND FUNCTIONS--------------------------------------------------

void uart_send_char(char c)
{
	while(!(USARTC0.STATUS & USART_DREIF_bm));
	USARTC0.DATA = c;
}


void uart_send_string(const char *str)
{
	while(*str)
	{
		uart_send_char(*str++);
	}
}

//Numeric UART

void uart_send_int(int16_t val)
{
	char buf[8]; //32768 is the longest int you can send 
	sprintf(buf, "%d", val); //send the val in dec format through string
	uart_send_string(buf);
}

void uart_send_uint(uint16_t val)
{
	char buf[7]; //65535 is the longest you can send unsigned
	sprintf(buf, "%u", val); //format to string with unsigned 
	uart_send_string(buf);
}

void uart_send_float(float val, uint8_t decimals)
{
	char buf[16]; //Longest for most float 
	switch (decimals)
	{
		case 0: sprintf(buf, "%.0f", val); break;
		case 1: sprintf(buf, "%.1f", val); break;
		case 2: sprintf(buf, "%.2f", val); break;
		case 3: sprintf(buf, "%.3f", val); break;
		default: sprintf(buf, "%.2f", val); break;
	}
	uart_send_string(buf);
}

/*
float function for passed decimals instead of cast ones

void uart_send_float(float val, uint8_t decimals)
{
	char buf[16];
	sprintf(buf, "%.*f", decimals, val);
	uart_send_string(buf);
}

*/

