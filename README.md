# STM32 Digital Synchronous Buck Converter
# STM32 数字同步整流 BUCK 降压电源

> A complete digital-controlled synchronous Buck converter: PCB hardware design (EasyEDA), embedded firmware (STM32F103C8T6), closed-loop PID voltage control, soft-start, and over-current protection.
>
> 一个完整的数字控制同步 Buck 降压电源项目：涵盖 PCB 硬件设计（立创 EDA）、嵌入式固件（STM32F103C8T6）、PID 闭环电压控制、软启动与过流保护。

---

## 📐 Project Overview / 项目简介

This project implements a **synchronous Buck (step-down) DC-DC converter** with fully digital control. The high-side and low-side MOSFETs are driven by an **IR2104S half-bridge driver** (built-in 520 ns dead-time, single-input mode). A **STM32F103C8T6** runs an **incremental PID voltage loop at 1 kHz** to regulate the output voltage, with a software ramp soft-start, latching over-current protection, OLED status display, and UART telemetry for real-time waveform visualization.

本项目实现了一款**全数字控制的同步 Buck 降压变换器**。上下管 MOSFET 由 **IR2104S 半桥驱动器**驱动（内置 520ns 死区，单输入模式）。**STM32F103C8T6** 以 **1kHz** 运行**增量式 PID 电压环**调节输出电压，并具备斜坡软启动、过流锁存保护、OLED 状态显示和串口实时波形输出。

| Parameter 参数 | Value 数值 |
|---|---|
| Input voltage 输入电压 | 8 – 24 V |
| Output voltage 输出电压 | 0 – 22.8 V（软件可调, VIN×0.95） |
| Switching frequency 开关频率 | 20 kHz (TIM1, ARR=3600) |
| Control loop 控制环 | 1 kHz incremental PID |
| Output current limit 过流阈值 | 3 A（锁存保护） |
| MCU 主控 | STM32F103C8T6 @ 72 MHz |
| Gate driver 驱动 | IR2104S（内置 520ns 死区） |

---

## ✨ Key Features / 主要特性

- **Synchronous rectification 同步整流** — low-side MOSFET conducts instead of body diode, improving efficiency.
- **Soft-start 软启动** — target voltage ramps at 0.012 V/ms (~12 V/s), preventing inrush current.
- **Incremental PID 增量式 PID** — with anti-windup and D-term low-pass filtering; output rate-limited to avoid overshoot.
- **Latching over-current protection 过流锁存保护** — trips at 3 A, immediately shuts down the driver (SD low) and opens the input relay.
- **OLED real-time display OLED 实时显示** — Vout / Iout / preset / ramp target / system state.
- **Button UI 按键交互** — MODE (start/stop/reset), ADD/REDUCE (adjust voltage), SET (confirm).
- **UART telemetry 串口遥测** — VOFA+ JustFloat protocol @ 50 Hz for real-time waveform.

---
<img width="1706" height="1279" alt="焊接图2" src="https://github.com/user-attachments/assets/2c3c7fa5-7f40-4acb-9077-1cc568501fc3" />



## ⚡ Hardware Design / 硬件设计

Designed with **EasyEDA Pro (立创 EDA)**. All design files are included in the repository:

| File 文件 | Description 说明 |
|---|---|
| `同步BUCK.eprj2` | EasyEDA Pro project 立创 EDA 工程 |
| `Gerber_PCB2_*.zip` | Manufacturing files 制板 Gerber 文件 |
| `BOM_*.xlsx` | Bill of materials 物料清单 |
| `SCH_Schematic1_*.png/svg` | Schematic 原理图 |
| `2D/3D_PCB2_*.png` | PCB renders PCB 渲染图 |

**Key components / 关键器件**: STM32F103C8T6 · IR2104S half-bridge driver · N-MOSFET pair (high/low side) · 0.01 Ω current-sense resistor + 50× op-amp · 10:1 voltage divider · 0.96" SSD1306 OLED · input relay.

