# Switch OC Suite

[![Join the chat at https://gitter.im/Switch-OC-Suite/community](https://badges.gitter.im/Switch-OC-Suite/community.svg)](https://gitter.im/Switch-OC-Suite/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Overclocking suite for Switch **(Mariko Only)** running on Atmosphere CFW. Support Horizon OS 11.0.0-13.2.0.

This project will not be actively maintained by me and I'm looking for collaborators. Open an issue if you are interested and would like to be added into the maintainer list.



## Disclaimer

### USE AT YOUR OWN RISK!

**I AM NOT RESPONSIBLE FOR ANYTHING BAD THAT MIGHT HAPPEN TO YOUR CONSOLE** (bans, internal component failure, etc.) by installing OC Suite or tinkering software/hardware with any info from this repo.



## Features

- **CPU/GPU/RAM Overclock** up to **2397.0/1497.6/2131.2 MHz**
- **Fan Control Optimization** at high load
- **Modded sys-clk and ReverseNX**(-Tools and -RT)
  - **No need to change clocks manually** after toggling modes in ReverseNX
  - Auto-Boost CPU for faster game loading
  - Global clock override for all profiles
- System Settings
  - Disable background services, less heat and power consumption in standby mode
  - Game recording and SysDVR streaming @ 60fps with high video bitrate
  - Option to change the threshold for chargers providing enough power
- **TinyMemBenchNX**: DRAM throughput and latency test based on [tinymembench](https://github.com/ssvb/tinymembench)
- MemTesterNX: A userspace utility for testing DRAM faults based on [memtester](https://pyropus.ca/software/memtester/)
  - For testing stability, GPU/DRAM-heavy games like BotW/MHR will do better jobs as it's easier to spot framebuffer corruption/freeze

#### Details

- **Overclock**

  - **Official X1+ CPU/GPU Max clock: 1963.5/1267.2 MHz**.
    - Anything above that are not in the table of official module. ([issue #4](https://github.com/KazushiMe/Switch-OC-Suite/issues/4))

  - **Recommended RAM clock: 1862.4 MHz**.
    - Only 1331.2 and MAX MHz could be selected in sys-clk.
    - Apply thermal paste on RAM chips and test with emuNAND before long-term usage.
    - DRAM Timing Table Adjustment:
      - 2131.2 MHz should be stable for some Micron chips with ~24.8GiB/s throughput without overvolting. (theoretical bandwidth: 31.76GiB/s @ 2131.2 MHz)
      - 1862.4 MHz is stable for all. (~23.0 GB/s)

  - See [issue #5](https://github.com/KazushiMe/Switch-OC-Suite/issues/5) for more info on DRAM OC and timings

  - For more info on available clock rates, see [README.md](https://github.com/KazushiMe/Switch-OC-Suite/tree/master/Source/sys-clk-OC) in sys-clk-OC.

- **Overvolt**

  - CPU overvolting: 1220 mV, up from default 1120 mV. Frequencies ≥ 2193 MHz will enable overvolting.

  - GPU overvolting: 1170 mV, default 1050 mV. Frequencies ≥ 1420 Mhz trigger overvolting. ([issue #4](https://github.com/KazushiMe/Switch-OC-Suite/issues/4))
    - You cannot set ≥ 1344 MHz without official chargers.

  - RAM [NOT RECOMMENDED]
    - Only diff patch for hekate bootloader is provided.
    - Edit `oc.ini` to change Vddq voltage value:
    ```ini
    [emc]
    volt=600000
    ```
    - Overvolting beyond 650mV is not safe and proved to be not much helpful.

- **Fan Control Optimization** at high load
  - Higher tolerable temperature and smoother fan curve. Set `holdable_tskin` to 56˚C. Previously it's set to 48˚C, so by default the fan would go crazy (80~100%) easily with a slight degree of OC.
  - Replace crappy factory thermal paste is preferred.
  - Place a thermal pad onto Wi-Fi/BT module (shielded, adjacent to antennas) to lower tskin temperature.

- **Modded sys-clk and ReverseNX**(-Tools and -RT)
  - **No need to change clocks manually** after toggling modes in ReverseNX
    - Add `/config/sys-clk/downclock_dock.flag` to use handheld clocks in Docked mode when Handheld mode is set in ReverseNX.
    - To **disable this feature**, use original version of ReverseNX-RT and delete `/config/sys-clk/ReverseNX_sync.flag`.
  - **Auto-Boost CPU for faster game loading**
    - Enable CPU Boost (1963.5 MHz) if CPU Core#3 (System Core) is stressed, especially when the game is loading assets from eMMC/SD card.
    - Auto-Boost will be enabled only when charger is connected. (>90% w/ PD charger or >95% w/ unsupported charger)
    - To **disable this feature**, simply remove `boost.flag` in `/config/sys-clk/ `.

- Disable background services, less heat and power consumption in standby mode
  - **Remove** the "Disable Background service" part in `/atmosphere/config/system_settings.ini` if you **use Nintendo Online services**.

- Game recording and SysDVR streaming @ 60fps with high video bitrate (7.5Mbps)
  - (Recommended)[dvr-patches](https://github.com/exelix11/dvr-patches): Allow screenshot/recording in any games and remove overlay image (copyright notice or logo).
  - For optimal streaming experience, SysDVR via USB interface is recommended.
  - Known Issues (won't fix)
    - Game recordings may be less than 30 seconds if higher bitrate is used.
    - It has noticeable performance impacts in demanding games.
    - Video duration shown in album will be twice than the actual value, while the playback speed is not affected.
  - To **disable** this feature, simply remove the `[am.debug]` section in `system_settings.ini`.

- Option to change the threshold for chargers providing enough power
    - Find the string `enough_power_threshold_mw` in `system_settings.ini`. The default value is `0x9858` (39,000 mW).
    - To lower the threshold, you may change the value to `0x4268` (17,000 mW). Now the system and "sys-clk" will see typical Power Delivery chargers that only supply up to 18W (9V/2A) as "Official Chargers".



## Installation

- Modded `loader.kip` with embedded pcv, ptm, am-no-copyright, ValidateAcidSignature patches
- Prebuilt sys-clk-OC and ReverseNX-RT modified for OC
- Hekate with DRAM overvolting patch
- `system-settings.ini` with some QoL improvements

1. **Restoring pcv backup if you have patched pcv module manually:** Launch the `patcher.te` script via TegraExplorer to restore your backup. Separated **ptm patches should be removed** to avoid conflicts.

2. Copy all the files in `SdOut` to the root of SD card. `system_settings.ini` should be edited manually.

3. Grab `x.x.x_loader_xxxx.x.kip` for your Atmosphere version and desired RAM frequency, rename it to `loader.kip` and place it in `/atmosphere/kips/`.

4. **Hekate-ipl bootloader:**

   - Rename the kip to `loader.kip` and add `kip1=atmosphere/kips/loader.kip` in `bootloader/hekate_ipl.ini`

   **Atmosphere Fusee bootloader:**

   - Fusee will load any kips in `/atmosphere/kips/` automatically.



## Build

Grab necessary patches from the repo, then compile sys-clk, ReverseNX-RT, hekate and Atmosphere (or loader only) with devkitpro.



## Acknowledgement

- CTCaer for [Hekate-ipl](https://github.com/CTCaer/hekate) bootloader, RE and hardware research
- [devkitPro](https://devkitpro.org/) for All-In-One homebrew toolchains
- masagrator for [ReverseNX-RT](https://github.com/masagrator/ReverseNX-RT) and [BatteryChargeInfoNX](https://github.com/masagrator/BatteryChargeInfoNX)
- Nvidia for [Tegra X1 Technical Reference Manual](https://developer.nvidia.com/embedded/dlc/tegra-x1-technical-reference-manual)
- RetroNX team for [sys-clk](https://github.com/retronx-team/sys-clk)
- SciresM and Reswitched Team for the state-of-the-art [Atmosphere](https://github.com/Atmosphere-NX/Atmosphere) CFW of Switch
- Switchbrew [wiki](http://switchbrew.org/wiki/) for Switch in-depth info
- ZatchyCatGames for RE and original OC loader patches for Atmosphere
