#include "menu.h"
#include "sys.h"
#include "stm32f10x.h"
// #include <math.h>
#include "Time_page.h"
// #include "voltage_mea.h"
#include "AHT20.h"
#include "Calculator.h"

// 菜单框架借用了B站UP 加油哦大灰狼 的menu菜单修改而来，在此表示感谢
// 状态机版本修改说明：
// 1. 将原轮询逻辑重构为状态机，提高可读性和可维护性
// 2. 保持输入输出接口完全不变，确保向后兼容
// 3. 修复了原代码中可能的逻辑缺陷
// 4. 添加了状态机复位和调试功能

/**
 * @brief 按键检测时间参数配置
 * @note d1: 长按识别阈值（单位：函数调用次数）
 *       数值越大，需要按住更长时间才识别为长按
 *       d2: 连击触发间隔（单位：函数调用次数）
 *       数值越大，连击触发频率越低
 */
uint8_t d1 = 15; /**< 长按阈值，原值15 */
uint8_t d2 = 1;  /**< 连击间隔阈值，原值1 */

/**
 * @brief 按键状态机状态定义
 * @details 使用状态机清晰描述按键的完整生命周期
 */
typedef enum {
    KEY_STATE_IDLE = 0,         /**< 空闲状态：按键未按下 */
    KEY_STATE_FIRST_PRESS,      /**< 首次按下：检测到按键按下，等待确认短按/长按 */
    KEY_STATE_REPEAT_TRIGGER,   /**< 重复触发：长按时按固定间隔触发事件 */
    KEY_STATE_LONG_PRESSED,     /**< 长按已识别：仅用于确认键，表示已触发长按事件 */
    KEY_STATE_DEBOUNCE          /**< 消抖状态：预留，可用于按键消抖处理 */
} KeyState_t;

/**
 * @brief 按键状态机结构体
 * @details 每个物理按键对应一个状态机实例
 */
typedef struct {
    KeyState_t current_state; /**< 当前状态 */
    uint8_t press_counter;    /**< 按下计数器，用于计时 */
    uint8_t repeat_counter;   /**< 连击计数器，用于控制连击频率 */
    uint8_t has_pending_event;/**< 事件待处理标志 */
    int8_t event_value;       /**< 待处理的事件值 */
    uint8_t debounce_time;    /**< 消抖时间（预留） */
} KeyFSM_t;

// 全局按键状态机实例声明
static KeyFSM_t roll_up_key_fsm;    /**< 上滚动键（PC13）状态机 */
static KeyFSM_t roll_down_key_fsm;  /**< 下滚动键（PA0）状态机 */
static KeyFSM_t enter_key_fsm;      /**< 确认键（PA1）状态机 */

// 按键状态机是否已初始化标志
static uint8_t key_fsms_initialized = 0;

/**
 * @brief 初始化按键状态机
 * @param p_fsm 指向要初始化的状态机的指针
 * @note 将状态机重置为初始状态，所有计数器清零
 */
static void key_fsm_init(KeyFSM_t* p_fsm) {
    if (p_fsm == NULL) {
        return;
    }

    p_fsm->current_state = KEY_STATE_IDLE;
    p_fsm->press_counter = 0;
    p_fsm->repeat_counter = 0;
    p_fsm->has_pending_event = 0;
    p_fsm->event_value = 0;
    p_fsm->debounce_time = 0;
}

/**
 * @brief 初始化所有按键状态机
 * @note 系统启动时调用，确保状态机处于已知状态
 */
static void key_fsms_initialize_all(void) {
    if (key_fsms_initialized) {
        return;
    }

    key_fsm_init(&roll_up_key_fsm);
    key_fsm_init(&roll_down_key_fsm);
    key_fsm_init(&enter_key_fsm);

    key_fsms_initialized = 1;
}

/**
 * @brief 获取按键GPIO状态
 * @param gpio_port GPIO端口
 * @param gpio_pin GPIO引脚
 * @return uint8_t 1=按键按下（高电平），0=按键释放（低电平）
 * @note 根据硬件连接，确认按键按下时的电平状态
 */
static inline uint8_t get_key_state(GPIO_TypeDef* gpio_port, uint16_t gpio_pin) {
    return (GPIO_ReadInputDataBit(gpio_port, gpio_pin) == Bit_SET) ? 1 : 0;
}

/**
 * @brief 更新滚动键状态机（向上/向下滚动）
 * @param p_fsm 指向滚动键状态机的指针
 * @param is_key_pressed 当前按键是否按下
 * @param event_direction 触发事件时返回的方向值（上=-1，下=1）
 * @note 实现原代码的完整逻辑：
 *       1. 首次按下立即触发事件
 *       2. 长按时先等待d1个周期，然后每d2个周期触发一次
 *       3. 释放后立即返回空闲状态
 */
