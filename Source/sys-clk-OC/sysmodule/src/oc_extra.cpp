#include "oc_extra.h"

SysClkProfile ReverseNXSync::GetProfile(SysClkProfile real) {
    switch (this->GetMode()) {
        case ReverseNX_Docked:
            return SysClkProfile_Docked;
        case ReverseNX_Handheld:
            if (real == SysClkProfile_Docked)
                return SysClkProfile_HandheldChargingOfficial;
        default:
            return real;
    }
}

ReverseNXMode ReverseNXSync::GetMode() {
    if (!this->m_sync_enabled)
        return ReverseNX_NotFound;
    if (this->m_rt_mode)
        return this->m_rt_mode;
    return this->m_tool_mode;
}

bool ReverseNXSync::CheckToolEnabled() {
    FILE *fp = fopen("/atmosphere/contents/0000000000534C56/flags/boot2.flag", "r");
    if (fp) {
        this->m_tool_enabled = true;
        fclose(fp);
    } else {
        this->m_tool_enabled = false;
    }
    return this->m_tool_enabled;
}

ReverseNXMode ReverseNXSync::GetToolModeFromPatch(const char* patch_path) {
    constexpr uint32_t DOCKED_MAGIC = 0x320003E0;
    constexpr uint32_t HANDHELD_MAGIC = 0x52A00000;
    FILE *fp = fopen(patch_path, "rb");
    if (fp) {
        uint32_t buf = 0;
        fread(&buf, sizeof(buf), 1, fp);
        fclose(fp);

        if (buf == DOCKED_MAGIC)
            return ReverseNX_Docked;
        if (buf == HANDHELD_MAGIC)
            return ReverseNX_Handheld;
    }

    return ReverseNX_NotFound;
}

ReverseNXMode ReverseNXSync::RecheckToolMode() {
    ReverseNXMode mode = ReverseNX_NotFound;
    if (this->m_tool_enabled) {
        const char* fileName = "_ZN2nn2oe16GetOperationModeEv.asm64"; // or _ZN2nn2oe18GetPerformanceModeEv.asm64
        const char* filePath = new char[72];
        /* Check per-game patch */
        snprintf((char*)filePath, 72, "/SaltySD/patches/%016lX/%s", this->m_app_id, fileName);
        mode = this->GetToolModeFromPatch(filePath);
        if (!mode) {
            /* Check global patch */
            snprintf((char*)filePath, 72, "/SaltySD/patches/%s", fileName);
            mode = this->GetToolModeFromPatch(filePath);
        }
        delete[] filePath;
    }

    return mode;
}

void PsmExt::ChargingHandler(bool fastChargingEnabled, uint32_t chargingLimit) {
    PsmChargeInfo* info = new PsmChargeInfo;
    Service* session = psmGetServiceSession();
    serviceDispatchOut(session, Psm_GetBatteryChargeInfoFields, *info);

    if (PsmIsFastChargingEnabled(info) != fastChargingEnabled)
        serviceDispatch(session, fastChargingEnabled ? Psm_EnableFastBatteryCharging : Psm_DisableFastBatteryCharging);

    if (PsmIsChargerConnected(info)) {
        u32 chargeNow = 0;
        if (R_SUCCEEDED(psmGetBatteryChargePercentage(&chargeNow))) {
            bool isCharging = PsmIsCharging(info);
            if (isCharging && chargingLimit < chargeNow)
                serviceDispatch(session, Psm_DisableBatteryCharging);
            if (!isCharging && chargingLimit > chargeNow)
                serviceDispatch(session, Psm_EnableBatteryCharging);
        }
    }

    delete info;
}

void Governor::Start() {
    m_stop_threads = false;
    svcSleepThread(8 * TICK_TIME_MAIN_NS);
    Result rc = 0;

    for (int core = 0; core < CORE_NUMS; core++) {
        if (m_t_cpuworker[core].handle)
            continue;
        s_CoreContext* s = InitCoreContext(&m_cpu_core_ctx[core], this, core);
        rc = threadCreate(&m_t_cpuworker[core], &CheckCpuUtilWorker, (void*)s, NULL, 0x1000, 0x20, core);
        if (rc) {
            ERROR_THROW("Cannot create thread m_t_cpuworker[%d]: %u", core, rc);
            return;
        }
        rc = threadStart(&m_t_cpuworker[core]);
        if (rc) {
            ERROR_THROW("Cannot start thread m_t_cpuworker[%d]: %u", core, rc);
            return;
        }
    }
    rc = threadCreate(&m_t_main, &Main, (void*)this, NULL, 0x1000, 0x3F, 3);
    if (rc) {
        ERROR_THROW("Cannot create thread m_t_main: %u", rc);
        return;
    }
    rc = threadStart(&m_t_main);
    if (rc) {
        ERROR_THROW("Cannot start thread m_t_main: %u", rc);
        return;
    }
}

