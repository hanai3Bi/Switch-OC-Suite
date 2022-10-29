# Switch OC Suite

[![License: GPL v2](https://img.shields.io/badge/License-GPL_v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html) 

Overclocking suite for Horizon OS (HOS) running on Atmosphere CFW.

This project will not be actively maintained or regularly updated along with Atmosphere CFW.


## DISCLAIMER: USE AT YOUR OWN RISK!

- Overclocking in general (often combined with overvolting and overheating) will _degrade internal components_ - SoC, VRM(Voltage Regulator Module), Battery, etc. - _faster_ than you and the manufacturer have expected.

- Higher RAM clocks without proper timings could be UNSTABLE and cause graphical glitches / instabilities / filesystem corruption. **Always make backup before usage.**

## Features

- **DRAM Overclock**

  - Most games are **bottlenecked by RAM bandwidth**

  - Safe:
    - Mariko: 1996.8 MHz has been tested stable for all (Samsung / Micron / Hynix), with built-in timing auto-adjustment.
    - Erista: 1862.4 MHz.

  - Unsafe: higher than 1996.8 MHz or overvolting
    <details>

    - Timing:
      - Timing parameters could be auto-adjusted (default) or overwritten with user-provided mtc table.
      - Customization: No GUI tool, requires [rebuilding](#Build).

    - DRAM bus overvolting (Erista Only).
      - Mariko: [use this to set DRAM bus voltage](https://gist.github.com/KazushiMe/6bb0fcbefe0e03b1274079522516d56d).

    </details>

- **Modded sys-clk and ReverseNX**(-RT)

  - Global profile
    - Designated a dummy title id `0xA111111111111111`.
    - Priority: "Temp overrides" > "Application profile" > "Global profile" > "System default".

  - Miscellaneous
    - Sync ReverseNX Mode: No need to change clocks manually after toggling modes in ReverseNX

- **[System Settings (Optional)](https://github.com/KazushiMe/Switch-OC-Suite/blob/master/system_settings.md)**


### Mariko Only

- **CPU/GPU Overclock**

  - Safe: CPU/GPU @ 1963/921 MHz

  - Unsafe: **NOT RECOMMENDED**
    - **Disabled by default**, toggle "Allow Unsafe Frequencies" on in overlay or add `allow_unsafe_freq=1` to `config.ini`
    - Power draw will be significant higher than what the mainboard was designed to tolerate at anything higher than 1963/921 MHz.
    - See [README for sys-clk-OC](https://github.com/KazushiMe/Switch-OC-Suite/blob/master/Source/sys-clk-OC/README.md)
    
  - Auto CPU Boost: For faster game loading
    - Enable CPU Boost (1785 MHz, could be configured higher) when CPU Core#3 (System Core) is stressed, especially when the game is loading assets from eMMC/SD card (I/O ops).
    - Auto-Boost will be enabled only when charger is connected.
  
  - View charger & battery info, toggle fast-charging(2A) or set charge limit (20% - 100%) in overlay
    - Note: Long-term use of charge limit may render the battery gauge inaccurate. Performing full cycles could help recalibration, or try [battery_desync_fix_nx](https://github.com/CTCaer/battery_desync_fix_nx).



## Installation

1. Download latest [release](https://github.com/KazushiMe/Switch-OC-Suite/releases/latest).

2. Copy all files in `SdOut` to the root of SD card.

3. Grab `x.x.x_loader.kip` for your Atmosphere version, rename it to `loader.kip` and place it in `/atmosphere/kips/`.

4. Customization
    <details>

    | Defaults   | Mariko        | Erista       |
    | ---------- | ------------- | ------------ |
    | CPU OC     | 2397 MHz Max  | Disabled     |
    | CPU Boost  | 1785 MHz      | N/A          |
    | CPU Volt   | 1220 mV Max   | Disabled     |
    | GPU OC     | 1305 MHz Max  | N/A          |
    | RAM OC     | 1996 MHz Max  | 1862 MHz Max |
    | RAM Volt   | N/A           | Disabled     |
    | RAM Timing | Auto-Adjusted | Disabled     |

    </details>

  - Loader configurator
    - Grab [ldr_config.py](https://github.com/KazushiMe/Switch-OC-Suite/raw/master/ldr_config.py) and modify values in `cust_conf` dict.
    - `python ldr_config.py loader.kip -s` will save your configuration in-place.

5. **Hekate-ipl bootloader**
   - Rename the kip to `loader.kip` and add `kip1=atmosphere/kips/loader.kip` in `bootloader/hekate_ipl.ini`
   - Erista: Minerva module conflicts with HOS DRAM training. Recompile with frequency changed is recommeded, although you could simply remove `bootloader/sys/libsys_minerva.bso`.

   **Atmosphere Fusee bootloader:**
   - Fusee will load any kips in `/atmosphere/kips/` automatically.


### Patching sysmodules manually

This method is only served as reference as it could damage your MMC file system if not handled properly.

Patched sysmodules would be persistent until pcv or ptm was updated in new HOS (normally in `x.0.0`).

<details>

  Tools:
  - Lockpick_RCM
  - TegraExplorer
  - [hactool](https://github.com/SciresM/hactool)
  - [nx2elf](https://github.com/shuffle2/nx2elf)
  - elf2nso from [switch-tools](https://github.com/switchbrew/switch-tools/)
  - [hacpack](https://github.com/The-4n/hacPack)

  1. Dump `prod.keys` with Lockpick_RCM
  2. Dump HOS firmware with TegraExplorer
  3. Configure and run `test_patch.sh` to generate patched pcv & ptm sysmodules in nca
  4. Replace nca in `SYSTEM:/Contents/registered/` with TegraExplorer
  5. `ValidateAcidSignature()` should be stubbed to allow unsigned sysmodules to load (a.k.a. `loader_patch`)

</details>



## Build

### Loader KIP

Grab necessary patches from the repo, then compile sys-clk, ReverseNX-RT and Atmosphere loader with devkitpro.

If you are to install nro forwarders, stub `ValidateAcidSignature()` with `R_SUCCEED();` in `Atmosphere/stratosphere/loader/source/ldr_meta.cpp` to make them work again.

Uncompress the kip to make it work with config editor: `hactool -t kip1 Atmosphere/stratosphere/loader/out/nintendo_nx_arm64_armv8a/release/loader.kip --uncompress=./loader.kip`



## Acknowledgement

- CTCaer for [Hekate-ipl](https://github.com/CTCaer/hekate) bootloader, RE and hardware research
- [devkitPro](https://devkitpro.org/) for All-In-One homebrew toolchains
- masagrator for [ReverseNX-RT](https://github.com/masagrator/ReverseNX-RT) and info on BatteryChargeInfoFields in psm module
- Nvidia for [Tegra X1 Technical Reference Manual](https://developer.nvidia.com/embedded/dlc/tegra-x1-technical-reference-manual)
- RetroNX team for [sys-clk](https://github.com/retronx-team/sys-clk)
- SciresM and Reswitched Team for the state-of-the-art [Atmosphere](https://github.com/Atmosphere-NX/Atmosphere) CFW of Switch
- Switchbrew [wiki](http://switchbrew.org/wiki/) for Switch in-depth info
- ZatchyCatGames for RE and original OC loader patches for Atmosphere