static void update_roll_key_fsm(KeyFSM_t* p_fsm, uint8_t is_key_pressed, int8_t event_direction) {
    // 参数检查
    if (p_fsm == NULL) {
        return;
    }

    // 每次调用开始时清除待处理事件标志
    p_fsm->has_pending_event = 0;
    p_fsm->event_value = 0;

    // 状态机主逻辑
    switch (p_fsm->current_state) {
        case KEY_STATE_IDLE:
            if (is_key_pressed) {
                // 按键首次按下：立即触发事件
                p_fsm->current_state = KEY_STATE_FIRST_PRESS;
                p_fsm->press_counter = 0;
                p_fsm->has_pending_event = 1;
                p_fsm->event_value = event_direction;

                // 调试跟踪：记录状态转移
                #ifdef KEY_FSM_DEBUG
                debug_log("Roll key: IDLE -> FIRST_PRESS, event=%d", event_direction);
                #endif
            }
            break;

        case KEY_STATE_FIRST_PRESS:
            if (is_key_pressed) {
                // 按键保持按下状态
                p_fsm->press_counter++;

                if (p_fsm->press_counter >= d1) {
                    // 达到长按阈值，进入重复触发状态
                    p_fsm->current_state = KEY_STATE_REPEAT_TRIGGER;
                    p_fsm->repeat_counter = 0;

                    // 触发第一次重复事件
                    p_fsm->has_pending_event = 1;
                    p_fsm->event_value = event_direction;

                    #ifdef KEY_FSM_DEBUG
                    debug_log("Roll key: FIRST_PRESS -> REPEAT_TRIGGER");
                    #endif
                }
            } else {
                // 按键已释放，返回空闲状态
                p_fsm->current_state = KEY_STATE_IDLE;

                #ifdef KEY_FSM_DEBUG
                debug_log("Roll key: FIRST_PRESS -> IDLE (released)");
                #endif
            }
            break;

        case KEY_STATE_REPEAT_TRIGGER:
            if (is_key_pressed) {
                // 按键保持长按状态
                p_fsm->repeat_counter++;

                if (p_fsm->repeat_counter >= d2) {
                    // 达到连击间隔，触发事件并重置计数器
                    p_fsm->repeat_counter = 0;
                    p_fsm->has_pending_event = 1;
                    p_fsm->event_value = event_direction;
                }
            } else {
                // 按键释放，返回空闲状态
                p_fsm->current_state = KEY_STATE_IDLE;

                #ifdef KEY_FSM_DEBUG
                debug_log("Roll key: REPEAT_TRIGGER -> IDLE (released)");
                #endif
            }
            break;

        default:
            // 异常状态，重置为IDLE
            p_fsm->current_state = KEY_STATE_IDLE;
            break;
    }
}

/**
 * @brief 更新确认键状态机
 * @param p_fsm 指向确认键状态机的指针
 * @param is_key_pressed 当前按键是否按下
 * @note 实现原代码的完整逻辑：
 *       1. 按下后立即开始计时
 *       2. 在d1个周期内释放：触发短按事件（返回1）
 *       3. 按下超过d1个周期：触发长按事件（返回2）
 *       4. 长按事件只触发一次，直到按键释放
 */
static void update_enter_key_fsm(KeyFSM_t* p_fsm, uint8_t is_key_pressed) {
    // 参数检查
    if (p_fsm == NULL) {
        return;
    }

    // 每次调用开始时清除待处理事件标志
    p_fsm->has_pending_event = 0;
    p_fsm->event_value = 0;

    // 状态机主逻辑
    switch (p_fsm->current_state) {
        case KEY_STATE_IDLE:
            if (is_key_pressed) {
                // 按键首次按下，开始计时但不立即触发事件
                p_fsm->current_state = KEY_STATE_FIRST_PRESS;
                p_fsm->press_counter = 0;

                #ifdef KEY_FSM_DEBUG
                debug_log("Enter key: IDLE -> FIRST_PRESS");
                #endif
            }
            break;

        case KEY_STATE_FIRST_PRESS:
            if (is_key_pressed) {
                // 按键保持按下状态，继续计时
                p_fsm->press_counter++;

                if (p_fsm->press_counter >= d1) {
                    // 达到长按阈值，触发长按事件
                    p_fsm->current_state = KEY_STATE_LONG_PRESSED;
                    p_fsm->has_pending_event = 1;
                    p_fsm->event_value = 2; // 长按事件值

                    #ifdef KEY_FSM_DEBUG
                    debug_log("Enter key: FIRST_PRESS -> LONG_PRESSED, event=2");
                    #endif
                }
            } else {
                // 按键在达到长按阈值前释放，触发短按事件
                p_fsm->current_state = KEY_STATE_IDLE;
                p_fsm->has_pending_event = 1;
                p_fsm->event_value = 1; // 短按事件值

                #ifdef KEY_FSM_DEBUG
                debug_log("Enter key: FIRST_PRESS -> IDLE, event=1 (short press)");
                #endif
            }
            break;

        case KEY_STATE_LONG_PRESSED:
            if (!is_key_pressed) {
                // 长按后按键释放，返回空闲状态
                p_fsm->current_state = KEY_STATE_IDLE;

                #ifdef KEY_FSM_DEBUG
                debug_log("Enter key: LONG_PRESSED -> IDLE (released)");
                #endif
            }
            // 长按保持期间不重复触发事件
            break;

        default:
            // 异常状态，重置为IDLE
            p_fsm->current_state = KEY_STATE_IDLE;
            break;
    }
}

