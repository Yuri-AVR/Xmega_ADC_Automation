/*
 * tmp006.c
 *
 * Created: 6/28/2026 7:09:39 PM
 *  Author: Yuri
 *
 * TMP006 Driver
 * Uses TWI PORTE for I2c
 * DRDY on PD0
 *
 *Temp calc from  datasheet
 *Not calibrated
 */ 

#include "tmp006.h"
#include "uart.h"
#include <math.h>
#include <util/delay.h>

//---------------------------------------------
// TWI Setup Ports
//---------------------------------------------

static void twi_init(void)
{
	//PEO -> SDA INPUT, OPEN DRAIN
	//PE1 -> SCL OUTPUT, OPEN DRAIN
	PORTE.DIRSET = PIN1_bm;
	PORTE.DIRCLR = PIN2_bm;
	
	TWIE.MASTER.BAUD = TWI_BAUD_400K;
	TWIE.MASTER.CTRLA = TWI_MASTER_ENABLE_bm;
	
	//Start as idle to not conflict
	TWIE.MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;
}

//----------------------------------------------
//	TWIE START
//----------------------------------------------

static void twi_start(uint8_t addr_rw)
{
	
	//Writing to addr register triggers start and address transmission
	TWIE.MASTER.ADDR = addr_rw;
	
	//Wait for interrupt flag as addr is sent and ACK/NACK is recieved
	//WIF set on address sent, RIF set on data recieve
	if (addr_rw & 0x01)
	{
		//Read direction. Wait for RIF
		while(!(TWIE.MASTER.STATUS & (TWI_MASTER_RIF_bm | TWI_MASTER_WIF_bm)));
	}
	else
	{
		//Write direction - wait for WIF and address ACK
		while (!(TWIE.MASTER.STATUS & TWI_MASTER_WIF_bm));
	}
}

//Load data to TWI register and wait for the status to clear
static void twi_write_byte(uint8_t data)
{
	TWIE.MASTER.DATA = data;
	while(!(TWIE.MASTER.STATUS & TWI_MASTER_WIF_bm));
}

//Read data from the ACK line
static uint8_t twi_read_byte_ack(void)
{
	//Send ACK. Need to send every time for next byte to be sent from device
	TWIE.MASTER.CTRLC = TWI_MASTER_CMD_RECVTRANS_gc;
	while(!(TWIE.MASTER.STATUS & TWI_MASTER_RIF_bm));
	return TWIE.MASTER.DATA;
	
}

//Read NACK if not
static uint8_t twi_read_byte_nack(void)
{
	
	//Send NACK and STOP when done reading
	TWIE.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
	while (!(TWIE.MASTER.STATUS & TWI_MASTER_RIF_bm));
	return TWIE.MASTER.DATA;
}

//STOP TWI
static void twi_stop(void)
{
	TWIE.MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
}


//------------------------------------------------------
//	TMP006 REGISTER ACCESS
// Writes to the TMP006 directly, this is different than sending
// strings to an instrument. Bits are pushed
//------------------------------------------------------

static void tmp006_write_reg(uint8_t ptr, uint16_t value)
{
	twi_start((TMP006_ADDR << 1)| 0);
	twi_write_byte(ptr);	//pointer register select
	twi_write_byte((value >> 8) & 0xFF); //MSB bit first
	twi_write_byte(value & 0xFF); //LSB
	twi_stop();
	
}

/*
*	Read 16bit value from register
*	Transaction: START | ADDR+W | PTR | Sr | ADDR+R | MSB | ACK | LSB | NACK | STOP
*/

static int16_t tmp006_read_reg(uint8_t ptr)
{
	//Write phase: set the pointer register
	twi_start((TMP006_ADDR << 1)| 0);
	twi_write_byte(ptr);
	
	// Repeat START then switch to read
	twi_start((TMP006_ADDR<<1)|1);
	
	uint8_t msb = twi_read_byte_ack(); // Read MSB, send ACK
	uint8_t lsb = twi_read_byte_nack(); // Read LSB, send NACK and STOP
	
	return(int16_t)((msb<<8) | lsb);
}

// Main Functions --------------


uint8_t tmp006_init(void)
{
	//Configure the TWIE 
	
	twi_init();
	
	//DRDY on PD0, it sinks the current from the pin which is set high
	PORTD.DIRCLR = PIN0_bm;
	
	//Delay for the sensor to start up. Might delete later?
	_delay_ms(10);
	
	//Verify sensor MAN and Device ID. 0xFE MAN and 0xFF ID
	uint16_t mfr = (uint16_t)tmp006_read_reg(TMP006_REG_MFR_ID); 
	uint16_t dev = (uint16_t)tmp006_read_reg(TMP006_REG_DEV_ID);
	
	if (mfr != TMP006_MFR_ID_EXPECTED || dev != TMP006_DEV_ID_EXPECTED)
	{
		uart_send_string("TMP006 ERROR: sensor not found\r\n");
		uart_send_string("	MFR ID: 0x"); uart_send_uint(mfr); uart_send_string("\r\n");
		uart_send_string("	DEV ID: 0X"); uart_send_uint(dev); uart_send_string("\r\n");
		
		return 0;
	}
	
	uart_send_string("TMP006 OK: MFR=0X5449 DEV=0X0067\r\n");
	
	//Set mode to continuous conversion at 1 per sec, DRDY enable
	tmp006_write_reg(TMP006_REG_CONFIG, TMP006_CONFIG_ON);
	
	return 1; //set to one to check as flag
}

uint8_t tmp006_data_read(void)
{
	//DRDY is set low --> means conversion complete
	return !(PORTD.IN & PIN0_bm);
}

float tmp006_get_tdie(void)
{
	int16_t raw = tmp006_read_reg(TMP006_REG_TDIE);
	
	//14 bit value [15:2]. Bit shift right >> 2 then divide by 32
	return (float)(raw >> 2)/32.0f;
}

float tmp006_get_tobj(void)
{
	//Read both values
	int16_t raw_vobj = tmp006_read_reg(TMP006_REG_VOBJ);
	int16_t raw_tdie = tmp006_read_reg(TMP006_REG_TDIE);
	
	//Do conversion
	// 156.25 nV per LSB
	float vobj = (float)raw_vobj * 156.25e-9f;
	
	//TDIE, right shift 2 bits, divide by 32 and convert to K
	float tdie	= ((float)(raw_tdie >> 2)/32.0f) + 273.15f;
	
	//Equations to take die and vobj values to create a real temp value
	
	float dT   = tdie - 298.15f;

	// Step 2: thermopile sensitivity S, corrected for die temperature
	float S    = TMP006_S0 * (1.0f + (1.75e-3f * dT) - (1.678e-5f * dT * dT));

	// Step 3: self-heating voltage offset
	float vos  = -2.94e-5f + (-5.7e-7f * dT) + (4.63e-9f * dT * dT);

	// Step 4: corrected thermopile voltage with second-order term
	float dV   = vobj - vos;
	float fv   = dV + (13.4f * dV * dV);

	// Step 5: Stefan-Boltzmann quartic root to get object temperature
	float tobj = powf(powf(tdie, 4.0f) + (fv / S), 0.25f);

	// Return in Celsius
	return tobj - 273.15f;
	
}