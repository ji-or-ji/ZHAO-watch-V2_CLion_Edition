#ifndef __OLED_DATA_H
#define __OLED_DATA_H

#include <stdint.h>

/*中文字符字节宽度*/
#define OLED_CHN_CHAR_WIDTH			3		//UTF-8编码格式给3，GB2312编码格式给2

/*字模基本单元*/
typedef struct 
{
	char Index[OLED_CHN_CHAR_WIDTH + 1];	//汉字索引
	uint8_t Data[24];						//字模数据
} ChineseCell_t;

/*ASCII字模数据声明*/
extern const uint8_t OLED_F8x16[][16];
extern const uint8_t OLED_F6x8[][6];

/*汉字字模数据声明*/
extern const ChineseCell_t OLED_CF12x12[];

/*图像数据声明*/
extern const uint8_t Diode[];

extern const uint8_t t1[];
extern const uint8_t t2[];
extern const uint8_t t3[];
extern const uint8_t t4[];
extern const uint8_t t5[];
extern const uint8_t t6[];
extern const uint8_t t7[];
extern const uint8_t t8[];
extern const uint8_t t9[];
extern const uint8_t t0[];

extern const uint8_t i1[];
extern const uint8_t i2[];
extern const uint8_t i3[];
extern const uint8_t i4[];
extern const uint8_t i5[];
extern const uint8_t i6[];
extern const uint8_t i7[];
extern const uint8_t i8[];
extern const uint8_t i9[];
extern const uint8_t i0[];

extern const uint8_t mp3repetition[];
extern const uint8_t mp3play[];
extern const uint8_t mp3stop[];
extern const uint8_t mp3left[];
extern const uint8_t mp3right[];

extern const uint8_t bug[];

extern const uint8_t APPsetting[];
extern const uint8_t APPYK315Mhz[];
extern const uint8_t APPWXZF[];
extern const uint8_t APPZFBZF[];
extern const uint8_t APPADDV[];
extern const uint8_t APPBIPAN[];
extern const uint8_t APPGAME[];
extern const uint8_t APPCalculator[];



extern const uint8_t APPTLYsmall[];

extern const uint8_t SKM_WX[];
extern const uint8_t SKM_ZFB[];


/*按照上面的格式，在这个位置加入新的图像数据声明*/
//...

#endif


/*****************江协科技|版权所有****************/
/*****************jiangxiekeji.com*****************/
