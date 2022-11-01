#pragma once
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <nxExt.h>
#include <sysclk.h>
#include <switch.h>
#include "errors.h"
#include "file_utils.h"
#include "clocks.h"

class CpuCoreUtil {
public:
    CpuCoreUtil (int coreid, uint64_t ns);
    uint32_t Get();

protected:
    const int m_core_id;
    const uint64_t m_wait_time_ns;
    static constexpr uint64_t IDLETICKS_PER_MS = 192;
    static constexpr uint32_t UTIL_MAX = 100'0;

    uint64_t GetIdleTickCount();
};

class GpuCoreUtil {
public:
    GpuCoreUtil (uint32_t nvgpu_field);
    uint32_t Get();

protected:
    uint32_t m_nvgpu_field;
    static constexpr uint64_t NVGPU_GPU_IOCTL_PMU_GET_GPU_LOAD = 0x80044715;
};

class ReverseNXSync {
public:
    ReverseNXSync ();

    void ToggleSync(bool enable)    { m_sync_enabled = enable; };
    void Reset(uint64_t app_id)     { m_app_id = app_id; SetRTMode(ReverseNX_NotFound); GetToolMode(); }
    ReverseNXMode GetRTMode()           { return m_rt_mode; };
    void SetRTMode(ReverseNXMode mode)  { m_rt_mode = mode; };
    ReverseNXMode GetToolMode()         { return m_tool_mode = RecheckToolMode(); };
    SysClkProfile GetProfile(SysClkProfile real);
    ReverseNXMode GetMode();

protected:
    std::atomic<ReverseNXMode> m_rt_mode;
    ReverseNXMode m_tool_mode;
    uint64_t m_app_id = 0;
    bool m_tool_enabled;
    bool m_sync_enabled;

    ReverseNXMode GetToolModeFromPatch(const char* patch_path);
    ReverseNXMode RecheckToolMode();
};

namespace PsmExt {
    void ChargingHandler(bool fastChargingEnabled, uint32_t chargingLimit);
};

class Governor {
public:
    Governor();
    ~Governor();

    void Start();
    void Stop();
    void SetMaxHz(uint32_t max_hz, SysClkModule module);
    void SetAutoCPUBoost(bool enabled) { m_syscore_autoboost = enabled; };
    void SetCPUBoostHz(uint32_t boost_hz) { m_cpu_freq.boost_hz = boost_hz; };
    void SetPerfConf(uint32_t id);

protected:
    // Parameters for sampling
    static constexpr uint64_t SAMPLE_RATE = 200;
    static constexpr uint64_t TICK_TIME_NS = 1000'000'000 / SAMPLE_RATE;
    static constexpr uint64_t SYSTICK_HZ = 19200000;

    static constexpr int CORE_NUMS = 4;
    static constexpr int SYS_CORE_ID = (CORE_NUMS - 1);

    bool m_running = false;
    bool m_syscore_autoboost = false;
    Thread m_t_cpuworker[CORE_NUMS], m_t_main;

    uint32_t m_nvgpu_field;
    uint32_t m_perf_conf_id;
    SysClkApmConfiguration *m_apm_conf;

    typedef struct {
        SysClkModule module;
        uint32_t* hz_list;
        uint32_t  target_hz;
        uint32_t  min_hz;
        uint32_t  max_hz;
        uint32_t  boost_hz;
        uint32_t  utilref_hz;

        uint32_t GetNormalizedUtil(uint32_t raw_util);
        void SetNextFreq(uint32_t norm_util);
        void SetHz();
        void Boost();
    } s_FreqContext;
    s_FreqContext m_cpu_freq, m_gpu_freq;

    typedef struct {
        Governor*   self;
        int         id;
        uint32_t    util;
        uint64_t    tick;
    } s_CoreContext;
    s_CoreContext m_cpu_core_ctx[CORE_NUMS];

    // PELT: https://github.com/torvalds/linux/blob/master/kernel/sched/pelt.c
    // Util_acc_n = Util_0 + Util_1 * D + Util_2 * D^2 + ... + Util_n * D^n
    // To approximate D (decay multiplier):
    //   After 100 ms (if SAMPLE_RATE == 200, 20 samples)
    //   (UTIL_MAX * D)^20 ≈ 1 (UTIL_MAX decayed to 1)
    // D = 0.707946... ≈ 5799 / 8192 (epsilon < 0.0001)
    // Util_acc_20 ≈ 3419, Util_acc_40 ≈ 3420, Util_acc_inf ≈ 3420
    static constexpr uint32_t UTIL_MAX = 100'0;
    struct s_Util {
        uint32_t util_acc = 0;

        static constexpr uint32_t DECAY_DIVIDENT = 5799;
        static constexpr uint32_t DECAY_DIVISOR  = 8192;
        static constexpr uint32_t UTIL_ACC_MAX   = 3420;

        uint32_t Get()              { return (util_acc * UTIL_MAX / UTIL_ACC_MAX); };
        void Update(uint32_t util)  { util_acc = util_acc * DECAY_DIVIDENT / DECAY_DIVISOR + util; };
    };

    static void CpuUtilWorker(void* args);
    static void Main(void* args);

    // Get max from a sliding window in O(1)
    static constexpr size_t WINDOW_SIZE = SAMPLE_RATE / 10;
    template <typename T>
    class SWindowMax {
    protected:
        typedef struct {
            T item;
            T max;
        } s_Entry;

        struct s_Stack {
            s_Entry m_stack[WINDOW_SIZE] = {};
            size_t m_next = 0;

            bool  empty() { return m_next == 0; };
            s_Entry top() { return m_stack[m_next-1]; };
            s_Entry pop() { return m_stack[--m_next]; };
            void   push(s_Entry item) {
                if (m_next == WINDOW_SIZE)
                    return;
                m_stack[m_next++] = item;
            };
        };

        s_Stack enqStack;
        s_Stack deqStack;

        void Push(s_Stack& stack, T item) {
            s_Entry n;
            n.item = item;
            n.max = enqStack.empty() ? item : std::max(item, enqStack.top().max);
            stack.push(n);
        }

        void Pop() {
            if (deqStack.empty()) {
                while (!enqStack.empty())
                    Push(deqStack, enqStack.pop().max);
            }
            deqStack.pop();
        }

    public:
        SWindowMax() { deqStack.m_next = WINDOW_SIZE; }

        void Add(T item) { Pop(); Push(enqStack, item); }

        T Max() {
            if (!enqStack.empty()) {
                T enqMax = enqStack.top().max;
                if (!deqStack.empty()) {
                    T deqMax = deqStack.top().max;
                    return std::max(deqMax, enqMax);
                }
                return enqMax;
            }
            if (!deqStack.empty())
                return deqStack.top().max;
            return 0;
        }
    };
};
