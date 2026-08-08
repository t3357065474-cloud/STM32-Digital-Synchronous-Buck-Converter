/**
  * @file    pid.c
  * @brief   PID 控制模块实现（增量式PID，纯C零依赖）
  *
  * 增量式PID：
  *   输出 = 上次输出 + Δu
  *   Δu = kp*(ek - ek_1) + ki*ek + kd*(ek - 2*ek_1 + ek_2)
  * 内置：
  *   - 抗积分饱和：输出卡限幅时削弱积分，防过冲
  *   - 微分滤波：D项过一阶低通，抑制ADC高频噪声
  */
#include "pid.h"

/* 微分滤波系数：越小滤波越强，一般0.7~0.95 */
#define PID_D_FILTER_ALPHA  0.9f

void PID_Init(PID_Handle *pid, float kp, float ki, float kd,
              float delta_max, float output_max, float output_min)
{
  if (pid == 0) return;
  pid->kp          = kp;
  pid->ki          = ki;
  pid->kd          = kd;
  pid->delta_max   = delta_max;
  pid->output_max  = output_max;
  pid->output_min  = output_min;
  pid->ek_1        = 0.0f;
  pid->ek_2        = 0.0f;
  pid->output      = output_min;   /* 从下限起步，配合软启动从0爬升 */
  pid->d_filt      = 0.0f;
}

void PID_Reset(PID_Handle *pid)
{
  if (pid == 0) return;
  pid->ek_1   = 0.0f;
  pid->ek_2   = 0.0f;
  pid->output = pid->output_min;   /* 回到下限(0)，软启动从0爬升 */
  pid->d_filt = 0.0f;
}

float PID_Update(PID_Handle *pid, float setpoint, float feedback)
{
  float ek;         /* 本次误差 */
  float delta;      /* 本次增量 Δu */

  if (pid == 0) return 0.0f;

  ek = setpoint - feedback;               /* 误差 = 目标 - 实际 */

  /* 抗积分饱和：输出卡在限幅（说明输出已到极限还在加），削弱本次积分，
   * 避免误差反向后积分还很大导致过冲 */
  float anti_windup = 1.0f;
  if (pid->output >= pid->output_max) anti_windup = 0.0f;
  if (pid->output <= pid->output_min) anti_windup = 0.0f;

  /* 三部分增量 */
  float p = pid->kp * (ek - pid->ek_1);               /* 比例：误差变化率 */
  float i = pid->ki * ek * anti_windup;               /* 积分：当前误差(带抗饱和) */

  /* 微分：误差二阶差分，先过一阶低通滤波 */
  float d_raw = pid->kd * (ek - 2*pid->ek_1 + pid->ek_2);
  pid->d_filt = PID_D_FILTER_ALPHA * pid->d_filt + (1.0f - PID_D_FILTER_ALPHA) * d_raw;
  float d = pid->d_filt;

  delta = p + i + d;

  /* 单次增量限幅：防止输出突变导致震荡 */
  if (delta >  pid->delta_max) delta =  pid->delta_max;
  if (delta < -pid->delta_max) delta = -pid->delta_max;

  pid->output += delta;                   /* 累加：增量式核心 */

  /* 输出总限幅 */
  if (pid->output > pid->output_max) pid->output = pid->output_max;
  if (pid->output < pid->output_min) pid->output = pid->output_min;

  /* 保存历史误差 */
  pid->ek_2 = pid->ek_1;
  pid->ek_1 = ek;

  return pid->output;
}
