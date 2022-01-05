# Switch OC Suite

[![License: GPL v2](https://img.shields.io/badge/License-GPL_v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html) [![Join the chat at https://gitter.im/Switch-OC-Suite/community](https://badges.gitter.im/Switch-OC-Suite/community.svg)](https://gitter.im/Switch-OC-Suite/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Overclocking suite for **Mariko Only** Switch running on Atmosphere CFW. Support Horizon OS 13.0.0-13.2.0.

This project will not be actively maintained.

I'd appreciate if someone is willing to contribute. But if you are releasing somewhere else (with or without your own modifications), be sure you are complying with GPL v2 license and _include necessary warnings for users_.



## DISCLAIMER: USE AT YOUR OWN RISK!

- Overclocking in general (often combined with overvolting and overheating) will _degrade internal components_ - SoC, VRM(Voltage Regulator Module), Battery, etc. - _faster_ than you and the manufacturer have expected.

- There is **no dynamic frequency scaling** in HorizonOS (HOS), which makes _overclocking acts differently than PC_ or other mobile devices. The console will be _sticking to what frequency you've set in the long term_, until you close the game or put it into sleep.

- Most games are _bottlenecked by RAM bandwidth_, **ONLY ramp up RAM clock** beyond official maximum (1963/1267/1600) to 1862/1996 MHz if you want to _stay safe_.

- Aula (OLED model) is not tested and I don't know if it works, even they have same X1+ SoC.


### Why not Erista?

- Tegra X1 on Erista is on TSMC 20nm HPM node, consumes much more power (~2x) and generates much more heat, compared to Tegra X1+ on Mariko (TSMC 16nm FinFET).
  - Snapdragon 810 (4 x A57 @ 2.0GHz + 4 x A53) also uses 20nm HPM, see how it plagued Android phones in 2014.

- The board power supply is quite limited, even if you've done cooling mod.
  - You could spot battery draining at higher clocks under stress test, even with official 39W PD charger.
  - CPU / GPU performance at max clocks will be worse if power supply is not enough.

- RAM overclock is already available online.



## Features

- **CPU/GPU/RAM Overclock**

  - Safe: Official Maximum (1963/1267/1600) with DRAM OC

    - CPU/GPU: 1963/1267 MHz exists in official pcv module, but is not unlocked.

    - DRAM: 2131.2 MHz @ 600 mV (stock)
      - 1862.4 / 1996.8 MHz should be stable for all (Samsung / Micron/ Hynix).
      - 2131.2 MHz might be stable for some chips.
      - Theoretically, you could replace 2x2GB DRAM chips (dual-channel) with 2x4GB DRAM chips from SAMSUNG (quad-channel) to attain doubled bandwidth.

  - Unsafe: All maxed out **(NOT RECOMMENDED)**

    - Why NOT RECOMMENDED?
      - See `Current Flow` in sys-clk-OC overlay `Miscellaneous` (on battery) or measure power draw from charger yourself.

    - CPU: 2397 MHz @ 1220 mV (overvolting from 1120 mV)
      - Performance depends on CPU speedo (higher speedo === lower voltage required === better silicon quality).
        - You'd get somewhere between 2360 to 2390 MHz performance for real.
      - This is where floating point performance maxed out.
      - ≥ 2193 MHz will _ENABLE OVERVOLTING_.

    - GPU: 1497 MHz @ 1170 mV (overvolting from 1050 mV)
      - Not tested. Not sure if it has any benefit over 1267 MHz. GPU (OpenGL) benchmark is required.
      - ≥ 1344 MHz _cannot be set without official chargers_.
      - ≥ 1420 MHz will _ENABLE OVERVOLTING_.

    - DRAM: Overvolting
      - [Use this to set DRAM bus voltage](https://gist.github.com/KazushiMe/6bb0fcbefe0e03b1274079522516d56d).

- **Modded sys-clk and ReverseNX**(-RT)

  - No need to change clocks manually after toggling modes in ReverseNX (Optional)
    - To disable this feature, use original version of ReverseNX-RT and remove `/config/sys-clk/ReverseNX_sync.flag`.

  - Auto-Boost CPU for faster game loading (Optional)
    - Enable CPU Boost (1785 MHz) when CPU Core#3 (System Core) is stressed, especially when the game is loading assets from eMMC/SD card (I/O ops).
    - Auto-Boost will be enabled only when charger is connected.
    - To disable this feature, remove `/config/sys-clk/boost.flag`.

  - Permanent global clock override
    - Expected usage: set maximum DRAM clocks for all games and profiles.

  - View charger & battery info, toggle charging/fast-charging(2A) in overlay
    - Extend battery life expectancy by maintaining battery charge at 40% - 60% and disabling fast charging if possible.
    - Known issue: Fast charging toggle will be reset in-game.

- System Settings (Optional)

  - Cherry-pick from below and add them manually.

  - Fan Control Optimization at high load
    - `[tc]`
    - Set `holdable_tskin` to 56˚C (default 48˚C), less audible fan noise.
    - Replacing stock thermal paste and adding thermal pad on Wi-Fi/BT module(shielded, adjacent to antennas) is recommended.

  - Disable background services, less heat and power consumption in standby mode
    - `;Disable Background service`
    - Don't add this if you **use Nintendo Online services**.

  - Game recording and SysDVR streaming @ 60fps with high video bitrate (7.5Mbps)
    - `[am.debug]`
    - Recommended: [dvr-patches](https://github.com/exelix11/dvr-patches): Allow screenshot/recording in any games and remove overlay image (copyright notice or logo).
      - For optimal streaming experience, SysDVR via USB interface is recommended.
    - Known Issues (won't fix)
      - Game recordings may be less than 30 seconds if higher bitrate is used.
      - It has noticeable performance impacts in demanding games.
      - Video duration shown in album will be doubled, while the playback speed or mp4 file itself are not affected.

  - Change the threshold for chargers providing enough power
    - `[psm]`
    - `enough_power_threshold_mw` is be the threshold of "Official Charger"
      - E.g. set it to `0x4268` (17,000 mW) and 18W power bank will be "Official Charger".

- **TinyMemBenchNX**: DRAM throughput and latency test based on [tinymembench](https://github.com/ssvb/tinymembench)

- **MemTesterNX**: A userspace utility for testing DRAM faults and stability based on [memtester](https://pyropus.ca/software/memtester/)
  - Now with multi-thread support and "stress DRAM" option, it should be able to test DRAM stability with adjusted timings.



## Installation

1. Copy all the files in `SdOut` to the root of SD card. `system_settings.ini` should be edited manually.

2. Grab `x.x.x_loader_xxxx.x.kip` for your Atmosphere version and desired RAM frequency, rename it to `loader.kip` and place it in `/atmosphere/kips/`.

3. **Hekate-ipl bootloader**
   - Rename the kip to `loader.kip` and add `kip1=atmosphere/kips/loader.kip` in `bootloader/hekate_ipl.ini`

   **Atmosphere Fusee bootloader:**
   - Fusee will load any kips in `/atmosphere/kips/` automatically.



## Build

Grab necessary patches from the repo, then compile sys-clk, ReverseNX-RT and Atmosphere loader with devkitpro.



## Acknowledgement

- CTCaer for [Hekate-ipl](https://github.com/CTCaer/hekate) bootloader, RE and hardware research
- [devkitPro](https://devkitpro.org/) for All-In-One homebrew toolchains
- masagrator for [ReverseNX-RT](https://github.com/masagrator/ReverseNX-RT) and info on BatteryChargeInfoFields in psm module
- Nvidia for [Tegra X1 Technical Reference Manual](https://developer.nvidia.com/embedded/dlc/tegra-x1-technical-reference-manual)
- RetroNX team for [sys-clk](https://github.com/retronx-team/sys-clk)
- SciresM and Reswitched Team for the state-of-the-art [Atmosphere](https://github.com/Atmosphere-NX/Atmosphere) CFW of Switch
- Switchbrew [wiki](http://switchbrew.org/wiki/) for Switch in-depth info
- ZatchyCatGames for RE and original OC loader patches for Atmosphere
