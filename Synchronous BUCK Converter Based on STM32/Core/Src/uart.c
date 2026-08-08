/**
  * @file    uart.c
  * @brief   串口通讯模块实现（基于STM32 HAL库，固定走USART1）
  *          无需初始化，include 后直接调用
  */
#include "uart.h"
#include "main.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <stdarg.h>

/* ──── 移植配置（换项目必看）──── */
/* 如果新项目用的串口句柄不叫 huart1（比如 huart2），改下面这行的名字即可 */
extern UART_HandleTypeDef huart1;

void uart_printf(const char *fmt, ...)
{
  char buf[128];
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  if (n > 0)
  {
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)n, HAL_MAX_DELAY);
  }
}

/* VOFA+ JustFloat协议：连续发n个float(小端4字节) + 帧尾 0x00 0x00 0x80 0x7F */
void uart_send_vofa(const float *data, uint8_t n)
{
  if (data == 0 || n == 0) return;

  HAL_UART_Transmit(&huart1, (uint8_t *)data, (uint16_t)n * 4, HAL_MAX_DELAY);

  static const uint8_t tail[4] = {0x00, 0x00, 0x80, 0x7F};
  HAL_UART_Transmit(&huart1, (uint8_t *)tail, 4, HAL_MAX_DELAY);
}
