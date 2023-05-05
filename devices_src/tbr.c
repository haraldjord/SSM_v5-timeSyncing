/*
 * tbr.c
 *
 *  Created on: Mar 30, 2017
 *      Author: Waseemh
 * 
 *  Edited: Spring 2022
 *      Author: Jon Andreas Kornberg
 */


#include "../devices_header/tbr.h"

/*
 * shared variables
 */
extern uint8_t node_id;
extern bool tbr_basic_sync_msg;
extern bool tbr_advance_sync_msg;
extern bool tbr_connected;
extern bool tbr_in_sync;

/*
 * private variables
 */
static struct fifo_descriptor fifo_tbr_msgs_s;
static fifo_t fifo_tbr_msgs = &fifo_tbr_msgs_s;
static tbr_msg_t tbr_msgs[TBR_FIFO_LENGTH];
static tbr_msg_t msg = {.codetype = 0};
static uint8_t tagDetection_buf[1024];

static uint16_t tbrSN = 0;
static bool is_rx_SN = false;
static bool ack01 = false;
static bool ack02 = false;


/*
 * private functions
 */
static uint8_t calculateLuhnDigit(uint32_t time) {
	uint8_t timeStampArray[9];
	uint8_t luhn_sum = 0;
	/* Make an array of digits of the timestamp */
	for (int i = 8; i >= 0; i--){
		uint8_t digit = time % 10;
		timeStampArray[i] = digit;
		time /= 10;
	}

	/* Calculate luhn sum */
	for (int i = 0; i < 9; ++i) {
		if (i % 2 == 0) timeStampArray[i] *= 2;
		if (timeStampArray[i] > 9) luhn_sum += timeStampArray[i] - 9;
		else luhn_sum += timeStampArray[i];
	}

	return ((10 - (luhn_sum % 10)) % 10);
}

static void debug_rs485_buffer(char * str) {
	debug_str("TBR: RS485 buffer: \"");
	for (int i = 0; i< 1024; i++) {
		if (str[i] == 0) break;
		else if (str[i] == '\r') {
			debug_str("\\r");
		}
		else {
			debug_char(str[i]);
		}
	}
	debug_str("\"\n");
}
/*
static char rs485_recieve_char_TEST( void ) {
	static int i = 0;

	static const char test_str[] =
	"SN=000632 ><>\r\0$000632,1647960808,999,R256,255,,30,69,2467\r$000632,1647960868,999,R04K,4095,,31,70,2468\r$000632,1647960900,945,R64K,65535,,32,71,2469\r$000632,1647960960,999,S256,255,255,33,72,2470\r$000632,1647961020,945,R01M,1048575,,34,73,2471\r$000632,1647961080,945,S64K,65535,255,35,74,2472\r$000632,1647961140,945,HS256,255,65535,36,75,2473\r$000632,1647961200,945,DS256,255,65535,37,76,2474\r$000632,1647961260,945,OPi,1048575,,38,77,2475\r$000632,1647961320,945,OPs,65535,4095,39,63,2476\r$000632,1647961350,TBR Sensor,550,255,255,77,2477\r\0\0\0\0\0\0\0\0\0"
	;
	char c = test_str[i];
	if (++i >= 550) i = 0;

	return c;
}
*/
static uint8_t resolve_tagCodetype(uint8_t tagProt, uint8_t tagFreq) {
	uint8_t codetype = tagProt;
	int8_t diff_freq = (int8_t)tagFreq - 69;

	if (diff_freq > 0) {
		codetype = tagProt + 16*diff_freq;
	}
	else if (diff_freq < 0) {
		codetype = tagProt + 144 + (tagFreq - 63)*16;
	}

	return codetype;
}