void Governor::Stop() {
    m_stop_threads = true;
    svcSleepThread(8 * TICK_TIME_MAIN_NS);

    threadWaitForExit(&m_t_main);
    threadClose(&m_t_main);

    for (int core = 0; core < CORE_NUMS; core++) {
        threadWaitForExit(&m_t_cpuworker[core]);
        threadClose(&m_t_cpuworker[core]);
    }
}

void Governor::SetMaxHz(uint32_t max_hz, SysClkModule module) {
    if (!max_hz) // Fallback to apm configuration
        max_hz = Clocks::GetStockClock(m_apm_conf, (SysClkModule)module);

    switch (module) {
        case SysClkModule_CPU:
            m_cpu_freq.idx_max_hz = FindIndex(&m_cpu_freq, max_hz);
            break;
        case SysClkModule_GPU:
            m_gpu_freq.idx_boost_hz = m_gpu_freq.idx_max_hz = FindIndex(&m_gpu_freq, max_hz);
            break;
        case SysClkModule_MEM:
            m_mem_freq = max_hz;
            Clocks::SetHz(SysClkModule_MEM, max_hz);
            break;
        default:
            break;
    }
}

void Governor::SetPerfConf(uint32_t id) {
    m_perf_conf_id = id;
    m_apm_conf = Clocks::GetEmbeddedApmConfig(id);
}

uint32_t Governor::FindIndex(s_Freq* f, uint32_t hz) {
    uint32_t idx = 0, hz_in_list;
    while ((hz_in_list = f->hz_list[idx]) != 0) {
        if (hz == hz_in_list)
            return idx;
        idx++;
    }
    ERROR_THROW("[mgr] Cannot find hz: %lu", hz);
    return 0;
}

bool Governor::TargetRamp(s_Freq* f, FREQ_RAMP_DIRECTION dir) {
    uint8_t idx_old = f->idx_target_hz;

    switch (dir) {
        case RAMP_UP:
            f->idx_target_hz++;
            if (f->idx_target_hz > f->idx_max_hz)
                f->idx_target_hz = f->idx_max_hz;
            break;
        case RAMP_DOWN:
            if (f->idx_target_hz > 0)
                f->idx_target_hz--;
            if (f->idx_target_hz < f->idx_min_hz)
                f->idx_target_hz = f->idx_min_hz;
            break;
        case RAMP_MAX:
            f->idx_target_hz = f->idx_max_hz;
            break;
        case RAMP_MIN:
            f->idx_target_hz = f->idx_min_hz;
            break;
        case RAMP_BOOST:
            f->idx_target_hz = f->idx_boost_hz;
            break;
    }

    uint8_t idx_new = f->idx_target_hz;
    bool changed = idx_old != idx_new;
    return changed;
}

void Governor::SetHz(s_Freq* f) {
    uint32_t hz = f->hz_list[f->idx_target_hz];
    if (hz)
        Clocks::SetHz(f->module, hz);
}

void Governor::SetBoostHz(s_Freq* f) {
    f->idx_target_hz = f->idx_boost_hz;
    if (f->module == SysClkModule_CPU && f->idx_max_hz > f->idx_boost_hz)
        f->idx_target_hz = f->idx_max_hz;
    SetHz(f);
}

Governor::s_CoreContext* Governor::InitCoreContext(
    s_CoreContext* context, Governor* self, int64_t id
) {
    memset(reinterpret_cast<void*>(context), 0, sizeof(s_CoreContext));
    context->self = self;
    context->id = id;
    return context;
}

void Governor::CheckCpuUtilWorker(void* args) {
    s_CoreContext* s = static_cast<s_CoreContext*>(args);
    int64_t coreid = s->id;
    Governor* self = s->self;

    bool isSystemCore = (coreid == CORE_NUMS - 1);
    if (isSystemCore)
        self->CheckCpuUtilWorkerSysCore();
    else
        self->CheckCpuUtilWorkerAppCore(coreid);
}

void Governor::CheckCpuUtilWorkerAppCore(int64_t coreid) {
    constexpr uint64_t STUCK_TICKS = 2;
    s_Queue<uint64_t> q;
    while (!m_stop_threads) {
        bool isBusy = m_core3_stuck_cnt > STUCK_TICKS * (CORE_NUMS - 1);
        if (isBusy) {
            m_core3_stuck_cnt = 0;
            SetBoostHz(&m_cpu_freq);
            svcSleepThread(STUCK_TICKS * TICK_TIME_CPU_NS);
        } else {
            m_core3_stuck_cnt++;
        }

        uint64_t load = CpuCoreUtil(coreid, TICK_TIME_CPU_MS).Get();
        q.PopAndPush(load);
        m_cpu_core_ctx[coreid].util = q.GetAvg();
    }
}

