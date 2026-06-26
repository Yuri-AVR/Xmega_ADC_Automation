/*
 * adc_light.h
 *
 * Created: 6/25/2026 10:32:23 PM
 *  Author: Yuri
 */ 


#ifndef ADC_LIGHT_H_
#define ADC_LIGHT_H_

#include <stdint.h>


/* ADC Initial

Configure ADCA to single ended conv on PA0
Call before any read
loads calibration values

*/

void adc_init(void);

/*
 ADC Read
 Trigger a conversion and return 12 bit result
 Blocks until conversion flag is set
*/

uint16_t adc_read(void);

// ADC Reading average
// Pass the samples to average

uint16_t adc_read_avg(uint8_t n);


//ADC to lux
// Converts the ADC count to a value of lux

uint16_t adc_to_lux(uint16_t adc_count);


#endif /* ADC_LIGHT_H_ */