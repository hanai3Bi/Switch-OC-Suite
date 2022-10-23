#pragma once
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stack>
#include <nxExt.h>
#include <sysclk.h>
#include <switch.h>
#include "errors.h"
#include "file_utils.h"
#include "clocks.h"

class CpuCoreUtil {
public:
    CpuCoreUtil (int coreid = -2, uint64_t ms = 1):
        m_core_id(coreid), m_wait_time_ms(ms), m_wait_time_ns(ms * 1000'000ULL) {};

    inline uint64_t Get()      { Start(); WaitForStop(); Stop(); return Calculate(); };
    inline void Start()        { m_idletick = GetIdleTickCount(); };
    inline void WaitForStop()  { svcSleepThread(m_wait_time_ns); };
    inline void Stop()         { m_idletick = GetIdleTickCount() - m_idletick; };

    static constexpr uint64_t TICKS_PER_MS = 192;
    inline uint64_t Calculate() { return 100'0 - m_idletick * 10 / (TICKS_PER_MS * m_wait_time_ms); };

protected:
    const int m_core_id;
    const uint64_t m_wait_time_ms, m_wait_time_ns;
    uint64_t m_idletick;

    inline uint64_t GetIdleTickCount() {
        uint64_t idletick = 0;
        svcGetInfo(&idletick, InfoType_IdleTickCount, INVALID_HANDLE, m_core_id);
        return idletick;
    };
};

class GpuCoreUtil {
public:
    GpuCoreUtil (uint32_t nvgpu_field, uint64_t ms = 1):
        m_nvgpu_field(nvgpu_field), m_wait_time_ns(ms * 1000'000ULL) {};

    inline uint64_t Get()      { Wait(); return GetLoad(); };
    inline void Wait()         { svcSleepThread(m_wait_time_ns); };
    inline uint32_t GetLoad()  {
        uint32_t load;
        nvIoctl(m_nvgpu_field, NVGPU_GPU_IOCTL_PMU_GET_GPU_LOAD, &load);
        // if (R_FAILED(rc)) {
        //     ERROR_THROW("[mgr] nvIoctl() failed: 0x%lX", rc);
        // }
        return load;
    };

protected:
    uint32_t m_nvgpu_field;
    const uint64_t m_wait_time_ns;
    static constexpr uint64_t NVGPU_GPU_IOCTL_PMU_GET_GPU_LOAD = 0x80044715;
};

class ReverseNXSync {
public:
    ReverseNXSync ()
        : m_rt_mode(ReverseNX_NotFound), m_tool_mode(ReverseNX_NotFound) {
        CheckToolEnabled();
    };

    void ToggleSync(bool enable)    { m_sync_enabled = enable; };
    void Reset(uint64_t app_id)     { m_app_id = app_id; SetRTMode(ReverseNX_NotFound); GetToolMode(); }
    ReverseNXMode GetRTMode()           { return m_rt_mode; };
    void SetRTMode(ReverseNXMode mode)  { m_rt_mode = mode; };
    ReverseNXMode GetToolMode()         { return m_tool_mode = RecheckToolMode(); };
    SysClkProfile GetProfile(SysClkProfile real);
    ReverseNXMode GetMode();

protected:
    ReverseNXMode m_rt_mode, m_tool_mode;
    uint64_t m_app_id = 0;
    bool m_tool_enabled;
    bool m_sync_enabled;

    bool CheckToolEnabled();
    ReverseNXMode GetToolModeFromPatch(const char* patch_path);
    ReverseNXMode RecheckToolMode();
};

namespace PsmExt {
    void ChargingHandler(bool fastChargingEnabled, uint32_t chargingLimit);
};

class Governor {
public:
    Governor()  {
        memset(reinterpret_cast<void*>(&m_cpu_freq), 0, sizeof(m_cpu_freq));
        memset(reinterpret_cast<void*>(&m_gpu_freq), 0, sizeof(m_gpu_freq));

        m_cpu_freq.module = SysClkModule_CPU;
        m_gpu_freq.module = SysClkModule_GPU;

        m_cpu_freq.hz_list = &sysclk_g_freq_table_cpu_hz[0];
        m_gpu_freq.hz_list = &sysclk_g_freq_table_gpu_hz[0];

        m_cpu_freq.idx_boost_hz = FindIndex(&m_cpu_freq, 1785'000'000);

        m_gpu_freq.idx_boost_hz = FindIndex(&m_gpu_freq, 76'800'000);
        m_gpu_freq.idx_min_hz = FindIndex(&m_gpu_freq, 153'600'000);

        nvInitialize();
        Result rc = nvOpen(&m_nvgpu_field, "/dev/nvhost-ctrl-gpu");
        if (R_FAILED(rc)) {
            ERROR_THROW("[mgr] nvOpen() failed: 0x%lX", rc);
            nvExit();
        }
    };

    ~Governor() {
        Stop();
        nvClose(m_nvgpu_field);
        nvExit();
    };

    void Start();
    void Stop();
    void SetMaxHz(uint32_t max_hz, SysClkModule module);
    void SetCPUBoostHz(uint32_t hz) { m_cpu_freq.idx_boost_hz = FindIndex(&m_cpu_freq, hz); };
    void SetPerfConf(uint32_t id);

protected:
    // Parameters for sampling
    static constexpr uint64_t SAMPLE_RATE_MAIN = 60, SAMPLE_RATE_GPU = 60;
    static constexpr uint64_t SAMPLE_RATE_CPU = SAMPLE_RATE_GPU / 2;
    static constexpr uint64_t UPDATE_CONTEXT_RATE = 60;
    static constexpr uint64_t TICK_TIME_CPU_MS = 1000  / SAMPLE_RATE_CPU;
    static constexpr uint64_t TICK_TIME_CPU_NS = 1E9   / SAMPLE_RATE_CPU;
    static constexpr uint64_t TICK_TIME_GPU_MS = 1000  / SAMPLE_RATE_GPU;
    static constexpr uint64_t TICK_TIME_MAIN_MS = 1000 / SAMPLE_RATE_MAIN;
    static constexpr uint64_t TICK_TIME_MAIN_NS = 1E9  / SAMPLE_RATE_MAIN;

    // Parameters for frequency ramp threshold
    static constexpr uint64_t CPU_THR_RAMP_DOWN = 70'0;
    static constexpr uint64_t CPU_THR_RAMP_UP   = 90'0;
    static constexpr uint64_t GPU_THR_RAMP_DOWN = 60'0;
    static constexpr uint64_t GPU_THR_RAMP_UP   = 80'0;
    static constexpr uint64_t GPU_THR_RAMP_MAX  = 90'0;

    static constexpr int CORE_NUMS = 4;

    bool m_stop_threads = false;
    Thread m_t_cpuworker[CORE_NUMS], m_t_main;
    std::atomic<uint64_t> m_core3_stuck_cnt = 0;

    uint32_t m_nvgpu_field;
    uint32_t m_mem_freq;
    uint32_t m_perf_conf_id;
    SysClkApmConfiguration *m_apm_conf;

    typedef enum {
        RAMP_UP,
        RAMP_DOWN,
        RAMP_MAX,
        RAMP_MIN,
        RAMP_BOOST,
    } FREQ_RAMP_DIRECTION;

    typedef struct {
        SysClkModule module;
        uint32_t* hz_list;
        uint8_t idx_target_hz;
        uint8_t idx_min_hz;
        uint8_t idx_max_hz;
        uint8_t idx_boost_hz;
    } s_Freq;
    s_Freq m_cpu_freq, m_gpu_freq;

    static uint32_t FindIndex(s_Freq* f, uint32_t hz);
    static bool TargetRamp(s_Freq* f, FREQ_RAMP_DIRECTION dir);
    static void SetHz(s_Freq* f);
    static void SetBoostHz(s_Freq* f);

    typedef struct {
        Governor*   self;
        int64_t     id;
        uint64_t    util;
    } s_CoreContext;
    s_CoreContext m_cpu_core_ctx[CORE_NUMS];

    s_CoreContext* InitCoreContext(s_CoreContext* context, Governor* self, int64_t id = 0);

    static void CheckCpuUtilWorker(void* args);
    static void Main(void* args);

private:
    static constexpr size_t QUEUE_SIZE = 8;
    template <typename T>
    struct s_Queue {
        // Much faster than <queue> from stl
        T queue[QUEUE_SIZE] = { 0 };
        T sum = 0;
        T pos = 0;

        T GetAvg()   { return sum / QUEUE_SIZE; };
        T GetFirst() { return queue[pos % QUEUE_SIZE]; };
        T GetLast()  { return queue[(pos - 1) % QUEUE_SIZE]; };
        T PopAndPush(T val_to_push) {
            T val_to_pop;
            sum -= (val_to_pop = GetFirst()); // Pop and subtract from sum
            sum += (queue[pos % QUEUE_SIZE] = val_to_push); // Push and add to sum
            pos++;
            return val_to_pop;
        }
    };

    void CheckCpuUtilWorkerSysCore();
    void CheckCpuUtilWorkerAppCore(int64_t coreid);
};
