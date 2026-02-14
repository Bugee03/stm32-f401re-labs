# Lesson 01 — GPIO Register-Level LED Blink (STM32F401RE)

## 🎯 Goal
Blink LD2 (PA5) using direct register manipulation (no HAL GPIO functions).

---

## 🧠 What I Learned

- How to enable peripheral clock using RCC
- How GPIO MODER configures pin mode
- Why BSRR is safer than ODR
- How atomic set/reset works
- Basic understanding of AHB1 bus

---

## ⚙️ Registers Used

### 1️⃣ RCC AHB1 Enable
```c
RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;


2️⃣ GPIO MODER
```c
GPIOA->MODER |= (1U << (5U * 2U));
```

3️⃣ GPIO BSRR
```c
GPIOA->BSRR = (1U << 5U);
GPIOA->BSRR = (1U << (5U + 16U));
```

🛠 Hardware
	•	Board: STM32 Nucleo F401RE
	•	LED: LD2 (PA5)

