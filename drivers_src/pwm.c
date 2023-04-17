/*
 * pwm.c
 *
 *  Created on: Mar 24, 2017
 *      Author: Waseemh
 */

#include "../drivers_header/pwm.h"
void pwm_init(){
    /*
	TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
	timerCCInit.cofoa=timerOutputActionSet;
	timerCCInit.cmoa=timerOutputActionClear;
  	timerCCInit.mode=timerCCModePWM;
  	TIMER_Init_TypeDef timerPWMInit = TIMER_INIT_DEFAULT;
  	timerPWMInit.enable=false;
  	timerPWMInit.prescale=timerPrescale1;
	CMU_ClockEnable(PWM_CLK, true);
	TIMER_TopSet(PWM_TIMER, PWM_TOP);
	TIMER_CounterSet(PWM_TIMER, 0);
	TIMER_CompareSet(PWM_TIMER, PWM_CHNL, PWM_CMPR);
	TIMER_CompareBufSet(PWM_TIMER, PWM_CHNL, PWM_CMPR);
	PWM_TIMER->ROUTE |= (1 << 16) |(1 << 0);		//Location 1, CC_2 routing for PE10
	TIMER_IntEnable(PWM_TIMER, TIMER_IF_CC0);
	TIMER_InitCC(PWM_TIMER, PWM_CHNL, &timerCCInit);
	TIMER_Init(PWM_TIMER, &timerPWMInit);
				 */
  	TIMER_Init_TypeDef timerCCInit = TIMER_INIT_DEFAULT;
  	timerCCInit.enable=false;
  	timerCCInit.prescale=timerPrescale1;
	CMU_ClockEnable(PWM_CLK, true);
	TIMER_TopSet(PWM_TIMER, 0);
	TIMER_CounterSet(PWM_TIMER, 0);
	TIMER_IntEnable(PWM_TIMER, TIMER_IF_OF);
	TIMER_Init(PWM_TIMER, &timerCCInit);
	return;
}

