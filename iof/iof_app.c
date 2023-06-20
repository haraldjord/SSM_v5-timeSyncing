/*
 * iof_app.c
 *
 *  Created on: 10. feb. 2022
 *      Author: Jon Andreas Kornberg
 *  Edited: Spring 2023
 *      Author Harald Jordalen
 */

#include "iof_app.h"

#define LFXO_FREQUENCY		32768
#define WAKEUP_INTERVAL_SEC	1
#define BURTC_TOP			((LFXO_FREQUENCY * WAKEUP_INTERVAL_SEC) - 1)

#define   SYNC_PPS                10
#define   BASIC_SYNCH_SECONDS     10
#define   ADVANCE_SYNCH_SECONDS   120



/*
 * Shared variables
 */
bool tbr_connected = false;
bool tbr_in_sync = false;

bool update_GPS = false;
bool get_UNIX_time = false;

osjob_t app_job;
uint8_t node_id = 0xFF;
pos_t last_pos = {.fix = 0, .numSV = 0};
uint32_t iof_unix_ts = 0; // latest time stamp from gps



/*
 * Private variables
 */

static osjob_t gnss_job;
static osjob_t tbr_job;
static osjob_t blink_job;
static osjob_t display_job;
static osjob_t rs232_rx_job;

static uint32_t  one_sec_top_ref=32768;

// TBR
static bool tbr_do_advance_sync = false;

// GNSS
bool gnss_acquisition = false;

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
	display_put_string(2, 50, "LoRa: Disconnected", font_small);
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


static bool first_pool = false;
static void poll_navdata( osjob_t *j ) {

    debug_str("IOF: GNSS starting search...\n");
    gnss_clearAll_ubxMsgs();
    gnss_clear_validData();
    while(!gnss_poll_navdata()){      // poll nav data until valid timestamp and location is found
    }

    // get UNIX time
    iof_unix_ts = gnss_get_timeData(); // data is 1 second old
    //  get position
    gnss_get_posData(&last_pos);

    if (!first_pool){
        // somehow the first pooling of GPS has delay before PPS signal is enable, solved by calling the function again on restart or cold start.
        first_pool = true;
        os_setTimedCallback(j, os_getTime() + sec2osticks(5), poll_navdata); // call 5 sec to prevent race condition
    }
    else{
        gnss_acquisition = false;
        tbr_do_advance_sync = true;
    }
}


static parse_rs232(osjob_t *j){
  parse_rs232_buffer();
}


static void ping_tbr_rx_SN( osjob_t *j ) {
	tbr_connected = tbr_ping_is_rx_SN();
	//if (tbr_connected) debug_str("IOF: TBR ping success\n");
	//else  debug_str("IOF: TBR ping failed\n");
}

static void ping_tbr( osjob_t *j ) {
	tbr_ping();

	// schedule SN reception
	os_setTimedCallback(j, os_getTime() + sec2osticks(2), ping_tbr_rx_SN);
}

static void sync_tbr_basic_is_act(osjob_t * j){
  if (tbr_is_ack01()){
      tbr_in_sync = true;
      tbr_connected = true;
      //debug_str("IOF: basic time sync of TBR successed\n");
  }
  else{
      tbr_in_sync = false;
      tbr_connected = false;
      debug_str("IOF: basic time sync of TBR FAILED!\n");
  }

}

static void sync_tbr_advance_is_ack( osjob_t *j ) {
	if (tbr_is_ack02()) {
		tbr_in_sync = true;
		tbr_connected = true;
		//debug_str("IOF: Advanced time sync of TBR successed\n");
	}
	else {
		tbr_in_sync = false;
		debug_str("IOF: Advanced time sync of TBR FAILED!\n\n");
	}
}

static void sync_tbr( osjob_t *j ) {
  if(tbr_do_advance_sync){
      tbr_do_advance_sync = false;
      tbr_advancedTimeSync(iof_unix_ts);
      os_setTimedCallback(j, os_getTime() + sec2osticks(1), sync_tbr_advance_is_ack); // schedule ack reception
  }
  else if(!tbr_do_advance_sync){
      tbr_basicTimeSync();
      os_setTimedCallback(j, os_getTime() + sec2osticks(1), sync_tbr_basic_is_act);
  }
  else{
      debug_str("Invalid syncing of TBR!\n");
  }
}