static void parse_tbr_buf(void){ // prepare buffer to be sendt over rs-232
  // add or remove data here:

  /*// general data
    bool newTagDetection;
  uint8_t   codetype; // resolved from <tagProt> and <tagFreq>, or TBR_SENSOR_CODETYPE
    uint16_t    tbrSN;
    uint32_t    unix_ts;
  uint32_t  memAddr;

  // tag specific data
    uint16_t    millisec;
    uint8_t     tagProt;
    uint32_t    tagID;
    uint16_t    tagData;
    uint8_t     SNR;
  uint8_t   tagFreq;

  // TBR sensor specific data
    uint16_t    temperature;
    uint8_t     avgNoise;
    uint8_t     peakNoise;
    uint8_t     noiseFreq;
   */

  //TAG msg format to be send: "$<tbrSN>,<unix_ts>,<millisec>,<tagProt>,<tagID>,<tagData>,<SNR>,<tagFreq>,<memAddr>\r"

  if (tbr_connected && tbr_in_sync && tbr_advance_sync_msg && msg.newTagDetection){
        tbr_advance_sync_msg = false;
        sprintf(tagDetection_buf, "ID:%d, tbrSN: %d , timestamp: %lu,%u tagProt: %u, tagID:%lu, tagData:%u, SNR:%u, tagFreq:%u, memAddr:%lu, Info: Advance time sync successed!\n", node_id, msg.tbrSN, msg.unix_ts, msg.millisec, msg.tagProt, msg.tagID, msg.tagData, msg.SNR, msg.tagFreq, msg.memAddr);
        debug_str(tagDetection_buf);
  }
  else if (tbr_connected && tbr_in_sync && tbr_basic_sync_msg && msg.newTagDetection){
      tbr_basic_sync_msg = false;
      sprintf(tagDetection_buf, "ID:%d, tbrSN: %d , timestamp: %lu,%u tagProt: %u, tagID:%lu, tagData:%u, SNR:%u, tagFreq:%u, memAddr:%lu, Info: Basic time sync successed!\n", node_id, msg.tbrSN, msg.unix_ts, msg.millisec, msg.tagProt, msg.tagID, msg.tagData, msg.SNR, msg.tagFreq, msg.memAddr);
      debug_str(tagDetection_buf);
  }
  else if (msg.newTagDetection) {
      sprintf(tagDetection_buf, "ID:%d, tbrSN: %d , timestamp: %lu,%u tagProt: %u, tagID:%lu, tagData:%u, SNR:%u, tagFreq:%u, memAddr:%lu, Info: []\n", node_id, msg.tbrSN, msg.unix_ts, msg.millisec, msg.tagProt, msg.tagID, msg.tagData, msg.SNR, msg.tagFreq, msg.memAddr);
      debug_str(tagDetection_buf);
  }




  //sprintf(tagDetection_buf, "ID:%d, tbrSN: %d , timestamp: %lu,%u tagProt: %u, tagID:%lu, tagData:%u, SNR:%u, tagFreq:%u, memAddr:%lu\n", node_id, msg.tbrSN, msg.unix_ts, msg.millisec, msg.tagProt, msg.tagID, msg.tagData, msg.SNR, msg.tagFreq, msg.memAddr);
  //debug_str(tagDetection_buf);

}

