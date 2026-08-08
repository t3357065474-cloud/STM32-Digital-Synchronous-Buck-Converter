/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "button.h"
#include "uart.h"
#include "oled.h"
#include "pid.h"
#include "measure.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define VIN_MAX 24.0f//输入电压最大值
#define VOUT_MAX (VIN_MAX*0.95f)//输出电压最大值
#define VOUT_MIN 0.0f//输出电压最小值
#define IOUT_TRIP 0.8f//输出电流跳闸阈值
#define PWM_ARR 3599U//ARR的值
#define PWM_CCR_MAX ((uint16_t)(PWM_ARR * 0.95f))//CCR的最大值。




/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
//volatile 单词意思为易变的，让系统不要随意优化他，注意他时刻会变。
volatile uint16_t adc_buf[2] = {0};//定义一个全局变量：存放两个十六位的采样数据，一个电流一个电压。
volatile float vout = 0.0f;//定义输出电压。
volatile float iout = 0.0f;//定义输出电流。
volatile float vout_avg = 0.0f;//输出平均电压（最近10帧平均，供OLED显示）
volatile float iout_avg = 0.0f;//输出平均电流（最近10帧平均，供OLED显示）
static volatile float vout_sum = 0.0f;//ADC回调里vout的累计和，OLED刷新取平均后清零
static volatile float iout_sum = 0.0f;//ADC回调里iout的累计和，OLED刷新取平均后清零
static volatile uint32_t vout_cnt = 0;//本次OLED帧内vout采样次数
static volatile uint32_t iout_cnt = 0;//本次OLED帧内iout采样次数
volatile float v_ref = 0.0f;//定义目标电压。
volatile float v_preref = 0.0f;//定义预设目标电压。
volatile float v_ref_ramp = 0.0f;//斜坡目标值：启动时从0逐步爬到v_ref，实现软启动
volatile uint16_t pwm_ccr = 0;//定义PWM。
volatile uint8_t sys_state = 0;//系统状态：0=待机冷态(继电器断开驱动关断) 1=正常输出 2=过流锁存
PID_Handle v_pid;   //电压环PID实例
volatile uint8_t pid_enable = 0;//PID使能：1=运行(正常输出态)，0=不运行(待机/锁存)




