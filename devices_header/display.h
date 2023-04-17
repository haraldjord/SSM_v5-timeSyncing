/*
 * display.c
 *
 *  Created on: 6. sep. 2018
 *      Author: mvols
 *
 *  Expanded on: Feb 2020
 *      Author: MariusSR
 *
 */

#ifndef DEVICES_HEADER_DISPLAY_H_
#define DEVICES_HEADER_DISPLAY_H_

#include "../drivers_header/spi.h"
#include "../drivers_header/delay.h"

char display_str_buf[256];

typedef enum{
	font_small=0,
	font_medium,
	font_large
}font_size_t;

#define DISPLAY_LINES 			128
#define DISPLAY_DOTS_PER_LINE	128

#define DISPLAY_UPDATE_FLAG	(1<<0)
#define DISPLAY_COM			(1<<1)
#define DISPLAY_ALL_CLEAR	(1<<2)

void display_init( void );

void display_dummy( void );
void display_clear_and_update( void );
void display_update( void );

void display_put_string(uint8_t x, uint8_t y, char* data, font_size_t size);
void display_draw(uint8_t value, uint8_t x, uint8_t y);
void display_draw_horisontal_line(uint8_t y, uint8_t x_start, uint8_t x_end);
void display_draw_horisontal_whole_line(uint8_t y);
void display_draw_vertical_line(uint8_t x, uint8_t y_start, uint8_t y_end);
void display_draw_vertical_whole_line(uint8_t x);
void draw_box(uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end);

void display_activity_dot_outline( void );
void display_activity_dot(bool show);
void display_activity_dot_toggle( void );

void display_clear( void );
void display_clear_main_area( void );
//void display_clear_line(uint8_t line);
//void display_clear_lines(uint8_t from_line, uint8_t to_line);
void display_clear_lines_from(uint8_t from_line);

#endif /* DEVICES_HEADER_DISPLAY_H_ */
