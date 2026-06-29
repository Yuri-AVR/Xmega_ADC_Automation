/*
 * tmp006.h
 *
 * Created: 6/28/2026 6:54:26 PM
 *  Author: Yuri
 *
 *Driver for the TMP006 Sensor IR
 *Connected via TWI PORT E on Xplianed Board
 *	SDA -> PE0 (J4 Pin1)
 *	SCL -> PE1 (J4 Pin2)
 *	DRDY -> PD0 (J4 Pin 5)
 *
 *	i2c Adress: 0x40 with ADO AD1 tied to GND
 */ 


#ifndef TMP006_H_
#define TMP006_H_

#include <avr/io.h>
#include <stdint.h>


//I2C address
#define TMP006_ADDR	0x40

//Register pointers from datasheet
#define TMP006_REG_VOBJ		0X00
#define	TMP006_REG_TDIE		0X01
#define TMP006_REG_CONFIG	0X02
#define TMP006_REG_MFR_ID	0xFE
#define TMP006_REG_DEV_ID	0XFF

//Configure these register values for default settings
//MOD=111 (continious), CR=010 (1 conversion per second, 4 samples average), EN=1 (DRDY pin enable)
#define TMP006_CONFIG_ON	0x7500
// MOD=000 Power down
#define TMP006_CONFIG_OFF 0X0000

//ID VALUES FROM MANUF
#define	TMP006_MF_ID_EXPECTED 0X5449
#define TMP006_DEV_ID_EXPECTED 0X0067

//Calibration Factor
#define TMP006_S0	6.4e-14f

//TWI Baud for 400khz when using 32mhz clock. (32M/[2*400k]) - 5 = 35
#define TWI_BAUD_400K	35

/*
* Initialize
*Configure peripheral, verify ID and start conversion
*DRDY pin enabled. 
*Returns 1 if sensor found and verified, 0 on failure
*Return is unsinged 8 bit
*Call afer clock and uart init()
*/

uint8_t tmp006_init(void);

/*
*Tmp target obj
*Reads VOBJ and TDIE registers and applies the math from datasheet
*Returns object temp in C
*Returns float
*Call when tmp006_data_read()
*/

float tmp006_get_tobj(void);

/*
*tmp006 get tdie
*Returns raw die temp in C
*Should be value before math
*Also a float
*/

float tmp006_get_tdie(void);

#endif /* TMP006_H_ */