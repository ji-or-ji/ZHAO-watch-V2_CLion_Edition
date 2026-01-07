#ifndef __SERIAL1_H
#define __SERIAL1_H

#include <stdio.h>

extern char Seria1_RxPacket[];
extern uint8_t Serial_RxFlag;

void Seria1_Init(void);

void Seria1_SendByte(uint8_t Byte);
void Seria1_SendArray(uint8_t *Array, uint16_t Length);
void Seria1_SendString(char *String);



#endif
