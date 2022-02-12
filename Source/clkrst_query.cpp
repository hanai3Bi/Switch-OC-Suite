#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>
#include <switch.h>

/* Recompile nx-hbloader with following added in config.json "kernel_capabilities"
    {
        "type": "map",
        "value": {
            "address": "0x60006000",
            "size": "0x1000",
            "is_ro": false,
            "is_io": true
        }
    }
*/

void waitForKey(PadState pad) {
    while (appletMainLoop())
    {
        padUpdate(&pad);

        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_A)
            break;

        if (kDown & HidNpadButton_Plus || kDown & HidNpadButton_B) {
            consoleExit(NULL);
            exit(0);
        }

        consoleUpdate(NULL);
    }
}

#define CLK_RST_IO_BASE 0x60006000
#define CLK_RST_IO_SIZE 0x1000

#define REG(OFFSET)                 (*reinterpret_cast<volatile u32 *>(OFFSET))
#define GET_BITS(VAL, HIGH, LOW)    ((VAL & ((1UL << (HIGH + 1UL)) - 1UL)) >> LOW)
#define GET_BIT(VAL, BIT)           GET_BITS(VAL, BIT, BIT)

static u64 clkrst_base = 0;
static u64 clkrst_size = 0;

// From jetson nano kernel
typedef enum {
    /* divider = 2 */
    CLK_PLLX  = 5,
    CLK_PLLM  = 2,
    CLK_PLLMB = 37,
    /* PLLX & PLLG are backup PLLs for CPU & GPU */
    /* divider = 1 */
    CLK_CCLK_G = 18, // A57 CPU cluster
    CLK_EMC   = 36,
} PTO_ID; // PLL Test Output Register ID

/* See if GM20B clock GPC PLL regs are accessible. */

#define PLLX_MISC0 0xE4
#define PLLM_MISC2 0x9C

double ptoGetMHz(PTO_ID pto_id, u32 divider = 1, u32 presel_reg = 0, u32 presel_mask = 0) {
    u32 pre_val, val, presel_val;

    if (presel_reg) {
        val = REG(clkrst_base + presel_reg);
        usleep(10);
        presel_val = val & presel_mask;
        val &= ~presel_mask;
        val |= presel_mask;
        REG(clkrst_base + presel_reg) = val;
        usleep(10);
    }

    constexpr u32 cycle_count = 16;
    pre_val = REG(clkrst_base + 0x60);
    val = BIT(23) | BIT(13) | (cycle_count - 1);
    val |= pto_id << 14;

    REG(clkrst_base + 0x60) = val;
    usleep(10);
    REG(clkrst_base + 0x60) = val | BIT(10);
    usleep(10);
    REG(clkrst_base + 0x60) = val;
    usleep(10);
    REG(clkrst_base + 0x60) = val | BIT(9);
    usleep(500);

    while(REG(clkrst_base + 0x64) & BIT(31))
        ;

    val = REG(clkrst_base + 0x64);
    val &= 0xFFFFFF;
    val *= divider;

    double rate_mhz = (u64)val * 32768. / cycle_count / 1000. / 1000.;
    usleep(10);
    REG(clkrst_base + 0x60) = pre_val;
    usleep(10);

    if (presel_reg) {
        val = REG(clkrst_base + presel_reg);
        usleep(10);
        val &= ~presel_mask;
        val |= presel_val;
        REG(clkrst_base + presel_reg) = val;
        usleep(10);
    }

    return rate_mhz;
}

