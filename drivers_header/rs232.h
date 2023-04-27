/*
 * rs232.h
 *
 *  Created on: Mar 27, 2017
 *      Author: Waseemh
 *
 * 	Edited on: Spring 2022
 * 		Author: Jon Andreas Kornberg
 */

#ifndef SRC_RS232_H_
#define SRC_RS232_H_

#define ENABLE_RS232 true

#ifdef ENABLE_RS232
#define ENABLE_RS232_FLAG 1
#else
#define ENABLE_RS232_FLAG 0
#endif


#include "../drivers_header/pinmap.h"
#include "em_usart.h"
#include "../FIFO/FIFO.h"


#define 	RS232_BAUDRATE 		115200
#define 	RS232_USART			USART0
#define 	RS232_USART_CLK		cmuClock_USART0
#define 	RS232_RX_IRQn		USART0_RX_IRQn
#define 	RS232_TX_IRQn		USART0_TX_IRQn
#define 	RS232_TX_ISR		USART0_TX_IRQHandler
#define 	RS232_RX_ISR		USART0_RX_IRQHandler

#define     FIFO_RS232_TX_SIZE	256
#define     FIFO_RS232_RX_SIZE	16

void 	rs232_init( void );
void 	rs232_enable( void );
void 	rs232_disable( void );
void 	rs232_transmit_string(char* data,uint8_t length);
void 	rs232_transmit_char(char data);
char 	rs232_receive( void );
void 	rs232_reset( void );
void 	rs232_shutdown( void );

#endif /* SRC_RS232_H_ */
