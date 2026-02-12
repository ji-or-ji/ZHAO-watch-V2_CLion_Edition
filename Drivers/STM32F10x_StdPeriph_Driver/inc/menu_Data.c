#include <string.h>
#include <math.h>
#include "menu.h"
#include "Delay.h"
#include "OLED.h"
#include "AD.h"
#include "voltage_mea.h"
#include "MyRTC.h"
#include "MPU6050.h"
#include "CH7800play.h"
#include "Time_page.h"

#include "Serial2.h"
#include "Serial3.h"



void volt(void)
{
	struct option_class2 option_list[] = {
		{"- 退出",Function,RETURN},                  //{选项名，需要运行的函数}
		{"- 电压表",Function,voltage_mt},
		{"- 示波器",Function,voltage_waveform},
		{".."}								//结尾标志,方便计算数量
	};
	run_menu2(option_list);

}

//*****************无线遥控*******************************************//

#include "RF_Ev1527.h"

void YAOKON315M(void)
{
	int8_t p ;
	uint8_t pas = 1,fa=0;
	GUODUsn=1;
	
		OLED_Clear();	   
	    OLED_Update();
	while(1)
	{	
       OLED_transition(1);
		OLED_Clear();
		
		Time_page_small_Timer(0,0);//获取时间并在指定位置显示
		Time_page_BAT(89,0);

		
		OLED_ShowString(4, 24, "当前通道:", OLED_6X8);
		OLED_ShowNum(60,20,pas,2,OLED_8X16);

		
		OLED_ShowString(4, 48, "格式:EV1527", OLED_6X8);
		OLED_ShowString(75, 48, "下拨发送", OLED_6X8);


		if(fa)OLED_DrawRectangle(73,46,52,16,OLED_UNFILLED);
		else OLED_DrawRectangle(58,18,20,20,OLED_UNFILLED);
	
		
		OLED_Update();		
		
	   p=menu_Roll_event();
	   if(p==1){
		   
			if(fa)EV1527_Send(pas);
			else if(pas<15)pas++;	
		   
	   }
	   else if(p==-1)
	   {	
			if(!fa){ if(pas>1)pas--; }	   
	   }

	   
		p=menu_Enter_event();
		if(p==1){
			fa=!fa;			
		}
		else if(p==2)
		{		
			GUODUsn=1;			
			OLED_transition(0);
			return;	
		}
			
	}
}

//***************************************************************//

void DID(void)
{
	int8_t p;
	GUODUsn=1;
	CH7800_Init();
		
		OLED_Clear();	   
	    OLED_Update();
	while(1)
	{
		
	




		
       OLED_transition(1);
	   p = menu_Roll_event();   
	   if( p == 1){ 

		CH7800_Send(Loudness,30);
		delay_ms(100);
		CH7800_Send(Play_audio,1);
		delay_ms(500);
	   }	 
       else if( p == -1){ 


	   }	
		p=menu_Enter_event();
		if(p==1){
			
			GUODUsn=1;
			OLED_transition(0);
			
			//进入某某函数//
			
			GUODUsn=1;
			
		}
		else if ( p==2 ){
		
			GUODUsn=1;
			OLED_transition(0);
			return;
			
		}	
	}
}




//****************Life_Play************************************************//


extern uint8_t OLED_DisplayBuf[8][128];		//拿来吧你

uint8_t Nodes_Cache[8][128];				//计算缓存

void Update_Display(void)					//上传屏幕
{
	memcpy(OLED_DisplayBuf, Nodes_Cache, 1024);
}

void Point_life(uint8_t X, uint8_t Y)		//写点生
{
	Nodes_Cache[(Y/8)][X] |= (0x01 << (Y%8));
}

void Point_death(uint8_t X, uint8_t Y)		//写点死
{
	Nodes_Cache[(Y/8)][X] &= ~(0x01 << (Y%8));
}

uint8_t CountPointRound(uint8_t X, uint8_t Y)		//统计点周围细胞数量
{
	return ( 
	OLED_GetPoint(X-1, Y-1) + 	OLED_GetPoint(X  , Y-1) + 	OLED_GetPoint(X+1, Y-1) + 
	OLED_GetPoint(X-1, Y  ) + 								OLED_GetPoint(X+1, Y  ) + 
	OLED_GetPoint(X-1, Y+1) + 	OLED_GetPoint(X  , Y+1) + 	OLED_GetPoint(X+1, Y+1)
	);
}