---

## 💾 Firmware Design / 固件设计

```
Core/
├── Inc/  Src/
│   ├── main.c        — system state machine, init, main loop
│   ├── pid.c         — incremental PID (pure C, portable)
│   ├── measure.c     — ADC raw → voltage/current conversion
│   ├── button.c      — debounce + long-press repeat (non-blocking)
│   ├── oled.c        — SSD1306 software-I2C, local diff refresh
│   ├── uart.c        — printf + VOFA+ JustFloat protocol
│   └── stm32f1xx_it.c — TIM1 update IRQ, DMA IRQ
```

**State machine / 状态机**: `STANDBY` (0) → `RUNNING` (1) → `FAULT-LATCH` (2, over-current) — MODE button starts/stops/resets, with latching shutdown on over-current.

**Key algorithms / 关键算法**:
- **Incremental PID**: `Δu = Kp·(eₖ−eₖ₋₁) + Ki·eₖ + Kd·(eₖ−2eₖ₋₁+eₖ₋₂)`, with per-step limit (Δmax=5) and anti-windup.
- **Soft-start ramp**: target voltage rises 0.012 V per ms toward the reference, so the PID never sees a step input.
- **Over-current latch**: 3 A trip → driver SD pulled low + relay opened + PWM forced to 0.

---

## 🕹️ How to Use / 使用方法

| Button 按键 | Action 功能 |
|---|---|
| `MODE` | Start output / stop output / reset after fault 启动/停机/复位 |
| `ADD` / `REDUCE` | Adjust preset voltage (0.1 V steps, long-press repeats) 调压（0.1V步进，长按连发） |
| `SET` | Confirm preset → becomes control target 确认目标电压 |

**OLED 显示**: line0 `BUCK 20kHz` · line1 state (STANDBY/RUNNING/FAULT) · line2 `Vout` · line3 `Iout` · line4 `Ramp` (soft-start target) · line5 `Pre` (preset).

**VOFA+**: select protocol **JustFloat**, baud 115200 → live curves of Vout / Iout at 50 Hz.

---

## 🛠️ Build & Flash / 编译烧录

**Toolchain / 工具链**: STM32CubeMX (generate .ioc) + Keil MDK-ARM (AC5, C99 enabled).

1. Open `MDK-ARM/Synchronous BUCK Converter Based on STM32.uvprojx` with Keil uVision.
2. Build (F7) and download (F8) via ST-Link.
3. ST HAL library is already included (`Drivers/`), or regenerate it from `.ioc` with CubeMX anytime.

---

## 🗂️ Repository Structure / 仓库结构

```
同步BUCK/
├── 同步BUCK.eprj2              # EasyEDA Pro project
├── Gerber_*.zip                # PCB manufacturing
├── BOM_*.xlsx                  # Bill of materials
├── SCH_*.png / *.svg           # Schematic
├── 2D/3D_PCB2_*.png            # PCB renders
├── 实物PCB*.jpg / 焊接图*.jpg   # Real hardware photos
└── Synchronous BUCK Converter Based on STM32/
    ├── Core/                   # Application source (this repo's focus)
    ├── Drivers/                # ST HAL / CMSIS library
    ├── MDK-ARM/                # Keil project
    └── *.ioc                   # CubeMX configuration
```

---

## 🔭 Future Work / 改进方向

- Hardware over-current protection via TIM1 break input (comparator) 硬件级过流保护（TIM1 刹车输入）
- ADC sampling synchronized with PWM (TIM1 TRGO) 与 PWM 同步的 ADC 采样，降低纹波混叠
- Double-buffer DMA for more efficient sampling DMA 双缓冲
- Current-loop cascade or digital voltage feed-forward 电流内环/前馈
- Efficiency measurement & thermal documentation 效率测试与散热数据

---

*Project for learning and demonstration. Designed, soldered and verified on real hardware (see photos).*
*本作品用于学习与展示，已在真实硬件上完成焊接与验证（见实物照片）。*