void Governor::CheckCpuUtilWorkerSysCore() {
    s_Queue<uint64_t> q;
    int64_t coreid = CORE_NUMS - 1;
    while (!m_stop_threads) {
        uint64_t load = CpuCoreUtil(coreid, TICK_TIME_CPU_MS).Get();
        q.PopAndPush(load);
        m_cpu_core_ctx[coreid].util = q.GetAvg() * 7 / 8; // Adjusted, Multipler: 0.875
    }
}

void Governor::Main(void* args) {
    Governor* self = static_cast<Governor*>(args);
    uint32_t nvgpu_field = self->m_nvgpu_field;

    auto GetCpuUtil = [self]() {
        uint64_t cpu_util = self->m_cpu_core_ctx[0].util;
        for (size_t i = 1; i < CORE_NUMS; i++) {
            if (cpu_util < self->m_cpu_core_ctx[i].util)
                cpu_util = self->m_cpu_core_ctx[i].util;
        }
        return cpu_util;
    };

    struct s_MaxQueue {
        uint32_t queue[QUEUE_SIZE] = { 0 };
        size_t   pos = 0;
    } q;

    auto GetGpuUtil = [nvgpu_field, q]() mutable {
        uint32_t load = GpuCoreUtil(nvgpu_field, TICK_TIME_GPU_MS).Get();
        if (load > 20) { // Ignore load <= 2.0%
            q.queue[q.pos % QUEUE_SIZE] = load;
            q.pos++;
        } else {
            load = q.queue[(q.pos - 1) % QUEUE_SIZE];
        }
        // Get max of the queue
        for (size_t i = 1; i < QUEUE_SIZE; i++) {
            size_t p = (q.pos + i - 1) % QUEUE_SIZE;
            if (load < q.queue[p])
                load = q.queue[p];
        }

        return load;
    };

    uint64_t update_ticks = SAMPLE_RATE_MAIN;
    bool CPUBoosted = false;
    bool GPUBoosted = false; // Limited to 76.8 MHz, literally

    while (!self->m_stop_threads) {
        self->m_core3_stuck_cnt = 0;

        bool shouldUpdateContext = update_ticks++ >= SAMPLE_RATE_MAIN;
        if (shouldUpdateContext) {
            update_ticks = 0;
            uint32_t hz = Clocks::GetCurrentHz(SysClkModule_GPU);
            // Sleep mode detected, wait 1 tick
            while (!hz) {
                self->m_core3_stuck_cnt = 0;
                svcSleepThread(TICK_TIME_MAIN_NS);
                hz = Clocks::GetCurrentHz(SysClkModule_GPU);
            }

            GPUBoosted = apmExtIsBoostMode(self->m_perf_conf_id, true);
            CPUBoosted = apmExtIsBoostMode(self->m_perf_conf_id, false);

            self->m_gpu_freq.idx_target_hz = FindIndex(&self->m_gpu_freq, hz);
            if (GPUBoosted)
                SetBoostHz(&self->m_gpu_freq);

            hz = Clocks::GetCurrentHz(SysClkModule_CPU);
            self->m_cpu_freq.idx_target_hz = FindIndex(&self->m_cpu_freq, hz);
            if (CPUBoosted)
                SetBoostHz(&self->m_cpu_freq);

            hz = Clocks::GetCurrentHz(SysClkModule_MEM);
            if (!self->m_mem_freq)
                self->m_mem_freq = hz;
            if (hz != self->m_mem_freq)
                Clocks::SetHz(SysClkModule_MEM, self->m_mem_freq);
        } else {
            if (!GPUBoosted) {
                uint32_t gpu_util = GetGpuUtil();
                if (gpu_util > GPU_THR_RAMP_MAX) {
                    if (TargetRamp(&self->m_gpu_freq, RAMP_MAX))
                        SetHz(&self->m_gpu_freq);
                } else if (gpu_util > GPU_THR_RAMP_UP) {
                    if (TargetRamp(&self->m_gpu_freq, RAMP_UP))
                        SetHz(&self->m_gpu_freq);
                } else if (gpu_util < GPU_THR_RAMP_DOWN) {
                    if (TargetRamp(&self->m_gpu_freq, RAMP_DOWN))
                        SetHz(&self->m_gpu_freq);
                }
            }
            if (!CPUBoosted) {
                uint64_t cpu_util = GetCpuUtil();
                if (cpu_util > CPU_THR_RAMP_UP) {
                    if (TargetRamp(&self->m_cpu_freq, RAMP_UP))
                        SetHz(&self->m_cpu_freq);
                } else if (cpu_util < CPU_THR_RAMP_DOWN) {
                    if (TargetRamp(&self->m_cpu_freq, RAMP_DOWN))
                        SetHz(&self->m_cpu_freq);
                }
            }
        }

        svcSleepThread(TICK_TIME_MAIN_NS);
    }
}

