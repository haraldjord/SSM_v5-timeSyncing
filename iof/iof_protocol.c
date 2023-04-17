/*
 * iof.c
 *
 *  Created on: 2. feb. 2022
 *      Author: Jon Andreas Kornberg
 */

#include "iof_protocol.h"

extern uint32_t	iof_unix_ts;
extern pos_t last_pos;

static bool frame_hasTagMsg = false;

static uint32_t ref_ts = 0;

/*
	Private functions
*/

// uint8_t *off is pointer to offset byte in frame
static uint8_t add_IOFheader_to_frame(uint8_t *off, uint8_t headerFlag) {
	uint16_t tbr_SN = tbr_get_SN();

	off[0]	= (tbr_SN >> 6) & 0xFF;		// byte00
	off[1]	= (tbr_SN << 2) & 0xFF;		// byte01 bit 7 -> bit 2
	off[1] |= (headerFlag) & 0x3;		// byte01 bit 1 -> bit 0
	off[2]	= (ref_ts >> 24) & 0xFF;	// byte02
	off[3]	= (ref_ts >> 16) & 0xFF;	// byte03
	off[4]	= (ref_ts >> 8) & 0xFF;		// byte04
	off[5]	= (ref_ts) & 0xFF;			// byte05

	return 6; // IOFheader length
}

// uint8_t *off is pointer to offset byte in frame
static uint8_t add_SLIMpacket_to_frame(uint8_t *off) {
	pos_t p;
	memcpy(&p, &last_pos, sizeof(pos_t));

	// SLIM status: Battery status or air temperature
	static bool bat_info = true;
	bat_info = !bat_info;
	uint16_t bat_temp_value = 0;
	// battery status in SLIM status field
	if (bat_info) {
		bat_temp_value =  analog_read(BATTERY);
	}
	// air temperature in SLIM status field
	else {
		int16_t temp = 4096 + analog_read(TEMPERATURE)/10;
		if (temp < 0) temp = 0;
		else if (temp > 8191) temp = 8191;
		bat_temp_value = temp;
		bat_temp_value |= (1 << 13);
	}
	bat_temp_value &= 0x3FFF; // 14-bit value

	// longitude scaled with 10^7. 10 digits -1800,000,000 -> 1800,000,000 
	int32_t lon_int = p.longitude;
	// round to 8 digits
	if (lon_int < 0) {
		if ((lon_int % 100) <-49)	lon_int = (lon_int/100) - 1;
		else						lon_int = (lon_int/100);
	}
	else {
		if ((lon_int % 100) > 49)	lon_int = (lon_int/100) + 1;
		else						lon_int = (lon_int/100);
	}
	//add offset to get positive value
	uint32_t longitude = lon_int + 18000000;
	longitude &= 0x3FFFFFF; // 26-bit value

	// latitude scaled with 10^7. 9 digits -900,000,000 -> 900,000,000 
	int32_t lat_int = p.latitude;
	// round to 7 digits
	if (lat_int < 0) {
		if ((lat_int % 100) < -49)	lat_int = (lat_int/100) - 1;
		else						lat_int = (lat_int/100);
	}
	else {
		if ((lat_int % 100) > 49)	lat_int = (lat_int/100) + 1;
		else						lat_int = (lat_int/100);
	}
	//add offset to get positive value
	uint32_t latitude = lat_int + 9000000;
	latitude &= 0x1FFFFFF; // 25-bit value

	// PDOP scaled with 100. Allowed values 1 -> 1270 (4 digits)
	uint16_t PDOP = p.pDOP;
	if (PDOP > 1270) PDOP = 1270;
	// round to 3 digits
	if ((PDOP % 10) > 4)	PDOP = (PDOP/10) + 1;
	else					PDOP = (PDOP/10);
	PDOP &= 0x7F; // 7-bit value

	uint8_t		FIX				= p.fix & 0x7; // 3-bit value
	uint8_t		n_sat			= p.numSV;
	if (n_sat > 31) n_sat = 31;
	n_sat &= 0x1F; // 5-bit value

	off[0]	= (bat_temp_value >> 6) & 0xFF;		// byte00
	off[1]	= (bat_temp_value << 2) & 0xFF;		// byte01: bit 7 -> bit 2
	off[1] |= (longitude >> 24) & 0xFF;			// byte01: bit 1 -> bit 0
	off[2]	= (longitude >> 16) & 0xFF;			// byte02
	off[3]	= (longitude >> 8) & 0xFF;			// byte03
	off[4]	= (longitude) & 0xFF;				// byte04
	off[5]	= (PDOP << 1) & 0xFF;				// byte05: bit 7 -> bit 1
	off[5] |= (latitude >> 24) & 0xFF;			// byte05: bit 0
	off[6]	= (latitude >> 16) & 0xFF;			// byte06
	off[7]	= (latitude >> 8) & 0xFF;			// byte07
	off[8]	= (latitude) & 0xFF;				// byte08
	off[9]	= (FIX << 5) & 0xFF;				// byte09: bit 7 -> bit 5
	off[9] |= (n_sat) & 0xFF;					// byte09: bit 4 -> bit 0

	return 10; // SLIMpacket length
}

