/*
 * tbr.h
 *
 *  Created on: Mar 29, 2017
 *      Author: Waseemh
 * 
 *  Edited: Spring 2022
 *      Author: Jon Andreas Kornberg
 */
#ifndef SRC_TBR_H_
#define SRC_TBR_H_

#include "../hal/debug.h"
#include "../drivers_header/rs485.h"
#include "../drivers_header/delay.h"

#define TBR_BACKOFF_DELAY   18      // Response time from TBR

#define TAG_PROTOCOL_R256 0
#define TAG_PROTOCOL_R04K 1
#define TAG_PROTOCOL_R64K 2
#define TAG_PROTOCOL_S256 3
#define TAG_PROTOCOL_R01M 4
#define TAG_PROTOCOL_S64K 5
#define TAG_PROTOCOL_HS256 6
#define TAG_PROTOCOL_DS256 7
#define TAG_PROTOCOL_OPi 8
#define TAG_PROTOCOL_OPs 9
#define TBR_SENSOR_CODETYPE 0xFF

#define TBR_FIFO_LENGTH 10

// TBR Sensor msg: "$<tbrSN>,<unix_ts>,TBR Sensor,<temperature>,<avgNoise>,<peakNoise>,<noiseFreq>,<memAddr>\r"
// TAG msg: "$<tbrSN>,<unix_ts>,<millisec>,<tagProt>,<tagID>,<tagData>,<SNR>,<tagFreq>,<memAddr>\r"
typedef struct {
	// general data
	uint8_t		codetype; // resolved from <tagProt> and <tagFreq>, or TBR_SENSOR_CODETYPE
    uint16_t    tbrSN;
    uint32_t    unix_ts;
	uint32_t	memAddr;

	// tag specific data
    uint16_t    millisec;
    uint8_t     tagProt;
    uint32_t    tagID;
    uint16_t    tagData;
    uint8_t     SNR;
	uint8_t		tagFreq;

	// TBR sensor specific data
    uint16_t    temperature;
    uint8_t     avgNoise;
    uint8_t     peakNoise;
    uint8_t     noiseFreq;
} tbr_msg_t;


/*
 * public functions
 */

/*
	@brief Init tbr message fifo and rs485.
*/
void tbr_init( void );

/*
	@brief Get next tbr msg from tbr msg fifo buffer.
	
	@param msg (tbr_msg_t*) pointer to msg copy address destination.

	@return (bool) True if msg was copied. False if msg fifo buffer is empty.
*/
bool tbr_get_next_tbrMsg(tbr_msg_t* msg);

/*
	@brief Get the serial number of the connected TBR if resolved.

	@return (uint32_t) TBR SN (1 -> 999999) or 0 if no TBR SN resolved.
*/
uint32_t tbr_get_SN( void );

/*
	@brief Send ping to TBR.
*/
void tbr_ping( void );

/*
	@brief Check if ping is acked with SN.

	@return (bool) True if TBR respond. False if TBR does not respond.
*/
bool tbr_ping_is_rx_SN( void );

/*
	@brief Sends basic sync command to TBR: "(+)"
	TBR expect this to be sent at a whole decasecond.
	NOTE: This function does not check ack.
	To check ack -> call tbr_is_ack01()
*/
void tbr_basicTimeSync( void );

/*
	@brief Sends advanced sync command to TBR: "(+)123456789X" where 'X' is Luhn digit.
	The function calculate Luhn based on timestamp provided.
	TBR expect this to be sent at a whole decasecond.
	NOTE: This function does not check ack.
	To check ack -> call tbr_is_ack02().

	@param t unix timestamp.
*/
void tbr_advancedTimeSync(uint32_t t);

/*
	@brief Check if ack01 is received since last basic time sync.
	The TBR needs time to respond -> TBR_BACKOFF_DELAY.

	@return (bool) True if ack is received. False if not.
*/
bool tbr_is_ack01( void );

/*
	@brief Check if ack02 is received since last advanced time sync.
	The TBR needs time to respond -> TBR_BACKOFF_DELAY.

	@return (bool) True if ack is received. False if not.
*/
bool tbr_is_ack02( void );


#endif /* SRC_TBR_H_ */