void OLED_Rotation_C_P(int8_t CX, int8_t CY, float* PX, float* PY, int16_t Angle)//旋转点
{
	float Theta = (3.14 / 180) * Angle;
	float Xd = *PX - CX;
	float Yd = *PY - CY;
	
	*PX = (Xd) * cos(Theta) - (Yd) * sin(Theta) + CX;// + 0.5;
	*PY = (Xd) * sin(Theta) + (Yd) * cos(Theta) + CY;// + 0.5;
}

void Game_Of_Life_Turn(void)		//刷新游戏回合
{
	uint8_t x, y;
	uint8_t Count;
	
	for(y = 0; y < 64; y ++)
	{
		for(x = 0; x < 128; x ++)
		{
			Count = CountPointRound(x, y);
			if(OLED_GetPoint(x, y))
			{
				if(Count < 2 || Count > 3)
				{
					Point_death(x, y);
				}
			}
			else
			{
				if(Count == 3)
				{
					Point_life(x, y);
				}
			}
		}
	}	
	Update_Display();
}

void Game_Of_Life_Seed(int16_t seed)		//游戏初始化种子
{
	srand(seed);
	for(uint8_t i = 0; i < 8; i ++)
		for(uint8_t j = 0; j < 128; j ++)
	{
		Nodes_Cache[i][j] = rand();
	}
	Update_Display();
}

void Game_Of_Life_Play(void)		//游戏开始
{
	GUODUsn=1;
	
	uint8_t x1 = 8, x2 = 16, y1 = 32, y2 = 32;
	int8_t shift = -1;
	
	
	OLED_Clear();
	uint8_t i, j;
	for (j = 0; j < 8; j ++)				//遍历8页
	{
		for (i = 0; i < 128; i ++)			//遍历128列
		{
			Nodes_Cache[j][i] = 0x00;	//将显存数组数据全部清零
		}
	}

	
//	Game_Of_Life_Seed(1);		//放置种子
	
	int8_t p;
	while(1)
	{
		Game_Of_Life_Turn();
		
		if(shift > 0) {y2 += menu_Roll_event()*8;}
		else {x2 += menu_Roll_event()*8;}
		x2 %= 128;
		y2 %= 64;
		OLED_DrawLine(x1, y1, x2, y2);
		if((x2 - x1) > 1) {x1 += (x2 - x1) / 8 + 1;}
		else if((x2 - x1) < -1) {x1 += (x2 - x1) / 8 - 1;}
		else {x1 = x2;}
		if((y2 - y1) > 1) {y1 += (y2 - y1) / 2 +1;}
		else if((y2 - y1) < -1) {y1 += (y2 - y1) / 2 - 1;}
		else{y1 = y2;}
		
//		OLED_Rotation_C_P(64, 32, &px, &py, Angle);
//		OLED_DrawLine(64, 32, px+0.5, py+0.5);
		
		OLED_Update();
		OLED_transition(1);

		//Delay_ms(100);
		
		p=menu_Enter_event();
		if(p==1) {shift = -shift;}	
		else if ( p==2 ){
			GUODUsn=1;
			OLED_transition(0);
			return;
		}	
	}
}


//*********************************************************************//

