/*
 * lpwan_manager.h
 *
 *  Created on: Jun 29, 2017
 *      Author: Waseemh
 */

#ifndef RESOURCE_MANAGERS_HEADER_LPWAN_MANAGER_H_
#define RESOURCE_MANAGERS_HEADER_LPWAN_MANAGER_H_

#include "../lmic/lmic.h"
#include "../hal/debug.h"
#include "../lmic/hal.h"
#include "../FIFO/FIFO.h"
#include "em_rmu.h"

#include "../devices_header/led_and_switch.h"

#define LORAWAN_MAX_PAYLOAD_SIZE 120
#define LORAWAN_BUFFER_LENGTH 10

typedef struct {
	u1_t payload[LORAWAN_MAX_PAYLOAD_SIZE];
	u1_t length;
} payload_t;


/// LoRaWAN DevEUI, unique device ID (0xLSB, 0xxx, ......, 0xMSB)
static const u1_t DEVEUI[8]  = { 0x00,	0x06,	0x00,	0x00,	0x00,	0x00,	0x78,	0x82};      // Conduit
//static const u1_t DEVEUI[8]  = { 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x78, 0x82 };                //TTN

/// LoRaWAN Application identifier (0xLSB, 0xxx, ......, 0xMSB)
static const u1_t APPEUI[8]  = {  0x88,  0x99,	0x22,	0x55,	0x44,	0x22,	0x11,	0x00};      // Conduit
//static const u1_t APPEUI[8]  = { 0x94, 0xB4, 0x02, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };                // TTN

///DEVKEY (0xMSB, 0xxx, ......, 0xLSB i.e. normal format)
static const u1_t DEVKEY[16] = { 0x00,	0x11,	0x00,	0x22,	0x00,	0x33,	0x00,	0x44,	0x00,	0x55,	0x00,	0x66,	0x00,	0x77,	0x00,	0x88};      // Conduit
//static const u1_t DEVKEY[16] = { 0x5F, 0xB4, 0x13, 0xFD, 0xE3, 0x8F, 0x17, 0x23, 0xF5, 0xC1, 0xD6, 0x96, 0x30, 0xD4, 0x90, 0x47 };                                //TTN

/*
 * public variables
 */



/*
 * public functions
 */

/*
	@brief Get current LoRaWAN spreading factor (SF)

	@return (uint8_t) Spreading factor (SF)
*/
uint8_t lpwan_get_SF( void );

/*
	@brief Get current LoRaWAN transmit power.

	@return (uint8_t) Transmit power
*/
uint8_t lpwan_get_txPow( void );

/*
	@brief Check if device has joined LoRaWAN network.

	@return (bool) True if joined, false otherwise.
*/
bool	lpwan_isJoined( void );

/*
	@brief Check if last LoRaWAN transmission is complete.

	@return (bool) True if joined, false otherwise.
*/
bool	lpwan_isTXcomplete( void );

/*
	@brief Queue LoRaWAN uplink. Sent when LoRaWAN dutycycle allows.

	@param payload (payload_t*) pointer to payload struct.

	@param confirmed (u1_t) If 0: Unconfirmed uplink is enqueued.
	If 1: Confirmed uplink is enqueued.
*/
void	lpwan_queue_uplink(payload_t* payload, u1_t confirmed);

/*
	@brief Initialize LoRaWAN stack with LMiC.
	Also initialize Confirmed Uplink buffer.
*/
void	lpwan_init( void );

void	onEvent (ev_t ev);



#endif /* RESOURCE_MANAGERS_HEADER_LPWAN_MANAGER_H_ */
