/*
 * ublox_gps.c
 *
 *  Created on: Apr 25, 2017
 *      Author: Waseemh
 *      Author: MariusSR
 * 
 * 	Edited: Spring 2022
 * 		Author: Jon Andreas Kornberg
 */

#include "../devices_header/ublox.h"
#include "../devices_header/ublox_msg.h"

/*
 * private variables
 */
static ubx_msg_t ubx_msgs[UBX_MAX_BUFFER_LENGTH];
static struct fifo_descriptor fifo_ubx_msgs_s;
static fifo_t fifo_ubx_msgs = &fifo_ubx_msgs_s;

static sat_data_t sat_data = {0,0};
static nav_data_t nav_data = {.valid_time = false, .valid_pos = false, .fix = 0, .numSV = 0};
/*
 * private functions
 */
static void rx_ubx_msgs( void ) {
	uint8_t c = 0;

	// countes consecutive 0xFF bytes to prevent abortion of msg containing 0xFF
	uint8_t consecxFF = 0;

	spi_cs_clear(gps);
	while(c != 0xFF && consecxFF < 10) {
		if (c == 0xFF) consecxFF++;
		// Check second sync-byte
		c = spi_read_byte();
		if (c != SYNCH_1) break;
		// Check second sync-byte
		c = spi_read_byte();
		if (c != SYNCH_2) break;

		ubx_msg_t msg;
		msg.ubx_class 	= spi_read_byte();
		msg.ubx_id 		= spi_read_byte();

		msg.length		= spi_read_byte();
		msg.length 	   |= spi_read_byte() << 8;

		if (msg.length > UBX_MAX_PAYLOAD_SIZE) {
			// discard msg
			for (uint16_t i = 0; i < msg.length; i++) {
				spi_read_byte();
			}
		}
		else {
			for (uint16_t i = 0; i < msg.length; i++) {
				msg.payload[i] = spi_read_byte();
			}
			fifo_add(fifo_ubx_msgs, &msg);
		}
		// discard checksum
		spi_dummy_read_n_byte(2);

		// check if new message in while condition
		c = spi_read_byte();
	}
	spi_cs_set(gps);
}

static bool send_cmd_rx_ack(uint8_t const *cmd, uint8_t cmd_size) {
    uint8_t ubx_class = cmd[2];
    uint8_t ubx_id    = cmd[3];
	bool ack = false;

	spi_cs_clear(gps);
	for(uint8_t i = 0; i < cmd_size; i++) {
		spi_write_byte(cmd[i]);
	}
	spi_cs_set(gps);
	delay_ds(1);
	rx_ubx_msgs();

	ubx_msg_t msg;
	while(fifo_get(fifo_ubx_msgs, &msg)) {
		if (msg.ubx_class == ACK && msg.ubx_id == ACK_ACK && msg.payload[0] == ubx_class && msg.payload[1] == ubx_id) ack = true;
	}

    return ack;
}


static void send_cmd_no_validation(uint8_t const *cmd, uint8_t cmd_size) {

    spi_cs_clear(gps);
	for(uint8_t i = 0; i < cmd_size; i++) {
		spi_write_byte(cmd[i]);
	}
	spi_cs_set(gps);
}

static void fletcher16(uint8_t *data, uint16_t size, uint8_t *crc_a, uint8_t *crc_b) {
	uint8_t a = 0;
	uint8_t b = 0;

	for (uint16_t i = 0; i < size; i++) {
		a += data[i];
		b += a;
	}
	*crc_a = a;
	*crc_b = b;
}

static uint32_t utc_time_to_unix_time(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec) {
	const short days_since_beginning_of_year[12] = {0,31,59,90,120,151,181,212,243,273,304,334};

	uint8_t leap_years = ((year-1)-1968)/4 - ((year-1)-1900)/100 + ((year-1)-1600)/400;

	uint32_t days_since_1970 = (year-1970)*365 + leap_years + days_since_beginning_of_year[month-1] + day-1;

	if ( (month > 2) && (year%4==0) && (year%100!=0 || year%400==0)) days_since_1970 += 1;

	return sec + 60*(min + 60*(hour + 24*days_since_1970));
}

static bool config_clear( void ) {
	uint8_t cmd[20];
	memcpy(cmd, UBX_CFG_CFG_CLEAR, 20);

	fletcher16(&cmd[2], 16, &cmd[18], &cmd[19]);

	return send_cmd_rx_ack(cmd, sizeof(cmd));
}

