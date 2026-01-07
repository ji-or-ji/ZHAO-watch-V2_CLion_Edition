#ifndef __SERIAL3_H
#define __SERIAL3_H

#include <stdio.h>

extern char Serial3_RxPacket[];
extern uint8_t Serial3_RxFlag;
extern uint8_t p0, p1,we;

void Serial3_Init(void);
void Serial3_Power_OFF(void);

void Serial3_SendByte(uint8_t Byte);
void Serial3_SendArray(uint8_t *Array, uint16_t Length);
//void Serial3_SendString(char *String);



#endif
