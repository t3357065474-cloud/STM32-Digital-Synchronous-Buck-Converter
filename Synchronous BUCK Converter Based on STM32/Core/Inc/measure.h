/**
  * @file    measure.h
  * @brief   测量换算模块（基于STM32 HAL库）：把ADC原始值换算成电压/电流
  *
  * ────────────────────────────────────────────────────────────
  * 使用方法（主函数中）
  * ────────────────────────────────────────────────────────────
  * 1) 在 main.c 中 include：
  *       #include "measure.h"
  *
  * 2) ADC中断/DMA回调里，把原始值换算成物理量：
  *       vout = measure_voltage(adc_buf[1]);   // ADC码值 → 电压(V)
  *       iout = measure_current(adc_buf[0]);   // ADC码值 → 电流(A)
  *
  * 3) 换算原理（先除满量程得比例，再乘参考电压，再乘增益/校准）：
  *       物理量 = (adc_raw / 4095) × 参考电压 × 电路增益 × 校准系数
  *
  * ────────────────────────────────────────────────────────────
  * 换项目时改 measure.c 顶部的宏，逻辑不用动
  * ────────────────────────────────────────────────────────────
  *   1. ADC参考电压 ADC_VREF：常见 3.3V 或 5V
  *   2. ADC分辨率 ADC_RESOLUTION：常见 4095(12位) 或 1023(10位)
  *   3. 电压通道增益 VOLTAGE_GAIN：分压比（如10倍分压就填10）
  *   4. 电流采样电阻 CURRENT_RSEN_OHM：采样电阻值(Ω)
  *   5. 电流放大倍数 CURRENT_AMP_GAIN：运放放大倍数（如50倍）
  *   6. 校准系数 VOLTAGE_CAL/ CURRENT_CAL：实测标定用，默认1.0
  *   某个通道不用，对应函数不调用即可，不影响其他
  * ────────────────────────────────────────────────────────────
  */
#ifndef __MEASURE_H
#define __MEASURE_H

#include <stdint.h>

/* ADC原始值 → 电压(V) */
float measure_voltage(uint16_t adc_raw);

/* ADC原始值 → 电流(A) */
float measure_current(uint16_t adc_raw);

#endif /* __MEASURE_H */