void whwhSK(uint8_t w)
{	
	OLED_Clear();
	OLED_Reverse();
		if(w)
		{for(u8 i=128;i>108;i--)
			{	OLED_UpdateArea(i,0,1,64);
				delay_ms(10);
			}
			for(u8 i=108;i>98;i--)
			{	OLED_UpdateArea(i,0,1,64);
				delay_ms(7);
			}
			for(u8 i=98;i>0;i--)
			{	OLED_UpdateArea(i,0,1,64);
				delay_ms(1);
			}
			OLED_Clear();
			OLED_ShowImage(41, 0, 45, 45, SKM_ZFB );
			OLED_ShowString(34, 51, "支付宝收款", OLED_6X8);
			for(u8 i=128;i>28;i--)
			{	OLED_UpdateArea(i,0,1,64);
				delay_ms(1);
			}
			for(u8 i=28;i>8;i--)
			{	OLED_UpdateArea(i,0,1,64);
				delay_ms(7);
			}
			for(u8 i=8;i>0;i--)
			{	OLED_UpdateArea(i,0,1,64);
				delay_ms(10);
			}
		}
		else
		{
			for(u8 i=0;i<20;i++)
			{	OLED_UpdateArea(i,0,1,64);
				delay_ms(10);
			}
			for(u8 i=20;i<30;i++)
			{	OLED_UpdateArea(i,0,1,64);
				delay_ms(7);
			}
			for(u8 i=30;i<128;i++)
			{	OLED_UpdateArea(i,0,1,64);
				delay_ms(1);
			}
			OLED_Clear();
			OLED_ShowImage(41, 0, 45, 45, SKM_WX );
			OLED_ShowString(40, 51, "微信收款", OLED_6X8);
			for(u8 i=0;i<100;i++)
			{	OLED_UpdateArea(i,0,1,64);
				delay_ms(1);
			}
			for(u8 i=100;i<120;i++)
			{	OLED_UpdateArea(i,0,1,64);
				delay_ms(7);
			}
			for(u8 i=120;i<128;i++)
			{	OLED_UpdateArea(i,0,1,64);
				delay_ms(10);
			}
		}
		
}


void WeChat_SK(void)
{
	int8_t p;
	uint8_t w=0;	
	GUODUsn=1;
	while(GUODUsn)
	{
		delay_ms(5);
		OLED_transition(1);	
	}

	whwhSK(w);	
	
	while(1)
	{
		
	   OLED_Update();
		
	   p = menu_Roll_event();   
	   if( p == 1){ 
		  w = !w ;
			whwhSK(w);	
	   }	 
       else if( p == -1){ 
		  w = !w ;
			whwhSK(w);	
	   }	
		p=menu_Enter_event();
		if(p==1){
			
			GUODUsn=1;
			OLED_transition(0);
			return;
			
		}
		else if ( p==2 ){
		
			GUODUsn=1;
			OLED_transition(0);
			return;
			
		}	
	}

}

void ZFB_SK(void)
{	
	int8_t p;
	uint8_t w=1;
	
	GUODUsn=1;
	while(GUODUsn)
	{
		delay_ms(5);
		OLED_transition(1);	
	}

	whwhSK(w);	
	
	while(1)
	{
		
	   OLED_Update();
		
	   p = menu_Roll_event();   
	   if( p == 1){ 
		  w = !w ;
			whwhSK(w);	
	   }	 
       else if( p == -1){ 
		  w = !w ;
			whwhSK(w);	
	   }	
		p=menu_Enter_event();
		if(p==1){
			
			GUODUsn=1;
			OLED_transition(0);
			return;
			
		}
		else if ( p==2 ){
		
			GUODUsn=1;
			OLED_transition(0);
			return;
			
		}	
	}

}


//*******************光标样式调整**************************************************//


extern uint8_t Cursor;

void System_Cursor_BKP(uint8_t ttt)
{ 
	uint16_t temp;
	/*备份寄存器访问使能*/
	PWR_BackupAccessCmd(ENABLE);
	temp = (0xAB<<8) | ttt;
	BKP_WriteBackupRegister(BKP_DR3, temp);
}

void System_Cursor1(void)
{	
	Cursor=0;
	System_Cursor_BKP(Cursor);
}
void System_Cursor2(void)
{ 
	Cursor=1; 
	System_Cursor_BKP(Cursor);
}
void System_Cursor3(void)
{ 
	Cursor=2;
	System_Cursor_BKP(Cursor);
}

void System_Cursor(void)
{
	
	struct option_class2 option_list[] = {
		{"- 返回",1,0},               
		{"- 反色",1, System_Cursor1},                 
		{"- 矩形",1, System_Cursor2},
		{"- 指针",1, System_Cursor3},
		{".."}								
	};
	run_menu2(option_list);
}


//***************抬腕唤醒************************************************//


extern uint8_t POWER_OFF_MODE;

uint8_t RAWA=0;//抬腕唤醒启用标志位
uint8_t RAWAtri=110;//抬腕触发值
uint16_t RAWAtrigger;//实际抬腕触发值
uint8_t Pick_up=0;//拿起检测标志位