/**
 * @brief 滚动事件检测函数（状态机版本）
 * @return int8_t 事件返回值：
 *                -1 = 向上滚动（PC13按下）
 *                 1 = 向下滚动（PA0按下）
 *                 0 = 无事件
 * @note 1. 下拨键（PA0）优先级高于上拨键（PC13），与原逻辑一致
 *       2. 函数需要定期调用（建议5-20ms调用周期）
 *       3. 首次按下立即触发，长按时按d2间隔重复触发
 *       4. 注意：PC13是STM32内置用户按键，通常是低电平有效
 *          但原代码使用高电平判断，需要确认硬件连接
 */
int8_t menu_Roll_event(void) {
    // 初始化检查
    if (!key_fsms_initialized) {
        key_fsms_initialize_all();
    }

    // 读取GPIO状态
    // 注意：原代码使用高电平判断按键按下，需要根据实际硬件确认
    uint8_t is_down_pressed = get_key_state(GPIOA, GPIO_Pin_0);  // PA0: 下拨键
    uint8_t is_up_pressed = get_key_state(GPIOC, GPIO_Pin_13);   // PC13: 上拨键

    // 更新下拨键状态机（PA0）
    update_roll_key_fsm(&roll_down_key_fsm, is_down_pressed, 1);

    // 更新上拨键状态机（PC13）
    update_roll_key_fsm(&roll_up_key_fsm, is_up_pressed, -1);

    // 检查事件（下拨优先于上拨，与原逻辑一致）
    if (roll_down_key_fsm.has_pending_event) {
        return roll_down_key_fsm.event_value;
    }

    if (roll_up_key_fsm.has_pending_event) {
        return roll_up_key_fsm.event_value;
    }

    return 0;
}

/**
 * @brief 确认事件检测函数（状态机版本）
 * @return int8_t 事件返回值：
 *                 1 = 短按确认
 *                 2 = 长按确认
 *                 0 = 无事件
 * @note 1. 检测PA1引脚（确认键）
 *       2. 短按：按下后在d1个周期内释放
 *       3. 长按：按下并保持超过d1个周期
 *       4. 长按事件只触发一次，直到按键释放
 */
int8_t menu_Enter_event(void) {
    // 初始化检查
    if (!key_fsms_initialized) {
        key_fsms_initialize_all();
    }

    // 读取确认键GPIO状态（PA1）
    uint8_t is_enter_pressed = get_key_state(GPIOA, GPIO_Pin_1);

    // 更新确认键状态机
    update_enter_key_fsm(&enter_key_fsm, is_enter_pressed);

    // 返回事件值
    if (enter_key_fsm.has_pending_event) {
        return enter_key_fsm.event_value;
    }

    return 0;
}

/**
 * @brief 复位所有按键状态机到空闲状态
 * @note 在以下场景调用：
 *       1. 菜单切换时
 *       2. 系统复位后
 *       3. 进入休眠模式前
 *       4. 发生异常需要恢复时
 */
void menu_reset_all_keys(void) {
    key_fsm_init(&roll_up_key_fsm);
    key_fsm_init(&roll_down_key_fsm);
    key_fsm_init(&enter_key_fsm);

    #ifdef KEY_FSM_DEBUG
    debug_log("All keys reset to IDLE state");
    #endif
}

/**
 * @brief 获取指定按键的当前状态（调试和监控用）
 * @param key_id 按键标识：
 *               0 = 上拨键（PC13）
 *               1 = 下拨键（PA0）
 *               2 = 确认键（PA1）
 * @return KeyState_t 按键的当前状态
 */
KeyState_t menu_get_key_state(uint8_t key_id) {
    switch (key_id) {
        case 0:
            return roll_up_key_fsm.current_state;
        case 1:
            return roll_down_key_fsm.current_state;
        case 2:
            return enter_key_fsm.current_state;
        default:
            return KEY_STATE_IDLE;
    }
}

