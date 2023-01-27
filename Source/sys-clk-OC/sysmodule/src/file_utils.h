/*
 * --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

#pragma once
#include <switch.h>
#include <time.h>
#include <vector>
#include <string>
#include <cstring>
#include <atomic>
#include <cstdarg>
#include <sysclk.h>

#define FILE_CONFIG_DIR "/config/sys-clk"
#define FILE_FLAG_CHECK_INTERVAL_NS 5000000000ULL
#define FILE_CONTEXT_CSV_PATH FILE_CONFIG_DIR "/context.csv"
#define FILE_LOG_FLAG_PATH FILE_CONFIG_DIR "/log.flag"
#define FILE_LOG_FILE_PATH FILE_CONFIG_DIR "/log.txt"

class FileUtils
{
  public:
    static void Exit();
    static Result Initialize();
    static bool IsInitialized();
    static void InitializeAsync();
    static void LogLine(const char *format, ...);
    static void WriteContextToCsv(const SysClkContext* context);
    static void ParseLoaderKip();
    static Result mkdir_p(const char* dirpath);
  protected:
    static const uint16_t CUST_REV = 3;
    typedef struct CustTable {
        uint8_t  cust[4] = {'C', 'U', 'S', 'T'};
        uint16_t custRev;
        uint16_t mtcConf;
        uint32_t marikoCpuMaxClock;
        uint32_t marikoCpuBoostClock;
        uint32_t marikoCpuMaxVolt;
        uint32_t marikoGpuMaxClock;
        uint32_t marikoEmcMaxClock;
        uint32_t marikoEmcVolt;
        uint32_t eristaCpuMaxVolt;
        uint32_t eristaEmcMaxClock;
        uint32_t commonEmcMemVolt;
    } CustTable;

    static void RefreshFlags(bool force);
    static Result CustParser(const char* path, size_t filesize);
};