void Raise_Wake(void)
{
	if(!POWER_OFF_MODE)//如果低功耗没启用
	{		
		if(RAWA)
		{
			MPU6050_CYCLe_Init();
		}
		else
		{
			MPU6050_RES();
			Pick_up=0;
			RAWAtrigger=RAWAtri*70;
			PWR_BackupAccessCmd(ENABLE);
			BKP_WriteBackupRegister(BKP_DR4, (0xAC<<8)|RAWAtri);//将触发值写入BKP寄存器
		}
	}
	else
	{
		OLED_DrawRectangle(0,31,96,14,OLED_UNFILLED);
		OLED_ShowString(1, 32, "请关闭低功耗模式", OLED_6X8);
		OLED_Update();
		RAWA=0;
		delay_ms(500);
		return;		
	}
}

void Raise_Wake_trigger(void)
{
	uint16_t temp;
	RAWAtrigger=RAWAtri*70;//将抬腕触发值对应到实际触发值，满值17850
	/*备份寄存器访问使能*/
	PWR_BackupAccessCmd(ENABLE);
	temp = (0xAC<<8) | RAWAtri;
	BKP_WriteBackupRegister(BKP_DR4, temp);//将触发值写入BKP寄存器

}

void Pick_UP(void)
{
	if(!RAWA)
	{
		OLED_DrawRectangle(0,31,96,14,OLED_UNFILLED);
		OLED_ShowString(1, 32, "请开启抬腕亮屏", OLED_6X8);
		OLED_Update();
		Pick_up=0;
		delay_ms(500);		
	}
	
}

//******************************************************//

void DGHmode(void)
{
	if(RAWA)
	{
		OLED_DrawRectangle(0,31,84,14,OLED_UNFILLED);
		OLED_ShowString(1, 32, "请关闭抬腕亮屏", OLED_6X8);
		OLED_Update();
		POWER_OFF_MODE=0;
		delay_ms(500);
		return;		
	}
}



//***********系统电压显示*******************************************//

extern float RTI,VIN;

void System_voltage(void)
{
	
	GUODUsn=1;
		int8_t p;
		uint16_t ADValue ;
		float voltage;
	while(1)
	{
	
	    OLED_Clear();	
		
		OLED_ShowFloatNum(0, 2, VIN, 2, 3, OLED_6X8);
		
		ADValue = AD_GetValue(ADC_Channel_8);		
				
		OLED_ShowNum(5, 29, ADValue, 4, OLED_6X8);
		
		voltage = ((float)ADValue * VIN ) / 4095  * 2 ;
		
		OLED_ShowASCII(0,20,"battery",OLED_6X8);		
		OLED_ShowFloatNum(55, 20, voltage, 2, 2, OLED_6X8);

		
		ADValue = AD_GetValue(ADC_Channel_9);
		
		OLED_ShowNum(5, 49, ADValue, 4, OLED_6X8);
		
	    voltage = ((float)ADValue * VIN ) / 4095 ;
		voltage = voltage*RTI;
		
		OLED_ShowASCII(0,40,"external",OLED_6X8);	
		OLED_ShowFloatNum(55, 40, voltage , 2,2, OLED_6X8);
		
		OLED_Update();	
		OLED_transition(1);		
		p=menu_Enter_event();
		if ( p ){
			GUODUsn=1;
			OLED_transition(0);			
			return;
		}	
	}
}

//*****************子菜单—设置—信息*************************************//

//这是给你看的开机动画o(￣▽￣)ｄ，不过开机动画不是这个，你是删不掉真正的开机动画的( ﹁ ﹁ )																																																																								//提示：主要的49
void KAIJIDONGHUA(void){OLED_Clear();OLED_Update();float gx=40,gy=24;float gt=62;GUODUsn=1;delay_ms(500);for(uint16_t i=0;i<220;i++){OLED_Clear();gy/=1.1;if(gy<0.5){gx/=1.1;gt=22+gx;OLED_ShowASCII(30+42-(gt*2),19,"ZHAO-Watch",OLED_8X16);OLED_ShowASCII(28+22-gt,36,"V2.10",OLED_6X8);OLED_ClearArea(0,20,gt,24);}OLED_DrawLine(gt,20,gt,44-gy);OLED_DrawLine(gt-1,20,gt-1,44-gy);OLED_Update();OLED_transition(1);}GUODUsn=1;OLED_transition(0);OLED_Clear();OLED_Update();}

