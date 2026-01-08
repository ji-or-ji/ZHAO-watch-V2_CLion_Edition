#include "stm32f10x.h"               
#include "Delay.h"  
#include "Serial3.h"               

uint8_t CHvolume = 25;//模块音量//最好和配置文件里面一样

//CH7800发送指令
void CH7800_Send(uint8_t CMD,uint8_t DATA)
{
   uint8_t Send_buf[8];	
	
	Send_buf[0] = 0x7E;    //头
    Send_buf[1] = CMD;     //控制指令
	Send_buf[2] = 0x00;    
    Send_buf[3] = 0x02;    //数据长度
    Send_buf[4] = 0x00;
    Send_buf[5] = DATA;    //data
    Send_buf[6] = 0xEF;    //尾

	for (u8 i = 0; i < 7; i ++)	//遍历数组
	{
		Serial3_SendByte(Send_buf[i]);//依次调用Serial3_SendByte发送每个字节数据
	}

}


//唤醒语音循环播放,1--3,3段
void CH7800_Send_reply(void)
{     static uint8_t reply=0;
	
	  reply++;		
	  CH7800_Send(0x16,CHvolume);
		delay_ms(100);
	  CH7800_Send(0x13,reply);
	
	if(reply>=3)reply=0;	
}

//回复语音循环播放,4--6,3段
void CH7800_Send_return(void)
{	static uint8_t retu=3;
	
	  retu++;		
	  CH7800_Send(0x16,CHvolume);
		delay_ms(100);
	  CH7800_Send(0x13,retu);
	
	if(retu>=6)retu=3;	
}


//报时还没写
void CH7800_TelltheTime(uint8_t Hours ,uint8_t minutes)
{	


}


void CH7800_Init(void)
{
    Serial3_Init();
}
