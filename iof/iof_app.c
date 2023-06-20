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
#define   ADVANCE_SYNCH_SECONDS   120 // todo is this a good time interval?



////////////////////////////////////////////////
/// pulse per second time synchronization
#define N_SAMPLES   4
#define BASE_2_N  16    //-1 done inside if...
uint32_t  one_sec_top_ref=32768; // must be extern as its used in iof_app.c
static  bool    letimer_running=false;
bool     restart_timer_by_PPSPulse = false;
bool BURTC_restarted = false;
static  int     last_letimer_count=65535;
static  uint16_t  average_n=0;
static  uint32_t  avergae_sum=0;
static  uint32_t  ref_count=0;

//////////////////////////////////////////////////



/*
 * Shared variables
 */
extern  uint32_t  one_sec_top_ref;
extern bool restart_timer_by_PPSPulse;
//extern BURTC_restarted;
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

static osjob_t send_tbr_buffer_job;
static osjob_t PPS_counter;

// TBR

static bool tbr_do_advance_sync = false;

//static bool send_slimData = true; OBSOLETE

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

/*static void poll_navdata( osjob_t *j ) {
	static uint8_t retries = 0;
	static uint32_t search_ts = 0;
	if (retries == 0) {
		search_ts = iof_unix_ts;
		debug_str("IOF: GNSS starting search...\n");
		gnss_force_ON();
		gnss_clearAll_ubxMsgs();
		gnss_clear_validData();

		gnss_acquisition = true;
		gnss_poll_navdata();
	}

  if (BURTC_restarted){ // end job and set unix time after BURTC timer is restarted by PPS pulse.
      BURTC_restarted = false;
      // UNIX Time
      iof_unix_ts = 1 + gnss_get_timeData(); // data is 1 second old
      tbr_do_advance_sync = true;
      // POSITION
      gnss_get_posData(&last_pos);
      retries = 0;
      os_clearCallback(j);
      gnss_acquisition = false;
  }
	// if success OR more than 120 retries -> stop polling
  else if (gnss_is_validPosData() && gnss_is_validTimeData()) {
		sprintf(debug_str_buf, "IOF: GNSS fix acquired! Time to fix: %lu. Retries: %u\n", iof_unix_ts - search_ts, retries);
		debug_str(debug_str_buf);
		//gnss_force_OFF(); do not turn off to use the 1PPS timer synchronization

		restart_timer_by_PPSPulse = true;
		os_setTimedCallback(j, os_getTime() + sec2osticks(1), poll_navdata);

	}
	else if (retries >= 120) {
		//gnss_force_OFF();
		debug_str("IOF: GNSS search timeout...\n");
		retries = 0;
		os_clearCallback(j);

		last_pos.fix = 0;
		last_pos.numSV = 0;
		gnss_acquisition = false;
	}
	// else continue polling
	else {
    gnss_poll_navdata();
		os_setTimedCallback(j, os_getTime() + sec2osticks(1), poll_navdata);
		retries++;
	}
}*/