static void parse_tbr_msg(char* msg_str){

	char tagProt_str[16] = "TAG_PROTOCOL";
	char msgProperty[9][16];
	char endChar;

	bool broken_msg = true;

	/*
		TBR sensor reading
		TBR Sensor msg: "$<tbrSN>,<unix_ts>,TBR Sensor,<temperature>,<avgNoise>,<peakNoise>,<noiseFreq>,<memAddr>\r"
		EXAMPLE: "$001234,1527073500,TBR Sensor,323,10,16,69,2469\r"
	*/
	if (8 == sscanf(msg_str, "$%6[^,],%10[^,],TBR Sensor,%3[^,],%3[^,],%3[^,],%2[^,],%7[^,$\r]%1c", msgProperty[0], msgProperty[1], msgProperty[2], msgProperty[3], msgProperty[4], msgProperty[5], msgProperty[6], &endChar)) {
		if (endChar == '\r') {
			broken_msg = false;

			msg.codetype	= TBR_SENSOR_CODETYPE;
			msg.tbrSN		= strtoul(msgProperty[0], NULL, 10);
			msg.unix_ts		= strtoul(msgProperty[1], NULL, 10);
			msg.temperature	= strtoul(msgProperty[2], NULL, 10);
			msg.avgNoise	= strtoul(msgProperty[3], NULL, 10);
			msg.peakNoise	= strtoul(msgProperty[4], NULL, 10);
			msg.noiseFreq	= strtoul(msgProperty[5], NULL, 10);
			msg.memAddr		= strtoul(msgProperty[6], NULL, 10);
		}
	}
	/*
		Tag detection with data
		TAG msg: "$<tbrSN>,<unix_ts>,<millisec>,<tagProt>,<tagID>,<tagData>,<SNR>,<tagFreq>,<memAddr>\r"
		EXAMPLE: "$001234,1527073474,945,S256,241,150,32,69,2467\r"
	*/
	else if (10 == sscanf(msg_str, "$%6[^,],%10[^,],%3[^,],%6[^,],%7[^,],%5[^,],%2[^,],%2[^,],%7[^,$\r]%1c", msgProperty[0], msgProperty[1], msgProperty[2], msgProperty[3], msgProperty[4], msgProperty[5], msgProperty[6], msgProperty[7], msgProperty[8], &endChar)) {
		if (endChar == '\r') {
			broken_msg = false;

			msg.tbrSN	= strtoul(msgProperty[0], NULL, 10);
			msg.unix_ts	= strtoul(msgProperty[1], NULL, 10);
			msg.millisec= strtoul(msgProperty[2], NULL, 10);
			strcpy(tagProt_str, msgProperty[3]);
			msg.tagID	= strtoul(msgProperty[4], NULL, 10);
			msg.tagData	= strtoul(msgProperty[5], NULL, 10);
			msg.SNR		= strtoul(msgProperty[6], NULL, 10);
			msg.tagFreq	= strtoul(msgProperty[7], NULL, 10);
			msg.memAddr	= strtoul(msgProperty[8], NULL, 10);

			msg.newTagDetection = true;
		}
	}
	/*
		Tag detection without data
		TAG msg: "$<tbrSN>,<unix_ts>,<millisec>,<tagProt>,<tagID>,,<SNR>,<tagFreq>,<memAddr>\r"
		EXAMPLE: "$001234,1527073474,945,R256,241,,32,69,2467\r"
	*/
	else if (9 == sscanf(msg_str, "$%6[^,],%10[^,],%3[^,],%6[^,],%7[^,],,%2[^,],%2[^,],%7[^,$\r]%1c", msgProperty[0], msgProperty[1], msgProperty[2], msgProperty[3], msgProperty[4], msgProperty[6], msgProperty[7], msgProperty[8], &endChar)) {
		if (endChar == '\r') {
			broken_msg = false;

			msg.tbrSN	= strtoul(msgProperty[0], NULL, 10);
			msg.unix_ts	= strtoul(msgProperty[1], NULL, 10);
			msg.millisec= strtoul(msgProperty[2], NULL, 10);
			strcpy(tagProt_str, msgProperty[3]);
			msg.tagID	= strtoul(msgProperty[4], NULL, 10);
			// no tagData
			msg.SNR		= strtoul(msgProperty[6], NULL, 10);
			msg.tagFreq	= strtoul(msgProperty[7], NULL, 10);
			msg.memAddr	= strtoul(msgProperty[8], NULL, 10);

			msg.newTagDetection = true;
		}
	}


	if (broken_msg) {
		debug_str("\nTBR: Broken msg from TBR\n");
		return;	
	}

	// resolve protocol/code type if tag detection
	if (msg.codetype != TBR_SENSOR_CODETYPE) {
		if 		(strncmp(tagProt_str, "R256", 4) == 0) msg.tagProt = TAG_PROTOCOL_R256;
		else if (strncmp(tagProt_str, "R04K", 4) == 0) msg.tagProt = TAG_PROTOCOL_R04K;
		else if (strncmp(tagProt_str, "R64K", 4) == 0) msg.tagProt = TAG_PROTOCOL_R64K;
		else if (strncmp(tagProt_str, "S256", 4) == 0) msg.tagProt = TAG_PROTOCOL_S256;
		else if (strncmp(tagProt_str, "R01M", 4) == 0) msg.tagProt = TAG_PROTOCOL_R01M;
		else if (strncmp(tagProt_str, "S64K", 4) == 0) msg.tagProt = TAG_PROTOCOL_S64K;
		else if (strncmp(tagProt_str, "HS256", 5) == 0) msg.tagProt = TAG_PROTOCOL_HS256;
		else if (strncmp(tagProt_str, "DS256", 5) == 0) msg.tagProt = TAG_PROTOCOL_DS256;
		else if (strncmp(tagProt_str, "OPi", 3) == 0) msg.tagProt = TAG_PROTOCOL_OPi;
		else if (strncmp(tagProt_str, "OPs", 3) == 0) msg.tagProt = TAG_PROTOCOL_OPs;
		else {
			sprintf(debug_str_buf, "TBR: Unknown tag protocol: \"%s\"\n", tagProt_str);
			debug_str(debug_str_buf);
			return;
		}

		msg.codetype = resolve_tagCodetype(msg.tagProt, msg.tagFreq);
	}

	fifo_add(fifo_tbr_msgs, &msg);
	tbrSN = msg.tbrSN;
}


