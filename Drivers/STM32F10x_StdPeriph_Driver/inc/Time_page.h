#ifndef __TIME_PAGE_H
#define __TIME_PAGE_H
#include "stm32f10x.h"
#include <string.h>

void Time_page(void);
void Time_page_io_init(void);
void Time_page_Timer(u8 x,u8 y);
void Time_page_small_Timer(u8 x,u8 y);
void Time_page_date(uint8_t x,uint8_t y);
void Time_page_week(uint8_t x,uint8_t y);
void Time_page_BAT(u8 x,u8 y);
void Time_page_Adjust(void);
void Time_page_figures(u8 x,u8 y,u8 num,u8 size);

void Time_page_External_sensors(uint8_t x,uint8_t y);


#endif