//void gps_led_pwm_test( void ) {
//
//////    CHIP_Init();
////
////            CHIP_Init();
////			//CMU_HFRCOBandSet(cmuHFRCOBand_7MHz );
////			//CMU_OscillatorEnable(cmuOsc_HFRCO, true, true);
////			CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
////			//CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFRCO);
////
////			CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
////			CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
////			CMU_ClockDivSet(cmuClock_HF, cmuClkDiv_8);
////
////    CMU_ClockEnable(cmuClock_GPIO, true);
////    CMU_ClockEnable(cmuClock_TIMER0, true);
////
////    // Enable LED output
////    GPIO_PinModeSet(LED_GPS_RADIO_PORT, LED_GPS, gpioModePushPull, 0);
////    GPIO_PinModeSet(LED_GPS_RADIO_PORT, LED_RADIO, gpioModePushPull, 0);
////    GPIO_PinOutSet(LED_GPS_RADIO_PORT, LED_GPS);
////    GPIO_PinOutSet(LED_GPS_RADIO_PORT, LED_RADIO);
////    //delay_ms(10);
////    GPIO_PinOutClear(LED_GPS_RADIO_PORT, LED_GPS);
////    //for (int i = 0; i< 200; ++i) {
////    //    delay_ms(10);
////    //}
////
////    // Create the timer count control object initializer
////    TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
////    timerCCInit.mode = timerCCModePWM;
////    timerCCInit.cmoa = timerOutputActionToggle;
////
////    // Configure CC channel 0
////    TIMER_InitCC(TIMER0, TIMER_CHANNEL, &timerCCInit);
////
////    // Route CC0 to location 4 (PC2) and enable pin for cc0
//////    TIMER0->ROUTE |= (TIMER_ROUTE_CC1PEN | TIMER_ROUTE_LOCATION_LOC4);
////    TIMER0->ROUTE |= (TIMER_ROUTE_CDTI0PEN | TIMER_ROUTE_LOCATION_LOC4);
////
////    // Set Top Value
////    TIMER_TopSet(TIMER0, TIMER_TOP);
////
////    // Set the PWM duty cycle here!
////    TIMER_CompareBufSet(TIMER0, TIMER_CHANNEL, DUTY_CYCLE);
////
////    // Create a timerInit object, based on the API default
////    TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
//////    timerInit.prescale = timerPrescale256;
////    timerInit.prescale = timerPrescale8;
////
////    TIMER_Init(TIMER0, &timerInit);
////    TIMER_Enable(TIMER0, true);
////    while (1);
////
//////    TIMER_Enable(TIMER0, true);
////
////    return;
//
//    CHIP_Init();
//
//    CMU_ClockEnable(cmuClock_GPIO, true);
//    CMU_ClockEnable(cmuClock_TIMER0, true);
//
//    // Enable LED output
//    GPIO_PinModeSet(LED_GPS_RADIO_PORT, LED_GPS, gpioModePushPull, 0);
//    GPIO_PinOutSet(LED_GPS_RADIO_PORT, LED_GPS);
//
//    // Create the timer count control object initializer
//    TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
//    timerCCInit.mode = timerCCModePWM;
//    timerCCInit.cmoa = timerOutputActionToggle;
//
//    // Configure CC channel 2
//    TIMER_InitCC(TIMER0, TIMER_CHANNEL, &timerCCInit);
//
//    // Route CC2 to location 1 (PE3) and enable pin for cc2
//    TIMER0->ROUTE |= (TIMER_ROUTE_CC0PEN | TIMER_ROUTE_LOCATION_LOC4);
////    TIMER0->ROUTE |= (TIMER_ROUTE_CDTI0PEN | TIMER_ROUTE_LOCATION_LOC4);
//
//    // Set Top Value
//    TIMER_TopSet(TIMER0, TIMER_TOP);
//
//    // Set the PWM duty cycle here!
//    TIMER_CompareBufSet(TIMER0, TIMER_CHANNEL, DUTY_CYCLE);
//
//    // Create a timerInit object, based on the API default
//    TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
//    timerInit.prescale = timerPrescale256;
//
//    TIMER_Init(TIMER0, &timerInit);
//
//    while (1)
//        ;
//}

void pwm_enable( void ) {
	TIMER_Enable(PWM_TIMER,true);
}


void pwm_disable( void ) {
	TIMER_Enable(PWM_TIMER,false);
}


void pwm_reset( void ) {
	TIMER_Reset(PWM_TIMER);
}


void pwm_us_delay(uint16_t us_delay) {
	uint32_t 	converted_top	=0;
	converted_top=(7*us_delay);
	TIMER_TopSet(PWM_TIMER, converted_top);
	TIMER_TopBufSet(PWM_TIMER, converted_top);
	TIMER_CounterSet(PWM_TIMER, 0);
	pwm_enable();
	while(!TIMER_IntGetEnabled(PWM_TIMER));
	TIMER_IntClear(PWM_TIMER,TIMER_IF_OF);
	pwm_disable();
}


//NOT used functions
void pwm_generate_pulses(int num_pulses) {
	int loop_var=0;
	TIMER_CompareSet(PWM_TIMER, PWM_CHNL, PWM_CMPR);
	pwm_enable();
	for (loop_var=1;loop_var<num_pulses;loop_var++){
		while(!TIMER_IntGetEnabled(PWM_TIMER));
		TIMER_IntClear(PWM_TIMER,TIMER_IF_CC0);
	}
	TIMER_CompareBufSet(PWM_TIMER, PWM_CHNL, PWM_TOP+1);
	return;
}


void pwm_continuous_mode() {
	TIMER_CompareBufSet(PWM_TIMER, PWM_CHNL, PWM_TOP+1);
	pwm_enable();
	return;
}
