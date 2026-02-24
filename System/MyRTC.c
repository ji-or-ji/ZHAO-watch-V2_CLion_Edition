/**
 * @file MyRTC.c
 * @brief 超轻量级 RTC 驱动（不含标准库时间函数）
 *
 * 本文件使用纯手工算法实现日期时间与秒计数器的相互转换，
 * 完全避免了 mktime() 和 localtime() 这两个体积较大的标准库函数。
 * 经测试，可节省约 2~3 KB 的 Flash 空间。
 *
 * 设计基准：2000-01-01 00:00:00 对应秒计数器 0。
 * 时区处理：与原有代码保持一致 —— RTC 计数器保存的是 UTC 时间，
 *          因此在设置时需要将本地时间（东八区）减去 8 小时，
 *          读取时需要将 UTC 时间加上 8 小时得到本地时间。
 *
 * 闰年判断：为简化代码，且手表通常只使用 2000~2099 年，仅判断 year % 4 == 0 即可。
 * 星期计算：已知 2000-01-01 是星期六，通过总天数推算。
 *
 * 作者 ：Deepseek[编写]、是季不是鸡[审查]
 * 时间 ：2026.2.15
 * 版本 ：V1.1.0_for_ZHAO-Watch-V2.10
 */

#include "stm32f10x.h"          // 包含 STM32 标准外设库头文件

void MyRTC_SetTime(void);
void MyRTC_ReadTime(void);

/**
 * @brief 全局时间数组
 *
 * 数组索引及含义：
 *   [0] 年   (2000 ~ 2099)   —— 使用 uint16_t 存储
 *   [1] 月   (1 ~ 12)        —— 使用 uint16_t 存储（实际值 <=12，但为保持数组类型一致，仍为 uint16_t）
 *   [2] 日   (1 ~ 31)        —— uint16_t
 *   [3] 时   (0 ~ 23)        —— uint16_t
 *   [4] 分   (0 ~ 59)        —— uint16_t
 *   [5] 秒   (0 ~ 59)        —— uint16_t
 *   [6] 星期 (0 = 星期日, 1 = 星期一, ..., 6 = 星期六)  —— uint16_t
 */
uint16_t MyRTC_Time[] = {2007, 8, 1, 11, 45, 12, 0};

/* 平年各月份的天数表 */
static const uint8_t monthDays[12] = {
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
};

/**
 * @brief 判断是否为闰年（仅适用于 2000 ~ 2099 年）
 *
 * @param year 年份（例如 2007）
 * @return uint8_t 1 表示闰年，0 表示平年
 * @note  2000~2099 年间，能被 4 整除的年份就是闰年
 *        （因为 2100 年才是世纪年且不能被 400 整除）
 *        若需要支持更广的时间范围，可将判断改为：
 *        ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
 */
static uint8_t isLeapYear(uint16_t year) {
    return year % 4 == 0;
}

/**
 * @brief 将本地日期时间转换为自 2000-01-01 00:00:00 以来的秒数（UTC 基准）
 *
 * @param year   年份 (2000~2099)
 * @param month  月份 (1~12)
 * @param day    日期 (1~31)
 * @param hour   小时 (0~23)
 * @param minute 分钟 (0~59)
 * @param second 秒   (0~59)
 * @return uint32_t 从基准点开始的秒数（注意：此值为本地时间对应的绝对秒数，
 *                  但基准点是 UTC 的 2000-01-01 00:00:00，因此该值可理解为
 *                  “本地时间在 UTC 时间轴上的秒数” —— 即假设输入为 UTC 时间计算的结果。
 *                  若要得到真正的 UTC 秒数（即 RTC 计数器应该保存的值），
 *                  需要根据时区进行修正：本地时间 = UTC 时间 + 时区偏移。
 *                  东八区偏移为 +8 小时，因此 UTC 秒数 = 本函数返回值 - 8*3600。
 */
static uint32_t DateTimeToSeconds(uint16_t year, uint8_t month, uint8_t day,
                                   uint8_t hour, uint8_t minute, uint8_t second) {
    uint32_t days = 0;      // 从基准年到目标日期前一天的总天数
    uint16_t y;
    uint8_t m;

    /* 累加整年的天数 */
    for (y = 2000; y < year; y++) {
        days += isLeapYear(y) ? 366 : 365;
    }

    /* 累加目标年内、目标月份之前各月的天数 */
    for (m = 1; m < month; m++) {
        days += monthDays[m - 1];
        if (m == 2 && isLeapYear(year)) {
            days += 1;      // 闰年 2 月多一天
        }
    }

    /* 加上当月已过的天数（day - 1，因为当天还没过完） */
    days += day - 1;

    /* 将天数转换为秒数，并加上当天的时分秒 */
    uint32_t totalSec = days * 86400UL;          // 一天 86400 秒
    totalSec += hour * 3600UL;
    totalSec += minute * 60UL;
    totalSec += second;

    return totalSec;
}

