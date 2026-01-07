#include "stm32f10x.h"    
#include <stdint.h>  
#include <string.h>  
#include <stdio.h> 
#include "OLED.h"
#include "ASR_su_21t.h"
#include "MPU6050.h"

uint8_t POWER_OFF = 0;  //待机标志位，1表示需要待机，不允许人为改动
uint8_t POWER_OFF_MODE = 1;  //选择低功耗模式，0为正常待机功耗稍高，1为低功耗待机功耗低


void BackEnd(void)   
{
	if(Voicelogo)  //检测到语音指令
	{
		OLED_Power(1);
		OLED_WriteCommand(0x81);	//设置对比度
		OLED_WriteCommand(0xFF);   //0x00~0xFF

		ASR_dispose();	
		
		OLED_Clear();				//清空显存数组
		OLED_Update();				//更新显示，清屏	
		OLED_Power(0);
	}
	
}
	

extern uint8_t RAWA;

void Power_OFF(void)
{
	
	GPIO_WriteBit(GPIOA, GPIO_Pin_15, Bit_SET);
	
	Voicelogo=0;
	
	OLED_Clear();				//清空显存数组
	OLED_Update();				//更新显示，清屏
	
	OLED_Power(0);

	if(RAWA)MPU6050_WriteReg(0X38, 0X01);     //开启MPU6050数据就绪中断
	while(POWER_OFF)//如果需要待机
	{	
		if(!POWER_OFF_MODE)
		{   
			PWR_EnterSTOPMode(PWR_Regulator_ON, PWR_STOPEntry_WFI);
		}
		else
		{		/*使能WKUP引脚*/
			PWR_WakeUpPinCmd(ENABLE);	 //使能位于PA0的WKUP引脚，WKUP引脚上升沿唤醒待机模式
			PWR_EnterSTANDBYMode();		//STM32进入停止模式，并等待指定的唤醒事件（WKUP上升沿或RTC闹钟）
		}
		BackEnd();                    //一个后处理函数，决定被唤醒之后要干什么		
	}
	  	SystemInit();               //重新配置时钟

		if(RAWA)MPU6050_WriteReg(0X38, 0X00);     //关闭MPU6050数据就绪中断
	
		OLED_Power(1);

		OLED_WriteCommand(0x81);	//设置对比度
		OLED_WriteCommand(0x00);   //0x00~0xFF
     
	
	GPIO_WriteBit(GPIOA, GPIO_Pin_15, Bit_RESET);	
}






extern int8_t mubz;
extern uint8_t Cursor;
extern uint8_t RAWAtri;//抬腕触发值
extern uint16_t RAWAtrigger;//实际抬腕触发值
extern float RTI;



//检查BKP寄存器，查看是否有需要载入的变量
void Load_Para(void)   
{
	uint16_t temp;

		/*备份寄存器访问使能*/
	PWR_BackupAccessCmd(ENABLE);
	
	//BKP_DR1被RTC用了
	
	temp = BKP_ReadBackupRegister(BKP_DR2);//读取保存在备份寄存器里的手电筒亮度
	if( (temp>>8 )==0xAE ){ mubz = BKP_ReadBackupRegister(BKP_DR2);}//检查标志位

	temp = BKP_ReadBackupRegister(BKP_DR3);
	if( (temp>>8 )==0xAB ){ Cursor = BKP_ReadBackupRegister(BKP_DR3);}//检查标志位

	temp = BKP_ReadBackupRegister(BKP_DR4);
	if( (temp>>8 )==0xAC ){//检查标志位
		RAWAtri = BKP_ReadBackupRegister(BKP_DR4);
		RAWAtrigger=RAWAtri*70;
	}
	
	
	
	temp = BKP_ReadBackupRegister(BKP_DR5);
	if( (temp>>8 )==0xAD ){//检查标志位
		// 合并为uint32_t  
		uint32_t temp = ( (uint32_t)BKP_ReadBackupRegister(BKP_DR6) << 16 ) | (uint32_t)BKP_ReadBackupRegister(BKP_DR7); 
		memcpy(&RTI, &temp, sizeof(float));//转换回float类型
	}
	
	

}
