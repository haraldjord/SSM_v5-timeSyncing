/*
 * lpwan_manager.c
 *
 *  Created on: Jun 29, 2017
 *      Author: Waseemh
 *      Author: MariusSR
 *
 * 	Edited: Spring 2022
 * 		Author: Jon Andreas Kornberg
 */

#include "../resource_managers_header/lpwan_manager.h"

extern uint8_t node_id;
/*
 * Private variables
 */
static bool			tx_complete = true;
static bool			joined = false;

static struct fifo_descriptor fifo_lorawan_UpCnf_s;
static fifo_t fifo_lorawan_UpCnf = &fifo_lorawan_UpCnf_s;
static payload_t lorawan_UpCnf[LORAWAN_BUFFER_LENGTH];

static payload_t lorawan_UpUnc;

static uint8_t dr = 5;
static uint8_t tx_pow = 0;
/*
 * Public variables
 */


/*
 * LMIC callbacks
 */
// provide application router ID (8 bytes, LSBF)
void os_getArtEui (u1_t* buf) {
    memcpy(buf, APPEUI, 8);
}


// provide device ID (8 bytes, LSBF)
void os_getDevEui (u1_t* buf) {
    memcpy(buf, DEVEUI, 8);
    buf[1] = node_id;
}


// provide device key (16 bytes)
void os_getDevKey (u1_t* buf) {
    memcpy(buf, DEVKEY, 16);
}


/*
 * private functions
 */


/*
	If transmission is ongoing or not joined -> return
	else if not received ack on last UPCnf -> resend
	else if any UpCnf queued -> send next UpCnf
	else if any UpUnc queued -> send UpUnc
	else return
*/
static void send_next_uplink( void ) {
	static payload_t pl = {.length = 0};
	if (tx_complete && joined) {
		lmic_tx_error_t err = LMIC_ERROR_SUCCESS;

		if (!fifo_is_empty(fifo_lorawan_UpCnf)) {
			fifo_get(fifo_lorawan_UpCnf, &pl);
			err = LMIC_setTxData2_strict(1, pl.payload, pl.length, 1);
		}
		else if (lorawan_UpUnc.length > 0) {
			err = LMIC_setTxData2_strict(1, lorawan_UpUnc.payload, lorawan_UpUnc.length, 0);
			lorawan_UpUnc.length = 0;
		}
		else {
			return;
		}

		// handle send error
		switch(err) {
			case LMIC_ERROR_SUCCESS:
				tx_complete = false;
				break;

			case LMIC_ERROR_TX_BUSY:
				debug_str("LMIC: LMIC_ERROR_TX_BUSY\n");
				break;

			case LMIC_ERROR_TX_TOO_LARGE:
				debug_str("LMIC: LMIC_ERROR_TX_TOO_LARGE\n");
				debug_str("LMIC: Message discarded\n");
				break;

			case LMIC_ERROR_TX_NOT_FEASIBLE:
				debug_str("LMIC: LMIC_ERROR_TX_NOT_FEASIBLE\n");
				debug_str("LMIC: Message discarded\n");
				break;

			case LMIC_ERROR_TX_FAILED:
				debug_str("LMIC: LMIC_ERROR_TX_FAILED\n");
				break;
		}
	}
}

/*
* public functions
*/
uint8_t lpwan_get_SF( void ) {
	return 12 - dr;
}

uint8_t lpwan_get_txPow( void ) {
	return tx_pow;
}

bool lpwan_isJoined() {
	return joined;
}

bool lpwan_isTXcomplete() {
	return tx_complete;
}

void lpwan_queue_uplink(payload_t* payload, u1_t confirmed) {
    if (confirmed) {
		fifo_add(fifo_lorawan_UpCnf, payload);
	}
	else {
		memcpy(&lorawan_UpUnc, payload, sizeof(payload_t));
	}
	send_next_uplink();
}

void lpwan_init( void ) {
    debug_str("LoRaWAN init\n");
	
	// create tx buffer
	fifo_create_static(fifo_lorawan_UpCnf, lorawan_UpCnf, LORAWAN_BUFFER_LENGTH, sizeof(payload_t));

    // Reset MAC state
    LMIC_reset();
    // Start joining
    LMIC_startJoining();
}

void onEvent (ev_t ev) {
    debug_event(ev);

	if ( (dr != LMIC.datarate) || (tx_pow != LMIC.radio_txpow) ){
		dr = LMIC.datarate;
		tx_pow = LMIC.radio_txpow;

		sprintf(debug_str_buf, "LMIC: SF%u, TX_POWER = %u\n", 12 - dr, tx_pow);
		debug_str(debug_str_buf);
	}

    switch(ev) {
        // joining network
        case EV_JOINING:
			joined = false;
            break;

        // network joined, session established
        case EV_JOINED:
			joined = true;

            LMIC_setAdrMode(true);

        	sprintf(debug_str_buf, "LoRaWAN: Joined netid = %lu\n", LMIC.netid);
            debug_str(debug_str_buf);

			send_next_uplink();
			status_led_radio(false);
            break;

        // joining network failed after retrying
        case EV_JOIN_FAILED:
			joined = false;
            // Reset MAC state
			LMIC_reset();
			// Start joining
			LMIC_startJoining();
            break;

        // scheduled data sent (optionally data received)
        case EV_TXCOMPLETE:
			tx_complete = true;
            status_led_radio(false);

			send_next_uplink();
	        break;

        // no adrack received from network
        case EV_LINK_DEAD:
			joined = false;
            // Reset MAC state
			LMIC_reset();
			// Start joining
			LMIC_startJoining();
            break;

		// The link was dead, but now is alive again
		case EV_LINK_ALIVE:
			joined = true;
			send_next_uplink();
			break;

        // scheduling transmission
        case EV_TXSTART:
            status_led_radio(true);
            break;

		case EV_TXCANCELED:
			tx_complete = true;
			send_next_uplink();
			status_led_radio(false);
			break;

        // join request sent, but no join accept received
        case EV_JOIN_TXCOMPLETE:
			status_led_radio(false);
            debug_str("LoRaWAN: No Join Accept received\n");
            break;
    }
}
