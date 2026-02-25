# Lab 01 — PWM → RC Low-Pass → ADC (PA0) + UART (ST-LINK VCP)

## Goal
- Generate PWM on PA6 (TIM3_CH1)
- Convert PWM to quasi-DC with RC low-pass
- Measure Vout using ADC1 on PA0 (ADC1_IN0)
- Print ADC value over UART (USART2 via ST-LINK VCP)

## Hardware
- Board: NUCLEO-F401RE
- PWM pin: PA6 (D10) / TIM3_CH1
- ADC pin: PA0 (A0) / ADC1_IN0
- Common GND

## Wiring
See: `wiring.md`

## Notes
- PWM amplitude is 3.3 V → ADC input must stay within 0..3.3 V
- RC time constant τ = R·C affects ripple and response time
