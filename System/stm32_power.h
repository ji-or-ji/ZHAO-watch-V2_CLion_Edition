#ifndef __STM32_POWER_H
#define __STM32_POWER_H

extern uint8_t POWER_OFF;
extern uint8_t POWER_OFF_MODE;

void Power_OFF(void);
void BackEnd(void);
void Load_Para(void); 

#endif
