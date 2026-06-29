#define F_CPU 32000000UL //2Mhz unsigned long

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "adc_light.h"
#include "uart.h"
#include "debug.h"

/*
Need some bounds for the ADC based off AVR and real life values
Lower bound for ADC
higher bound for direct sunlight (saturation)
*/


#define ADC_MIN		10		//Noise floor for ADC given 0 Photodiode outout
#define ADC_MAX		4090	//Saturation of the AVR ADC
#define LUX_MAX		1000	//Saturation of direct sunlight


#define BSEL 207

//Flag to mark end of ADC and ready to send via UART
//Reset after UART TX
volatile uint8_t send_flag = 0; 

//Swtich to the 32Mhz clock from the initial 2Mhz default
void clock_init(void)
{
	OSC.CTRL |= OSC_RC32MEN_bm;             // 1. enable the 32MHz oscillator
	while (!(OSC.STATUS & OSC_RC32MRDY_bm)); // 2. wait until it's stable
	CCP = CCP_IOREG_gc;                      // 3. unlock protected register
	CLK.CTRL = CLK_SCLKSEL_RC32M_gc;        // 4. switch NOW (must immediately follow CCP)
}

// TCC0 Overflow ISR -------------------------------
ISR(TCC0_OVF_vect)
{
	//Toggles UART. Volatile keeps it new at every call
	send_flag = 1;
}

//LED ------------------------------------------------

void led_init(void)
{
	PORTR.DIRSET = PIN0_bm; //Set Pin as output
	PORTR.OUTCLR = PIN0_bm; //Sets the pin LOW. Atomic behavior
}

void led_toggle(void)
{
	PORTR.OUTTGL = PIN0_bm; //Toggles pin. Specific behavior
}


//Timer --------------------------------------------------

void timer_init()
{
	TCC0.PER = 31249; //Period set by dividing this from 2Mhz
	TCC0.CNT = 0; //Initial value
	
	TCC0.INTCTRLA =	TC_OVFINTLVL_LO_gc; // Set Overflow level to low
	TCC0.CTRLA = TC_CLKSEL_DIV1024_gc; //Clock selection divide by 1024
	//Since the lock is 32Mhz we need to divide 1024 to not overflow the 16 bit
	//TCC0 which maxes out at 65535
	
	//2Mhz divided by 256 = 7812 per second. Times 2 for 1 sec is 15625
	
	PMIC_CTRL |= PMIC_LOLVLEN_bm; //Enable low level interrupts through the controller
	sei(); //Enables interrupts globally
}

//Main ------------------------------------------

int main(void)
{
	
	
	clock_init(); //Use 32Mhz clock
	debug_init(); //output the clock to P7
	uart_init(); //9600 baud rate, 8 bit no parity
	uart_send_string(
	"----------------------------------\r\n"
	"ADC Light Meter\r\n"
	"Created on 6/25/26 \r\n"
	"Shows RAW ADC Value and Light Value of	the Room\r\n"
	"----------------------------------\r\n"
	);
	led_init(); //Toggle every 1 sec
	timer_init(); //TCC0 overflow 
	adc_init(); //Enable ADC channel PA0 light sensor
	
	char buf[32]; //Array holds the formatted string from ADC
	
	while (1)
	{
		if (send_flag)
		{
			send_flag = 0; //reset ADC pull
			
			uint16_t raw = adc_read_avg(16); //save average to 16bit
			uint16_t lux = adc_to_lux(raw); //Takes in 16bit output as well
			
			//Send the lux value over UART within bounds
			
			if (raw < ADC_MIN)
			{
				uart_send_string("ERR: sensor low/disconnected\r\n");
			} 
			else if (raw > ADC_MAX)
			{
				uart_send_string("ERR: Saturated Diode\r\n");
			}
			else
			{
				//sprintnf ---> never writes past buffer
				sprintf(buf, "RAW: %u  LUX: %u\r\n", raw, lux);
				uart_send_string(buf);
			}
			
			
			//Show data sent TX LED
			led_toggle(); 
		}
	}
	
}