static void parse_tbr_SN(char* SN_str) {
	if ( (strncmp(SN_str, "SN=", 3) == 0) && (strncmp(&SN_str[10], "><>", 3) == 0) ) {
		if (sscanf(SN_str, "SN=%6hu ><>", &tbrSN)) {
			is_rx_SN = true;
		}
	}
}

static void parse_tbr_ack(char* ack_str) {
	if (strncmp(ack_str, "ack01", 5) == 0) ack01 = true;
	else if (strncmp(ack_str, "ack02", 5) == 0) ack02 = true;
}

//static void parse_rs485_buffer( void ) {
void parse_rs485_buffer( void ) {
  static char tbr_rx_buf[1024];
	tbr_rx_buf[1023] = 0; // null terminated string
	uint16_t len = 0;
	char c = 1;

	// while rs485 buffer is not empty
	while(c != 0) {
		c = rs485_recieve_char();
		if (len < 1023) {
			tbr_rx_buf[len++] = c;
		}
	}

	if (len > 1) {
		debug_rs485_buffer(tbr_rx_buf);
	}
	else {
		return;
	}

	uint16_t i = 0;
	while (i < len) {
		c = tbr_rx_buf[i];
		switch (c)
		{
			case '$': // New tag detection
				parse_tbr_msg(&tbr_rx_buf[i]);
				break;

			case 'S': // acknowledge after ping
				parse_tbr_SN(&tbr_rx_buf[i]);
				break;

			case 'a': // acknowledge of basic/advance sync
				parse_tbr_ack(&tbr_rx_buf[i]);
				break;
		}
		i++;
	}
}

/*
 * public functions
 */
void tbr_init(void) {
	debug_str("TBR init\n");
	fifo_create_static(fifo_tbr_msgs, tbr_msgs, TBR_FIFO_LENGTH, sizeof(tbr_msg_t));
	rs485_init();
	rs485_enable();
}

bool tbr_get_next_tbrMsg(tbr_msg_t* msg) {
	if (fifo_get(fifo_tbr_msgs, msg)) return true;
	else parse_rs485_buffer();

	return fifo_get(fifo_tbr_msgs, msg);
}

uint32_t tbr_get_SN( void ) {
	return tbrSN;
}

void tbr_ping( void ) {
	is_rx_SN = false;
	rs485_transmit_string("?", 1);
}

bool tbr_ping_is_rx_SN( void ) {
	if (is_rx_SN) return true;
	else parse_rs485_buffer();

	return is_rx_SN;
}

void tbr_basicTimeSync( void ) {
	ack01 = false;
	rs485_transmit_string("(+)", 3);
}

void tbr_advancedTimeSync(uint32_t t) {
	char str[16];
	t /= 10;
	sprintf(str, "(+)%9lu%1hu", t, calculateLuhnDigit(t));

	ack02 = false;
	rs485_transmit_string(str, 13);
}

bool tbr_is_ack01( void ) {
	if (ack01) return true;
	else parse_rs485_buffer();

	return ack01;
}

bool tbr_is_ack02( void ) {
	if (ack02) return true;
	else parse_rs485_buffer();

	return ack02;
}



void sendTBR_string(void){

  if (!tbr_connected){
      sprintf(tagDetection_buf, "ID:%d Not connected to TBR!\n", node_id);
      debug_str(tagDetection_buf);
  }else if (tbr_connected && !tbr_in_sync){
      sprintf(tagDetection_buf, "ID:%d no timesyncing\n!");
      debug_str(tagDetection_buf);
  }

  while(tbr_get_next_tbrMsg(&msg));{
      if (msg.newTagDetection == true){
      parse_tbr_buf();
      msg.newTagDetection = false;
      }
  }
}
