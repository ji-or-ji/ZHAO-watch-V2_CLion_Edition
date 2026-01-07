 
/**	
  *     每当我试图编写一段优雅的程序
  *     时而在逻辑的漩涡中打转
  *     时而在语法的暗礁上碰撞
  *     结果却总是像一幅抽象画
  *     令人捧腹却无从理解
  *     或许，我的能力就像那未解的谜题
  *     永远在寻找答案的路上
  *     却总是被自己的思维所困
  *
  *		发一下颠，还是菜(っ °Д°;)っ
  *	
  *     版本V2.10
  *		程序写得比较烂，酌情观看哈
  *     参考(Ctrl+C)了许多大佬的程序，在此表示感谢  o(*￣▽￣...
  *     如果有电池电压不准的情况请修改Time_page里面的BAT_volt_CAL进行校准，电压比实际多的话调小一点点，反之亦然
  *		如果编译有警告是正常的，因为有一些变量范围超了，不影响
  **/

#include <stdint.h>
#include "stm32f10x.h"
#include "OLED.h"
#include "Delay.h"
#include "menu.h"
#include "AD.h"
#include "Serial2.h"
#include "string.h"
#include "key.h"
#include "MyRTC.h"
#include "Time_page.h"
#include "stm32_power.h" 
#include "ASR_su_21t.h"
#include "RF_Ev1527.h"
#include "AHT20.h"
#include "MPU6050.h" //上来就一堆头文件  ;(._. )>
#include "CH7800play.h"

void STM32_rouse(void);

extern uint8_t GUODUsn;

int16_t GyroX;

void do_work(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);
	PWR_BackupAccessCmd(ENABLE);
	if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
	{
		float gx = 40, gy = 24;
		float gt = 62;
		GUODUsn = 1;
		delay_ms(500);
		for (uint16_t i = 0; i < 220; i++)
		{
			OLED_Clear();
			gy /= 1.1;
			if (gy < 0.5)
			{
				gx /= 1.1;
				gt = 22 + gx;
				OLED_ShowASCII(30 + 42 - (gt * 2), 19, "ZHAO-Watch", OLED_8X16);
				OLED_ShowASCII(28 + 22 - gt, 36, "V2.10", OLED_6X8);
				OLED_ClearArea(0, 20, gt, 24);
			}
			OLED_DrawLine(gt, 20, gt, 44 - gy);
			OLED_DrawLine(gt - 1, 20, gt - 1, 44 - gy);
			OLED_Update();
			OLED_transition(1);
		}
		GUODUsn = 1;
		OLED_transition(0);
		OLED_Clear();
		OLED_Update();
	} //还是被你发现了(＃°Д°)//这是真的开机动画o(￣▽￣)ｄ
}

int main(void)
{
	    Key_Init();             //按键初始化

		delay_init();		  //延时函数初始化

		OLED_Init();        //oled显示屏初始化
do_work();   //真的开机动画o(￣▽￣)ｄ
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);      // 开启AFIO时钟
		GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); //重映射配置,用PB3，PB4，PA15做普通IO，PA13&14用于SWD调试

		Time_page_io_init();                                   //表盘功能需要的检测引脚

     	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 	 //配置NVIC为分组2

		RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);//开启PWR的时钟

		AD_Init();				                         //AD转换器初始化

        MyRTC_Init();							       //RTC初始化

		STM32_rouse();                               //唤醒休眠所用的中断配置

		EV1527_IO_Init();                          //驱动搭载的EV1527芯片

		AHT20_Init();							 //温湿度传感器初始化

		CH7800_Init();						   //音频芯片初始化

		Serial2_Init();

		Load_Para();                        //检查BKP寄存器

		int8_t p;                         //存储按键值变量

		uint8_t XIUMIAN=0;              //临时的主屏幕休眠时间变量

		GUODUsn=1;

		   while(GUODUsn)			 //先亮个屏，同时防止按键检测过快
		   {
			   Time_page();
			   delay_ms(4);
			   OLED_transition(1);
		   }

	extern uint8_t RAWA;

   while(1)
   {


		 Time_page();  //刷新表盘


	   p = menu_Roll_event();	//获取按键

	   if(XIUMIAN>200){    //轮循等待关机时间
			p = -1;
		    XIUMIAN=0;
		}
	   else{ XIUMIAN++; }

	   	if(RAWA)//落腕熄屏
		{
			MPU6050_Get_GXData(&GyroX);
			if( GyroX < -4000 )
			{
				POWER_OFF = 1;
			}
		}
	   if( p == 1)
	   {
		   XIUMIAN=0;
		   GUODUsn=1;
		   OLED_transition(0);

			   main_menu2(); //进入菜单2

		   GUODUsn=1;
       }
       else if( p == -1 || POWER_OFF==1)
	   {
		   XIUMIAN=0;
		   GUODUsn=1;
		   OLED_transition(0);

			   POWER_OFF=1;
			   Power_OFF();//进入待机

		   GUODUsn=1;
	   }

		   while(GUODUsn)//这一段是为了防止开机后立即就去判断按键，防止误触发
		   {
			   Time_page();
				delay_ms(4);
			   OLED_transition(1);
		   }

	   p = menu_Enter_event();
	   if(p)
	   {
		   XIUMIAN=0;
		   GUODUsn=1;
		   OLED_transition(0);

			   main_menu1(); //进入菜单1

		   GUODUsn=1;
	   }
   }


}


