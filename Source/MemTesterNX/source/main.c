/*
 * memtester version 4
 *
 * Very simple but very effective user-space memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004-2020 Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 */

#define __version__ "4.5.1-full"

// Include the most common headers from the C standard library
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "types.h"
#include "sizes.h"
#include "tests.h"
#include <switch.h>

PadState pad;
unsigned short dividend;

void waitForAnyKey() {
    while (appletMainLoop())
    {
        padUpdate(&pad);

        u64 kDown = padGetButtonsDown(&pad);

        if (kDown)
            break;

        consoleUpdate(NULL);
    }
}

void ShowErr(const char* err, const char* details, Result rc) {
    AppletStorage errStor;
    LibAppletArgs args;
    AppletHolder currentApplet;

    appletCreateLibraryApplet(&currentApplet, AppletId_LibraryAppletError, LibAppletMode_AllForeground);
    libappletArgsCreate(&args, 1);
    libappletArgsPush(&args, &currentApplet);
    appletCreateStorage(&errStor, 0x1018);
    u8 argBuf[0x1018] = {0};
    argBuf[0] = 1;

    *(u64*)&argBuf[8] = (((rc & 0x1ffu) + 2000) | (((rc >> 9) & 0x1fff & 0x1fffll) << 32));
    strcpy((char*) &argBuf[24], err);
    strcpy((char*) &argBuf[0x818], details);
    appletStorageWrite(&errStor, 0, argBuf, 0x1018);
    appletHolderPushInData(&currentApplet, &errStor);

    appletHolderStart(&currentApplet);
    appletHolderJoin(&currentApplet);
    appletHolderClose(&currentApplet);
    appletHolderRequestExit(&currentApplet);
}

struct test tests[] = {
    { "Random Value", test_random_value },
    { "Compare XOR", test_xor_comparison },
    { "Compare SUB", test_sub_comparison },
    { "Compare MUL", test_mul_comparison },
    { "Compare DIV",test_div_comparison },
    { "Compare OR", test_or_comparison },
    { "Compare AND", test_and_comparison },
    { "Sequential Increment", test_seqinc_comparison },
    { "Solid Bits", test_solidbits_comparison },
    { "Block Sequential", test_blockseq_comparison },
    { "Checkerboard", test_checkerboard_comparison },
    { "Bit Spread", test_bitspread_comparison },
    { "Bit Flip", test_bitflip_comparison },
    { "Walking Ones", test_walkbits1_comparison },
    { "Walking Zeroes", test_walkbits0_comparison },
#ifdef TEST_NARROW_WRITES    
    { "8-bit Writes", test_8bit_wide_random },
    { "16-bit Writes", test_16bit_wide_random },
#endif
    { NULL, NULL }
};

int memtester_pagesize(void) {
    printf("using pagesize of 4096\n");
    return 4096;
}

/* Global vars - so tests have access to this information */
int use_phys = 0;
off_t physaddrbase = 0;

