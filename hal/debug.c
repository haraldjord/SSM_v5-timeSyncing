/*******************************************************************************
 * Copyright (c) 2014-2015 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    IBM Zurich Research Lab - initial API, implementation and documentation
 *******************************************************************************/


#include "debug.h"

#include "../lmic/lmic.h"
#include "../drivers_header/rs232.h"

/*
void setupSWOForPrint(void)
{
    // Enable GPIO clock.
    CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_GPIO;

    // Enable Serial wire output pin
    GPIO->ROUTE |= GPIO_ROUTE_SWOPEN;

#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_LEOPARD_FAMILY) || defined(_EFM32_WONDER_FAMILY)
    // Set location 0
    GPIO->ROUTE = (GPIO->ROUTE & ~(_GPIO_ROUTE_SWLOCATION_MASK)) | GPIO_ROUTE_SWLOCATION_LOC0;

    // Enable output on pin - GPIO Port F, Pin 2
    GPIO->P[5].MODEL &= ~(_GPIO_P_MODEL_MODE2_MASK);
    GPIO->P[5].MODEL |= GPIO_P_MODEL_MODE2_PUSHPULL;
#else
    // Set location 1
    GPIO->ROUTE = (GPIO->ROUTE & ~(_GPIO_ROUTE_SWLOCATION_MASK)) |GPIO_ROUTE_SWLOCATION_LOC1;
    // Enable output on pin
    GPIO->P[2].MODEH &= ~(_GPIO_P_MODEH_MODE15_MASK);
    GPIO->P[2].MODEH |= GPIO_P_MODEH_MODE15_PUSHPULL;
#endif

    // Enable debug clock AUXHFRCO
    CMU->OSCENCMD = CMU_OSCENCMD_AUXHFRCOEN;

    // Wait until clock is ready
    while (!(CMU->STATUS & CMU_STATUS_AUXHFRCORDY));

    // Enable trace in core debug
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    ITM->LAR  = 0xC5ACCE55;
    ITM->TER  = 0x0;
    ITM->TCR  = 0x0;
    TPI->SPPR = 2;
    TPI->ACPR = 0xf;
    ITM->TPR  = 0x0;
    DWT->CTRL = 0x400003FF;
    ITM->TCR  = 0x0001000D;
    TPI->FFCR = 0x00000100;
    ITM->TER  = 0x1;
}

void SWO_put(char c){
	ITM_SendChar(c);
}

void SWO_puts(char* str){
    int i = 0;
	    while((str[0] != 0) && (i<256)){
		    SWO_put(str[i]);
		    i++;
	    }
}
*/

void debug_init () {
	rs232_init();
	rs232_enable();

	// print banner
    debug_str("\r\n============= DEBUG STARTED =============\r\n");

}


void debug_led (char val) {
	if (val) rgb_on(false,true,true);
	else rgb_shutdown();
}


void debug_char (char c) {
	rs232_transmit_char(c);
}


void debug_str (char* str) {
	rs232_transmit_string(str, (uint8_t)strlen(str));
}


void debug_event (int ev) {
    static const char* evnames[] = {
        [EV_SCAN_TIMEOUT]   = "SCAN_TIMEOUT",
        [EV_BEACON_FOUND]   = "BEACON_FOUND",
        [EV_BEACON_MISSED]  = "BEACON_MISSED",
        [EV_BEACON_TRACKED] = "BEACON_TRACKED",
        [EV_JOINING]        = "JOINING",
        [EV_JOINED]         = "JOINED",
        [EV_RFU1]           = "RFU1",
        [EV_JOIN_FAILED]    = "JOIN_FAILED",
        [EV_REJOIN_FAILED]  = "REJOIN_FAILED",
        [EV_TXCOMPLETE]     = "TXCOMPLETE",
        [EV_LOST_TSYNC]     = "LOST_TSYNC",
        [EV_RESET]          = "RESET",
        [EV_RXCOMPLETE]     = "RXCOMPLETE",
        [EV_LINK_DEAD]      = "LINK_DEAD",
        [EV_LINK_ALIVE]     = "LINK_ALIVE",
        [EV_SCAN_FOUND]     = "SCAN_FOUND",
        [EV_TXSTART]        = "TXSTART",
        [EV_TXCANCELED]     = "TXCANCELED",
        [EV_RXSTART]        = "RXSTART",
        [EV_JOIN_TXCOMPLETE]= "JOIN_TXCOMPLETE"
    };
    sprintf(debug_str_buf,"LMIC: Event nr: %d <%s>\n",ev,evnames[ev]);
    debug_str(debug_str_buf);
}
