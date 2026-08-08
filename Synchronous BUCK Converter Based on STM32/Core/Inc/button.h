/**
  * @file    button.h
  * @brief   按键检测模块（基于STM32 HAL库）
  *
  * ────────────────────────────────────────────────────────────
  * 使用方法（主函数中）
  * ────────────────────────────────────────────────────────────
  * 1) 在 main.c 中 include 本头文件：
  *       #include "button.h"
  *
  * 2) 在主循环 while(1) 里，直接调用下面两个函数：
  *       // 连续调节类按键（如电压加减）：短按+1次，长按自动连续+1
  *       if (Button_IsPressed(ADD_GPIO_Port, ADD_Pin))
  *       {
  *         v_preref += 0.1f;      // 长按期间会每隔50ms自动进入一次
  *       }
  *
  *       // 单击确认类按键（如启动/确认）：长按不会连发，只响应一次
  *       if (Button_IsClicked(MODE_GPIO_Port, MODE_Pin))
  *       {
  *         HAL_GPIO_WritePin(PWM_SD_GPIO_Port, PWM_SD_Pin, GPIO_PIN_SET);
  *         HAL_GPIO_WritePin(PROTECT_GPIO_Port, PROTECT_Pin, GPIO_PIN_SET);
  *       }
  *
  * 3) 选函数的原则：
  *       想要“按住连续调”  → 用 Button_IsPressed
  *       想要“只认一次单击” → 用 Button_IsClicked
  *
  * 4) 重要技巧：
  *       - 每个按键在主循环里每圈【只调用一次】，不要同一个按键
  *         在两处都调，否则状态会错乱。
  *       - 该模块为【非阻塞】实现，内部用 HAL_GetTick 计时，
  *         不占用 while 循环时间，不会卡死主循环。
  *       - 【只能在主循环等正常循环里调用】。禁止在中断里调用，
  *         中断里 HAL_GetTick 不走，计时会乱。
  *       - 低电平按下（按键接GND）是默认配置；若你的按键是
  *         高电平按下（按键接VCC），改 button.c 顶部的
  *         BTN_ACTIVE_LEVEL 为 GPIO_PIN_SET 即可。
  *       - 消抖20ms、长按0.3s起效、连发间隔50ms，如需调整
  *         改 button.c 顶部的宏即可，逻辑不用动。
  * ────────────────────────────────────────────────────────────
  */
#ifndef __BUTTON_H
#define __BUTTON_H

#include "stm32f1xx_hal.h"

/* 只要按下就返回1：短按返回一次，长按(0.3秒后)每50ms返回一次
 * 用于连续调节的场景，如 ADD/REDUCE 调电压 */
uint8_t Button_IsPressed(GPIO_TypeDef *port, uint16_t pin);

/* 只有短按返回1：长按不重复
 * 用于单击确认的场景，如 MODE 启动/复位、SET 确认 */
uint8_t Button_IsClicked(GPIO_TypeDef *port, uint16_t pin);

#endif /* __BUTTON_H */
