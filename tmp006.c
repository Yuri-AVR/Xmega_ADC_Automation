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
//------------------------------------------------------

