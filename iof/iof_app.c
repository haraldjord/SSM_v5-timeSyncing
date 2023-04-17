/*
 * iof_app.c
 *
 *  Created on: 10. feb. 2022
 *      Author: Jon Andreas Kornberg
 */

#include "iof_app.h"

#define LFXO_FREQUENCY		32768
#define WAKEUP_INTERVAL_SEC	1
#define BURTC_TOP			((LFXO_FREQUENCY * WAKEUP_INTERVAL_SEC) - 1)

/*
 * Shared variables
 */

osjob_t app_job;
uint8_t node_id = 0xFF;
pos_t last_pos = {.fix = 0, .numSV = 0};
uint32_t iof_unix_ts = 0;

/*
 * Private variables
 */

static osjob_t gnss_job;
static osjob_t tbr_job;
static osjob_t blink_job;
static osjob_t display_job;

// TBR
static bool tbr_connected = false;
static bool tbr_in_sync = false;
static bool tbr_do_sync = false;

static bool send_slimData = true;

// GNSS
static bool gnss_acquisition = false;

static void iof_update_display(osjob_t *j) {
	display_clear_main_area();

	// GNSS
	if (gnss_acquisition) {
		display_put_string(2, 18, "GNSS:\n  Searching", font_medium);
	}
	else {
		sprintf(display_str_buf, "GNSS:\n FIX:%1u SV:%2u", last_pos.fix, last_pos.numSV);
		display_put_string(2, 18, display_str_buf, font_medium);
	}
	display_draw_horisontal_whole_line(46);

	// LoRaWAN
	if (lpwan_isJoined()) {
		sprintf(display_str_buf, "LoRa: Joined\n SF%u TxPow: %u", lpwan_get_SF(), lpwan_get_txPow());
		display_put_string(2, 50, display_str_buf, font_medium);
	}
	else {
		display_put_string(2, 50, "LoRa: Joining", font_medium);
	}
	display_draw_horisontal_whole_line(78);

	// TBR
	if (tbr_connected) {
		display_put_string(2, 82, "TBR: Connected", font_medium);
		if (tbr_in_sync) {
			display_put_string(2, 98, " TIMESYNC OK", font_medium);
		}
		else {
			display_put_string(2, 98, " NO TIMESYNC", font_medium);
		}
	}
	else {
		display_put_string(2, 82, "TBR:\n NOT connected", font_medium);
	}

	display_update();
}

static void LEDs_clear( osjob_t *j) {

	// GNSS
	status_led_gps(false);

	// LoRaWAN
	if (!lpwan_isJoined()) {
		status_led_radio(false);
	}

	// Status LED
	set_status_led(false, false);
}

static void LEDs_set( osjob_t *j) {

	// GNSS
	if (gnss_acquisition) {
		status_led_gps(true);
	}

	// LoRaWAN
	if (!lpwan_isJoined()) {
		status_led_radio(true);
	}

	// Status LED
	if (tbr_connected){
		set_status_led(false, true);
	}
	else {
		set_status_led(true, false);
	}
	os_setTimedCallback(&blink_job, os_getTime() + ms2osticks(100), LEDs_clear);
}

static void poll_navdata( osjob_t *j ) {
	static uint8_t retries = 0;
	static uint32_t search_ts = 0;
	if (retries == 0) {
		search_ts = iof_unix_ts;
		debug_str("IOF: GNSS starting search...\n");
		gnss_force_ON();
		gnss_clearAll_ubxMsgs();
		gnss_clear_validData();

		gnss_acquisition = true;
	}

	// if success OR more than 120 retries -> stop polling
	gnss_poll_navdata();
	if (gnss_is_validPosData() && gnss_is_validTimeData()) {
		sprintf(debug_str_buf, "IOF: GNSS fix acquired! Time to fix: %lu. Retries: %u\n", iof_unix_ts - search_ts, retries);
		debug_str(debug_str_buf);
		gnss_force_OFF();

		// UNIX Time
		iof_unix_ts = 1 + gnss_get_timeData(); // data is 1 second old
		tbr_do_sync = true;

		// POSITION
		gnss_get_posData(&last_pos);

		retries = 0;
		os_clearCallback(j);

		gnss_acquisition = false;
	}
	else if (retries >= 120) {
		gnss_force_OFF();
		debug_str("IOF: GNSS search timeout...\n");
		retries = 0;
		os_clearCallback(j);

		last_pos.fix = 0;
		last_pos.numSV = 0;
		gnss_acquisition = false;
	}
	// else continue polling
	else {
		os_setTimedCallback(j, os_getTime() + sec2osticks(1), poll_navdata);
		retries++;
	}
}

