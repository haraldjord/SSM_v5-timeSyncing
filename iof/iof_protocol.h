/*
 * iof.h
 *
 *  Created on: 2. feb. 2022
 *      Author: Jon Andreas Kornberg
 */

#ifndef IOF_IOF_PROTOCOL_H_
#define IOF_IOF_PROTOCOL_H_

#include <stdint.h>
#include <stdbool.h>

#include "../devices_header/tbr.h"
#include "../resource_managers_header/lpwan_manager.h"
#include "../devices_header/analog.h"
#include "../devices_header/ublox.h"

#define IOF_HEADER_LENGTH 6

/*
	@brief Build IOF frame if any new data packets to send.

	@param frame (uint8_t*) Pointer to memory where frame is to be written to.
	@param SLIMpacket (bool) If true: SLIM packet is added to frame. Also sets flag in IOF header.
	@param tbr_in_sync (bool) If false: sets flag in IOF header. 

	@return (uint8_t) Frame length in bytes, 0 if no data to be sent.
*/
uint8_t iof_build_frame(uint8_t* frame, bool SLIMpacket, bool tbr_in_sync);

/*
	@brief Check if last built frame has tag detection packet in it.

	@return (bool) True if tag detection packet is present,
	false otherwise.
*/
bool iof_frame_hasTagMsg( void );

#endif /* IOF_IOF_H_ */
