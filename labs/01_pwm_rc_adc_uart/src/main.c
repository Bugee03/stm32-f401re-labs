#include "stm32f4xx.h"

#define BAUDRATE 115200U
#define PWM_HZ   20000U

/* RC: R=1k, C=10uF -> tau=0.01s, fc~15.9Hz */

static void delay_cycles(volatile uint32_t n){ while(n--) __NOP(); }

/* ---------- clocks helpers ---------- */
static uint32_t get_hclk_hz(void){ return SystemCoreClock; }

static uint32_t get_pclk1_hz(void)
{
    uint32_t hclk = get_hclk_hz();
    uint32_t ppre1 = (RCC->CFGR >> RCC_CFGR_PPRE1_Pos) & 0x7U;
    static const uint8_t tbl[8] = {1,1,1,1,2,4,8,16};
    return hclk / tbl[ppre1];
}

static uint32_t get_tim3_clk_hz(void)
{
    uint32_t pclk1 = get_pclk1_hz();
    uint32_t ppre1 = (RCC->CFGR >> RCC_CFGR_PPRE1_Pos) & 0x7U;
    return (ppre1 >= 4) ? (2U * pclk1) : pclk1;
}

/* ---------- UART2 (PA2 TX, PA3 RX) ---------- */
static void uart2_init_115200(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    (void)RCC->AHB1ENR; (void)RCC->APB1ENR;

    /* PA2/PA3 -> AF7 */
    GPIOA->MODER &= ~((3U<<(2*2)) | (3U<<(3*2)));
    GPIOA->MODER |=  ((2U<<(2*2)) | (2U<<(3*2)));
    GPIOA->AFR[0] &= ~((0xFU<<(2*4)) | (0xFU<<(3*4)));
    GPIOA->AFR[0] |=  ((7U<<(2*4)) | (7U<<(3*4)));

    uint32_t pclk1 = get_pclk1_hz();
    USART2->CR1 = 0;
    USART2->BRR = (pclk1 + (BAUDRATE/2U)) / BAUDRATE; /* oversampling 16 */
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

static void uart2_putc(char c){ while(!(USART2->SR & USART_SR_TXE)){} USART2->DR = (uint8_t)c; }
static char uart2_getc(void){ while(!(USART2->SR & USART_SR_RXNE)){} return (char)USART2->DR; }

static void uart2_print(const char*s){ while(*s) uart2_putc(*s++); }

static void uart2_print_u32(uint32_t v)
{
    char b[11]; int i=0;
    if(!v){ uart2_putc('0'); return; }
    while(v && i<10){ b[i++] = (char)('0' + (v%10U)); v/=10U; }
    while(i--) uart2_putc(b[i]);
}

static void uart2_print_i32(int32_t v)
{
    if (v < 0) { uart2_putc('-'); uart2_print_u32((uint32_t)(-v)); }
    else uart2_print_u32((uint32_t)v);
}

/* read integer from UART (simple, line-based) */
static int32_t uart2_read_int(void)
{
    char buf[16];
    int idx = 0;
    while (1) {
        char c = uart2_getc();
        if (c == '\r' || c == '\n') {
            uart2_print("\r\n");
            break;
        }
        if (idx < (int)sizeof(buf)-1) {
            buf[idx++] = c;
            uart2_putc(c); /* echo */
        }
    }
    buf[idx] = 0;

    int sign = 1;
    int i = 0;
    if (buf[0] == '-') { sign = -1; i = 1; }

    int32_t val = 0;
    for (; buf[i]; i++) {
        if (buf[i] < '0' || buf[i] > '9') break;
        val = val*10 + (buf[i]-'0');
    }
    return sign * val;
}

/* ---------- PWM TIM3_CH1 on PA6 ---------- */
static void pwm_tim3_ch1_pa6_init(uint32_t pwm_hz)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    (void)RCC->AHB1ENR; (void)RCC->APB1ENR;

    /* PA6 AF2 (TIM3_CH1) */
    GPIOA->MODER &= ~(3U << (6U*2U));
    GPIOA->MODER |=  (2U << (6U*2U));
    GPIOA->AFR[0] &= ~(0xFU << (6U*4U));
    GPIOA->AFR[0] |=  (2U   << (6U*4U));
    GPIOA->OTYPER &= ~(1U << 6U);
    GPIOA->PUPDR  &= ~(3U << (6U*2U));
    GPIOA->OSPEEDR |= (3U << (6U*2U));

    uint32_t timclk = get_tim3_clk_hz();

    uint32_t psc = 0;
    uint32_t arr = (timclk / pwm_hz) - 1U;
    while (arr > 65535U) {
        psc++;
        arr = (timclk / (pwm_hz * (psc + 1U))) - 1U;
    }

    TIM3->CR1 = 0;
    TIM3->PSC = (uint16_t)psc;
    TIM3->ARR = (uint16_t)arr;
    TIM3->CCR1 = (uint16_t)((arr + 1U) / 2U);

    TIM3->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM3->CCMR1 |=  (6U << TIM_CCMR1_OC1M_Pos); /* PWM1 */
    TIM3->CCMR1 |=  TIM_CCMR1_OC1PE;

    TIM3->CCER |= TIM_CCER_CC1E;
    TIM3->CR1  |= TIM_CR1_ARPE;
    TIM3->EGR   = TIM_EGR_UG;
    TIM3->CR1  |= TIM_CR1_CEN;
}