/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* 初始化电压环PID：kp=0.01 ki=1 单次增量限幅5 输出0~PWM_CCR_MAX（初值，实测再调） */
  PID_Init(&v_pid, 0.01f, 1.0f, 0.0f, 5, PWM_CCR_MAX, 0);

  HAL_ADC_Start_DMA (&hadc1,(uint32_t *)adc_buf,2);//开启DMA，每次搬两个数据到adc_buf存储。
  HAL_TIM_PWM_Start (&htim1,TIM_CHANNEL_1);//开启PWM。

  /* TIM1更新中断已在CubeMX的NVIC里使能，这里只需启动更新中断事件 */
  HAL_TIM_Base_Start_IT(&htim1);

  OLED_Init();//初始化OLED

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (sys_state == 0)//待机冷态，按MODE启动输出(接通继电器+使能驱动)
    {
      if (Button_IsClicked(MODE_GPIO_Port, MODE_Pin))
      {
        sys_state = 1;//把系统设置为正常输出
        pid_enable = 1;   // 启动PID电压环
        v_ref_ramp = 0.0f;   // 软启动：斜坡目标从0开始
        pwm_ccr = 0;   // 软启动：占空比强制从0开始，由PID逐周期爬升，避免上电大电流冲击
        PID_Reset(&v_pid);   // 清零PID内部输出/误差，软启动真正从0爬升
        HAL_GPIO_WritePin (PWM_SD_GPIO_Port,PWM_SD_Pin,GPIO_PIN_SET);//恢复IR2104使能
        HAL_GPIO_WritePin (PROTECT_GPIO_Port,PROTECT_Pin,GPIO_PIN_SET);//继电器吸合，接通输入
      }
    }
    else if (sys_state == 2)//过流锁存态，按MODE复位重启(清零电压后重新启动)
    {
      if (Button_IsClicked(MODE_GPIO_Port, MODE_Pin))
      {
        sys_state = 1;
        pid_enable = 1;   // 复位后重启PID
        v_ref_ramp = 0.0f;   // 软启动：斜坡目标从0开始
        v_preref = 0.0f;
        v_ref = 0.0f;
        PID_Reset(&v_pid);   // 彻底清零PID内部输出/误差，软启动真正从0爬升
        pwm_ccr = 0;   // 软启动：复位后占空比同样从0爬升，防止带故障值瞬间重载
        HAL_GPIO_WritePin (PWM_SD_GPIO_Port,PWM_SD_Pin,GPIO_PIN_SET);//恢复IR2104使能
        HAL_GPIO_WritePin (PROTECT_GPIO_Port,PROTECT_Pin,GPIO_PIN_SET);//继电器重新吸合
      }
    }
    else if (sys_state == 1)//正常输出态
    {
      //MODE短按停机：回到待机冷态（拉低SD停驱动 + 断开继电器）
      if (Button_IsClicked(MODE_GPIO_Port, MODE_Pin))
      {
        sys_state = 0;
        pid_enable = 0;   // 停PID
        v_ref_ramp = 0.0f;   // 软启动：斜坡目标从0开始
        v_preref = 0.0f;
        v_ref = 0.0f;
        PID_Reset(&v_pid);   // 清零PID内部输出/误差。
        HAL_GPIO_WritePin (PWM_SD_GPIO_Port,PWM_SD_Pin,GPIO_PIN_RESET);//拉低IR2104的SD，停驱动
        HAL_GPIO_WritePin (PROTECT_GPIO_Port,PROTECT_Pin,GPIO_PIN_RESET);//断开继电器，切断输入
        pwm_ccr=0;
        __HAL_TIM_SET_COMPARE (&htim1,TIM_CHANNEL_1,pwm_ccr);//写死PWM输出为0
      }
      //按钮检测动作：ADD/REDUCE长按可连续调压，SET短按确认
      if (Button_IsPressed(ADD_GPIO_Port, ADD_Pin))
      {
        v_preref +=0.1f;
      }
      if (Button_IsPressed(REDUCE_GPIO_Port, REDUCE_Pin))
      {
        v_preref -=0.1f;
      }
      if (Button_IsClicked(SET_GPIO_Port, SET_Pin))
      {
        v_ref = v_preref;
      }
      //输出和预输出限幅
      if (v_preref > VOUT_MAX)
      {
        v_preref = VOUT_MAX;
      }
      if (v_preref < VOUT_MIN)
      {
        v_preref = VOUT_MIN;
      }
      if (v_ref > VOUT_MAX)
      {
        v_ref = VOUT_MAX;
      }
      if (v_ref < VOUT_MIN)
      {
        v_ref = VOUT_MIN;
      }
    }
    //VOFA+ JustFloat 实时波形：每20ms发一帧 vout/iout（50Hz，阻塞发送12字节约1ms，主循环里安全）
    static uint32_t t_vofa = 0;
    if (HAL_GetTick() - t_vofa >= 20)
    {
      t_vofa = HAL_GetTick();
      float data[2] = {vout, iout};
      uart_send_vofa(data, 2);
    }

    //OLED 显示：每100ms刷一次屏（10Hz，软件I2C较慢，与VOFA分开跑避免主循环卡顿）
    static uint32_t t_oled = 0;
    if (HAL_GetTick() - t_oled >= 100)
    {
      t_oled = HAL_GetTick();
      /* 系统状态显示：0=待机 1=正常输出 2=过流锁存 */
      switch (sys_state)
      {
        case 0: OLED_ShowString(0, 1, "STANDBY   "); break;
        case 1: OLED_ShowString(0, 1, "RUNNING   "); break;
        case 2: OLED_ShowString(0, 1, "FAULT!    "); break;
        default: break;
      }
      OLED_ShowString(0, 0, "BUCK 20kHz");
      /* 本帧累计平均存进最近10帧缓冲，显示的是10帧均值（约1秒平滑窗口） */
      static float vout_frame[10] = {0};
      static float iout_frame[10] = {0};
      static uint8_t vout_fidx = 0;
      static uint8_t iout_fidx = 0;
      static uint8_t vout_fcnt = 0;
      static uint8_t iout_fcnt = 0;
      if (vout_cnt > 0)
      {
        vout_frame[vout_fidx] = vout_sum / vout_cnt;
        vout_sum = 0.0f;
        vout_cnt = 0;
        vout_fidx = (uint8_t)((vout_fidx + 1) % 10);
        if (vout_fcnt < 10) vout_fcnt++;
        float vout_ftotal = 0.0f;
        for (uint8_t i = 0; i < vout_fcnt; i++) vout_ftotal += vout_frame[i];
        vout_avg = vout_ftotal / vout_fcnt;
      }
      if (iout_cnt > 0)
      {
        iout_frame[iout_fidx] = iout_sum / iout_cnt;
        iout_sum = 0.0f;
        iout_cnt = 0;
        iout_fidx = (uint8_t)((iout_fidx + 1) % 10);
        if (iout_fcnt < 10) iout_fcnt++;
        float iout_ftotal = 0.0f;
        for (uint8_t i = 0; i < iout_fcnt; i++) iout_ftotal += iout_frame[i];
        iout_avg = iout_ftotal / iout_fcnt;
      }
      OLED_ShowString(0, 2, "Vout:");
      OLED_ShowDecimal(6, 2, vout_avg, 2, 2);     // 实际输出电压（最近10帧平均）
      OLED_ShowString(0, 3, "Iout:");
      OLED_ShowDecimal(6, 3, iout_avg, 2, 2);     // 实际输出电流（最近10帧平均）
      OLED_ShowString(0, 4, "Ramp:");
      OLED_ShowDecimal(6, 4, v_ref_ramp, 2, 2); // 动态目标(PID实际追踪的斜坡值)
      OLED_ShowString(0, 5, "Pre :");
      OLED_ShowDecimal(6, 5, v_preref, 2, 2); // 预设电压(ADD/REDUCE调这个)
      
      OLED_Update();
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 2;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 3600-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, PROTECT_Pin|PWM_SD_Pin|OLED_SCL_Pin|OLED_SDA_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PROTECT_Pin PWM_SD_Pin */
  GPIO_InitStruct.Pin = PROTECT_Pin|PWM_SD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : OLED_SCL_Pin OLED_SDA_Pin */
  GPIO_InitStruct.Pin = OLED_SCL_Pin|OLED_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : REDUCE_Pin ADD_Pin MODE_Pin SET_Pin */
  GPIO_InitStruct.Pin = REDUCE_Pin|ADD_Pin|MODE_Pin|SET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
  void HAL_ADC_ConvCpltCallback (ADC_HandleTypeDef *hadc)//ADC回调函数，中断之后自动调用此函数。
  {
    if(hadc->Instance==ADC1)//如果触发中断的是ADC1
    {
      iout = measure_current(adc_buf[0]);//ADC码值 → 实际输出电流(A)
      vout = measure_voltage(adc_buf[1]);//ADC码值 → 实际输出电压(V)
      /* 两帧OLED之间累计取平均：ADC回调只累加，OLED刷新时求平均并清零 */
      vout_sum += vout;
      vout_cnt++;
      iout_sum += iout;
      iout_cnt++;
      if (iout>=IOUT_TRIP)
      {
        if (sys_state == 1)//只在正常输出态触发过流锁存，只触发一次
        {
          sys_state = 2;
          pid_enable = 0;
          HAL_GPIO_WritePin (PWM_SD_GPIO_Port,PWM_SD_Pin,GPIO_PIN_RESET);//拉低IR2104的SD，立即停驱动
          HAL_GPIO_WritePin (PROTECT_GPIO_Port,PROTECT_Pin,GPIO_PIN_RESET);//断开继电器，切断输入电源
          PID_Reset(&v_pid);   // 清零PID内部输出/误差
          pwm_ccr=0;
          v_preref=0.0f;
          v_ref=0.0f;
          v_ref_ramp=0.0f;
          __HAL_TIM_SET_COMPARE (&htim1,TIM_CHANNEL_1,pwm_ccr);//写死PWM输出为0
        }
      }
    }
  }

  /* TIM1更新中断调用：每20个PWM周期(1ms)执行一次PID电压环 */
  void Pid_Tick_1ms(void)
  {
    static uint16_t pid_count = 0;
    pid_count++;
    if (pid_count < 20) return;        /* 20kHz / 20 = 1kHz，即每1ms调一次 */
    pid_count = 0;

    if (pid_enable == 1 && sys_state == 1)   /* 正常输出态才跑PID */
    {
      /* 软启动斜坡：目标电压从0逐步爬到v_ref（升压），
       * 也支持向下跟踪（降压），约1秒变化12V(0.012V/ms)，
       * 让PID追一个平滑变化的目标，避免电压突变猛冲 */
      if (v_ref_ramp < v_ref)
      {
        v_ref_ramp += 0.005f;
        if (v_ref_ramp > v_ref) v_ref_ramp = v_ref;   /* 到顶锁定 */
      }
      else if (v_ref_ramp > v_ref)
      {
        v_ref_ramp -= 0.005f;
        if (v_ref_ramp < v_ref) v_ref_ramp = v_ref;   /* 到底锁定 */
      }

      pwm_ccr = (uint16_t)PID_Update(&v_pid, v_ref_ramp, vout);
      __HAL_TIM_SET_COMPARE (&htim1,TIM_CHANNEL_1,pwm_ccr);
    }
  }
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
