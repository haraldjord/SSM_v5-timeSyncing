/*
 * rs485.c
 *
 *  Created on: Mar 29, 2017
 *      Author: Waseemh
 * 
 * 	Edited on: Spring 2022
 * 		Author: Jon Andreas Kornberg
 */

#include "../drivers_header/rs485.h"

/*
 * private variables
 */
static struct fifo_descriptor fifo_rs485_tx_s;
static fifo_t fifo_rs485_tx = &fifo_rs485_tx_s;
static uint8_t tx_data[FIFO_RS485_TX_SIZE];

static struct fifo_descriptor fifo_rs485_rx_s;
static fifo_t fifo_rs485_rx = &fifo_rs485_rx_s;
static uint8_t rx_data[FIFO_RS485_RX_SIZE];

/*
 * public variables
 */

/*
 * private functions
 */
/*
 * public functions
 */

void rs485_init(void) {
	fifo_create_static(fifo_rs485_tx, tx_data, FIFO_RS485_TX_SIZE, sizeof(uint8_t));
	fifo_create_static(fifo_rs485_rx, rx_data, FIFO_RS485_RX_SIZE, sizeof(uint8_t));

	LEUART_Init_TypeDef leuartInit = {
			.enable=	leuartDisable,
			.baudrate=	RS485_BAUDRATE,
			.databits=	leuartDatabits8,
			.parity=	leuartNoParity,
			.refFreq=   0,
			.stopbits= 	leuartStopbits1
	};

	CMU_ClockEnable(cmuClock_CORELE, true);
	CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_CORELEDIV2);
	CMU_ClockEnable(cmuClock_LFB, true);

	LEUART_FreezeEnable(RS485_UART,true);

	CMU_ClockEnable(RS485_UART_CLK, true);
	GPIO_PinModeSet(RS485_PORT, RS485_TX, gpioModePushPull, 1);
	GPIO_PinModeSet(RS485_PORT, RS485_DE, gpioModePushPull, 1);
	GPIO_PinModeSet(RS485_PORT, RS485_RE, gpioModePushPull, 1);
	GPIO_PinModeSet(RS485_PORT, RS485_RX, gpioModeInput, 0);

	LEUART_Reset(RS485_UART);
	LEUART_Init(RS485_UART, &leuartInit);
	RS485_UART->ROUTE |= (0x1UL <<9) | LEUART_ROUTE_TXPEN | LEUART_ROUTE_RXPEN;		//LOC2, TX & RX are enabled

	LEUART_IntClear(RS485_UART, _LEUART_IFC_MASK);
	LEUART_IntEnable(RS485_UART, LEUART_IEN_RXDATAV);
	NVIC_ClearPendingIRQ(RS485_IRQn);
	NVIC_EnableIRQ(RS485_IRQn);

	LEUART_FreezeEnable(RS485_UART,false);

	rs485_rx_mode();
}

void rs485_enable(void) {
	LEUART_Enable(RS485_UART, leuartEnable);
}


void rs485_disable(void) {
	LEUART_Enable(RS485_UART, leuartDisable);
}

void rs485_reset(void) {
	LEUART_Reset(RS485_UART);
}

void rs485_transmit_string(char* data,uint8_t length) {

	LEUART_IntDisable(RS485_UART, LEUART_IEN_TXBL); //prevent race condition with ISR
	for(uint8_t i = 0; i<length; i++) {
		fifo_add(fifo_rs485_tx, &data[i]);
	}
	rs485_tx_mode();
	LEUART_IntEnable(RS485_UART, LEUART_IEN_TXBL);
}

void rs485_transmit_char(char data) {

	LEUART_IntDisable(RS485_UART, LEUART_IEN_TXBL); //prevent race condition with ISR
	fifo_add(fifo_rs485_tx, &data);
	rs485_tx_mode();
	LEUART_IntEnable(RS485_UART, LEUART_IEN_TXBL);
}

char rs485_recieve_char(void) {
	char temp_char = 0;

	LEUART_IntDisable(RS485_UART, LEUART_IEN_RXDATAV); //prevent race condition with ISR
	fifo_get(fifo_rs485_rx, &temp_char);
	LEUART_IntEnable(RS485_UART, LEUART_IEN_RXDATAV);

	return  temp_char;
}

void rs485_tx_mode(void) {
	GPIO_PinOutSet(RS485_PORT, RS485_RE);
	GPIO_PinOutSet(RS485_PORT, RS485_DE);
}

void rs485_rx_mode(void) {
	GPIO_PinOutClear(RS485_PORT, RS485_DE);
	GPIO_PinOutClear(RS485_PORT, RS485_RE);
}

void rs485_low_power_mode( bool flag ) {
    if ( flag ) {
        GPIO_PinOutSet(RS485_PORT, RS485_RE);
        GPIO_PinOutClear(RS485_PORT, RS485_DE);
    }
    else rs485_rx_mode();
}

/*
 * INT handlers
 */
void RS485_ISR() {
	uint32_t int_mask = LEUART_IntGetEnabled(RS485_UART);
	LEUART_IntClear(RS485_UART, int_mask);

	if (int_mask & LEUART_IF_RXDATAV){
		while (LEUART_StatusGet(RS485_UART) & LEUART_STATUS_RXDATAV) {
			uint8_t c = LEUART_Rx(RS485_UART);
			fifo_add(fifo_rs485_rx, &c);
		}
	}
	if (int_mask & LEUART_IF_TXBL) {
		if(!fifo_is_empty(fifo_rs485_tx)) {
			uint8_t c;
			fifo_get(fifo_rs485_tx, &c);
			LEUART_Tx(RS485_UART, c);

			delay_ms(1);
		}
		else {
			rs485_rx_mode();
			LEUART_IntDisable(RS485_UART, LEUART_IEN_TXBL);
		}
	}
}

