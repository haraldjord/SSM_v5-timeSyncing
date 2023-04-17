/*
 * rs232.c
 *
 *  Created on: Mar 27, 2017
 *      Author: Waseemh
 *
 * 	Edited on: Spring 2022
 * 		Author: Jon Andreas Kornberg
 */

#include "../drivers_header/rs232.h"

/*
 * private variables
 */
static struct fifo_descriptor fifo_rs232_tx_s;
static fifo_t fifo_rs232_tx = &fifo_rs232_tx_s;
static uint8_t tx_data[FIFO_RS232_TX_SIZE];

static struct fifo_descriptor fifo_rs232_rx_s;
static fifo_t fifo_rs232_rx = &fifo_rs232_rx_s;
static uint8_t rx_data[FIFO_RS232_RX_SIZE];

/*
 * public functions
 */
void rs232_init( void ) {
    if (ENABLE_RS232_FLAG) {
		fifo_create_static(fifo_rs232_tx, tx_data, FIFO_RS232_TX_SIZE, sizeof(uint8_t));
        fifo_create_static(fifo_rs232_rx, rx_data, FIFO_RS232_RX_SIZE, sizeof(uint8_t));

        USART_InitAsync_TypeDef usartInit = USART_INITASYNC_DEFAULT;
        usartInit.enable = usartDisable;
        usartInit.baudrate = RS232_BAUDRATE;

		CMU_ClockEnable(cmuClock_HFPER, true);
        CMU_ClockEnable(RS232_USART_CLK, true);
		CMU_ClockEnable(cmuClock_GPIO, true);


        GPIO_PinModeSet(RS232_PORT, RS232_TX, gpioModePushPull, 1);
        GPIO_PinModeSet(RS232_FORCEOFF_PORT, RS232_FORCEOFF, gpioModePushPull, 1);
        GPIO_PinModeSet(RS232_PORT, RS232_RX, gpioModeInput, 0);

        USART_Reset(RS232_USART);
        USART_InitAsync(RS232_USART, &usartInit);
        RS232_USART->ROUTE |= (0 << 8) | USART_ROUTE_RXPEN | USART_ROUTE_TXPEN;        //LOC1, TX & RX are enabled
    
        USART_IntClear(RS232_USART, _USART_IF_MASK);
        USART_IntEnable(RS232_USART, USART_IF_RXDATAV);
		NVIC_ClearPendingIRQ(RS232_TX_IRQn);
        NVIC_ClearPendingIRQ(RS232_RX_IRQn);
		NVIC_EnableIRQ(RS232_TX_IRQn);
        NVIC_EnableIRQ(RS232_RX_IRQn);
    }
    else rs232_shutdown();
}

void rs232_enable( void ) {
    if (ENABLE_RS232_FLAG) USART_Enable(RS232_USART, usartEnable);
}


void rs232_disable( void ) {
    if (ENABLE_RS232_FLAG) USART_Enable(RS232_USART, usartDisable);
}


void rs232_transmit_string(char* data, uint8_t length) {
    if (ENABLE_RS232_FLAG) {
		USART_IntDisable(RS232_USART, USART_IF_TXBL); //prevent race condition with ISR
        for (uint8_t i = 0; i < length; i++) {
			fifo_add(fifo_rs232_tx, &data[i]);
        }
		USART_IntEnable(RS232_USART, USART_IF_TXBL);
    }
}


void rs232_transmit_char(char data) {
    if (ENABLE_RS232_FLAG) {
		USART_IntDisable(RS232_USART, USART_IF_TXBL); //prevent race condition with ISR
		fifo_add(fifo_rs232_tx, &data);
		USART_IntEnable(RS232_USART, USART_IF_TXBL);
    }
}


char rs232_receive( void ) {
	char temp_char='\0';
    if (ENABLE_RS232_FLAG) {
		USART_IntDisable(RS232_USART, USART_IF_RXDATAV); //prevent race condition with ISR
        fifo_get(fifo_rs232_rx, &temp_char);
		USART_IntEnable(RS232_USART, USART_IF_RXDATAV);
    }
	return temp_char;
}


void rs232_reset( void ) {
    if (ENABLE_RS232_FLAG) USART_Reset(RS232_USART);
}


void rs232_shutdown( void ) {
	GPIO_PinOutClear(RS232_FORCEOFF_PORT, RS232_FORCEOFF);
}


/*
 * INT handlers
 */

void RS232_TX_ISR(){
    uint32_t int_mask = USART_IntGetEnabled(RS232_USART);
    if (int_mask & USART_IF_TXBL) {
		USART_IntClear(RS232_USART, USART_IF_TXBL);

        if (ENABLE_RS232_FLAG) {
			if (!fifo_is_empty(fifo_rs232_tx)) {
				uint8_t c;
				fifo_get(fifo_rs232_tx, &c);
				USART_Tx(RS232_USART, c);
			}
			else {
				USART_IntDisable(RS232_USART, USART_IF_TXBL);
			}			
        }
    }
}

void RS232_RX_ISR(){
    uint32_t int_mask = USART_IntGetEnabled(RS232_USART);
    if (int_mask & USART_IF_RXDATAV) {
		USART_IntClear(RS232_USART, USART_IF_RXDATAV);
        if (ENABLE_RS232_FLAG) {
            uint8_t c = USART_Rx(RS232_USART);
            fifo_add(fifo_rs232_rx, &c);
			rs232_transmit_char(c);
        }
    }
}
