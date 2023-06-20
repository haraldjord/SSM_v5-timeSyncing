#include "../lmic/oslmic.h"
#include "../iof/iof_app.h"
#include "../devices_header/display.h"
#include "../drivers_header/rs232.h"


extern osjob_t app_job;


void setup_osc_and_clock(void) {
	

    CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
    CMU_OscillatorEnable(cmuOsc_HFXO, true, true);

    CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
    CMU_ClockDivSet(cmuClock_HF, cmuClkDiv_8);
}

int main() {

	CHIP_Init();

	setup_osc_and_clock();

	delay_init();

	debug_init();

	/* DEBUG
	int clock_freq = CMU_ClockFreqGet(cmuOsc_LFXO);
    sprintf(debug_str_buf, "cmuOsc_LFXO freq: %d\n", clock_freq);
    debug_str(debug_str_buf);

    clock_freq = CMU_ClockFreqGet(cmuOsc_HFXO);
    sprintf(debug_str_buf, "cmuOsc_HFXO freq: %d\n", clock_freq);
    debug_str(debug_str_buf);

    clock_freq = CMU_ClockFreqGet(cmuClock_HF);
    sprintf(debug_str_buf, "cmuClock_HF freq: %d\n", clock_freq);
    debug_str(debug_str_buf);
	*/

	// initialize runtime env
    os_init();
    // setup initial job
    os_setCallback(&app_job, iof_app_init);
    // execute scheduled jobs and events
    os_runloop();
    // (not reached)
	return 0;
}
