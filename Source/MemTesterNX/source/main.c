/*
 * MemTesterNX
 * based on memtester version 4
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

#define __version__ "4.5.1-multithread"

// Include the most common headers from the C standard library
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "types.h"
#include "sizes.h"
#include "tests.h"
#include <switch.h>

PadState pad;
unsigned short dividend = 1;

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

double gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)((int64_t)tv.tv_sec * 1000000 + tv.tv_usec) / 1000000.;
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
    { "Bit Flip (Slow)", test_bitflip_comparison },
    { "Walking Ones", test_walkbits1_comparison },
    { "Walking Zeroes", test_walkbits0_comparison },
#ifdef TEST_NARROW_WRITES    
    { "8-bit Writes", test_8bit_wide_random },
    { "16-bit Writes", test_16bit_wide_random },
#endif
    { NULL, NULL }
};

struct test stress_tests[] = {
    { "Stress memcpy x128", test_stress_memcpy },
    { "Stress memset x128", test_stress_memset },
    { "Stress memcmp x 32", test_stress_memcmp },
    { NULL, NULL }
};

int memtester_pagesize(void) {
    printf("using pagesize of 4096\n");
    return 4096;
}

/* Global vars - so tests have access to this information */
int use_phys = 0;
off_t physaddrbase = 0;
int testJobId[4] = {0};
int testWorkerReport[4] = {0};
size_t bufsize[4], wantbytes[4];
void volatile *aligned[4];
Thread threads[5];
struct test* test_select = tests;

void testWorker(int div)
{
    // Get ready
    while (testJobId[div] != -2)
        svcSleepThread(10000000ULL);

    while (true)
    {
        int currentJobId = -1;

        // Stuck address
        while (testJobId[div] != currentJobId)
            svcSleepThread(10000000ULL);

        if (!test_stuck_address(aligned[div], bufsize[div]/sizeof(ul)))
        {
            testWorkerReport[div] = 1;
            currentJobId++;
        }
        else
        {
            testWorkerReport[div] = -1;
            return;
        }

        // tests[], currentJobId = 0 here
        while (test_select[currentJobId].name)
        {
            while (testJobId[div] != currentJobId)
                svcSleepThread(10000000ULL);

            int test_rc = test_select[currentJobId].fp(aligned[div], ((size_t)aligned[div] + bufsize[div]/2), bufsize[div]/sizeof(ul)/2);
            if (!test_rc)
            {
                testWorkerReport[div] = 1;
                currentJobId++;
            }
            else
            {
                testWorkerReport[div] = -1;
                return;
            }
        }
    }
}

void testWorker0(void*) { testWorker(0); }
void testWorker1(void*) { testWorker(1); }
void testWorker2(void*) { testWorker(2); }
void testWorker3(void*) { testWorker(3); }

void* workers[] = {
    testWorker0,
    testWorker1,
    testWorker2,
    testWorker3,
};

void LblUpdate()
{
    smInitialize();
    lblInitialize();
    LblBacklightSwitchStatus lblstatus = LblBacklightSwitchStatus_Disabled;
    lblGetBacklightSwitchStatus(&lblstatus);
    if (lblstatus) {
        lblSwitchBacklightOff(0);
    } else {
        lblSwitchBacklightOn(0);
    }
    lblExit();
    smExit();
}