/**
 * @brief 设置按键时间参数
 * @param long_press_threshold 长按阈值（单位：函数调用次数）
 * @param repeat_interval 连击间隔（单位：函数调用次数）
 * @note 允许运行时调整按键灵敏度
 */
void menu_set_key_params(uint8_t long_press_threshold, uint8_t repeat_interval) {
    d1 = long_press_threshold;
    d2 = repeat_interval;

    #ifdef KEY_FSM_DEBUG
    debug_log("Key params updated: d1=%d, d2=%d", d1, d2);
    #endif
}

/**
 * @brief 获取当前按键参数
 * @param p_long_press 长按阈值输出指针
 * @param p_repeat_interval 连击间隔输出指针
 */
void menu_get_key_params(uint8_t* p_long_press, uint8_t* p_repeat_interval) {
    if (p_long_press) {
        *p_long_press = d1;
    }
    if (p_repeat_interval) {
        *p_repeat_interval = d2;
    }
}

// ==================== 保持与原有代码完全兼容的参数 ====================
// 注意：以下参数名称和值必须保持不变，确保与其他模块的兼容性

int8_t Speed_Factor = 12;    /**< 光标动画速度系数，原值12 */
float  Roll_Speed = 1.3;     /**< 滚动动画速度系数，原值1.3 */

// ==================== 使用示例和注意事项 ====================

/**
 * 典型使用方式：
 *
 * // 在主循环中定期调用（建议5-10ms周期）
 * while (1) {
 *     // 检测滚动事件
 *     int8_t roll_event = menu_Roll_event();
 *     if (roll_event == 1) {
 *         // 处理向下滚动
 *         move_cursor_down();
 *     } else if (roll_event == -1) {
 *         // 处理向上滚动
 *         move_cursor_up();
 *     }
 *
 *     // 检测确认事件
 *     int8_t enter_event = menu_Enter_event();
 *     if (enter_event == 1) {
 *         // 处理短按确认
 *         execute_selection();
 *     } else if (enter_event == 2) {
 *         // 处理长按确认
 *         show_context_menu();
 *     }
 *
 *     // 其他系统任务
 *     delay_ms(5); // 保持稳定的调用周期
 * }
 *
 * 注意事项：
 * 1. 确保GPIO已正确配置为上拉/下拉输入模式
 * 2. 确保调用周期稳定，否则会影响长按时间判断
 * 3. 在系统复位或模式切换时调用menu_reset_all_keys()
 * 4. 如果硬件按键电平与代码判断相反，需要修改get_key_state()函数
 */

// ==================== 硬件配置检查清单 ====================
/**
 * 请确认以下硬件配置：
 *
 * 1. 下拨键（PA0）：
 *    - 连接方式：按下时输出高电平？低电平？
 *    - 上拉/下拉电阻配置
 *
 * 2. 上拨键（PC13）：
 *    - STM32内置用户按键，通常是按下=低电平
 *    - 原代码使用高电平判断，需要确认是否外接了上拉电阻
 *
 * 3. 确认键（PA1）：
 *    - 连接方式：按下时输出高电平？低电平？
 *    - 上拉/下拉电阻配置
 *
 * 如果电平判断相反，修改get_key_state()函数：
 *    原：return (GPIO_ReadInputDataBit(gpio_port, gpio_pin) == Bit_SET) ? 1 : 0;
 *    改为：return (GPIO_ReadInputDataBit(gpio_port, gpio_pin) == Bit_RESET) ? 1 : 0;
 */
/**
  * 函    数：主菜单1
  * 参    数：无
  * 返 回 值：无
  * 说    明：按照格式添加选项，不要超过64个,可能会出问题，下次改成链表试试看
  *
  * 格    式：{ 选项名，需要运行的函数(无参数,无返回值)，1份32x32的数组图片 }
  **/

void main_menu1(void)
{
	
	struct option_class1 option_list[] = {  //引用菜单类型1结构体option_class1
		
		{"返回表盘"	  	  , RETURN	,APPBIPAN	 	}, 
		{"电压测量"  	  , volt   , APPADDV	}, 
		{"315Mhz遥控" 	  , YAOKON315M	, APPYK315Mhz},		
		{"收款微信"   	  , WeChat_SK  , APPWXZF },
		{"收款支付宝"   	  , ZFB_SK  ,APPZFBZF },
		{"Life_Play"	  , Game_Of_Life_Play ,APPGAME}, 
		{"计算器" 	      , Calculator ,APPCalculator	},  
		{"设置"     	  , System_settings  ,APPsetting },  
		{".."}	//结尾标志,方便计算数量
		
	};
	
	run_menu1(option_list);  //将参数载入菜单第1函数刷新显示
}