// Main program entrypoint
int main(int argc, char* argv[])
{
    if(appletGetAppletType() != AppletType_Application) {
        ShowErr("Running in applet mode\nMemTesterNX requires full memory mode.\nPlease launch hbmenu by holding R on an APP (e.g. a game) NOT an applet (e.g. Gallery)", "", 0);
        return 0;
    }

    appletReportUserIsActive();

    consoleInit(NULL);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    padInitializeDefault(&pad);

    ul loop, i;
    unsigned short div, maxdiv = 0;
    size_t pagesize, wantraw, wantbytes[4], wantbytes_orig, bufsize[4], halflen, count;
    ptrdiff_t pagesizemask;
    void volatile *buf[4], *aligned[4];
    ulv *bufa, *bufb;
    int memshift;
    ul testmask = 0;
    ull totalmem = 0;

    printf("MemTesterNX version " __version__ " (%d-bit)\n", UL_LEN);
    printf("Based on memtester. Copyright (C) 2001-2020 Charles Cazabon.\n");
    printf("Licensed under the GNU General Public License version 2 (only).\n\n");
    printf("4.5.1-full supports full RAM test (up to 8GB for devkit).\n");
    printf("It will looping forever until an error shows up or you manually exit to HOME screen.\n\n");
    printf("Press A: long test\nPress B: fast test\nPress any other key: exit\n\n");

    while (appletMainLoop())
    {
        padUpdate(&pad);

        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_A)
        {
            dividend = 1;
            break; 
        }
        else if (kDown & HidNpadButton_B)
        {
            dividend = 4;
            break;
        }
        else if (kDown)
        {
            consoleExit(NULL);
            exit(0);
        }

        consoleUpdate(NULL);
    }

    pagesize = memtester_pagesize();
    pagesizemask = (ptrdiff_t) ~(pagesize - 1);
    printf("pagesizemask is 0x%tx\n", pagesizemask);
    consoleUpdate(NULL);
    
    memshift = 20; /* megabytes */
    wantraw = 2048;
    
    wantbytes_orig = ((size_t) wantraw << memshift);

    // Allocate as much RAM as possible.
    for(div = 0; div <= 3; div++)
    {

        buf[div] = NULL;
        wantbytes[div] = wantbytes_orig;

        while (!buf[div] && wantbytes[div]) {
            buf[div] = (void volatile *) malloc(wantbytes[div]);
            if (!buf[div])
                wantbytes[div] -= pagesize;
        }

        bufsize[div] = wantbytes[div];
        printf("Alloc %d: got  %lluMB (%llu bytes)\n", div+1, (ull) wantbytes[div] >> 20, (ull) wantbytes[div]);
        consoleUpdate(NULL);

        /* Do alighnment here as well, as some cases won't trigger above if you
           define out the use of mlock() (cough HP/UX 10 cough). */
        if ((size_t) buf[div] % pagesize) {
            /* printf("aligning to page -- was 0x%tx\n", buf); */
            aligned[div] = (void volatile *) ((size_t) buf[div] & pagesizemask) + pagesize;
            /* printf("  now 0x%tx -- lost %d bytes\n", aligned,
             *      (size_t) aligned - (size_t) buf);
             */
            bufsize[div] -= ((size_t) aligned[div] - (size_t) buf[div]);
        } else {
            aligned[div] = buf[div];
        }

        totalmem += wantbytes[div];

        if((wantbytes[div] >> 20) < 2047)
        {
            maxdiv = div;
            break;
        }
    }

    printf("\nTotal RAM allocated: %lldMB\n\n", totalmem >> 20);
    consoleUpdate(NULL);

    for(loop=1;;loop++)
    {
        printf("Loop %lu:\n", loop);

        for(div=0; div<=maxdiv; div++)
        {
            printf("Alloc %d:\n", div+1);
            printf("  %-20s: ", "Stuck Address");
            consoleUpdate(NULL);

            halflen = bufsize[div] / 2;            
            count = halflen / sizeof(ul);
            bufa = (ulv *) aligned[div];
            bufb = (ulv *) ((size_t) aligned[div] + halflen);

            if (!test_stuck_address(aligned[div], bufsize[div] / sizeof(ul))) {
                 printf("ok\n");
            }
            else
            {
                printf("Alloc %d: Error!\nPress any key to exit.\n", div+1);
                waitForAnyKey();
                consoleExit(NULL);
                return 0;
            }
            consoleUpdate(NULL);
            for (i=0;;i++) {
                if (!tests[i].name) break;
                /* If using a custom testmask, only run this test if the
                   bit corresponding to this test was set by the user.
                 */
                if (testmask && (!((1 << i) & testmask))) {
                    continue;
                }
                printf("  %-20s: ", tests[i].name);
                if (!tests[i].fp(bufa, bufb, count)) {
                    printf("ok\n");
                }
                else
                {
                    printf("Alloc %d: Error!\nPress any key to exit.\n", div+1);
                    waitForAnyKey();
                    consoleExit(NULL);
                    return 0;
                }
                consoleUpdate(NULL);
                /* clear buffer */
                memset((void *) buf[div], 255, wantbytes[div]);
            }
            printf("\n");
            consoleUpdate(NULL);
        }
    }

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}