static void time_manager_init( void ) {

  iof_unix_ts = 0;

  /////////////GPS PPS and INT pins////////////
  GPIO_PinModeSet(GPS_SIG_PORT, GPS_TIME_PULSE, gpioModeInput, 0);
  GPIO_IntConfig(GPS_SIG_PORT,GPS_TIME_PULSE,true,false,true);
  GPIO_IntClear(_GPIO_IF_MASK);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);


  /////////////BURTC and LETIMER////////////
  //Setup and initialize BURTC
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
    //BURTC_CompareSet(0, BURTC_TOP);
    BURTC_CompareSet(0, one_sec_top_ref);
    BURTC_IntDisable(_BURTC_IF_MASK);
    BURTC_IntEnable(BURTC_IF_COMP0);
	NVIC_ClearPendingIRQ(BURTC_IRQn);
	NVIC_EnableIRQ(BURTC_IRQn);

  //Setup and initialize LETIMER
  LETIMER_Init_TypeDef  letimer_init=LETIMER_INIT_DEFAULT;
  letimer_init.enable=false;
  CMU_ClockEnable(cmuClock_LETIMER0, true);
  LETIMER_Reset(LETIMER0);
  LETIMER_Init(LETIMER0,&letimer_init);

  // Start BURTC
    BURTC_Enable(false); // BURTC not beeing used
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


  // Init RTC interrupts
  time_manager_init();

  // update display
  gnss_acquisition = true;
  os_setCallback(&display_job, iof_update_display);


	// GNSS/GPS
	gnss_init();
	os_setCallback(&gnss_job, poll_navdata);

	// Analog inteface to read battery or temperature
	analog_init();

	// LoRaWAN
	//lpwan_init(); //LoRa radio not beeing used.

	// Init complete. Turn off status LED
	set_status_led(false, false);
}

void iof_app( osjob_t *j ) {
	if (get_UNIX_time){
      get_UNIX_time = false;
      sprintf(debug_str_buf, "ID: %d , UNIX: %lu\n",node_id, (iof_unix_ts));
      debug_str(debug_str_buf);
	}
	/************ LoRa removed
	payload_t pl;
	// build IOF frame
	pl.length = iof_build_frame(pl.payload, send_slimData, tbr_in_sync);
	send_slimData = false;
	if (pl.length > 0) lpwan_queue_uplink(&pl, iof_frame_hasTagMsg());
	************/

	sendTBR_string();
	sendGPS_data();
	if (!tbr_connected) {
		os_setCallback(&tbr_job, ping_tbr);
	}
}




void GPIO_EVEN_IRQHandler() {
    //debug_str("\tEVEN IRQ\n");
    u4_t int_mask = GPIO_IntGetEnabled();
    int_mask &= ~(1UL << GPS_INT);   // TODO: hvordan bør en sørge for at bare even int blir cleared?
    GPIO_IntClear(int_mask);

    //if (int_mask & (1 << RADIO_IO_0)) radio_irq_handler(0); // LoRa removed
    //if (int_mask & (1 << RADIO_IO_2)) radio_irq_handler(2);

    if (int_mask & (1 << GPS_TIME_PULSE)){
        //debug_str("GPS TIME PULSE\n");
        //status_led_gps_toggle();

        if(!gnss_acquisition){ // GPS active

            iof_unix_ts++;

            if ((iof_unix_ts % BASIC_SYNCH_SECONDS) == 0){
                //debug_str("tbr_do_sync handler\n");
                os_setCallback(&tbr_job, sync_tbr); // basic sync every 10 sec
            }

            if ((iof_unix_ts % ADVANCE_SYNCH_SECONDS == 0) || update_GPS) {
                update_GPS = false;
                gnss_acquisition = true;
                os_setCallback(&display_job, iof_update_display);
                os_setCallback(&gnss_job, poll_navdata);
            }

            if((iof_unix_ts- node_id) % 5 == 0) {
                os_setCallback(&rs232_rx_job, parse_rs232);
                os_setCallback(&app_job, iof_app);
                os_setCallback(&blink_job, LEDs_set);
                os_setCallback(&display_job, iof_update_display);
              }
        }
    }
}







void BURTC_IRQHandler(void) { /*BURTC must be activated in initialization*/
    //debug_str("Inside BURTC_IRQHandler\n");
  /*
    uint32_t int_mask = BURTC_IntGet();
	  BURTC_IntClear(int_mask);

    if (int_mask & BURTC_IF_COMP0){
		static uint16_t ticks = 0;
		static uint16_t startup_time_min = 0;

		//iof_unix_ts++;
		ticks++; // 1 ticks = 1 second!
		//sprintf(debug_str_buf, "Unix time: %lu\n", iof_unix_ts);
		//debug_str(debug_str_buf);
  }
  */


}