/**
  * 函    数：主菜单2
  * 参    数：无
  * 返 回 值：无
  * 说    明：按照格式添加选项，不要超过64个,可能会出问题，下次改成链表试试看
  *
  * 格    式：{ 选项名 ， 选项类型 ，需要运行的函数(无参数,无返回值)(可选) , &变量(可选) }
  *
  *	    选项类型:  
  *		Display    仅展示 		  此状态下被执行时，不作任何动作，函数位不填，变量地址位不填
  *		Function   可进入函数:    此状态下被执行时, 进入一次被选中函数，变量地址位不填
  *   	ON_OFF     开关:         此状态下被执行时, 将传入的地址的被选中变量取反一次，再进入被选中函数 
  *		Number     变量:		此状态下被执行时, 将进入在菜单内部的 变量调节器，每调节一次就进入被选中函数一次
  *     //注意，需要返回功能时类型使用Function，需要运行的函数填RETURN，其他不填
  *     //当类型为开关或变量时，RETURN充当空函数作用
  */
void main_menu2(void)
{
	struct option_class2 option_list[] = {	//引用菜单类型2结构体option_class2
		
		{"- 返回"        ,Function  , RETURN  			},
		{"- 手电筒"	     ,Function  , shoudt  			},
		{"- 秒表"	     ,Function  , error   			},                 
		{"- 闹钟"	     ,Function  , error 			},
//		{"- 定时器"      ,Function  , error  			},
		{"- 时间调整"    ,Function  , Time_page_Adjust},
		{".."}	//结尾标志,方便计算数量
		
	};
	 
	run_menu2(option_list);	 //将参数载入菜单2函数刷新显示
}

uint8_t Cursor=0;
extern uint8_t GUODUsn;

/**
  * 函 数：菜单运行
  * 参 数：选项列表
  * 说 明：把选项列表显示出来,并根据按键事件执行相应操作
  */
