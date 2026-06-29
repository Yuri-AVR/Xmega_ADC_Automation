/*
 * debug.h
 *
 * Created: 6/28/2026 5:18:32 PM
 *  Author: Yuri
 */ 


#ifndef DEBUG_H_
#define DEBUG_H_

#include <avr/io.h>

#ifdef DEBUG

void debug_init(void); //Clock out PC7

#else

//If doing a Release, build these calls and compile to nothing
#define debug_init()



#endif /* DEBUG_H_ */

#endif 