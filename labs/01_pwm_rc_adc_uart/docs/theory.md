# Theory (short)

## PWM average value
For PWM with amplitude VDD and duty D (0..1):
Vavg ≈ D · VDD  (after low-pass filtering)

## RC low-pass cutoff
fc = 1 / (2πRC)

Rule of thumb:
fc << fpwm  (e.g., fpwm/50 or smaller) to reduce ripple.
Larger RC → smoother output but slower response.