void run_menu2(struct option_class2* option)
{
	
	int8_t  Catch_i = 1;		   //选中下标
	int8_t  Cursor_i = 0;	      //光标下标
	int8_t  Show_i = 0; 		 //显示起始下标
	int8_t  Max = 0;			//选项数量
	int8_t  Roll_Event = 0;	   //按键事件
	uint8_t Prdl ;            //进度条单格长度
	float  mubiao = 0 , mubiao_up = 0;   //目标进度条长度，上次进度条长度

	
	
	while(option[++Max].Name[0] != '.');    	       //获取条目数量,如果文件名开头为'.'则为结尾;
	Max--;											  //不打印".."
	
	for(int8_t i = 0; i <= Max; i++)				//计算选项名宽度;
	{		
		option[i].NameLen = Get_NameLen(option[i].Name);
	}
	
	static float Cursor_len_d0 = 0, Cursor_len_d1 = 0, Cursor_i_d0 = 0, Cursor_i_d1 = 0;//光标位置和长度的起点终点
	
	int8_t Show_d = 0, Show_i_temp = Max;				        //显示动画相关
		
	Prdl = 64/Max;	  // 屏幕高度 / 条目数量
	
	GUODUsn=1;
	
	while(1)
	{
		OLED_Clear();
		
		Roll_Event = menu_Roll_event();				      //获取按键事件
		if(Roll_Event)							         //如果有按键事件
		{
			Cursor_i += Roll_Event;					   //更新下标
			Catch_i += Roll_Event;
			
			if(Catch_i < 0) {Catch_i = 0;}			//限制选中下标
			if(Catch_i > Max) {Catch_i = Max;}
			
			if(Cursor_i < 0) {Cursor_i = 0;}     //限制光标位置
			if(Cursor_i > 3) {Cursor_i = 3;}
			if(Cursor_i > Max) {Cursor_i = Max;}
		}
		
	/**********************************************************/
	/*显示相关*/
		
		Show_i = Catch_i - Cursor_i;			    //计算显示起始下标
  			                                       
		
		if(Show_i - Show_i_temp)			     //如果下标有偏移
		{
			Show_d = (Show_i - Show_i_temp) * MEHE;
			Show_i_temp = Show_i;
		}
		if(Show_d) {Show_d /= Roll_Speed;}	//滚动变化速度
		
		for(int8_t i = 0; i < 5; i++)	  //遍历显示选项名和对应参数
		{	
			if(Show_i + i > Max ) {break;}			
			OLED_ShowString(2, (i* MEHE)+Show_d +2, option[Show_i + i].Name, OLED_6X8);
			
			
			if( option[Show_i + i].mode == ON_OFF )  				//如果是开关型
			{
				 OLED_DrawRectangle(96, (i* MEHE)+Show_d +2, 12, 12, OLED_UNFILLED);
				if( *( option[Show_i + i].Num) )    				//如果开关打开(传入值为真)
				{
					OLED_DrawRectangle(99, (i* MEHE)+Show_d +5, 6, 6, OLED_FILLED);
				}
			}
			else if( option[Show_i + i].mode == 3 )   				//如果是变量型
			{
				//如果百位不为零
				if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 0 - 1) % 10 )              
				OLED_ShowNum(92, (i* MEHE)+Show_d +6, *( option[Show_i + i].Num), 3, OLED_6X8);
				//如果十位不为零
				else if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 1 - 1) % 10 )
				OLED_ShowNum(95, (i* MEHE)+Show_d +6, *( option[Show_i + i].Num), 2, OLED_6X8);
				//如果个位不为零
				else if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 2 - 1) % 10 )
				OLED_ShowNum(98, (i* MEHE)+Show_d +6, *( option[Show_i + i].Num), 1, OLED_6X8);
				
				else   
				OLED_ShowNum(98, (i* MEHE)+Show_d +6, 0, 1, OLED_6X8);
			}
		}

	/**********************************************************/
	/*光标相关*/
		
	
		Cursor_i_d1 = Cursor_i * MEHE;						//轮询光标目标位置
		Cursor_len_d1 = option[Catch_i].NameLen * WORD_H + 4;	   //轮询光标目标宽度
		
		/*计算此次循环光标位置*///如果当前位置不是目标位置,当前位置向目标位置移动
		if((Cursor_i_d1 - Cursor_i_d0) > 1) {Cursor_i_d0 += (Cursor_i_d1 - Cursor_i_d0) / Speed_Factor + 1;}		
		else if((Cursor_i_d1 - Cursor_i_d0) < -1) {Cursor_i_d0 += (Cursor_i_d1 - Cursor_i_d0) / Speed_Factor - 1;}
		else {Cursor_i_d0 = Cursor_i_d1;}
		
		/*计算此次循环光标宽度*/
		if((Cursor_len_d1 - Cursor_len_d0) > 1) {Cursor_len_d0 += (Cursor_len_d1 - Cursor_len_d0) / Speed_Factor + 1;}
		else if((Cursor_len_d1 - Cursor_len_d0) < -1) {Cursor_len_d0 += (Cursor_len_d1 - Cursor_len_d0) / Speed_Factor - 1;}
		else {Cursor_len_d0 = Cursor_len_d1;}
	
	
	  //显示光标//
		if(Cursor==0)	OLED_ReverseArea2(0, Cursor_i_d0, Cursor_len_d0, 16);           //对指定位置取饭

		else if(Cursor==1) OLED_DrawRectangle(0, Cursor_i_d0, Cursor_len_d0, 16, OLED_UNFILLED);
		
		else if(Cursor==2) OLED_ShowString(Cursor_len_d0, Cursor_i_d0+2, "<-", OLED_6X8);	//尾巴光标
		
							
	/**********************************************************/
		//进度条//
			
		if(Catch_i== Max) {mubiao = 64 ;}	
		else{ mubiao = Prdl * Catch_i ;}
		
		if(mubiao_up<mubiao) { mubiao_up += 1.4 ;}
		if(mubiao_up>mubiao) { mubiao_up -= 1.4 ;}
		
		OLED_DrawLine(123, 0, 127, 0);
		OLED_DrawLine(125, 0, 125, 63);  
		OLED_DrawLine(123, 63, 127, 63);  

		OLED_DrawRectangle(123, 0, 5, mubiao_up, OLED_FILLED);
		
	/**********************************************************/
	   //刷新屏幕//	
		
		OLED_Update();
		
		OLED_transition(1);
		
	/**********************************************************/
	   //获取按键//
		if(menu_Enter_event())			                  	
		{
			if( option[Catch_i].mode == Function )          //如果是可进入函数
			{
				/*如果功能不为空则执行功能,否则返回*/
				if(option[Catch_i].func) 
				{
					
					GUODUsn=1;
					OLED_transition(0);
					
					option[Catch_i].func();
					
					GUODUsn=1;

				}
				else 
				{
				
					GUODUsn=1;
					OLED_transition(0);
					
					return;
				}
			}
			else if( option[Catch_i].mode == ON_OFF )                    //如果是开关
			{
				
				*( option[Catch_i].Num) =  !*( option[Catch_i].Num);  //当前变量取反
				
				if( option[Catch_i].func){option[Catch_i].func();} //执行功能一次
				
			}
			else if( option[Catch_i].mode == Number )            //如果是变量
			{
				
				GUODUsn=1;
				OLED_transition(0);
				
				//内部的变量调节器//
				int8_t p;
				float NP=0 , NP_target = 96;               //变量框框大小，目标框框大小
				float GP=0 , GP_target ;                 //变量条条长度，目标条条长度
//				float tap = 80/255;	     				 //条条单长	0.31
				
				
				GUODUsn=1;	
				
				while(1) 								
				{
					if( NP < NP_target )              //如果变量框框没画完
					{
						if( NP_target - NP > 25 ) NP += 5 ;
						else if( NP_target - NP > 5 ) NP += 2.5 ;
						else if( NP_target - NP > 0 ) NP += 0.5 ;
						
						OLED_DrawRectangle(16, 8, NP, NP/2, OLED_UNFILLED);
						OLED_ClearArea(17, 9, NP-2, NP/2-2);
						
					   OLED_Update();												
					}
					else
					{
//						 GP_target = *( option[Catch_i].Num) * tap ;						
						 GP_target = *( option[Catch_i].Num) * 0.31 ;
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
						
						OLED_ClearArea(24, 43, 80, 3);
						OLED_DrawRectangle(24, 43, GP , 3, OLED_FILLED);												
						
						OLED_ClearArea(55, 20, 18, 8);
						//如果百位不为零
						if ( *( option[Catch_i].Num) / OLED_Pow(10, 3 - 0 - 1) % 10 )              
						OLED_ShowNum(55, 20, *( option[Catch_i].Num), 3, OLED_6X8);
						//如果十位不为零
						else if ( *( option[Catch_i].Num) / OLED_Pow(10, 3 - 1 - 1) % 10 )
						OLED_ShowNum(58, 20, *( option[Catch_i].Num), 2, OLED_6X8);
						//如果个位不为零
						else if ( *( option[Catch_i].Num) / OLED_Pow(10, 3 - 2 - 1) % 10 )
						OLED_ShowNum(61, 20, *( option[Catch_i].Num), 1, OLED_6X8);						
						else   
						OLED_ShowNum(61, 20, 0, 1, OLED_6X8);
						
					   OLED_Update();	
						
					}
					
						
					   OLED_transition(1);
					
					   p = menu_Roll_event();   
					   if( p == 1)
					   { 
							*( option[Catch_i].Num) += 1;
							if( option[Catch_i].func){option[Catch_i].func();} //执行功能一次
							
					   }	 
					   else if( p == -1)
					   { 
							*( option[Catch_i].Num) -= 1;
							if( option[Catch_i].func){option[Catch_i].func();} //执行功能一次
					   }
					   
					   p=menu_Enter_event();
					   if(p==1){
						   
						   GUODUsn=1;
						   OLED_transition(0);
						   OLED_Clear();
						   OLED_Update();	

						   GUODUsn=1;
						   break;											
					   }
					   else if ( p==2 ){
					
						   GUODUsn=1;
						   OLED_transition(0);
						   OLED_Clear();
						   OLED_Update();	

						   GUODUsn=1;
						   break;					
					}					   
				}
											
			}
			
			//如果既不是，变量，开关和函数，一率当做展示处理
			
			
		}
		
	}
}

	/**********************************************************/