static bool config_save( void ) {
	uint8_t cmd[20];
	memcpy(cmd, UBX_CFG_CFG_SAVE, 20);

	fletcher16(&cmd[2], 16, &cmd[18], &cmd[19]);

	return send_cmd_rx_ack(cmd, sizeof(cmd));
}

static bool config_port( void ) {
	uint8_t cmd[28];
	memcpy(cmd, UBX_CFG_PRT, 28);

	fletcher16(&cmd[2], 24, &cmd[26], &cmd[27]);

	return send_cmd_rx_ack(cmd, sizeof(cmd));
}

static bool config_gnss( void ) {
	uint8_t cmd[20];
	memcpy(cmd, UBX_CFG_GNSS, 20);

	fletcher16(&cmd[2], 16, &cmd[18], &cmd[19]);

	return send_cmd_rx_ack(cmd, sizeof(cmd));
}

static bool config_pm2( void ) {
	uint8_t cmd[56];
	memcpy(cmd, UBX_CFG_PM2, 56);

	fletcher16(&cmd[2], 52, &cmd[54], &cmd[55]);

	return send_cmd_rx_ack(cmd, sizeof(cmd));
}

static bool config_rxm( void ) {
	uint8_t cmd[10];
	memcpy(cmd, UBX_CFG_RXM, 10);

	fletcher16(&cmd[2], 6, &cmd[8], &cmd[9]);

	return send_cmd_rx_ack(cmd, sizeof(cmd));
}

static bool config_nav5( void ) {
	uint8_t cmd[44];
	memcpy(cmd, UBX_CFG_NAV5, 44);

	fletcher16(&cmd[2], 40, &cmd[42], &cmd[43]);

	return send_cmd_rx_ack(cmd, sizeof(cmd));
}

static bool config_navx5( void ) {
	uint8_t cmd[48];
	memcpy(cmd, UBX_CFG_NAVX5, 48);

	fletcher16(&cmd[2], 44, &cmd[46], &cmd[47]);

	return send_cmd_rx_ack(cmd, sizeof(cmd));
}

static void parse_nav_sat_payload(uint8_t *payload) {
	uint8_t numSV = payload[5];
	uint8_t numSV_40dBHz = 0;
	for (uint8_t SV = 0; SV < numSV; SV++) {
		if (payload[10 + 12*SV] > 40) numSV_40dBHz++;
	}
	sat_data.numSV = numSV;
	sat_data.numSV_40dBHz = numSV_40dBHz;
}

static bool poll_nav_sat( void ) {

	// check if nav_sat msg is recevied.
	// The reception will not succeed first time poll_nav_sat is called for a while.
	// but removes delay_ds(1)
	rx_ubx_msgs();	
	ubx_msg_t msg;
	while(fifo_get(fifo_ubx_msgs, &msg)) {
		if (msg.ubx_class == NAV && msg.ubx_id == NAV_SAT) {
			parse_nav_sat_payload(msg.payload);
			return true;
		}
	}

	uint8_t cmd[8];
	memcpy(cmd, UBX_NAV_SAT, 8);

	fletcher16(&cmd[2], 4, &cmd[6], &cmd[7]);
	send_cmd_no_validation(cmd, sizeof(cmd));
    
	return false;
}

static void parse_nav_pvt_payload(uint8_t *payload) {
	// see p. 375 in u-blox M8 Receiver description - Manual

	// if GNSSfixOK
	if (payload[21] & 0x01) {
		nav_data.valid_pos	= true;
		nav_data.fix		= payload[20];
		nav_data.longitude	= payload[24] | (payload[25] << 8) | (payload[26] << 16) | (payload[27] << 24);
		nav_data.latitude	= payload[28] | (payload[29] << 8) | (payload[30] << 16) | (payload[31] << 24);
		nav_data.pDOP		= payload[76] | (payload[77] << 8);

		// if time valid
		if (payload[11] & 0x7) {
			nav_data.valid_time	= true;
			nav_data.year		= payload[4] | (payload[5] << 8);
			nav_data.month		= payload[6];
			nav_data.day		= payload[7];
			nav_data.hour		= payload[8];
			nav_data.min		= payload[9];
			nav_data.sec		= payload[10];
		}
		else {
			nav_data.valid_time = false;
		}
	}
	else {
		nav_data.valid_pos	= false;
	}

	nav_data.numSV		= payload[23];
}

