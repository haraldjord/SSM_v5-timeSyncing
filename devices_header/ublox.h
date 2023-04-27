/*
 * ublox_gps.h - > ublox.h
 *
 *  Created on: Apr 25, 2017
 *      Author: Waseemh
 * 
 * 	Edited: Spring 2022
 * 		Author: Jon Andreas Kornberg
 */

#ifndef DEVICES_HEADER_UBLOX_H_
#define DEVICES_HEADER_UBLOX_H_

#include <stdint.h>

#include "../drivers_header/spi.h"
#include "../FIFO/FIFO.h"
#include "../hal/debug.h"
#include "../drivers_header/delay.h"

/*
 * Macros
 */

#define UBX_MAX_PAYLOAD_SIZE 248 // + 4 = 256
#define UBX_MAX_BUFFER_LENGTH 4 // 4*256 = 1024

/*
 * Structs and typedefs
 */

typedef struct {
	uint8_t ubx_class;
	uint8_t ubx_id;
	uint16_t length;
	
	uint8_t payload[UBX_MAX_PAYLOAD_SIZE];
} ubx_msg_t;

typedef struct {
	bool		valid_time;
	bool		valid_pos;
	bool    newData;

	uint16_t	year;
	uint8_t		month;
	uint8_t		day;
	uint8_t		hour;
	uint8_t		min;
	uint8_t		sec;

	uint8_t		numSV;
	int32_t		longitude;
	int32_t		latitude;
	uint16_t	pDOP;
	uint8_t		fix;
} nav_data_t;

typedef struct {
	int32_t		longitude;
	int32_t		latitude;
	uint16_t	pDOP;
	uint8_t		fix;
	uint8_t		numSV;
} pos_t;

typedef struct {
    uint8_t     numSV;
    uint8_t     numSV_40dBHz;
} sat_data_t;

typedef struct {
    uint8_t		fix;
    uint32_t	ttff;
} nav_status_data_t;

/*
 * public functions
 */

/*
	@brief Initialize SPI communication with uBlox
	and fifo buffer for received UBX messages.
	Configure the uBlox for operation.

	@return (bool) True if configuration is successful,
	false if configuration fails after retrying.
*/
bool gnss_init( void );

/*
	@brief Clear uBlox SPI transmit register and
	clear all received msg in fifo buffer.
*/
void gnss_clearAll_ubxMsgs( void );

/*
	@brief Reset valid position and valid tim flags.
*/
void gnss_clear_validData( void );

/*
	@brief Check if any navdata is received from uBlox,
	if not send a poll request for navdata.

	@return (bool) True if navdata was received, false if not.
*/
bool gnss_poll_navdata( void );

/*
	@brief Check if local position data is valid

	@return (bool) True if valid.
*/
bool gnss_is_validPosData( void );

/*
	@brief Check if local time data is valid

	@return (bool) True if valid.
*/
bool gnss_is_validTimeData( void );

/*
	@brief Get local position data.

	@param data (pos_t*) Ponter to struct where to save the position.
*/
void gnss_get_posData(pos_t *data);

/*
	@brief Get local time data.

	@return (uint32_t) Unix timestamp.
*/
uint32_t gnss_get_timeData( void );

/*
	@brief Set uBlox keep-awake pin.
	Wakes up the uBlox module and keeps it awake.
*/
void gnss_force_ON( void );

/*
	@brief Reset uBlox keep-awake pin.
	Forces the uBlox module to sleep/off.
*/
void gnss_force_OFF( void );

#endif /* DEVICES_HEADER_UBLOX_H_ */