void run_menu1(struct option_class1* option)
{
	int8_t  Catch_i = 1;		   //选中下标
	int8_t  Show_i = 0; 		 //显示起始下标
	int8_t  Max = 0;			//选项数量
	int8_t  Roll_Event = 0;	   //按键事件
	
	
	while(option[++Max].Name[0] != '.');    	       //获取条目数量,如果文件名开头为'.'则为结尾;
	Max--;											  //不打印".."
	
	for(int8_t i = 0; i <= Max; i++)				//计算选项名宽度;
	{		
		option[i].NameLen = Get_NameLen(option[i].Name);
	}
		
	int8_t Show_d = 127, Show_i_temp = Max , yi=10;				        //显示动画相关
			
	GUODUsn=1;
	
	float yp=0,ypt=0 ;
	
	OLED_Clear();
	OLED_Update();
	
	static int humidity,temperature;
	
	while(1)
	{
		OLED_Clear();
		
		Roll_Event = menu_Roll_event();				      //获取按键事件
		if(Roll_Event)							         //如果有按键事件
		{
			Catch_i += Roll_Event;
			
			if(Catch_i < 0) {Catch_i = 0;}			//限制选中下标
			if(Catch_i > Max) {Catch_i = Max;}
			
		}
		
	/**********************************************************/
	/*显示相关*/
		
		Show_i = Catch_i ;			    //计算显示起始下标
  			                                       
		
		if(Show_i - Show_i_temp)			     //如果下标有偏移
		{
			Show_d = (Show_i - Show_i_temp) * 40;
			Show_i_temp = Show_i;
			
			yp = option[Catch_i].NameLen * WORD_H ;
		}
		if(Show_d) {Show_d /= 1.15;}	//滚动变化速度
		if(yi) {yi /= 1.15;}	       //横向变化速度

		
		if(ypt < yp)
		{ 
			if( yp - ypt > 10 ) ypt += 5 ;
			else if( yp - ypt > 5 ) ypt += 2.5 ;
			else if( yp - ypt > 0 ) ypt += 0.5 ;
		}
		if(ypt > yp)
		{ 	
			if( ypt - yp > 10 ) ypt -= 5 ;
			else if( ypt - yp > 5 ) ypt -= 2.5 ;
			else if( ypt - yp > 0 ) ypt -= 0.5 ;
		}

		
		for(int8_t i = -2; i < 3; i++)	  //遍历显示选项名和对应参数
		{	
			int8_t temp = i * 40 + Show_d + 16;
			
			if(Show_i + i < 0) {continue;}
			if(Show_i + i > Max ) {break;}			
			
			OLED_ShowImage(78 + yi , temp , 32 , 32 , option[Show_i + i].Image);
			
			uint8_t tp = abs(Show_d/3);
			
			OLED_ShowString( 32 - yp/2, 30+tp, option[Catch_i].Name, OLED_6X8);
			
			OLED_DrawRectangle( 32 - (ypt/2) -2 ,43, ypt+4 ,1 ,OLED_UNFILLED);
			
			OLED_ClearArea(0, 44, 78, 4);
			
		}
		
	/**********************************************************/
	
		Time_page_small_Timer(0,0);	
		Time_page_BAT(-6,8);//这里可能会提示警告，可以不管它
		
	/**********************************************************/
		int Numtemp1,Numtemp2;
	
		AHT20_Get_data(&humidity,&temperature);//获取温湿度信息

		// 获取个位数字  
		Numtemp2 = humidity % 10;  
		// 获取除个位外的其他数字  
		Numtemp1 = humidity / 10;  
		OLED_ShowNum(0,56,Numtemp1,2,OLED_6X8);
		OLED_DrawPoint(0+12+1,56+6);
		OLED_DrawPoint(0+12+1,56+7);
		OLED_DrawPoint(0+12+2,56+6);
		OLED_DrawPoint(0+12+2,56+7);//不要怕，编译器会出手(指帮你提前算好像0+12+2之类的常量)
		OLED_ShowNum(0+12+3,56,Numtemp2,1,OLED_6X8);
		OLED_ShowASCII(0+18+3,56,"%",OLED_6X8);
		
		// 获取个位数字  
		Numtemp2 = temperature % 10;  
		// 获取除个位外的其他数字  
		Numtemp1 = temperature / 10; 		
		OLED_ShowNum(0+30+2,56,Numtemp1,2,OLED_6X8);
		OLED_DrawPoint(0+12+1+30+2,56+6);
		OLED_DrawPoint(0+12+1+30+2,56+7);
		OLED_DrawPoint(0+12+2+30+2,56+6);
		OLED_DrawPoint(0+12+2+30+2,56+7);
		OLED_ShowNum(0+12+3+30+2,56,Numtemp2,1,OLED_6X8);
		OLED_DrawPoint(0+18+3+30+1+2,56);
		OLED_ShowASCII(0+18+3+30+2+2,56,"C",OLED_6X8);
		
	/**********************************************************/
	   //刷新屏幕//	
		
		OLED_UpdateArea(78 + yi, 0, 35, 64);
		OLED_UpdateArea(0, 30, 78, 12);
		OLED_UpdateArea(0, 0, 54, 7);
		OLED_UpdateArea(0, 8, 40, 8);
		OLED_UpdateArea(0, 56, 60, 8);
		
		OLED_transition(1);
		
	/**********************************************************/
	   //获取按键//
		if(menu_Enter_event())			                  	
		{
			/*如果功能不为空则执行功能,否则返回*/
			if(option[Catch_i].func) 
			{
				
				GUODUsn=1;
				OLED_transition(0);
				
				option[Catch_i].func();
				
				OLED_Clear();
				OLED_Update();
				GUODUsn=1;
				yi=10;
				Show_d=-127;

			}
			else 
			{
			
				GUODUsn=1;
				OLED_transition(0);
				
				return;
			}
			
		}
	             
	}

}



    //计算选项名宽度;//
uint8_t Get_NameLen(char* String)
{
	uint8_t i = 0, len = 0;
	while(String[i] != '\0')			        //遍历字符串的每个字符
	{
		if(String[i] > '~'){len += 2; i += 2;}//如果不属于英文字符，因为GB2312编码格式长度加2（i+=2），UTF-8编码格式则加3
		else{len += 1; i += 1;}				 //属于英文字符长度加1
	}
	return len;
}


void OLED_ReverseArea2(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height)
{
	OLED_ReverseArea( X+1, Y, Width-2, 1);
	OLED_ReverseArea( X, Y+1, Width, Height-2);
	OLED_ReverseArea( X+1, Y+Height-1, Width-2, 1);
}