void System_information(void)
{
	struct option_class2 option_list[] = {
		{"- 返回",Function,RETURN},
		{"-----STM32C8T6-----",Display},                 
		{"- RAM: 20K",Display},
		{"- FLASH: 64K",Display},
		{"-------------------",Display},
		{"- 软件版本V2.10",Display},
		{"- BiliBili 时光久泽",Display},
		{"-------------------",Display},
		{"-                 ",Display},
		{"-               ",Display},
		{"-             ",Display},
		{"-           ",Display},
		{"-         ",Display},
		{"-       ",Display},
		{"-     ",Display},
		{"-   ",Display},
		{"- ",Display},
		{"",Display},
		{"- ",Display},
		{"-    ",Display},
		{"-      ",Display},
		{"-        ",Display},
		{"-          ",Display},
		{"-            ",Display},
		{"-              ",Display},
		{"-                ",Display},
		{"-                 ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-",Display},
		{"-             ",Display},
		{"-           ",Display},//这里的空选项占了约0.8kb空间，不要的话可以删掉
		{"- 开机动画",Function,KAIJIDONGHUA},		
		{".."}								
	};
	run_menu2(option_list);
}

//********************USB 响应**************************************************//

void USB_CH7800(void)
{
	CH7800_Send(Loudness,CHvolume);//发个调整音量叫醒一下语音芯片，要不然它不知道在它睡觉的时候有东西插进来了
}


//*****************子菜单—设置*********************************************//

void System_settings(void)
{
	struct option_class2 option_list[] = {
		{"- 返回",Function,0},
		{"- 光标样式",Function, System_Cursor},
		{"- 抬腕亮屏",ON_OFF ,Raise_Wake ,&RAWA},
		{"- 抬腕检测速度",Number ,Raise_Wake_trigger ,&RAWAtri},
		{"- 拿起检测",ON_OFF ,Pick_UP ,&Pick_up},
		{"- 低功耗待机",ON_OFF , DGHmode  ,&POWER_OFF_MODE},
		{"- 系统电压",Function,System_voltage},
		{"- USB 响应",Function,USB_CH7800},
		{"- 扬声器音量",Number,Display,&CHvolume},
		{"- 信息",Function,System_information},
		{".."}								//结尾标志,方便计算数量
	};
	run_menu2(option_list);
}

//***********************************************************//

void error(void)
{
	int8_t p;
	GUODUsn=1;
	
		OLED_Clear();	
		OLED_ShowString(39, 16, "待开发中", OLED_6X8);
		OLED_ShowString(0, 52, "当然你也可以自己来写", OLED_6X8);
		OLED_Update();
	while(1)
	{

		
		
       OLED_transition(1);
	   p = menu_Roll_event();   
	   if(p){ 

			GUODUsn=1;
			OLED_transition(0);
			return;
	   }	 
	   p = menu_Enter_event();
	   if(p){			
			GUODUsn=1;
			OLED_transition(0);
			return;
	   }
	}
}

//*******************手电筒***************************************//

