/*
 * rs485.h
 *
 *  Created on: Mar 27, 2017
 *      Author: Waseemh
 * 
 * 	Edited on: Spring 2022
 * 		Author: Jon Andreas Kornberg
 */

#ifndef SRC_RS485_H_
#define SRC_RS485_H_

#include "../drivers_header/pinmap.h"
#include "em_leuart.h"
#include "../FIFO/FIFO.h"
#include "../drivers_header/delay.h"

#define 	RS485_BAUDRATE 		115200
#define 	RS485_UART			LEUART0
#define 	RS485_UART_CLK		cmuClock_LEUART0
#define 	RS485_ISR			LEUART0_IRQHandler
#define 	RS485_IRQn			LEUART0_IRQn

#define 	FIFO_RS485_TX_SIZE		32
#define 	FIFO_RS485_RX_SIZE		1024

typedef enum{
	cmd_sn_req=0,
	cmd_basic_sync,
	cmd_advance_sync
}tbr_cmd_t;


	/*
	 * global shared variables
	 */

	/*
	 * private functions
	 */
void 	rs485_tx_mode( void );
void 	rs485_rx_mode( void );
	/*
	 * public functions
	 */
void 	rs485_init( void );
void 	rs485_enable( void );
void 	rs485_disable( void );
void 	rs485_reset( void );
void 	rs485_transmit_string( char* data,uint8_t length);
void	rs485_transmit_char( char data );
char 	rs485_recieve_char( void );
void    rs485_low_power_mode( bool flag );

#endif /* SRC_RS485_H_ */