/**
 * @brief 将从基准点开始的秒数转换为本地日期时间（UTC 基准）
 *
 * @param seconds 从 2000-01-01 00:00:00 开始的秒数（注意：通常传入的是 UTC 秒数，
 *                即 RTC 计数器直接读出的值。若要得到本地时间，需在调用前先加上时区偏移。
 *                东八区偏移为 +8 小时，因此 localSec = utcSec + 8*3600。）
 * @param year    输出年份指针 (uint16_t*)
 * @param month   输出月份指针 (uint8_t*)
 * @param day     输出日期指针 (uint8_t*)
 * @param hour    输出小时指针 (uint8_t*)
 * @param minute  输出分钟指针 (uint8_t*)
 * @param second  输出秒指针 (uint8_t*)
 * @param weekday 输出星期指针 (uint8_t*)
 */
static void SecondsToDateTime(uint32_t seconds, uint16_t *year, uint8_t *month, uint8_t *day,
                               uint8_t *hour, uint8_t *minute, uint8_t *second, uint8_t *weekday) {
    /* 分解出总天数与剩余秒数 */
    uint32_t days = seconds / 86400UL;
    uint32_t remSec = seconds % 86400UL;

    /* 计算星期几 */
    // 基准点 2000-01-01 是星期六，查阅日历可得（或自行验证）
    // 约定星期值：0 = 星期日, 1 = 星期一, ..., 6 = 星期六
    // 2000-01-01 对应星期六 = 6，所以总天数加上 6 后对 7 取余即可
    *weekday = (days + 6) % 7;

    /* 计算时分秒 */
    *hour = remSec / 3600UL;
    remSec %= 3600UL;
    *minute = remSec / 60UL;
    *second = remSec % 60UL;

    /* 根据总天数反推年份 */
    uint16_t y = 2000;
    while (1) {
        uint16_t daysInYear = isLeapYear(y) ? 366 : 365;
        if (days < daysInYear) break;       // 剩余天数不足一年，退出循环
        days -= daysInYear;
        y++;
    }
    *year = y;

    /* 根据剩余天数反推月份和日期 */
    uint8_t m;
    for (m = 1; m <= 12; m++) {
        uint8_t daysInMonth = monthDays[m - 1];
        if (m == 2 && isLeapYear(y)) {
            daysInMonth = 29;                // 闰年 2 月
        }
        if (days < daysInMonth) break;       // 剩余天数不足一个月
        days -= daysInMonth;
    }
    *month = m;
    *day = days + 1;                          // 天数从 0 开始，转换为实际日期需加 1
}

/*------------------------------------------------------------------------------
 * 以下为 RTC 硬件初始化及操作函数，保持原有逻辑不变，仅替换内部实现。
 * 包含两种初始化方式：LSE 版本（默认）和 LSI 版本（注释备选）。
 *----------------------------------------------------------------------------*/

/**
 * @brief RTC 初始化（使用 LSE 时钟）
 *
 * 该函数与原始代码完全一致，仅调用 MyRTC_SetTime() 来设置初始时间，
 * 而 MyRTC_SetTime() 内部已替换为自定义转换函数。
 */
void MyRTC_Init(void) {
    /* 开启 PWR 和 BKP 时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);

    /* 允许访问备份寄存器 */
    PWR_BackupAccessCmd(ENABLE);

    /* 通过备份寄存器标志位判断是否为首次配置 */
    if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5) {
        /* 开启 LSE 并等待就绪 */
        RCC_LSEConfig(RCC_LSE_ON);
        while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) != SET) {}

        /* 选择 LSE 作为 RTC 时钟源，并使能 RTC 时钟 */
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
        RCC_RTCCLKCmd(ENABLE);

        /* 等待 RTC 寄存器同步 */
        RTC_WaitForSynchro();
        RTC_WaitForLastTask();

        /* 设置预分频器，使计数频率为 1Hz（LSE = 32768Hz） */
        RTC_SetPrescaler(32768 - 1);
        RTC_WaitForLastTask();

        /* 写入初始时间（调用 MyRTC_SetTime） */
        MyRTC_SetTime();

        /* 写入标志位，表示已初始化 */
        BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
    } else {
        /* 非首次启动，只需等待同步 */
        RTC_WaitForSynchro();
        RTC_WaitForLastTask();
    }
}

