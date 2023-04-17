/*
 * delay.h
 *
 *  Created on: Feb, 2020
 *      Author: MariusSR
 */

#ifndef SRC_DELAY_H_
#define SRC_DELAY_H_

#include "../drivers_header/pinmap.h"
#include "em_timer.h"

/*
 * public variables
 */
/*
 * private functions
 */
/*
 * public functions
 */
void delay_init( void );
void delay_us( uint16_t us );
void delay_ms( uint16_t ms );
void delay_ds( uint16_t ds );

#endif /* SRC_DELAY_H_ */
