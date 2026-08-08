/**
  * @file    uart.h
  * @brief   串口通讯模块（基于STM32 HAL库，固定走USART1）
  *
  * ────────────────────────────────────────────────────────────
  * 使用方法（主函数中）
  * ────────────────────────────────────────────────────────────
  * 1) 在 main.c 中 include 本头文件：
  *       #include "uart.h"
  *    无需初始化，直接调用。
  *
  * 2) 打印文本（调试信息、状态值，用法同 printf）：
  *       uart_printf("Vout = %.2f V\r\n", vout);
  *       常用格式：%d 整数  %.2f 保留2位小数的浮点  %x 十六进制
  *       注意：printf 不会自动换行，字符串结尾要自己加 \r\n
  *
  * 3) 发波形数据（VOFA+ JustFloat 协议，上位机画实时曲线）：
  *       float data[2] = {vout, iout};
  *       uart_send_vofa(data, 2);   // 发2个float给VOFA+
  *       上位机选 JustFloat 协议，波特率115200（与代码一致）
  *
  * 4) 重要技巧：
  *       - 两个函数都是【阻塞发送】，只能在主循环里低频调用
  *         （如每100ms一次）。禁止在 ADC 等高速中断里调用，
  *         否则会拖死系统。
  *       - 打印内容多时，优先用 uart_send_vofa 发波形、
  *         uart_printf 发事件日志，不要每个周期都打印。
  *       - uart_printf 的 buf 是128字节，一次别打印太长
  *         的内容（超过128字符会截断）。
  *       - 串口换到别的 USART 或别的波特率：改 uart.c 顶部
  *         extern UART_HandleTypeDef huart1 那一行（句柄名）。
  *         波特率在 CubeMX 里改。
  * ────────────────────────────────────────────────────────────
  */
#ifndef __UART_H
#define __UART_H

#include <stdint.h>

void uart_printf(const char *fmt, ...);

/* VOFA+ JustFloat协议：发送n个float数据（带帧尾），上位机解析成实时波形
 * 例：float data[2]={vout, iout}; uart_send_vofa(data, 2);
 * 阻塞发送，只能在主循环里低频调用，禁止在中断里用 */
void uart_send_vofa(const float *data, uint8_t n);

#endif /* __UART_H */
