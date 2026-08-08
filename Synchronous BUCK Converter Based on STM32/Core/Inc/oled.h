/**
  * @file    oled.h
  * @brief   0.96寸 OLED 显示模块（SSD1306，128x64，I2C）
  *          软件I2C实现，不占硬件I2C外设，任意两个GPIO即可驱动
  *
  * ────────────────────────────────────────────────────────────
  * 使用方法（主函数中）
  * ────────────────────────────────────────────────────────────
  * 1) 在 main.c 中 include 本头文件：
  *       #include "oled.h"
  *
  * 2) 使用前初始化一次：
  *       OLED_Init();
  *
  * 3) 显示字符串（先画进显存，不会立刻上屏）：
  *       OLED_ShowString(0, 0, "Vout:");   // x:列(0~15) y:行(0~7)
  *
  * 4) 显示数字和浮点数：
  *       OLED_ShowNumber(0, 1, 1234, 4);         // 显示 1234，占4位
  *       OLED_ShowDecimal(0, 2, 12.34f, 2, 2);   // 显示 12.34，整数2位+小数2位
  *
  * 5) 所有画图操作后，调用一次刷新让内容上屏：
  *       OLED_Update();     // 局部刷新，只发变化的区域，不闪烁
  *       ★★★ 记住：只调 ShowXxx 不调 Update，屏幕不会变 ★★★
  *       （先画进显存缓冲，Update 才真正发送到屏幕）
  *
  * 6) 支持的字符（内置6x8 ASCII字体，95个）：
  *       大写A-Z、小写a-z、数字0-9
  *       符号：! " # $ % & ' ( ) * + , - . / : ; < = > ? @ [ \ ] ^ _ ` { | } ~
  *       空格、以及句点.用来显示小数
  *       不支持中文（16x16汉字点阵需额外取模，本模块未包含）
  *
  * 7) 重要技巧：
  *       - 屏幕分辨率128x64，文字高8像素：x是列(0~15)，y是行(0~7)
  *       - 先 ShowXxx 再 Update，Update 会一次性把改动发上屏
  *       - 数字位数不够会补0；浮点小数用 OLED_ShowDecimal
  *       - 想清屏：OLED_Clear(); 然后 OLED_Update();
  *       - 显示是阻塞的（几十微秒），可以每100ms刷新一次，
  *         不用每个主循环都刷新
  *       - 越界安全：x或y超出范围时自动忽略，不会写坏显存
  * ────────────────────────────────────────────────────────────
  */
#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>

#define OLED_WIDTH  128   /* 屏幕宽(像素) */
#define OLED_HEIGHT 64    /* 屏幕高(像素) */
#define OLED_PAGES  8     /* 64/8，显存按页分 */

/* 初始化屏幕（开机调用一次） */
void OLED_Init(void);

/* 清空显存（调用后需 OLED_Update 才生效） */
void OLED_Clear(void);

/* 把显存里的内容刷新到屏幕（局部刷新，只发变化区域，不闪烁） */
void OLED_Update(void);

/* 在(x, y)处显示字符串，x:0~15(列) y:0~7(行) */
void OLED_ShowString(uint8_t x, uint8_t y, const char *str);

/* 在(x, y)处显示整数，len为占位宽度，位数不足补0 */
void OLED_ShowNumber(uint8_t x, uint8_t y, int32_t num, uint8_t len);

/* 在(x, y)处显示浮点数：整数len位+小数点+小数len位 */
void OLED_ShowDecimal(uint8_t x, uint8_t y, float num, uint8_t ilen, uint8_t flen);

#endif /* __OLED_H */