// uint8_t *off is pointer to offset byte in frame
static uint8_t add_TBRpackets_to_frame(uint8_t *off) {
	uint8_t 	len 			= 0;
	tbr_msg_t 	msg;

	bool first_tbr_msg = true; // to set the ref ts
	while(tbr_get_next_tbrMsg(&msg)) {

		// timestamp of first tbr msg is reference timestamp
		if (first_tbr_msg) {
			first_tbr_msg = false;
			ref_ts = msg.unix_ts;
		}

		// if message timestamp is before ref timestamp
		if (ref_ts > msg.unix_ts) {
			off[len++] = 0;								//byte00
		}
		else {
			off[len++] = msg.unix_ts - ref_ts;			//byte00
		}

		off[len++] = msg.codetype;						//byte01

		// TBR sensor packet
		if (msg.codetype == TBR_SENSOR_CODETYPE) {
			off[len++] = (msg.temperature >> 8) & 0xFF;	//byte02
			off[len++] = (msg.temperature) & 0xFF;		//byte03
			off[len++] = (msg.avgNoise) & 0xFF;			//byte04
			off[len++] = (msg.peakNoise) & 0xFF;		//byte05
			off[len++] = (msg.noiseFreq) & 0xFF;		//byte06
		}
		// TAG detection packet
		else {
			switch (msg.tagProt)
			{
			case TAG_PROTOCOL_R256:
				off[len++] = msg.tagID & 0xFF;				//byte02
				break;

			case TAG_PROTOCOL_R04K:
				off[len++] = (msg.tagID >> 8) & 0xFF;		//byte02
				off[len++] = (msg.tagID) & 0xFF;			//byte03
				break;

			case TAG_PROTOCOL_R64K:
				off[len++] = (msg.tagID >> 8) & 0xFF;		//byte02
				off[len++] = (msg.tagID) & 0xFF;			//byte03
				break;

			case TAG_PROTOCOL_R01M:
				off[len++] = (msg.tagID >> 16) & 0xFF;		//byte02
				off[len++] = (msg.tagID >> 8) & 0xFF;		//byte03
				off[len++] = (msg.tagID) & 0xFF;			//byte04
				break;

			case TAG_PROTOCOL_S256:
				off[len++] = (msg.tagID) & 0xFF;			//byte02
				off[len++] = (msg.tagData) & 0xFF;			//byte03
				break;

			case TAG_PROTOCOL_S64K:
				off[len++] = (msg.tagID >> 8) & 0xFF;		//byte02
				off[len++] = (msg.tagID) & 0xFF;			//byte03
				off[len++] = (msg.tagData) & 0xFF;			//byte04
				break;

			case TAG_PROTOCOL_HS256:
				off[len++] = (msg.tagID) & 0xFF;			//byte02
				off[len++] = (msg.tagData >> 8) & 0xFF;		//byte03
				off[len++] = (msg.tagData) & 0xFF;			//byte04
				break;

			case TAG_PROTOCOL_DS256:
				off[len++] = (msg.tagID) & 0xFF;			//byte02
				off[len++] = (msg.tagData >> 8) & 0xFF;		//byte03
				off[len++] = (msg.tagData) & 0xFF;			//byte04
				break;

			case TAG_PROTOCOL_OPi:
				off[len++] = (msg.tagID >> 16) & 0xFF;		//byte02
				off[len++] = (msg.tagID >> 8) & 0xFF;		//byte03
				off[len++] = (msg.tagID) & 0xFF;			//byte04
				break;

			case TAG_PROTOCOL_OPs:
				off[len++] = (msg.tagID >> 8) & 0xFF;		//byte02
				off[len++] = (msg.tagID) & 0xFF;			//byte03
				off[len++] = (msg.tagData >> 8) & 0xFF;		//byte04
				off[len++] = (msg.tagData) & 0xFF;			//byte05
				break;

			default:
				off[len++] = (msg.tagID >> 16) & 0xFF;		//byte02
				off[len++] = (msg.tagID >> 8) & 0xFF;		//byte03
				off[len++] = (msg.tagID) & 0xFF;			//byte04
				break;
			}
			off[len]	= (msg.SNR << 2) & 0xFF;		//byte(len - 2) bit 7 -> bit 2
			off[len++] |= (msg.millisec >> 8) & 0xFF;	//byte(len - 2) bit 1 -> bit 0
			off[len++]	= (msg.millisec) & 0xFF;		//byte(len - 1)

			frame_hasTagMsg = true;
		}
	}

	return len; // TAGpacket length
}

/*
	Public functions
*/
uint8_t iof_build_frame(uint8_t* frame, bool SLIMpacket, bool tbr_in_sync) {
	uint8_t frame_len = IOF_HEADER_LENGTH;
	frame_hasTagMsg = false;
	ref_ts = iof_unix_ts;

	if (SLIMpacket)	frame_len += add_SLIMpacket_to_frame(&frame[frame_len]);

	frame_len += add_TBRpackets_to_frame(&frame[frame_len]);
	
	if (frame_len == IOF_HEADER_LENGTH) {
		frame_len = 0;
	}
	else {
		uint8_t headerFlag = 0;

		// frame has one SLIM-packet after header
		if (SLIMpacket)		headerFlag |= (1<<0);
		// tbr msg timestamp may be offset as tbr is not synced
		if (!tbr_in_sync)	headerFlag |= (1<<1);

		add_IOFheader_to_frame(&frame[0], headerFlag);
	}
	return frame_len;
}

bool iof_frame_hasTagMsg( void ) {
	return frame_hasTagMsg;
}
