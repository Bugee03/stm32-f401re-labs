# Lab: PWM (Bare-Metal) — TIM1_CH1 on PA8 (STM32F401RE)

## Goal
Configure and generate a stable PWM signal using **register-level** programming (no HAL/LL) on **STM32 Nucleo-F401RE**:
- Output pin: **PA8**
- Timer channel: **TIM1_CH1**
- Target example: ~**1 MHz**, ~**50% duty**

This lab is the minimal “base template” you can reuse for any PWM project: modulation, RC filter (PWM→DAC), motor/LED control, etc.

---

## Hardware / Connections
- Board: **Nucleo-F401RE**
- Probe (oscilloscope): **PA8** (PWM output) to scope CH1
- Scope GND clip: to **GND** on Nucleo

> Note: PA8 is available on Arduino header / MCU pin header depending on Nucleo revision. Use the pinout to locate PA8.

---

## Key idea (how PWM frequency is formed)
Timer counter runs from 0 to ARR, then wraps (update event).

**PWM frequency** (edge-aligned, upcount):
\[
f_{PWM} = \frac{TIM1CLK}{(PSC+1)\cdot(ARR+1)}
\]

**Duty cycle** (PWM mode 1):
\[
D \approx \frac{CCR1}{ARR+1}
\]
(Exact edge behavior depends on compare rules; this approximation is correct for typical PWM usage.)

Example (if TIM1CLK = 84 MHz):
- PSC = 0
- ARR = 83  →  84 MHz / 84 = **1 MHz**
- CCR1 = 42 → ~**50%**

---

## Why TIM1 needs “MOE”
TIM1 is an **advanced-control timer**. Even if channel is enabled (CC1E=1), the output will stay off until:
- **BDTR.MOE = 1** (Main Output Enable)

This is the #1 reason “PWM doesn’t appear” on TIM1/TIM8.

---

## Minimal PWM code (recommended base)
This version adds the must-have robustness:
- clears/sets CCMR1 bits safely
- enables preload (ARPE + OC1PE)
- forces update event (EGR=UG) so PSC/ARR/CCR are loaded before start

