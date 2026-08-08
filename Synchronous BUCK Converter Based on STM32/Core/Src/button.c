/**
  * @file    button.c
  * @brief   按键检测模块实现（基于STM32 HAL库）
  *          自动分配状态槽位：不同端口相同引脚号的按键也不会冲突
  *          参数调整只改文件顶部的宏，下面的逻辑不要动
  */
#include "button.h"

/* ──── 参数配置（换项目时按需调整）──── */
/* 换项目通常【不需要改】这里，除非：按键接法不同(高/低电平)、
 * 想调整手感(消抖/长按时间)、按键数超过16个 */
#define BTN_DEBOUNCE_MS        20              /* 消抖时间(ms) */
#define BTN_REPEAT_START_MS    300             /* 长按多久后开始自动重复(ms) */
#define BTN_REPEAT_INTERVAL_MS 50              /* 重复间隔(ms) */
#define BTN_ACTIVE_LEVEL       GPIO_PIN_RESET  /* 按键按下时的电平：低电平按下(按键接GND)；
                                                 若按键接VCC高电平按下，改为 GPIO_PIN_SET */
#define BTN_MAX_BUTTONS        16              /* 最多支持的按键数量 */

typedef struct
{
  GPIO_TypeDef *port;    /* 绑定的端口，NULL=槽位空闲 */
  uint16_t      pin;     /* 绑定的引脚 */
  uint32_t t_press;      /* 消抖确认按下的时刻 */
  uint32_t t_repeat;     /* 上次触发重复响应的时刻 */
  uint8_t  confirmed;    /* 1=已确认按下，0=松开 */
} KeyState;

static KeyState ks[BTN_MAX_BUTTONS];   /* 状态槽位，按(端口,引脚)自动分配 */

/* 查找按键对应的槽位；首次使用时自动占用一个空闲槽位，返回NULL表示槽位已满 */
static KeyState *FindSlot(GPIO_TypeDef *port, uint16_t pin)
{
  uint8_t i;
  KeyState *free_slot = 0;

  for (i = 0; i < BTN_MAX_BUTTONS; i++)
  {
    if (ks[i].port == port && ks[i].pin == pin) return &ks[i];
    if (free_slot == 0 && ks[i].port == 0) free_slot = &ks[i];
  }
  if (free_slot != 0)
  {
    free_slot->port = port;
    free_slot->pin  = pin;
  }
  return free_slot;
}

/* 底层扫描：返回 0=没动作 1=短按 2=长按重复 3=松开 */
static uint8_t Key_Scan(GPIO_TypeDef *port, uint16_t pin)
{
  KeyState *k = FindSlot(port, pin);
  if (k == 0) return 0;

  uint32_t now = HAL_GetTick();
  uint8_t  pressed = (HAL_GPIO_ReadPin(port, pin) == BTN_ACTIVE_LEVEL) ? 1u : 0u;

  if (pressed == 0u)                             /* 松开：清状态 */
  {
    k->confirmed = 0;
    k->t_press = 0;
    return 3;
  }

  if (k->confirmed == 0u)                        /* 未确认按下：消抖 */
  {
    if (k->t_press == 0u) { k->t_press = now; return 0; }
    if (now - k->t_press < BTN_DEBOUNCE_MS) return 0;
    k->confirmed = 1;                            /* 消抖通过 */
    k->t_press = now;
    k->t_repeat = now;
    return 1;                                    /* 短按 */
  }

  if (now - k->t_press >= BTN_REPEAT_START_MS)   /* 长按自动重复 */
  {
    if (now - k->t_repeat >= BTN_REPEAT_INTERVAL_MS)
    {
      k->t_repeat = now;
      return 2;
    }
  }
  return 0;
}

/* 只要按下就返回1：短按返回一次，长按期间周期返回（连续调节用） */
uint8_t Button_IsPressed(GPIO_TypeDef *port, uint16_t pin)
{
  uint8_t ev = Key_Scan(port, pin);
  return (ev == 1 || ev == 2) ? 1u : 0u;
}

/* 只有短按返回1：长按不重复（单击确认用） */
uint8_t Button_IsClicked(GPIO_TypeDef *port, uint16_t pin)
{
  return (Key_Scan(port, pin) == 1) ? 1u : 0u;
}
