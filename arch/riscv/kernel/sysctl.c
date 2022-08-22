#include "sysctl.h"

volatile sysctl_t *const sysctl = (volatile sysctl_t *)SYSCTL_BASE_ADDR;


int sysctl_pll_enable()
{
    /*
     *       ---+
     * PWRDN    |
     *          +-------------------------------------------------------------
     *          ^
     *          |
     *          |
     *          t1
     *                 +------------------+
     * RESET           |                  |
     *       ----------+                  +-----------------------------------
     *                 ^                  ^                              ^
     *                 |<----- t_rst ---->|<---------- t_lock ---------->|
     *                 |                  |                              |
     *                 t2                 t3                             t4
     */
    /* Do not bypass PLL */
    sysctl->pll1.pll_bypass1 = 0;
    /*
        * Power on the PLL, negtive from PWRDN
        * 0 is power off
        * 1 is power on
        */
    sysctl->pll1.pll_pwrd1 = 1;
    /*
        * Reset trigger of the PLL, connected RESET
        * 0 is free
        * 1 is reset
        */
    sysctl->pll1.pll_reset1 = 0;
    sysctl->pll1.pll_reset1 = 1;
    asm volatile("nop");
    asm volatile("nop");
    sysctl->pll1.pll_reset1 = 0;


    return 0;
}

static int sysctl_clock_device_en(uint32_t en)
{
    sysctl->pll1.pll_out_en1 = en;
    return 0;
}

/* the clock be the SYSCTL_CLOCK_PLL1 */
int sysctl_clock_enable()
{

    sysctl_clock_device_en(1);
    return 0;
}