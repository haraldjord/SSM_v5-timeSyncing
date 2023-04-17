/*
 * led_and_switch.c
 *
 *  Created on: Oct 4, 2018
 *      Author: Waseemh
 */


#include "../devices_header/led_and_switch.h"


void init_led_switch( void ) {
    CMU_ClockEnable(cmuClock_GPIO, true);

    // Leds
    GPIO_PinModeSet(LED_RED_GREEN_PORT, LED_R, gpioModePushPull, 0);
    GPIO_PinModeSet(LED_RED_GREEN_PORT, LED_G, gpioModePushPull, 0);
    GPIO_PinModeSet(LED_GPS_RADIO_PORT, LED_GPS, gpioModePushPull, 0);
    GPIO_PinModeSet(LED_GPS_RADIO_PORT, LED_RADIO, gpioModePushPull, 0);

    // External GPIO AUX pins
    GPIO_PinModeSet(EXT_AUX_PORT, AUX_0, gpioModePushPull, 0);
    GPIO_PinModeSet(EXT_AUX_PORT, AUX_1, gpioModePushPull, 0);
    GPIO_PinModeSet(EXT_AUX_PORT, AUX_2, gpioModePushPull, 0);
    GPIO_PinModeSet(EXT_AUX_PORT, AUX_3, gpioModePushPull, 0);

    // Address switch
    GPIO_PinModeSet(SWITCH_1_PORT, SW_1, gpioModeInputPull , 1);
    GPIO_PinModeSet(SWITCH_2_PORT, SW_2, gpioModeInputPull , 1);
    GPIO_PinModeSet(SWITCH_3_PORT, SW_3, gpioModeInputPull , 1);
    GPIO_PinModeSet(SWITCH_456_PORT, SW_4, gpioModeInputPull , 1);
    GPIO_PinModeSet(SWITCH_456_PORT, SW_5, gpioModeInputPull , 1);
    GPIO_PinModeSet(SWITCH_456_PORT, SW_6, gpioModeInputPull , 1);
    return;
}


uint8_t read_switch( void ) {
    uint8_t var=0;
    var|=(GPIO_PinInGet(SWITCH_1_PORT, SW_1)<<0);
    var|=(GPIO_PinInGet(SWITCH_2_PORT, SW_2)<<1);
    var|=(GPIO_PinInGet(SWITCH_3_PORT, SW_3)<<2);
    var|=(GPIO_PinInGet(SWITCH_456_PORT, SW_4)<<3);
    var|= (GPIO_PinInGet(SWITCH_456_PORT, SW_5)<<4);
    var|= (GPIO_PinInGet(SWITCH_456_PORT, SW_6)<<5) ;
    return var;
}

bool read_switch_pin( uint8_t pin ) {
    switch ( pin ) {
        case 0:  return GPIO_PinInGet(SWITCH_1_PORT,   SW_1);
        case 1:  return GPIO_PinInGet(SWITCH_2_PORT,   SW_2);
        case 2:  return GPIO_PinInGet(SWITCH_3_PORT,   SW_3);
        case 3:  return GPIO_PinInGet(SWITCH_456_PORT, SW_4);
        case 4:  return GPIO_PinInGet(SWITCH_456_PORT, SW_5);
        case 5:  return GPIO_PinInGet(SWITCH_456_PORT, SW_6);
        default: return false;
    }
}


void status_led_gps(bool status){
    if ( LED_ENABLE_FLAG ) {
        if (status == true) GPIO_PinOutSet(LED_GPS_RADIO_PORT, LED_GPS);
        else GPIO_PinOutClear(LED_GPS_RADIO_PORT, LED_GPS);
    }
}


void status_led_radio(bool status){
    if ( LED_ENABLE_FLAG ) {
        if (status == true) GPIO_PinOutSet(LED_GPS_RADIO_PORT, LED_RADIO);
        else GPIO_PinOutClear(LED_GPS_RADIO_PORT, LED_RADIO);
    }
}


void set_status_led(bool red, bool green){
    if ( LED_ENABLE_FLAG ) {
        if (red == true) GPIO_PinOutSet(LED_RED_GREEN_PORT, LED_R);
        else GPIO_PinOutClear(LED_RED_GREEN_PORT, LED_R);

        if (green == true) GPIO_PinOutSet(LED_RED_GREEN_PORT, LED_G);
        else GPIO_PinOutClear(LED_RED_GREEN_PORT, LED_G);
    }
    return;
}


void status_led_red_toggle( void ) {
    if ( LED_ENABLE_FLAG ) GPIO_PinOutToggle(LED_RED_GREEN_PORT, LED_R);
}


void status_led_green_toggle( void ) {
    if ( LED_ENABLE_FLAG ) GPIO_PinOutToggle(LED_RED_GREEN_PORT, LED_G);
}


void status_led_gps_toggle( void ) {
    if ( LED_ENABLE_FLAG ) GPIO_PinOutToggle(LED_GPS_RADIO_PORT, LED_GPS);
}


void status_led_radio_toggle( void ) {
    if ( LED_ENABLE_FLAG ) GPIO_PinOutToggle(LED_GPS_RADIO_PORT, LED_RADIO);
}

/// GPIO pins
void status_gpio(uint8_t pin, bool status) {
    if ( GPIO_DEBUG_ENABLE_FLAG ) {
        if (status == true) GPIO_PinOutSet(EXT_AUX_PORT, pin);
        else GPIO_PinOutClear(EXT_AUX_PORT, pin);
    }
}


void set_gpio(uint8_t pin) {
    if ( GPIO_DEBUG_ENABLE_FLAG ) status_gpio(pin, true);
}


void clear_gpio(uint8_t pin) {
    if ( GPIO_DEBUG_ENABLE_FLAG ) status_gpio(pin, false);
}


void gpio_toggle(uint8_t pin) {
    if ( GPIO_DEBUG_ENABLE_FLAG ) GPIO_PinOutToggle(EXT_AUX_PORT, pin);
}
