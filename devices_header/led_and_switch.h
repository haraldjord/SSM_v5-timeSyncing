/*
 * led_and_switch.h
 *
 *  Created on: Oct 4, 2018
 *      Author: Waseemh
 */

#ifndef DEVICES_HEADER_LED_AND_SWITCH_H_
#define DEVICES_HEADER_LED_AND_SWITCH_H_

#define LED_ENABLE true
#define GPIO_DEBUG_ENABLE true

#ifdef LED_ENABLE
#define LED_ENABLE_FLAG 1
#else
#define LED_ENABLE_FLAG 0
#endif

#ifdef GPIO_DEBUG_ENABLE
#define GPIO_DEBUG_ENABLE_FLAG 1
#else
#define GPIO_DEBUG_ENABLE_FLAG 0
#endif

#include "../drivers_header/pinmap.h"
#include "../drivers_header/delay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void 		init_led_switch( void );
uint8_t 	read_switch( void );
bool        read_switch_pin( uint8_t pin );
void 		status_led_gps( bool status );
void 		status_led_radio( bool status );
void 		set_status_led(bool red, bool green );
void        status_led_green_toggle( void );
void        status_led_red_toggle( void );
void        status_led_gps_toggle( void );
void        status_led_radio_toggle( void );


// External GPIO functions
void        status_gpio( uint8_t pin, bool status );
void        set_gpio( uint8_t pin );
void        clear_gpio( uint8_t pin );
void        gpio_toggle( uint8_t pin );

#endif /* DEVICES_HEADER_LED_AND_SWITCH_H_ */