int8_t mubz=50;//手电筒默认亮度
void shoudt(void)
{	
	int8_t p;
	uint16_t temp;
	GUODUsn=1;	
	
	OLED_Clear();	   
	OLED_Update();
	
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseSrtructure;        //TIM时间基数初始化结构体
	TIM_OCInitTypeDef	TIM_OCInitStructure;                //TIM输出比较功能结构体
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);   //使能GPIOA外设时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);    //重映射需开启AFIO时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);    //使能TIM3时钟 
	
	/* 端口（GPIO)设置 */
	GPIO_InitTypeDef  GPIO_InitStructure;                   //GPIO初始化结构体
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_7;                //PA7=T3 Ch2
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;        //速度 50MHz
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;          //复用推挽输出
	GPIO_Init(GPIOA,&GPIO_InitStructure);                  //初始化 GPIOA	
	
	/* TIM时间基数初始化 */
	TIM_TimeBaseSrtructure.TIM_Period= 100 - 1;           //计数器 TIMx_CNT计数 从0累加到ARR次后溢出，设置自动重装载值
	TIM_TimeBaseSrtructure.TIM_Prescaler= 72 - 1;        //设置预分频系数 PSC
	TIM_TimeBaseSrtructure.TIM_ClockDivision= 0;                       //采用时钟分割，见库函数。TIM_Clock_Division_CKD定义，查手册 00:Tdts=Tck_int:01:Tdts=2*Tck_int(2分频）
	TIM_TimeBaseSrtructure.TIM_CounterMode=TIM_CounterMode_Up;        //TIM 向上计数模式溢出
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseSrtructure);					  //初始化TIM3的时钟

	/* TIM外设初始化 */
	TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_PWM1;     			        //选择定时器模式：TIM 脉冲宽度调制模式1,模式1为比较低输出。
	TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;           //输出极性：TIM输出比较极性高
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;     //比较输出使能
	TIM_OCInitStructure.TIM_Pulse = mubz ;                            //设置占空比,即CCR
	
	TIM_OC2Init(TIM3,&TIM_OCInitStructure);
		
	/* 初始化CH1 */
	TIM_OC2PreloadConfig(TIM3,TIM_OCPreload_Enable);                  //CH2 预装载使能
	TIM_ARRPreloadConfig(TIM3,ENABLE);                            //使能 TIMx在ARR上的预装载寄存器

	TIM_Cmd(TIM3, ENABLE);                                    //使能 TIM3	
	
//	TIM_PrescalerConfig
//	TIM_SetAutoreload

	float GP=0 , GP_target;                 //条条长度，目标条条长度
	
	while(1)
	{	
	   OLED_Clear();
		
	   OLED_ShowNum(19,30,mubz,3,OLED_8X16);
	   OLED_ShowASCII(45,30,"%",OLED_8X16);	
		
		GP_target = mubz * 0.9 ;
		if(GP < GP_target)
		{ 
			if( GP_target - GP > 25 ) GP += 5 ;
			else if( GP_target - GP > 7 ) GP += 2.4 ;
			else if( GP_target - GP > 0 ) GP += 0.5 ;
		}
		if(GP > GP_target)
		{ 	
			if( GP - GP_target > 25 ) GP -= 5 ;
			else if( GP - GP_target > 7 ) GP -= 2.4 ;
			else if( GP - GP_target > 0 ) GP -= 0.5 ;
		}
		OLED_DrawRectangle(17, 50, 94 , 10, OLED_UNFILLED);
		OLED_DrawRectangle(19, 52, GP , 6, OLED_FILLED);
		
	   OLED_Update();
       OLED_transition(1);
	   p = menu_Roll_event();   
	   if( p == 1){ 
		   
		   mubz++;
		   if(mubz>100)mubz=100;
		   TIM_SetCompare2(TIM3, (uint8_t)mubz);//写CRR
		   
	   }	 
       else if( p == -1){ 
			mubz--;
		    if(mubz<0)mubz=0;
		    TIM_SetCompare2(TIM3, (uint8_t)mubz);//写CRR
	   }	
		p=menu_Enter_event();
		if(p){
		
			TIM_SetCompare2(TIM3, 0);//写CRR
			
			GUODUsn=1;
			OLED_transition(0);
					
			TIM_Cmd(TIM3, DISABLE);//TIM3失能
			
			temp = (0xAE<<8) | mubz;
			
			/*备份寄存器访问使能*/
			PWR_BackupAccessCmd(ENABLE);
			BKP_WriteBackupRegister(BKP_DR2, temp);//在备份寄存器写入自己规定的标志位，用于判断手电筒亮度是不是第一次执行配置
			
			return;	
		}	
	}
}




//功能模板

//void xxxxxxx(void)
//{
//	int8_t p;
//	GUODUsn=1;
//	
//	while(1)
//	{
//		
//		
//		
//	




//		
//       OLED_transition(1);
//	   p = menu_Roll_event(); //获取按键（上下）返回1为下，-1为上  
//	   if( p == 1){ 


//	   }	 
//       else if( p == -1){ 


//	   }	
//		p=menu_Enter_event(); //获取按键（确定）返回1确定，2长按
//		if(p==1){
//			
//			GUODUsn=1;
//			OLED_transition(0);
//			
//			//进入某某函数//
//			
//			GUODUsn=1;
//			
//		}
//		else if ( p==2 ){
//		
//			GUODUsn=1;
//			OLED_transition(0);
//			return;
//			
//		}	
//	}
//}