static void ping_tbr_rx_SN( osjob_t *j ) {
	tbr_connected = tbr_ping_is_rx_SN();
	if (tbr_connected) debug_str("IOF: TBR ping success\n");
	else debug_str("IOF: TBR ping failed\n");
}

static void ping_tbr( osjob_t *j ) {
	tbr_ping();

	// schedule SN reception
	os_setTimedCallback(j, os_getTime() + sec2osticks(1), ping_tbr_rx_SN);
}

static void sync_tbr_is_ack( osjob_t *j ) {
	if (tbr_is_ack02()) {
		tbr_in_sync = true;
		tbr_connected = true;
		debug_str("IOF: Advanced time sync of TBR successed\n");
	}
	else {
		tbr_in_sync = false;
		debug_str("IOF: Advanced time sync of TBR FAILED!\n\n");
	}
}

static void sync_tbr( osjob_t *j ) {
	tbr_advancedTimeSync(iof_unix_ts);

	// schedule ack reception
	os_setTimedCallback(j, os_getTime() + sec2osticks(1), sync_tbr_is_ack);
}

static void time_manager_init( void ) {

	RMU_ResetControl(rmuResetBU, rmuResetModeClear);
    /////////////BURTC ////////////
    // Setup and initialize BURTC
    BURTC_Init_TypeDef	burtc_init = BURTC_INIT_DEFAULT;
    burtc_init.enable = false;
    burtc_init.clkSel = burtcClkSelLFXO;
    burtc_init.compare0Top = true;
    burtc_init.mode = burtcModeEM4;
    BURTC_Reset();
    BURTC_Init(&burtc_init);
    BURTC_CompareSet(0, BURTC_TOP);
    BURTC_IntDisable(_BURTC_IF_MASK);
    BURTC_IntEnable(BURTC_IF_COMP0);
	NVIC_ClearPendingIRQ(BURTC_IRQn);
	NVIC_EnableIRQ(BURTC_IRQn);

	// Start BURTC
    BURTC_Enable(true);
}

void iof_app_init( osjob_t *j ) {
	init_led_switch();
	node_id = read_switch();
	set_status_led(true,false);

	// print header to display
	display_init();	
	display_clear();
	display_put_string(8*4, 2, "IOF", font_medium);
	sprintf(display_str_buf, "ID:%3u", node_id);
	display_put_string(10*8, 4, display_str_buf, font_small);
	display_draw_horisontal_whole_line(14);
	display_update();

	// TBR
	tbr_init();
	os_setCallback(&tbr_job, ping_tbr);

	// GNSS/GPS
	gnss_init();
	os_setCallback(&gnss_job, poll_navdata);

	// Analog inteface to read battery or temperature
	analog_init();

	// Init RTC interrupts
	time_manager_init();

	// LoRaWAN
	lpwan_init();

	// Init complete. Turn off status LED
	set_status_led(false, false);
}

void iof_app( osjob_t *j ) {
	sprintf(debug_str_buf, "IOF APP: UNIX: %lu\n", iof_unix_ts);
	debug_str(debug_str_buf);

	payload_t pl;
	// build IOF frame
	pl.length = iof_build_frame(pl.payload, send_slimData, tbr_in_sync);
	send_slimData = false;

	if (pl.length > 0) lpwan_queue_uplink(&pl, iof_frame_hasTagMsg());

	if (!tbr_connected) {
		os_setCallback(&tbr_job, ping_tbr);
	}

}

void BURTC_IRQHandler(void) {
    uint32_t int_mask = BURTC_IntGet();
	BURTC_IntClear(int_mask);

    if (int_mask & BURTC_IF_COMP0){
		static uint16_t ticks = 0;

		iof_unix_ts++;
		ticks++; // 1 ticks = 1 second!

		// sync tbr at next decasecond if new valid time data is available (tbr_do_sync)
		if (tbr_do_sync && (iof_unix_ts % 10 == 0)) {
			tbr_do_sync = false;
			os_setCallback(&tbr_job, sync_tbr);
		}

		// GNSS //
		// restart nav data polling
		if (ticks == 600) {
			os_setCallback(&gnss_job, poll_navdata);
		}

		// IOF APPLICATION //
		// schedule app once a minute
		if (ticks % 240 == 0) {
			os_setCallback(&app_job, iof_app);
		}

		// Status LEDs and display
		if(ticks % 5 == 0) {
			os_setCallback(&blink_job, LEDs_set);
			os_setCallback(&display_job, iof_update_display);
		}

		if (ticks >= 720) {
			send_slimData = true;
			ticks = 0;
		}
        //WDOG_Feed();
    }
}