//gnss_acquisition = true; // move before calling the function and update display
static bool first_pool = false;
static void poll_navdata( osjob_t *j ) {


    BURTC_CounterReset();
    BURTC_Enable(false); // stop BURTC timer to prevent race condition between BURTC and PPS signal.

    debug_str("IOF: GNSS starting search...\n");
    gnss_clearAll_ubxMsgs();
    gnss_clear_validData();
    while(!gnss_poll_navdata()){      // poll nav data until valid timestamp and location is found
    //delay_ms(100);                    // wait arbitrarly time before polling data again
    }

    iof_unix_ts = gnss_get_timeData(); // data is 1 second old
    // POSITION
    gnss_get_posData(&last_pos);
    /*
    delay_ms(500);// wait 0.2 sec to prevent race condition between PPS signal
    restart_timer_by_PPSPulse = true;
    while(!BURTC_restarted);       // wait until BURTC is restarted by PPS signal
       BURTC_restarted = false;     // todo rename variable

     */
    if (!first_pool){
        // somehow the first pooling of gps has a delay before PPS signal is enable, solved by calling the function again on restart or cold start.
        first_pool = true;
        os_setTimedCallback(j, os_getTime() + sec2osticks(5), poll_navdata); // call 2 sec to prevent race condition
    }
    else{
        gnss_acquisition = false;
        tbr_do_advance_sync = true;
    }


    //gnss_acquisition = false;
    //tbr_do_advance_sync = true;
  // if success OR more than 120 retries -> stop polling
  //if(is_BURTC_restarted() && getout == true){


    /*
    if (is_BURTC_restarted()){

      set_BURTC_restarted(false);
      // UNIX Time
      debug_str("unix time ok!\n");
      iof_unix_ts = 1 + gnss_get_timeData(); // data is 1 second old
      tbr_do_advance_sync = true;

      // POSITION
      gnss_get_posData(&last_pos);

      gnss_acquisition = false;
      retries = 0;
      os_clearCallback(j);
  }
  else if (gnss_is_validPosData() && gnss_is_validTimeData()) {
    sprintf(debug_str_buf, "IOF: GNSS fix acquired! Time to fix: %lu. Retries: %u\n", iof_unix_ts - search_ts, retries);
    debug_str(debug_str_buf);

    //sprintf(debug_str_buf, "BURTC_restarted: %d \t getout: %d\n", is_BURTC_restarted(), getout);
    //debug_str(debug_str_buf);
    //gnss_force_OFF();
    restart_timer_by_PPSPulse = true;

    os_setTimedCallback(j, os_getTime() + ms2osticks(1300), poll_navdata); // call 2 sec to prevent race condition

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

    retries++;
    sprintf(debug_str_buf, "retries: %d\n", retries);
    debug_str(debug_str_buf);
    os_setTimedCallback(j, os_getTime() + sec2osticks(1), poll_navdata);

  }*/
}


/*static void poll_navdata( osjob_t *j ) {
  static uint8_t retries = 0;
  static uint32_t search_ts = 0;
  if (retries == 0) {
      BURTC_Enable(false); // stop BURTC timer to prevent race condition between BURTC and PPS signal.
      search_ts = iof_unix_ts;
    debug_str("IOF: GNSS starting search...\n");
    gnss_force_ON();
    gnss_clearAll_ubxMsgs();
    gnss_clear_validData();

    gnss_acquisition = true;
  }

  // if success OR more than 120 retries -> stop polling
  //if(is_BURTC_restarted() && getout == true){
   if (is_BURTC_restarted()){

      set_BURTC_restarted(false);
      // UNIX Time
      debug_str("unix time ok!\n");
      iof_unix_ts = 1 + gnss_get_timeData(); // data is 1 second old
      tbr_do_advance_sync = true;

      // POSITION
      gnss_get_posData(&last_pos);

      gnss_acquisition = false;
      retries = 0;
      os_clearCallback(j);
  }
  else if (gnss_is_validPosData() && gnss_is_validTimeData()) {
    sprintf(debug_str_buf, "IOF: GNSS fix acquired! Time to fix: %lu. Retries: %u\n", iof_unix_ts - search_ts, retries);
    debug_str(debug_str_buf);

    //sprintf(debug_str_buf, "BURTC_restarted: %d \t getout: %d\n", is_BURTC_restarted(), getout);
    //debug_str(debug_str_buf);
    //gnss_force_OFF();
    restart_timer_by_PPSPulse = true;

    os_setTimedCallback(j, os_getTime() + ms2osticks(1300), poll_navdata); // call 2 sec to prevent race condition

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
    while(!gnss_poll_navdata());
    retries++;
    sprintf(debug_str_buf, "retries: %d\n", retries);
    debug_str(debug_str_buf);
    os_setTimedCallback(j, os_getTime() + sec2osticks(1), poll_navdata);

  }
}*/
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
      tbr_in_sync = false; // todo rename flag to tbr_in_sync_basic??
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
	/*
	payload_t pl;
	// build IOF frame
	pl.length = iof_build_frame(pl.payload, send_slimData, tbr_in_sync);
	send_slimData = false;
	if (pl.length > 0) lpwan_queue_uplink(&pl, iof_frame_hasTagMsg()); LoRa removed
	*/

	sendTBR_string();
	sendGPS_data();
	if (!tbr_connected) {
		os_setCallback(&tbr_job, ping_tbr);
	}
}


/*void debug_PPS_counter(osjob_t *j){
  sprintf(debug_str_buf, "one sec top ref: %lu\n", one_sec_top_ref);
  debug_str(debug_str_buf);
}*/





