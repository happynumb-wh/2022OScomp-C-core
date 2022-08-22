
#include <type.h>

/**
 * @brief       System controller object
 *
 *              The System controller is a peripheral device mapped in the
 *              internal memory map, discoverable in the Configuration String.
 *              It is responsible for low-level configuration of all system
 *              related peripheral device. It contain PLL controller, clock
 *              controller, reset controller, DMA handshake controller, SPI
 *              controller, timer controller, WDT controller and sleep
 *              controller.
 */

#define SYSCTL_BASE_ADDR    (0x50440000U)// + sizeof(uint32_t) * 14
/**
 * @brief       Git short commit id
 *
 *              No. 0 Register (0x00)
 */
typedef struct _sysctl_git_id
{
    uint32_t git_id : 32;
} __attribute__((packed, aligned(4))) sysctl_git_id_t;

/**
 * @brief       System clock base frequency
 *
 *              No. 1 Register (0x04)
 */
typedef struct _sysctl_clk_freq
{
    uint32_t clk_freq : 32;
} __attribute__((packed, aligned(4))) sysctl_clk_freq_t;

/**
 * @brief       PLL0 controller
 *
 *              No. 2 Register (0x08)
 */
typedef struct _sysctl_pll0
{
    uint32_t clkr0 : 4;
    uint32_t clkf0 : 6;
    uint32_t clkod0 : 4;
    uint32_t bwadj0 : 6;
    uint32_t pll_reset0 : 1;
    uint32_t pll_pwrd0 : 1;
    uint32_t pll_intfb0 : 1;
    uint32_t pll_bypass0 : 1;
    uint32_t pll_test0 : 1;
    uint32_t pll_out_en0 : 1;
    uint32_t pll_test_en : 1;
    uint32_t reserved : 5;
} __attribute__((packed, aligned(4))) sysctl_pll0_t;

/**
 * @brief       PLL1 controller
 *
 *              No. 3 Register (0x0c)
 */
typedef struct _sysctl_pll1
{
    uint32_t clkr1 : 4;
    uint32_t clkf1 : 6;
    uint32_t clkod1 : 4;
    uint32_t bwadj1 : 6;
    uint32_t pll_reset1 : 1;
    uint32_t pll_pwrd1 : 1;
    uint32_t pll_intfb1 : 1;
    uint32_t pll_bypass1 : 1;
    uint32_t pll_test1 : 1;
    uint32_t pll_out_en1 : 1;
    uint32_t reserved : 6;
} __attribute__((packed, aligned(4))) sysctl_pll1_t;


typedef struct _sysctl
{
    /* No. 0 (0x00): Git short commit id */
    sysctl_git_id_t git_id;
    /* No. 1 (0x04): System clock base frequency */
    sysctl_clk_freq_t clk_freq;
    /* No. 2 (0x08): PLL0 controller */
    sysctl_pll0_t pll0;
    /* No. 3 (0x0c): PLL1 controller */
    sysctl_pll1_t pll1;

} __attribute__((packed, aligned(4))) sysctl_t;


typedef enum _sysctl_pll_t
{
    SYSCTL_PLL0,
    SYSCTL_PLL1,
    SYSCTL_PLL2,
    SYSCTL_PLL_MAX
} sysctl_pll_t;

typedef enum _sysctl_clock_t
{
    SYSCTL_CLOCK_PLL0,
    SYSCTL_CLOCK_PLL1,
    SYSCTL_CLOCK_PLL2,
    SYSCTL_CLOCK_CPU,
    SYSCTL_CLOCK_SRAM0,
    SYSCTL_CLOCK_SRAM1,
    SYSCTL_CLOCK_APB0,
    SYSCTL_CLOCK_APB1,
    SYSCTL_CLOCK_APB2,
    SYSCTL_CLOCK_ROM,
    SYSCTL_CLOCK_DMA,
    SYSCTL_CLOCK_AI,
    SYSCTL_CLOCK_DVP,
    SYSCTL_CLOCK_FFT,
    SYSCTL_CLOCK_GPIO,
    SYSCTL_CLOCK_SPI0,
    SYSCTL_CLOCK_SPI1,
    SYSCTL_CLOCK_SPI2,
    SYSCTL_CLOCK_SPI3,
    SYSCTL_CLOCK_I2S0,
    SYSCTL_CLOCK_I2S1,
    SYSCTL_CLOCK_I2S2,
    SYSCTL_CLOCK_I2C0,
    SYSCTL_CLOCK_I2C1,
    SYSCTL_CLOCK_I2C2,
    SYSCTL_CLOCK_UART1,
    SYSCTL_CLOCK_UART2,
    SYSCTL_CLOCK_UART3,
    SYSCTL_CLOCK_AES,
    SYSCTL_CLOCK_FPIOA,
    SYSCTL_CLOCK_TIMER0,
    SYSCTL_CLOCK_TIMER1,
    SYSCTL_CLOCK_TIMER2,
    SYSCTL_CLOCK_WDT0,
    SYSCTL_CLOCK_WDT1,
    SYSCTL_CLOCK_SHA,
    SYSCTL_CLOCK_OTP,
    SYSCTL_CLOCK_RTC,
    SYSCTL_CLOCK_ACLK = 40,
    SYSCTL_CLOCK_HCLK,
    SYSCTL_CLOCK_IN0,
    SYSCTL_CLOCK_MAX
} sysctl_clock_t;


/**
 * @brief       Enable the PLL and power on with reset
 *
 * @param[in]   pll     The pll id
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
int sysctl_pll_enable();

/**
 * @brief       Enable clock for peripheral
 *
 * @param[in]   clock       The clock to be enable
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int sysctl_clock_enable();

