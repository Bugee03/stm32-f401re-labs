# Wiring

## PWM → RC → ADC

PA6 (D10, TIM3_CH1 PWM) → R → node Vout → PA0 (A0, ADC1_IN0)

Capacitor:
Vout → C → GND

PA6 ---- R ----+----> PA0 (A0)
               |
               C
               |
              GND

## Important
- If electrolytic capacitor is used:
  + to Vout, - to GND
- GND must be common (board GND)
