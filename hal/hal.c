/*******************************************************************************
 * Written by Jon Andreas Kornberg
 * Previous Contributers:
 *  - Waseemh
 *  - MaruisSR
 *******************************************************************************/
#include "../lmic/hal.h"
#include "debug.h"
#include "../drivers_header/delay.h"
#include "../drivers_header/spi.h"
#include "../devices_header/led_and_switch.h"
#include "em_emu.h"


// HAL state
static struct {
    int8_t irqlevel;
    uint64_t ticks;
} HAL;
////////////////////////////////////////////////
/// pulse per second time synchronization
/*
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
*/
////////////////////////////////////////////////

// sheared variables
extern bool gnss_acquisition;


const bit_t lmic_pins;
//////////////////////// STATIC FUNCTIONS ////////////////////////////

static void hal_io_init(){

    GPIO_PinModeSet(RADIO_IO_0345_PORT, RADIO_IO_0, gpioModeInputPull, 0);	//DIO0=PayLoadReady
    GPIO_PinModeSet(RADIO_IO_12_PORT,   RADIO_IO_1, gpioModeInputPull, 0);	//DIO1=FifoLevel
    GPIO_PinModeSet(RADIO_IO_12_PORT,   RADIO_IO_2, gpioModeInputPull, 0);	//DIO2=SyncAddr
    GPIO_PinModeSet(RADIO_IO_0345_PORT, RADIO_IO_3, gpioModeInputPull, 0);	//DIO3=FifoEmpty
    GPIO_PinModeSet(RADIO_IO_0345_PORT, RADIO_IO_4, gpioModeInputPull, 0);	//DIO4=PreambleDetect/RSSI
    GPIO_PinModeSet(RADIO_IO_0345_PORT, RADIO_IO_5, gpioModeInputPull, 0);	//DIO5=ModeReady

	GPIO_IntConfig(RADIO_IO_0345_PORT, RADIO_IO_0, true, false, true);
	GPIO_IntConfig(RADIO_IO_12_PORT, RADIO_IO_1, true, false, true);
	GPIO_IntConfig(RADIO_IO_12_PORT, RADIO_IO_2, true, false, true);

	GPIO_IntClear(_GPIO_IF_MASK);

	NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
	NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);

	NVIC_EnableIRQ(GPIO_ODD_IRQn);
	NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}

static void hal_spi_init(){
    spi_init();
	  spi_enable();
}

static void hal_time_init(){
    /* Ensure LE modules are accessible */
	CMU_ClockEnable(cmuClock_CORELE, true);

	/* Enable LFACLK in CMU (will also enable oscillator if not enabled) */
	CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);

	/* Use the prescaler to reduce power consumption. */
	CMU_ClockDivSet(cmuClock_RTC, cmuClkDiv_1);

	/* Enable clock to RTC module */
	CMU_ClockEnable(cmuClock_RTC, true);

    RTC_Init_TypeDef init;
	init.enable   = false;
	init.debugRun = false;
	init.comp0Top = false;
	RTC_Init(&init);

	/* Disable interrupt generation from RTC0 */
	RTC_IntDisable(_RTC_IF_MASK);
	RTC_IntEnable(RTC_IF_OF);	//Enable interrupt on overflow

	/* Enable interrupts */
	NVIC_ClearPendingIRQ(RTC_IRQn);
	NVIC_EnableIRQ(RTC_IRQn);

	/* Enable RTC */
	RTC_Enable(true);
}

static void hal_reset_radio( void ) {
    hal_disableIRQs();
    GPIO_PinModeSet(RADIO_IO_0345_PORT, RADIO_IO_5, gpioModePushPull, 0);

    GPIO_PinOutClear(RADIO_IO_0345_PORT, RADIO_IO_5);
    delay_ms(1);
    GPIO_PinOutSet(RADIO_IO_0345_PORT, RADIO_IO_5);
    delay_ms(10);

    GPIO_PinModeSet(RADIO_IO_0345_PORT, RADIO_IO_5, gpioModeInput, 0);
    hal_enableIRQs();
}
//////////////////////// INTERRUPT HANDLERS //////////////////////////

void RTC_IRQHandler(void) {
	//debug_str("\tRTC IRQ\n");
	if (RTC_IntGet() & RTC_IF_OF) {
		++HAL.ticks;
	}

    if (RTC_IntGet() & RTC_IF_COMP0) {       // expired
        // do nothing, only wake up cpu
    }
	RTC_IntClear(_RTC_IF_MASK); // clear IRQ flags
}

extern void radio_irq_handler(u1_t dio);
/*
bool is_BURTC_restarted(void){
  if (BURTC_restarted == true) return true;
  else {return false;}
}

void set_BURTC_restarted(bool state){
  if (state == true) BURTC_restarted = true;
  else{
      BURTC_restarted = false;
}
}*/