int main() {
    consoleInit(NULL);
    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    // Get clkrst MMIO virtual address
    Result rc = svcQueryIoMapping(&clkrst_base, &clkrst_size, CLK_RST_IO_BASE, CLK_RST_IO_SIZE);
    if (R_FAILED(rc)) {
        printf("[ERROR] svcQueryIoMapping: 0x%X\n", rc);
        consoleUpdate(NULL);
        waitForKey(pad);
        consoleExit(NULL);
    }

    printf("clkrst_base: 0x%lX\nclkrst_size: 0x%lX\n", clkrst_base, clkrst_size);

    {
        #define CLKRST_PLLX_BASE 0xE0
        u32 pllx_base = REG(clkrst_base + CLKRST_PLLX_BASE);
        printf("\n"\
                "PLLX_BASE:    0x%X\n"\
                "PLLX_ENABLE:  %lu\n"\
                "PLLX_REF_DIS: %lu\n"\
                "PLLX_LOCK:    %lu\n"\
                "PLLX_DIVP:    %lu\n"\
                "PLLX_DIVN:    %lu\n"\
                "PLLX_DIVM:    %lu\n",
                pllx_base,
                GET_BIT(pllx_base, 30),
                GET_BIT(pllx_base, 29),
                GET_BIT(pllx_base, 27),
                GET_BITS(pllx_base, 24, 20),
                GET_BITS(pllx_base, 15, 8),
                GET_BITS(pllx_base, 7, 0));
    }

    {
        #define CLKRST_PLLX_MISC 0xE4
        u32 pllx_misc = REG(clkrst_base + CLKRST_PLLX_MISC);
        printf("\n"\
                "PLLX_MISC:          0x%X\n"\
                "PLLX_FO_G_DISABLE:  %lu\n"\
                "PLLX_PTS:           %lu\n"\
                "PLLX_LOCK_ENABLE:   %lu\n",
                pllx_misc,
                GET_BIT(pllx_misc, 28),
                GET_BITS(pllx_misc, 23, 22),
                GET_BIT(pllx_misc, 18));
    }

    {
        #define CLKRST_PLLMB_SS_CFG 0x780
        u32 pllmb_ss_cfg = REG(clkrst_base + CLKRST_PLLMB_SS_CFG);
        printf("\n"\
                "PLLMB_SS_CFG: 0x%X\n"\
                "PLLMB_EN_SDM: %lu\n"\
                "PLLMB_EN_SSC: %lu\n",
                pllmb_ss_cfg,
                GET_BIT(pllmb_ss_cfg, 31),
                GET_BIT(pllmb_ss_cfg, 30));
    }

    {
        #define CLKRST_PLLMB_SS_CTRL1 0x784
        u32 pllmb_ss_ctrl1 = REG(clkrst_base + CLKRST_PLLMB_SS_CTRL1);
        printf("\n"\
                "PLLMB_SS_CTRL1:    0x%X\n"\
                "PLLMB_SDM_SSC_MAX: %lu\n"\
                "PLLMB_SDM_SSC_MIN: %lu\n",
                pllmb_ss_ctrl1,
                GET_BITS(pllmb_ss_ctrl1, 31, 16),
                GET_BITS(pllmb_ss_ctrl1, 15, 0));
    }


    {
        #define CLKRST_PLLM_BASE 0x90
        u32 pllm_base = REG(clkrst_base + CLKRST_PLLM_BASE);
        printf("\n"\
                "PLLM_BASE: 0x%X\n"\
                "PLLM_DIVP: %lu\n"\
                "PLLM_DIVN: %lu\n"\
                "PLLM_DIVM: %lu\n",
                pllm_base,
                GET_BITS(pllm_base, 24, 20),
                GET_BITS(pllm_base, 15, 8),
                GET_BITS(pllm_base, 7, 0));
    }

    {
        #define CLKRST_PLLMB_BASE 0x5e8
        u32 pllmb_base = REG(clkrst_base + CLKRST_PLLMB_BASE);
        printf("\n"\
                "PLLMB_BASE: 0x%X\n"\
                "PLLMB_DIVP: %lu\n"\
                "PLLMB_DIVN: %lu\n"\
                "PLLMB_DIVM: %lu\n",
                pllmb_base,
                GET_BITS(pllmb_base, 24, 20),
                GET_BITS(pllmb_base, 15, 8),
                GET_BITS(pllmb_base, 7, 0));
    }

    printf("\n"\
           "EMC:    %6.1f MHz\n"\
           "CCLK_G: %6.1f MHz\n"\
           "PLLX:   %6.1f MHz\n"\
           "PLLM:   %6.1f MHz\n"\
           "PLLMB:  %6.1f MHz\n",
           ptoGetMHz(CLK_EMC),
           ptoGetMHz(CLK_CCLK_G),
           ptoGetMHz(CLK_PLLX,  2, PLLX_MISC0, BIT(22)),
           ptoGetMHz(CLK_PLLM,  2, PLLM_MISC2, BIT(8)),
           ptoGetMHz(CLK_PLLMB, 2, PLLM_MISC2, BIT(9))
           );

    waitForKey(pad);

    consoleExit(NULL);

    return 0;
}
