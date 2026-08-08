/**
  * @file    measure.c
  * @brief   测量换算模块实现：ADC原始值 → 电压/电流
  *          纯换算公式，不碰任何外设（ADC/DMA由外部启动）
  */
#include "measure.h"

/* ──── 换算参数（换项目只需改这里）────
 * 按你的硬件电路填写，公式通用，数值不同而已 */
#define ADC_VREF         3.3f     /* ADC参考电压(V)：多数单片机3.3V */
#define ADC_RESOLUTION   4095.0f  /* ADC满量程：12位=4095，10位=1023 */
#define VOLTAGE_GAIN     10.0f    /* 电压通道增益：分压比，10倍分压填10 */
#define CURRENT_RSEN_OHM 0.01f    /* 电流采样电阻(Ω) */
#define CURRENT_AMP_GAIN 50.0f    /* 电流运放放大倍数：放大50倍填50 */
#define VOLTAGE_CAL      1.10592f     /* 电压校准系数：实测偏大/偏小就调 */
#define CURRENT_CAL      1.0f     /* 电流校准系数：同上 */

/* ADC原始值 → 电压(V)
 * 例：分压10倍，则 (adc/4095)*3.3*10 = 实际电压 */
float measure_voltage(uint16_t adc_raw)
{
  return (adc_raw / ADC_RESOLUTION) * ADC_VREF * VOLTAGE_GAIN * VOLTAGE_CAL;
}

/* ADC原始值 → 电流(A)
 * 电流回路：采样电阻压降 → 运放放大 → ADC
 * 例：0.01Ω电阻 + 50倍放大，则 (adc/4095)*3.3/(0.01*50) = 实际电流 */
float measure_current(uint16_t adc_raw)
{
  return (adc_raw / ADC_RESOLUTION) * ADC_VREF / (CURRENT_RSEN_OHM * CURRENT_AMP_GAIN) * CURRENT_CAL;
}