/*
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

        if(restart_timer_by_PPSPulse){ // restart BURTC timer at PPS pulse to sync all devices.
            restart_timer_by_PPSPulse = false;
            BURTC_CounterReset();
            BURTC_Enable(true);
            BURTC_restarted = true;
            //debug_str("BURTC enabled\n");

        }

        if(!gnss_acquisition){ // GPS active



        }




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
        }

        ref_count++; // OBSOLETE??

    }

}*/


void GPIO_ODD_IRQHandler() {        // par
    debug_str("\tODD IRQ\n");
  	u4_t int_mask = GPIO_IntGetEnabled();
    int_mask &= ~(1UL << RADIO_IO_0);        // TODO: hvordan bør en sørge for at bare odd int blir cleared?
    int_mask &= ~(1UL << RADIO_IO_2);        // TODO: hvordan bør en sørge for at bare odd int blir cleared?
    int_mask &= ~(1UL << GPS_TIME_PULSE);    // TODO: hvordan bør en sørge for at bare odd int blir cleared?
    GPIO_IntClear(int_mask);

	if (int_mask & (1 << RADIO_IO_1)) radio_irq_handler(1);

    if (int_mask & (1 << GPS_INT)) {};
}

/////////////////////////   HAL   ///////////////////////////////////
void hal_init_ex (const void *pHalData ) {
    memset(&HAL, 0x00, sizeof(HAL));

    hal_disableIRQs();

	  hal_io_init();	    // configure radio I/O and interrupt handler
    hal_spi_init();	    // configure radio SPI
    hal_time_init();    // configure timer and interrupt handler

    hal_enableIRQs();
 }

 void hal_pin_rxtx (u1_t val){

 }

 void hal_pin_rst (u1_t val){
	if (val == 0) hal_reset_radio();
 }

void hal_spi_write(u1_t cmd, const u1_t* buf, size_t len){
    spi_cs_clear(radio);
	spi_write_byte(cmd);
    for (size_t i = 0; i < len; i++) {
        spi_write_byte(buf[i]);
    }
    spi_cs_set(radio);
}

void hal_spi_read(u1_t cmd, u1_t* buf, size_t len){
    spi_cs_clear(radio);
    spi_write_byte(cmd);
    for (size_t i = 0; i < len; i++) {
        buf[i] = spi_read_byte();
    }
    spi_cs_set(radio);
}

CORE_DECLARE_IRQ_STATE;
void hal_disableIRQs (void){
    if (HAL.irqlevel++ == 0) {
        CORE_ENTER_ATOMIC();
    }
}

void hal_enableIRQs (void){
    if (--HAL.irqlevel == 0) {
        CORE_EXIT_ATOMIC();
    }
}

uint8_t hal_getIrqLevel (void){
    return HAL.irqlevel;
}

void hal_sleep (void){
    EMU_EnterEM1();
}

u4_t hal_ticks (void){
    hal_disableIRQs();
    u4_t t = HAL.ticks;
    u4_t cnt = RTC_CounterGet();
	if (RTC_IntGet() & RTC_IF_OF) {	// Overflow before we read CNT?
        cnt = RTC_CounterGet();
        ++t;	// Include overflow in evaluation but leave update of state to ISR once interrupts enabled again
    }
    hal_enableIRQs();
	return (t<<24)|cnt;
}

// return modified delta ticks from now to specified ticktime
static s4_t deltaticks (u4_t time) {
    u4_t t = hal_ticks();
    s4_t d = time - t;
    return d;
}

u4_t hal_waitUntil (u4_t time){
    s4_t d = deltaticks(time);
    if (d < 0) return (u4_t) -d;
    else if (d > 0)while (deltaticks(time) > 0);

    return 0;
}

u1_t hal_checkTimer (u4_t targettime){
    s4_t d = deltaticks(targettime);
    if (d < 5) {
        RTC_IntDisable(RTC_IF_COMP0);
        return 1;
    }
    else {
        RTC_IntClear(RTC_IF_COMP0);
        RTC_CompareSet(0, RTC_CounterGet() + d);
        RTC_IntEnable(RTC_IF_COMP0);
        return 0;
    }

}

void hal_failed (const char *file, u2_t line){
    debug_str("\n\nHAL FAILED!\n\n");

    NVIC_SystemReset();
	// HALT...
    hal_sleep();
    while(1);
}

void hal_set_failure_handler(const hal_failure_handler_t* h){

}

s1_t hal_getRssiCal (void) {
    return 0;   
}

ostime_t hal_setModuleActive (bit_t val){
    return 0;
}

bit_t hal_queryUsingTcxo(void){
    return 0;
}

uint8_t hal_getTxPowerPolicy(u1_t inputPolicy, s1_t requestedPower,	u4_t freq){
    return LMICHAL_radio_tx_power_policy_paboost;
}

void hal_pollPendingIRQs_helper(){

}

void hal_processPendingIRQs(void){

}