void keyListener()
{
    while (true)
    {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);
        if (kDown & HidNpadButton_Minus)
        {
            LblUpdate();
        }
        svcSleepThread(10000000ULL);
    }
}

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

    ull loop, i;
    unsigned short div, testThreads = 3;
    size_t pagesize, wantraw, wantbytes_orig;
    ptrdiff_t pagesizemask;
    void volatile *buf[4];
    int memshift;
    ull totalmem = 0;
    int numOfMallocs = 0;
    bool isDevKit8GB = false;

    printf("MemTesterNX version " __version__ " (%d-bit)\n"\
           "Based on memtester. Copyright (C) 2001-2020 Charles Cazabon, 2021 KazushiMe.\n"\
           "Licensed under the GNU General Public License version 2 (only).\n\n"\
           "Support full RAM test (up to 8GB) with 3-4 threads.\n"\
           "It will be looping forever until error occurs or user exits to HOME screen.\n\n"\
           "Press A: long test\n"\
           "Press X: fast test\n"\
           "Press Y: stress DRAM (memcpy, memset and memcmp)\n"\
           "Press any other key: exit\n\n",
           UL_LEN);

    while (appletMainLoop())
    {
        padUpdate(&pad);

        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_A)
        {
            break; 
        }
        else if (kDown & HidNpadButton_X)
        {
            dividend = 4;
            break;
        }
        else if (kDown & HidNpadButton_Y)
        {
            dividend = 16;
            test_select = stress_tests;
            break;
        }
        else if (kDown)
        {
            consoleExit(NULL);
            exit(0);
        }

        consoleUpdate(NULL);
    }

    // Disable auto sleep and request CPU Boost mode
    appletSetAutoSleepDisabled(true);
    appletSetCpuBoostMode(ApmCpuBoostMode_FastLoad);

    pagesize = memtester_pagesize();
    pagesizemask = (ptrdiff_t) ~(pagesize - 1);
    printf("pagesizemask is 0x%tx\n", pagesizemask);
    consoleUpdate(NULL);
    
    memshift = 20; /* megabytes */
    wantraw = 2048; // HOS limit
    
    wantbytes_orig = ((size_t) wantraw << memshift);

    // Allocate as much RAM as possible.
    for (div = 0; div <= 3; div++)
    {
        buf[div] = NULL;
        wantbytes[div] = wantbytes_orig;

        while (!buf[div] && wantbytes[div]) {
            buf[div] = (void volatile *) malloc(wantbytes[div]);
            if (!buf[div])
                wantbytes[div] -= pagesize;
        }

        totalmem += wantbytes[div];

        if ((wantbytes[div] >> memshift) < 2047)
        {
            numOfMallocs = div + 1;
            break;
        }
    }

    for (div = 0; div < numOfMallocs; div++)
    {
        free((void *)buf[div]);
    }

    // Unknown: Not sure if devkit could use full 8GB in application space
    if ((totalmem >> memshift) > 3 * 2047)
    {
        isDevKit8GB = true;
        testThreads = 4;
    }

    // Create workers
    for (div = 0; div < testThreads; div++)
    {
        Result rc;
        rc = threadCreate(&threads[div], workers[div], NULL, NULL, 0x1000, 0x2C, div == 3 ? -2 : div);
        if (R_FAILED(rc))
        {
            printf("Fatal: threadCreate[%d] failed: 0x%X\n", div, rc);
            consoleUpdate(NULL);
        }
        else
        {
            totalmem -= 0x1000;
        }
    }

    // keyListener
    {
        const int kl = testThreads;
        Result rc = threadCreate(&threads[kl], keyListener, NULL, NULL, 0x1000, 0x20, -2);
        if (R_FAILED(rc))
        {
            printf("Fatal: threadCreate[%d] failed: 0x%X\n", div, rc);
            consoleUpdate(NULL);
        }
        threadStart(&threads[kl]);
    }

    printf("\nTotal RAM available: %lldMB\n\n", totalmem >> memshift);
    consoleUpdate(NULL);

    // Redistribute RAM allocation equally to testThreads workers
    for (div = 0; div < testThreads; div++)
    {
        buf[div] = NULL;
        if (isDevKit8GB)
        {
            if (div != 3)
                wantbytes[div] = totalmem / 3;
            else
                wantbytes[div] = 2047;
        }
        else
        {
            wantbytes[div] = totalmem / testThreads;
        }

        while (!buf[div] && wantbytes[div]) {
            buf[div] = (void volatile *) malloc(wantbytes[div]);
            if (!buf[div])
                wantbytes[div] -= pagesize;
        }

        bufsize[div] = wantbytes[div];

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

        printf("Alloc %d: got  %lluMB (%llu bytes)\n", div+1, (ull) wantbytes[div] >> memshift, (ull) wantbytes[div]);
        consoleUpdate(NULL);
    }

    // Start workers
    for (div = 0; div < testThreads; div++)
    {
        Result rc;
        rc = threadStart(&threads[div]);
        if (R_FAILED(rc))
        {
            printf("Fatal: threadStart[%d] failed: 0x%X\n", div, rc);
            consoleUpdate(NULL);
        }
    }

    for(loop=1;;loop++)
    {
        // Set testJobId to -2(Ready)
        for (int j = 0; j < testThreads; j++)
            testJobId[j] = -2;

        printf("Loop %llu:\n", loop);

        // Stuck address (-1)
        printf("  %-20s: ", "Stuck Address");
        consoleUpdate(NULL);
        for (int j = 0; j < testThreads; j++)
            testJobId[j] = -1;

        for (int j = 0; j < testThreads; )
        {
            switch (testWorkerReport[j])
            {
                case 0:
                    svcSleepThread(10000000ULL);
                    break;
                case -1:
                    printf("Alloc %d/%d: Error detected!\nPress any key to exit.\n", j+1, testThreads+1);
                    consoleUpdate(NULL);
                    waitForAnyKey();
                    consoleExit(NULL);
                    return 0;
                case 1:
                    testWorkerReport[j] = 0;
                    j++;
                    continue;
            }
        }
        printf("ok\n");
        consoleUpdate(NULL);

        // tests[]
        for (i = 0 ;; i++) {
            if (!test_select[i].name)
                break;

            printf("  %-20s: ...", test_select[i].name);
            consoleUpdate(NULL);
            for (int j = 0; j < testThreads; j++)
                testJobId[j] = i;

            double start_sec = gettime();
            for (int j = 0; j < testThreads; )
            {
                switch (testWorkerReport[j])
                {
                    case 0:
                        svcSleepThread(10000000ULL);
                        continue;
                    case -1:
                        printf("Alloc %d/%d: Error detected!\nPress any key to exit.\n", j+1, testThreads+1);
                        consoleUpdate(NULL);
                        waitForAnyKey();
                        consoleExit(NULL);
                        return 0;
                    case 1:
                        testWorkerReport[j] = 0;
                        j++;
                        continue;
                }
            }
            double end_sec = gettime();
            printf("\b\b\bok! finished in %.1fs\n", end_sec - start_sec);
            consoleUpdate(NULL);
        }
    }

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}