static void pwm_set_duty_percent(uint32_t duty_percent)
{
    if (duty_percent > 100U) duty_percent = 100U;
    uint32_t arr = TIM3->ARR;
    uint32_t ccr = ((arr + 1U) * duty_percent) / 100U;
    if (ccr > arr) ccr = arr;
    TIM3->CCR1 = (uint16_t)ccr;
}

/* ---------- ADC1 on PA0 (ADC1_IN0) ---------- */
static void adc1_pa0_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    (void)RCC->AHB1ENR; (void)RCC->APB2ENR;

    GPIOA->MODER |= (3U << (0U*2U)); /* analog */
    GPIOA->PUPDR  &= ~(3U << (0U*2U));

    ADC->CCR &= ~ADC_CCR_ADCPRE;
    ADC->CCR |=  ADC_CCR_ADCPRE_0;   /* PCLK2/4 */

    ADC1->CR1 = 0;
    ADC1->CR2 = ADC_CR2_ADON;

    /* long sample time for RC source */
    ADC1->SMPR2 &= ~ADC_SMPR2_SMP0;
    ADC1->SMPR2 |=  (7U << ADC_SMPR2_SMP0_Pos);

    ADC1->SQR1 &= ~ADC_SQR1_L; /* 1 conversion */
    ADC1->SQR3 = 0;            /* ch0 */
}

static uint16_t adc1_read_ch0(void)
{
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while(!(ADC1->SR & ADC_SR_EOC)){}
    return (uint16_t)ADC1->DR;
}

/* --------- simple stats to estimate ripple --------- */
static void adc_stats(uint16_t *minv, uint16_t *maxv, uint32_t *avg)
{
    uint16_t mn = 4095, mx = 0;
    uint32_t sum = 0;
    for (int i = 0; i < 512; i++) {
        uint16_t v = adc1_read_ch0();
        if (v < mn) mn = v;
        if (v > mx) mx = v;
        sum += v;
        delay_cycles(2000); /* spread samples */
    }
    *minv = mn;
    *maxv = mx;
    *avg  = sum / 512U;
}

static uint32_t adc_to_mV(uint32_t adc)
{
    /* assume Vref = 3300mV */
    return (adc * 3300U) / 4095U;
}

int main(void)
{
    uart2_init_115200();
    pwm_tim3_ch1_pa6_init(PWM_HZ);
    adc1_pa0_init();

    uart2_print("\r\nPWM->RC(R=1k,C=10uF)->ADC demo\r\n");
    uart2_print("Connections:\r\n");
    uart2_print("PA6 -> R -> X ; X -> C -> GND ; X -> PA0(A0)\r\n");
    uart2_print("Enter duty 0..100 and press Enter.\r\n\r\n");

    /* start at 50% */
    pwm_set_duty_percent(50);

    while (1)
    {
        uart2_print("duty% = ");
        int32_t duty = uart2_read_int();
        if (duty < 0) duty = 0;
        if (duty > 100) duty = 100;

        pwm_set_duty_percent((uint32_t)duty);

        /* RC is slow (tau=10ms). Wait to settle. */
        delay_cycles(12000000); /* ~0.1..0.3s depending on clock; enough to see settle */

        uint16_t mn, mx;
        uint32_t av;
        adc_stats(&mn, &mx, &av);

        uart2_print("ADC avg="); uart2_print_u32(av);
        uart2_print(" ("); uart2_print_u32(adc_to_mV(av)); uart2_print("mV)");
        uart2_print("  min="); uart2_print_u32(mn);
        uart2_print("  max="); uart2_print_u32(mx);
        uart2_print("  ripple="); uart2_print_u32((uint32_t)(mx - mn));
        uart2_print("\r\n\r\n");
    }
}

