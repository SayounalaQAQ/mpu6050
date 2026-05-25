#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>

/**
  * @brief   初始化 OLED 屏幕
  */
void OLED_Init(void);

/**
  * @brief   清屏
  */
void OLED_Clear(void);

/**
  * @brief   在指定位置显示一个字符
  * @param   Line    行号（1-4）
  * @param   Column  列号（1-16）
  * @param   Char    要显示的字符
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);

/**
  * @brief   在指定位置显示字符串
  * @param   Line    行号（1-4）
  * @param   Column  列号（1-16）
  * @param   String  要显示的字符串
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);

/**
  * @brief   在指定位置显示无符号整数
  * @param   Line    行号（1-4）
  * @param   Column  列号（1-16）
  * @param   Number  要显示的数字
  * @param   Length  数字位数
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

/**
  * @brief   在指定位置显示有符号整数
  * @param   Line    行号（1-4）
  * @param   Column  列号（1-16）
  * @param   Number  要显示的数字
  * @param   Length  数字位数（不含符号位）
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);

/**
  * @brief   在指定位置显示十六进制数
  * @param   Line    行号（1-4）
  * @param   Column  列号（1-16）
  * @param   Number  要显示的数字
  * @param   Length  数字位数
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

/**
  * @brief   在指定位置显示二进制数
  * @param   Line    行号（1-4）
  * @param   Column  列号（1-16）
  * @param   Number  要显示的数字
  * @param   Length  数字位数
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

#endif