void GPIO_EVEN_IRQHandler() {
    //debug_str("\tEVEN IRQ\n");
    u4_t int_mask = GPIO_IntGetEnabled();
    int_mask &= ~(1UL << GPS_INT);   // TODO: hvordan bør en sørge for at bare even int blir cleared?
    GPIO_IntClear(int_mask);

    //if (int_mask & (1 << RADIO_IO_0)) radio_irq_handler(0);
    //if (int_mask & (1 << RADIO_IO_2)) radio_irq_handler(2);

    if (int_mask & (1 << GPS_TIME_PULSE)){
        //debug_str("GPS TIME PULSE\n");
        //status_led_gps_toggle();

        /*
        if(restart_timer_by_PPSPulse){ // restart BURTC timer at PPS pulse to sync all devices.
            restart_timer_by_PPSPulse = false;
            BURTC_CounterReset();
            BURTC_Enable(true);
            BURTC_restarted = true;
            //debug_str("BURTC enabled\n");

        }*/

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

        /*
        if(letimer_running){
          LETIMER_Enable(LETIMER0,false);
          //sprintf(debug_str_buf, "LETIMER0 counter: %ul\n", LETIMER_CounterGet(LETIMER0));
          //debug_str(debug_str_buf);
          if(LETIMER_CounterGet(LETIMER0)>last_letimer_count){
            avergae_sum+=(uint32_t)(LETIMER_CounterGet(LETIMER0)-last_letimer_count);
          }
          else{
            avergae_sum+=(uint32_t)(last_letimer_count-LETIMER_CounterGet(LETIMER0));
          }
          last_letimer_count=LETIMER_CounterGet(LETIMER0);
          letimer_running=false;
          average_n++;
          if(average_n>BASE_2_N-1){
            one_sec_top_ref=avergae_sum>>N_SAMPLES;
            avergae_sum=0;
            average_n=0;
          }
        }
        else{
          LETIMER_Enable(LETIMER0,true);
          letimer_running=true;
        }*/

        //ref_count++; // OBSOLETE??

    }

}







void BURTC_IRQHandler(void) {
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


      if ((ticks % SYNC_PPS) == 0 && ticks !=0) {
           if(one_sec_top_ref>32000 && one_sec_top_ref<33000){
               BURTC_CompareSet(0,one_sec_top_ref);
           }
           //os_setCallback(&PPS_counter, debug_PPS_counter);

      }
      // sync tbr at next decasecond if new valid time data is available (tbr_do_advance_sync)
      // TODO use iof_unix_ts to schedule syncing, to ensure that all tbr hydrophones are synced at the same time??
      /*if ((iof_unix_ts % BASIC_SYNCH_SECONDS) == 0){
        //debug_str("tbr_do_sync handler\n");
        os_setCallback(&tbr_job, sync_tbr); // basic sync every 10 sec, advanced sync when new valid time data is available (from GPS)
      }*/



      // GNSS //
      // restart nav data polling
      /*
      if (((iof_unix_ts)% (ADVANCE_SYNCH_SECONDS)) == 0 && ticks !=0) { // does it need new timestamp every 10 minute???
          //debug_str("nav data polling handler\n");
          gnss_acquisition = true;
          os_setCallback(&display_job, iof_update_display);
          os_setCallback(&gnss_job, poll_navdata);
      }*/

      /*
      if(iof_unix_ts % 10 && tbr_do_advance_sync){ // sync tbr at next decasecond after unix time is polled
            os_setCallback(&tbr_job, sync_tbr);
        }/*

      /*if((iof_unix_ts % (ADVANCE_SYNCH_SECONDS - 5)) == 0 && ticks != 0){ // restart BURTC timer 5 seconds before pulling new gps data.
          restart_timer_by_PPSPulse = true;
      }*/

      // IOF APPLICATION //
      // schedule app once 4 minutes
  /*
  if (ticks % 240 == 0) {
          //debug_str("schedule app after 4 minutes\n");
          //os_setCallback(&app_job, iof_app);
      }
*/
      // Status LEDs and display
      /*
      if(iof_unix_ts % 5 == 0) {
          //debug_str("status LED handler (5sec)\n");

          os_setCallback(&app_job, iof_app);
          //os_setCallback(&send_tbr_buffer_job, send_tbr_buffer);
          os_setCallback(&blink_job, LEDs_set);
          os_setCallback(&display_job, iof_update_display);
      }
      */

      /*
      if (ticks >= 720) {
          //debug_str("send slimData handler\n");
        //send_slimData = true;
        //ticks = 0;
      }
		//}
        //WDOG_Feed();
    }*/
}
