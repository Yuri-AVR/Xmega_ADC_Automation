/*
 * debug.c
 * Outputs the xmega clock to PC7 or Pin 8 on the J3 Header
 * Created: 6/28/2026 5:23:35 PM
 *  Author: Yuri
 */ 


#include "debug.h"

#ifdef DEBUG //Makes it stop here in Release

void debug_init(void)
{
	PORTCFG.CLKEVOUT = PORTCFG_CLKOUT_PC7_gc;
	PORTC.DIRSET = PIN7_bm;
	};
	
#endif //DEBUG