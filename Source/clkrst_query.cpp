#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
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

void waitForKeyA(PadState pad) {
    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);
        if (kDown & HidNpadButton_A)
            break;
        consoleUpdate(NULL);
    }
}

int main() {
    consoleInit(NULL);
    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    // Get clkrst MMIO virtual address
    #define CLK_RST_IO_BASE 0x60006000
    #define CLK_RST_IO_SIZE 0x1000
    u64 virtaddr_base = 0;
    u64 virtaddr_size = 0;
    Result rc = svcQueryIoMapping(&virtaddr_base, &virtaddr_size, CLK_RST_IO_BASE, CLK_RST_IO_SIZE);
    if (R_FAILED(rc)) {
        printf("[ERROR] svcQueryIoMapping: 0x%X\n", rc);
        consoleUpdate(NULL);
        waitForKeyA(pad);
        consoleExit(NULL);
        return -1;
    }

    printf("virtaddr_base: 0x%lX\nvirtaddr_size: 0x%lX\n", virtaddr_base, virtaddr_size);

    #define READ_REG(OFFSET)            (*reinterpret_cast<volatile u32 *>(OFFSET))
    #define GET_BITS(VAL, HIGH, LOW)    ((VAL & ((1UL << (HIGH + 1UL)) - 1UL)) >> LOW)
    #define GET_BIT(VAL, BIT)           GET_BITS(VAL, BIT, BIT)

    {
        #define CLKRST_PLLX_BASE 0xE0
        u32 pllx_base = READ_REG(virtaddr_base + CLKRST_PLLX_BASE);
        printf("\n"\
                "PLLX_BASE:    0x%X\n"\
                "PLLX_ENABLE:  %lu\n"\
                "PLLX_REF_DIS: %lu\n"\
                "PLLX_LOCK:    %lu\n"\
                "PLLX_DIVP:    %lu\n"\
                "PLLX_DIVN:    %lu\n"\
                "PLLX_DIVM:    %lu",
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
        u32 pllx_misc = READ_REG(virtaddr_base + CLKRST_PLLX_MISC);
        printf("\n"\
                "PLLX_MISC:          0x%X\n"\
                "PLLX_FO_G_DISABLE:  %lu\n"\
                "PLLX_PTS:           %lu\n"\
                "PLLX_LOCK_ENABLE:   %lu",
                pllx_misc,
                GET_BIT(pllx_misc, 28),
                GET_BITS(pllx_misc, 23, 22),
                GET_BIT(pllx_misc, 18));
    }

    {
        #define CLKRST_PLLMB_SS_CFG 0x780
        u32 pllmb_ss_cfg = READ_REG(virtaddr_base + CLKRST_PLLMB_SS_CFG);
        printf("\n"\
                "PLLMB_SS_CFG: 0x%X\n"\
                "PLLMB_EN_SDM: %lu\n"\
                "PLLMB_EN_SSC: %lu",
                pllmb_ss_cfg,
                GET_BIT(pllmb_ss_cfg, 31),
                GET_BIT(pllmb_ss_cfg, 30));
    }

    {
        #define CLKRST_PLLMB_SS_CTRL1 0x784
        u32 pllmb_ss_ctrl1 = READ_REG(virtaddr_base + CLKRST_PLLMB_SS_CTRL1);
        printf("\n"\
                "PLLMB_SS_CTRL1:    0x%X\n"\
                "PLLMB_SDM_SSC_MAX: %lu\n"\
                "PLLMB_SDM_SSC_MIN: %lu",
                pllmb_ss_ctrl1,
                GET_BITS(pllmb_ss_ctrl1, 31, 16),
                GET_BITS(pllmb_ss_ctrl1, 15, 0));
    }

    consoleUpdate(NULL);
    waitForKeyA(pad);
    consoleExit(NULL);

    return 0;
}
