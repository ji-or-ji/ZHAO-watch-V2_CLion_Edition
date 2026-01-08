#include "stm32f10x.h" 
#include "menu.h"
#include "OLED.h"
#include "Delay.h"  
#include "Serial2.h"  
#include "RF_Ev1527.h"
#include "CH7800play.h"
#include "Calculator.h"

extern uint8_t POWER_OFF;
uint8_t Voicelogo=0; //被语音识别唤醒标志位
uint8_t Voice=0;
uint8_t VoiceAPP=0;//需要打开应用
uint16_t Delay_await=0;//等待时间
void ASR_dispose(void)
{		
	Voice=1;
	
	CH7800_Send_reply();
	
		while(Voice)   //等待处理完毕//会在串口2中断被处理
		{		
			
			OLED_ShowASCII(0,56,"...?",OLED_6X8);
			OLED_Update();
			
			if(Delay_await>310 || POWER_OFF==0){//如果还没处理,或者被唤醒了，不等了
				Delay_await=0;
				Voicelogo=0;
				Voice=0;
				return;
			}
			Delay_await++;
		}
				
		CH7800_Send_return();
		
		if(VoiceAPP)
		{	
			switch(VoiceAPP)
			{ 
				case 1:shoudt();break;
				case 2:volt();break;
				case 3:WeChat_SK();break;
				case 4:Calculator();break;				
			}
			VoiceAPP=0;
		}		
		else 
		{	
			OLED_ShowASCII(0,56,".OK!",OLED_6X8);
			OLED_UpdateArea(0,56,24,8);	
			delay_ms(300);	
		}
		Voicelogo=0;
		Delay_await=0;
}

















