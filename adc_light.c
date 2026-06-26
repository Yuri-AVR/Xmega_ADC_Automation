/*
 * adc_light.c
 *
 * Created: 6/25/2026 6:50:35 PM
 *  Author: Yuri
 
 TEMT6000X01 light sensor ADC driver
 
 Reference voltage Note
 
 Xmega can't use VCC directly to scale the input for the comparitor.
 The internal references are:
 
 ADC_REFSELF_INT1V uses a 1V bandgap
 ADC_REFSELF_INTVCC Uses VCC/1.6 = 2.06 from the 3.3V VCC rail
 ADC_REFSELF_INTVCC2 Uses VCC/2
 ADC_REFSELF_AREFA Uses external source Pin PA0 for AREFA
 ADC_RESELF_AREFB external pin AREFB on PB0. This open on Port J3 pin 0 for later
 
 Values
 Ic x R = 20 mA * 4.7 Kohm
 Living room voltages are between 0.3 and 0.7V
 
 ADC count formula with VERF = 2.0625
 Count = (Vin/2.0625) * 4095
 
 Lux estimates with 2uA/Lux
 Vin = Count * (2.0625/4095)
 Ic_uA = Vin/4700 * 1e6
 Lux = Ic_uA/2
 
 Lux = adc_count/4095/47/2
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>


/*------------Calibration byte helper -------------*/

static uint8_t read_cal_byte(uint8_t index)
{
	uint8_t result;
	NVM.CMD = NVM_CMD_READ_CALIB_ROW_gc;
	result = pgm_read_byte(index);
	NVM.CMD = NVM_CMD_NO_OPERATION_gc;
	return result;
	
}

void adc_init(void)
{
	//Load factory cal
	ADCA.CALL = read_cal_byte(PRODSIGNATURES_ADCACAL0);
	ADCA.CALH = read_cal_byte(PRODSIGNATURES_ADCACAL1);

	
	//flush the ADC register
	
	ADCA.CTRLA = ADC_FLUSH_bm;
	
	
	//ADC Control B
	
	// Need unsinged output and 12 bit resolution. Left aligned in the register array [11:0]
	ADCA.CTRLB = ADC_RESOLUTION_12BIT_gc; 
	//Conmode 0 doesnt need to be set? Default
	
	//Reference the 3.3V rail with VCC/1.6
	ADCA.REFCTRL = ADC_REFSEL_INTVCC_gc;
	
	//Prescaler from 2Mhz down to 125khz 
	ADCA.PRESCALER = ADC_PRESCALER_DIV16_gc;
	
	// No Event trigerring
	ADCA.EVCTRL = 0;
	
	// Channel 0, single ended input, no gain
	ADCA.CH0.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;
	
	// Mux: positive input = Pin 0. Negative = internal ground (single ended)
	ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc; //SET TO PIN 0
	
	//NO interrupt. Use only conversion complete flag
	ADCA.CH0.INTCTRL = 0;
	
	//Enable the ADC channel
	ADCA.CTRLA = ADC_ENABLE_bm;
	
	//Allow the ref voltage to stop oscillating, probably not needed? Just for oscillator crystals
	_delay_ms(100);
	
}


/* Single Block ADC Read Routine

returns 12 bit unsigned (0-4095)
Xmega stores in the right aligned CH0.RES

It is good practice to throw away the first result after any MUX change
*/

uint16_t adc_read(void)
{
	//Clear any past interrupt flag
	ADCA.CH0.INTFLAGS = ADC_CH_CHIF_bm; // Clear if?
	
	//Trigger conversion
	ADCA.CH0.CTRL |= ADC_CH_START_bm; //start it. Different from ENABLE. This is START
	
	//Stay in this while until conversion is complete. Similar to UART buffer
	while (!(ADCA.CH0.INTFLAGS & ADC_CH_CHIF_bm));
	
	//Clear flag and return results
	ADCA.CH0.INTFLAGS = ADC_CH_CHIF_bm;
	
	return ADCA.CH0.RES;
}

/*------------Oversampling and average read

takes samples and returns their average
Can reduce noise but loses accuracy?
Use either n=8 or n=16

Realistically this takes down the true sample rate to 125khz/16 = 7khz

Still super fast?? But it takes ~20 clock cycles

*/

uint16_t adc_read_avg(uint8_t n)
{
	uint32_t sum = 0;
	for (uint8_t i = 0; i < n; i++){
		sum += adc_read();
	}
	return (uint16_t)(sum / n);
}

/*---------------Convert ADC Count to Lux levels

Derivation with all integer arithmetic

VREF = VCC/1.6 --> 2062.5 mV round to 2063
Vin_mV = adc * 2063/4095
Ic_nA = Vin_mV * 1000000 / 4700 Ohms law but in nA
Ic_uA = Ic_nA / 1000 ----> Scaling
Lux = Ic_uA / 2  ---> 2 uA per lux, 

uint32_t cast prevents overflow from the ADC of 4095
Xmega does 16bit math default, this casts it to now overflow

*/

uint16_t adc_to_lux(uint16_t adc_count)
{
	uint32_t vin_mv = ((uint32_t)adc_count*2063UL)/ 4095UL;
	
	uint32_t ic_ua = (vin_mv * 10UL) / 47UL;
	
	//Lux  = Ic_uA/2
	return (uint16_t)(ic_ua/ 2UL);
}