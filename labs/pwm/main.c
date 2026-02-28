#include "stm32f401xe.h"

static void tim1_pwm_pa8(uint32_t arr, uint32_t ccr)
{
    // Clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    // PA8 -> AF1 (TIM1_CH1)
    GPIOA->MODER &= ~(3U << (8 * 2));
    GPIOA->MODER |=  (2U << (8 * 2));

    GPIOA->AFR[1] &= ~(0xFU << ((8 - 8) * 4));
    GPIOA->AFR[1] |=  (1U   << ((8 - 8) * 4));

    // High speed (полезно для 1 MHz)
    GPIOA->OSPEEDR &= ~(3U << (8 * 2));
    GPIOA->OSPEEDR |=  (3U << (8 * 2));

    // TIM1 base
    TIM1->PSC = 0;
    TIM1->ARR = arr;
    TIM1->CCR1 = ccr;

    // PWM mode 1 on CH1 + preload
    // CC1S=00 (output), OC1M=110 (PWM1), OC1PE=1
    TIM1->CCMR1 &= ~((3U << 0) | (7U << 4) | (1U << 3));  // clear CC1S, OC1M, OC1PE
    TIM1->CCMR1 |=  (6U << 4) | (1U << 3);

    // Enable CH1 output
    TIM1->CCER &= ~(1U << 1);   // CC1P=0 (active high)
    TIM1->CCER |=  (1U << 0);   // CC1E=1

    // Auto-reload preload enable
    TIM1->CR1 |= (1U << 7);     // ARPE

    // Advanced timer main output enable
    TIM1->BDTR |= (1U << 15);   // MOE

    // Force update to load PSC/ARR/CCR
    TIM1->EGR = 1U;             // UG

    // Start
    TIM1->CR1 |= 1U;            // CEN
}

int main(void)
{
    // Если TIM1CLK = 84 MHz, то:
    // f = 84MHz / (ARR+1). Для 1 MHz -> ARR=83
    tim1_pwm_pa8(83, 70);       // ~1 MHz, ~50%

    while (1) { }
}

