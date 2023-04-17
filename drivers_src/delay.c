/*
 * delay.h
 *
 *  Created on: Feb, 2020
 *      Author: MariusSR
 */

#include "../drivers_header/delay.h"


TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;

void delay_init( void ) {
	CMU_ClockEnable(cmuClock_HFPER, true);
	CMU_ClockEnable(cmuClock_TIMER0, true);

	timerInit.enable = false;
}

// Works for delays up to 10000 us
void delay_us( uint16_t us ) {
	timerInit.prescale = timerPrescale1;
	TIMER_Init(TIMER0, &timerInit);
	TIMER_CounterSet(TIMER0, 0);
	TIMER_Enable(TIMER0, true);
	while(TIMER_CounterGet(TIMER0) < us*6);
	TIMER_Enable(TIMER0,false);
}

// Works for delays up to 1000 ms
void delay_ms( uint16_t ms ) {
    timerInit.prescale = timerPrescale128;
	TIMER_Init(TIMER0, &timerInit);
	TIMER_CounterSet(TIMER0, 0);
	TIMER_Enable(TIMER0, true);
	while(TIMER_CounterGet(TIMER0) < ms*47);
	TIMER_Enable(TIMER0,false);
}


void delay_ds( uint16_t ds ) {
    for ( int i = 0; i < ds; ++i ) delay_ms(100);
}
