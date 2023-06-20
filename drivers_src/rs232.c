/*
 * rs232.c
 *
 *  Created on: Mar 27, 2017
 *      Author: Waseemh
 *
 * 	Edited on: Spring 2022
 * 		Author: Jon Andreas Kornberg
 */

#include "../drivers_header/rs232.h"
#include <string.h>

/*
 * private variables
 */
static struct fifo_descriptor fifo_rs232_tx_s;
static fifo_t fifo_rs232_tx = &fifo_rs232_tx_s;
static uint8_t tx_data[FIFO_RS232_TX_SIZE];

static struct fifo_descriptor fifo_rs232_rx_s;
static fifo_t fifo_rs232_rx = &fifo_rs232_rx_s;
static uint8_t rx_data[FIFO_RS232_RX_SIZE];

/*
 * Public variables
*/
extern bool update_GPS;
extern bool get_UNIX_time;




 /*
 * public functions
 */
void rs232_init( void ) {
    if (ENABLE_RS232_FLAG) {
        fifo_create_static(fifo_rs232_tx, tx_data, FIFO_RS232_TX_SIZE, sizeof(uint8_t));
        fifo_create_static(fifo_rs232_rx, rx_data, FIFO_RS232_RX_SIZE, sizeof(uint8_t));

        USART_InitAsync_TypeDef usartInit = USART_INITASYNC_DEFAULT;
        usartInit.enable = usartDisable;
        usartInit.baudrate = RS232_BAUDRATE;

        CMU_ClockEnable(cmuClock_HFPER, true);
        CMU_ClockEnable(RS232_USART_CLK, true);
        CMU_ClockEnable(cmuClock_GPIO, true);


        GPIO_PinModeSet(RS232_PORT, RS232_TX, gpioModePushPull, 1);
        GPIO_PinModeSet(RS232_FORCEOFF_PORT, RS232_FORCEOFF, gpioModePushPull, 1);
        GPIO_PinModeSet(RS232_PORT, RS232_RX, gpioModeInput, 0);

        USART_Reset(RS232_USART);
        USART_InitAsync(RS232_USART, &usartInit);
        //RS232_USART->ROUTE |= (0 << 8) | USART_ROUTE_RXPEN | USART_ROUTE_TXPEN;        //LOC1, TX & RX are enabled
      RS232_USART->ROUTE |= (0 << 8)   | USART_ROUTE_TXPEN;                              //LOC1, TX  are enabled



        USART_IntClear(RS232_USART, _USART_IF_MASK);
        USART_IntEnable(RS232_USART, USART_IF_RXDATAV);
		NVIC_ClearPendingIRQ(RS232_TX_IRQn);
        NVIC_ClearPendingIRQ(RS232_RX_IRQn);
		NVIC_EnableIRQ(RS232_TX_IRQn);
        NVIC_EnableIRQ(RS232_RX_IRQn);
    }
    else rs232_shutdown();
}

void rs232_enable( void ) {
    if (ENABLE_RS232_FLAG) USART_Enable(RS232_USART, usartEnable);
}


void rs232_disable( void ) {
    if (ENABLE_RS232_FLAG) USART_Enable(RS232_USART, usartDisable);
}


void rs232_transmit_string(char* data, uint8_t length) {
    if (ENABLE_RS232_FLAG) {
		USART_IntDisable(RS232_USART, USART_IF_TXBL); //prevent race condition with ISR
        for (uint8_t i = 0; i < length; i++) {
			fifo_add(fifo_rs232_tx, &data[i]);
        }
		USART_IntEnable(RS232_USART, USART_IF_TXBL);
    }
}


void rs232_transmit_char(char data) {
    if (ENABLE_RS232_FLAG) {
		USART_IntDisable(RS232_USART, USART_IF_TXBL); //prevent race condition with ISR
		fifo_add(fifo_rs232_tx, &data);
		USART_IntEnable(RS232_USART, USART_IF_TXBL);
    }
}


char rs232_receive( void ) {
	char temp_char = 0;
    if (ENABLE_RS232_FLAG) {
		USART_IntDisable(RS232_USART, USART_IF_RXDATAV); //prevent race condition with ISR
        fifo_get(fifo_rs232_rx, &temp_char);
		USART_IntEnable(RS232_USART, USART_IF_RXDATAV);
    }
	return temp_char;
}


void rs232_reset( void ) {
    if (ENABLE_RS232_FLAG) USART_Reset(RS232_USART);
}


void rs232_shutdown( void ) {
	GPIO_PinOutClear(RS232_FORCEOFF_PORT, RS232_FORCEOFF);
}



void parse_rs232_buffer(void){
  static char rs232_rx_buffer[1024];
  rs232_rx_buffer[1023] = 0; // terminate string
  uint16_t len = 0;
  char c = 1;

  while(c != 0){
      c = rs232_receive();
      //rs232_transmit_char(c);
      if (len < 1023)
          rs232_rx_buffer[len++] = c;
  }

  if (len <=2 )
    return;

  int i = 0;
  while(i<=len){
      c = rs232_rx_buffer[i];
      switch (c)
      {
        case '#': // command from host computer
          //debug_str("getting somewhere\n");
          parse_rs232_command(&rs232_rx_buffer[i]); // todo make this function
          break;

      }
      i++;
  }
}


/*
void parse_rs232_command(char * msg_str){
/*
 * check for commands sent to the slim from host computer
 * todo if more commands are implemented, make typdef struct with dedicated commands.
 */

   //debug_str(msg_str);
   /*if (strcmp(msg_str, "#get_GPS_Pos\n") == 0) {
       update_GPS = true;
   }
}*/

void parse_rs232_command(char * msg_str){
    /*
    * check for commands sent to the slim from host computer
    * todo if more commands are implemented, make typdef struct with dedicated commands.
    */

    char substring[14]; // Allocate enough space for 14 characters plus the null terminator
    strncpy(substring, msg_str, 13); // Copy the first 14 characters
    substring[13] = '\0'; // Ensure the substring is null-terminated

    //debug_str(substring); // If you want to debug the substring, use this

    if (strcmp(substring, "#get_GPS_Pos\n") == 0) {
        update_GPS = true;
    }
    else if(strcmp(substring, "#get_UNIX_tim") == 0){
        get_UNIX_time = true;
    }
}




/*
 * INT handlers
 */

void RS232_TX_ISR(){
    uint32_t int_mask = USART_IntGetEnabled(RS232_USART);
    if (int_mask & USART_IF_TXBL) {
		USART_IntClear(RS232_USART, USART_IF_TXBL);

        if (ENABLE_RS232_FLAG) {
            if (!fifo_is_empty(fifo_rs232_tx)) {
            uint8_t c;
            fifo_get(fifo_rs232_tx, &c);
            USART_Tx(RS232_USART, c);
            }
        else {
          USART_IntDisable(RS232_USART, USART_IF_TXBL);
          }
        }
    }
}



void RS232_RX_ISR(){
    uint32_t int_mask = USART_IntGetEnabled(RS232_USART);
    if (int_mask & USART_IF_RXDATAV) {
          USART_IntClear(RS232_USART, USART_IF_RXDATAV);
        if (ENABLE_RS232_FLAG) {
            uint8_t c = USART_Rx(RS232_USART);
            fifo_add(fifo_rs232_rx, &c);
			      //rs232_transmit_char(c);
          }
    }
}