//中断处理函数
void EXTI0_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line0) == SET)	   //外部中断0号线触发的中断，摇杆下拨
	{		
		POWER_OFF = 0; //取消待机		
		EXTI_ClearITPendingBit(EXTI_Line0);		//清除中断标志位
	}
}

extern uint8_t Pick_up;//拿起检测标志位
extern uint8_t RAWAtri;//抬腕触发值
extern uint16_t RAWAtrigger;//实际抬腕触发值
uint8_t Addtrigger=0;//手表不动累加值

void EXTI9_5_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line6) == SET)		//外部中断6号线触发的中断，语音识别模块
	{
		if(POWER_OFF)SystemInit();
		
		Voicelogo = 1;	
		
		EXTI_ClearITPendingBit(EXTI_Line6);		//清除中断标志位
	}
	
	
	if (EXTI_GetITStatus(EXTI_Line7) == SET)		//外部中断7号线触发的中断，MPU6050
	{		
	    MPU6050_Get_GXData(&GyroX);//读取x轴陀螺仪数据
		if(GyroX>RAWAtrigger)//如果大于触发值
		{ 	
			POWER_OFF = 0; //取消待机
			Addtrigger= 0;
			RAWAtrigger = RAWAtri*70;//恢复触发值
		}
		else if(Pick_up)
		{
			if(-5<GyroX && GyroX<5)//如果手表没有动
			{ 
				if(Addtrigger<80)
				{
					Addtrigger++;//累加80次，实际大约5秒
				}
				else RAWAtrigger=5;//变为运动触发模式，一但检测到有点运动，马上唤醒
			}
			else Addtrigger = 0;
	    }
		
		
		EXTI_ClearITPendingBit(EXTI_Line7);		//清除中断标志位
	}

}



//用于唤醒的外部中断，统一在这里配置
void STM32_rouse(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	
	//PA0在Key_Init();里面已经配置过了，所以直接从PA6开始	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPD;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_6;	//SU-21t模块用
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_7;	//MPU6050用
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	
		
	/*AFIO选择中断引脚*/
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);//将外部中断的0号线映射到GPIOA
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource6);//将外部中断的6号线映射到GPIOA
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource7);//将外部中断的7号线映射到GPIOB
	
	/*EXTI初始化*/
	EXTI_InitTypeDef EXTI_InitStructure;				     //定义结构体变量
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;			    //指定外部中断线使能
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	   //指定外部中断线为中断模式
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;//指定外部中断线为上升沿触发

	EXTI_InitStructure.EXTI_Line = EXTI_Line0;			//选择配置外部中断
	EXTI_Init(&EXTI_InitStructure);					   //将结构体变量交给EXTI_Init，配置EXTI外设
	
	EXTI_InitStructure.EXTI_Line = EXTI_Line6;	     //选择配置外部中断
	EXTI_Init(&EXTI_InitStructure);					//将结构体变量交给EXTI_Init，配置EXTI外设
	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  //指定外部中断线为下降沿触发
	EXTI_InitStructure.EXTI_Line = EXTI_Line7;			    //选择配置外部中断
	EXTI_Init(&EXTI_InitStructure);						   //将结构体变量交给EXTI_Init，配置EXTI外设

	
	
	/*NVIC配置*/
	NVIC_InitTypeDef NVIC_InitStructure;				       //定义结构体变量
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  //指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//指定NVIC线路的抢占优先级为2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;	    //指定NVIC线路的响应优先级为2
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;	  //选择配置NVIC的0线
	NVIC_Init(&NVIC_InitStructure);						 //配置NVIC
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;  
	NVIC_Init(&NVIC_InitStructure);		
}

 //你在找什么( ﹁ ﹁ ) 开机动画吗
//抱歉它不在下面

 //说了不在这里了(づ￣3￣)
//你找不到的（*゜ー゜*）

//我才不会告诉你开机动画在设置里面可以看呢(⊙x⊙;)