/* 若 LSE 无法起振，可使用以下 LSI 版本的初始化（需取消注释并替换上述函数） */
/*
void MyRTC_Init(void) {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);

    PWR_BackupAccessCmd(ENABLE);

    if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5) {
        RCC_LSICmd(ENABLE);
        while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) != SET);

        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
        RCC_RTCCLKCmd(ENABLE);

        RTC_WaitForSynchro();
        RTC_WaitForLastTask();

        RTC_SetPrescaler(40000 - 1);   // LSI 典型值 40kHz
        RTC_WaitForLastTask();

        MyRTC_SetTime();

        BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
    } else {
        RCC_LSICmd(ENABLE);
        while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) != SET);

        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
        RCC_RTCCLKCmd(ENABLE);

        RTC_WaitForSynchro();
        RTC_WaitForLastTask();
    }
}
*/

/**
 * @brief 设置 RTC 时间（将 MyRTC_Time 数组中的值写入 RTC 计数器）
 *
 * 原有代码使用 mktime 后将结果减去了 8 小时，说明 RTC 计数器保存的是 UTC 时间。
 * 因此本函数先用 DateTimeToSeconds 计算出本地时间对应的绝对秒数，
 * 再减去 8 小时得到 UTC 秒数，最后写入 RTC_CNT。
 *
 * @note 由于 MyRTC_Time 中的月份、日等元素是 uint16_t 类型，但 DateTimeToSeconds
 *       期望 uint8_t 类型的参数，这里通过显式强制转换来消除可能的编译器警告。
 *       鉴于这些值的范围很小，转换是安全的。
 */
void MyRTC_SetTime(void) {
    /* 将 MyRTC_Time 中的本地时间转换为从 2000-01-01 开始的绝对秒数（相当于本地时间在 UTC 轴上的值） */
    uint32_t localSec = DateTimeToSeconds(MyRTC_Time[0],
                                          (uint8_t)MyRTC_Time[1],
                                          (uint8_t)MyRTC_Time[2],
                                          (uint8_t)MyRTC_Time[3],
                                          (uint8_t)MyRTC_Time[4],
                                          (uint8_t)MyRTC_Time[5]);

    /* 转换为 UTC 秒数（东八区减 8 小时） */
    uint32_t utcSec = localSec - 8 * 60 * 60;   // 若您希望 RTC 直接存储本地时间，可删除此行，并将下一行改为 utcSec = localSec

    /* 写入 RTC 计数器 */
    RTC_SetCounter(utcSec);
    RTC_WaitForLastTask();
}

/**
 * @brief 读取 RTC 时间（将 RTC 计数器中的值转换到 MyRTC_Time 数组）
 *
 * 原有代码将 RTC 计数器读出的值加上 8 小时后再调用 localtime，
 * 因此本函数先读取 RTC_CNT（UTC 秒数），加上 8 小时得到本地秒数，
 * 再通过 SecondsToDateTime 转换为日期时间。
 *
 * @note 由于 SecondsToDateTime 要求 month, day 等参数为 uint8_t* 类型，
 *       而 MyRTC_Time 中对应元素是 uint16_t 类型，直接传递指针会导致类型不匹配。
 *       因此我们使用局部 uint8_t 变量接收转换结果，然后再赋值给 MyRTC_Time 数组。
 */
void MyRTC_ReadTime(void) {
    /* 读取 RTC 计数器（UTC 秒数） */
    uint32_t utcSec = RTC_GetCounter();

    /* 转换为本地秒数（东八区加 8 小时） */
    uint32_t localSec = utcSec + 8 * 60 * 60;   // 若 RTC 直接存储本地时间，可删除此行，并将下一行改为 localSec = utcSec

    /* 定义临时变量接收转换结果（均为 uint8_t 类型） */
    uint16_t year;
    uint8_t month, day, hour, minute, second, weekday;

    /* 调用转换函数，将结果存入临时变量 */
    SecondsToDateTime(localSec, &year, &month, &day, &hour, &minute, &second, &weekday);

    /* 将临时变量的值赋给全局数组 MyRTC_Time（注意类型转换，year 本身就是 uint16_t，无需转换） */
    MyRTC_Time[0] = year;
    MyRTC_Time[1] = month;
    MyRTC_Time[2] = day;
    MyRTC_Time[3] = hour;
    MyRTC_Time[4] = minute;
    MyRTC_Time[5] = second;
    MyRTC_Time[6] = weekday;
}