static bool poll_nav_pvt( void ) {

	// check if nav_pvt msg is recevied.
	// The reception will probably not succeed first time poll_nav_pvt is called for a while.
	// but removes delay_ds(1)
	rx_ubx_msgs();
	ubx_msg_t msg;
	while(fifo_get(fifo_ubx_msgs, &msg)) {
		if (msg.ubx_class == NAV && msg.ubx_id == NAV_PVT) {
			parse_nav_pvt_payload(msg.payload);
			return true;
		}
	}

	uint8_t cmd[8];
	memcpy(cmd, UBX_NAV_PVT, 8);

	fletcher16(&cmd[2], 4, &cmd[6], &cmd[7]);
	send_cmd_no_validation(cmd, sizeof(cmd));

    return false;
}

static bool parse_nav_status_payload(uint8_t *payload, nav_status_data_t *status_data) {
	// see p. 388 in u-blox M8 Receiver description - Manual
	// if time valid
	if (payload[5] & 0x1) {
		status_data->fix	= payload[4];
		status_data->ttff	= payload[8];
		status_data->ttff	|= payload[9] << 8;
		status_data->ttff	|= payload[10] << 16;
		status_data->ttff	|= payload[11] << 24;
		return true;
	}
	else {
		return false;
	}
}


static bool poll_nav_status(nav_status_data_t *status_data) {

	// check if nav_status msg is recevied.
	// The reception will probably not succeed first time poll_nav_status is called for a while.
	// but removes delay_ds(1)
	rx_ubx_msgs();
	ubx_msg_t msg;
	while(fifo_get(fifo_ubx_msgs, &msg)) {
		if (msg.ubx_class == NAV && msg.ubx_id == NAV_STATUS) {
		    return parse_nav_status_payload(msg.payload, status_data);
		}
	}

    uint8_t cmd[8];
	memcpy(cmd, UBX_NAV_STATUS, 8);

	fletcher16(&cmd[2], 4, &cmd[6], &cmd[7]);
	send_cmd_no_validation(cmd, sizeof(cmd));

	return false;
}


/*
* public functions
*/
bool gnss_init( void ) {
	debug_str("GNSS init\n");
	spi_init();

	fifo_create_static(fifo_ubx_msgs, ubx_msgs, UBX_MAX_BUFFER_LENGTH, sizeof(ubx_msg_t));
	GPIO_PinModeSet(GPS_SIG_PORT, GPS_INT, gpioModePushPull, 0);
	gnss_force_ON();

    /// Reset non-volatile configurations to default, and load the configurations into the current config
	while(!config_clear());

    /// Configure ports
	while(!config_port());

	/// Configure GNSS
	//while (!( config_gnss() )){} on by default

	/// Configure NAV5
	while(!config_nav5());
	//while(!config_navx5());

	/// Configure manual duty-cycle with EXTINT, On/Off mode
	while(!config_pm2());
	/// Power Save Mode
	while(!config_rxm());

	/// Save config
	while(!config_save());
    return true;
}

void gnss_clearAll_ubxMsgs( void ) {
	rx_ubx_msgs();
	ubx_msg_t dummy;
	while(fifo_get(fifo_ubx_msgs, &dummy));
}

void gnss_clear_validData( void ) {
	nav_data.valid_pos = false;
	nav_data.valid_time = false;
}

bool gnss_poll_navdata( void ) {
	return poll_nav_pvt();
}

bool gnss_is_validPosData( void ) {
	return nav_data.valid_pos;
}

bool gnss_is_validTimeData( void ) {
	return nav_data.valid_time;
}

void gnss_get_posData(pos_t *data) {
	data->latitude	= nav_data.latitude;
	data->longitude	= nav_data.longitude;
	data->pDOP		= nav_data.pDOP;
	data->fix		= nav_data.fix;
	data->numSV		= nav_data.numSV;
}

uint32_t gnss_get_timeData( void ) {
	return utc_time_to_unix_time(nav_data.year, nav_data.month, nav_data.day, nav_data.hour, nav_data.min, nav_data.sec);
}

void gnss_force_ON( void ) {
	debug_str("GNSS: Force ON\n");
	GPIO_PinOutSet(GPS_SIG_PORT, GPS_INT);
}

void gnss_force_OFF( void ) {
	debug_str("GNSS: Force OFF\n");
	GPIO_PinOutClear(GPS_SIG_PORT, GPS_INT);
}
