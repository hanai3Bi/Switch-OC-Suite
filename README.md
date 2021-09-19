# Switch OC Suite

Overclocking suite for Switch **(Mariko Only)** running on Atmosphere CFW. Support Horizon OS 12.1.0.



## Notice

### Disclaimer

**Proceed with caution!**

**I AM NOT RESPONSIBLE (NOR IS ANYONE ELSE) FOR ANYTHING THAT MIGHT HAPPEN TO YOUR CONSOLE** (bans, internal component failure, etc.) by installing OC Suite or tinkering software/hardware with any info from this repo.



## Features

- **CPU/GPU/RAM Overclock** up to **2397.0/1344.0/2131.2 MHz**
- **Auto-Boost CPU for faster game loading**
- **Fan Control Optimization** at high load
- **Modded sys-clk and ReverseNX**(-Tools and -RT)
  - **No need to change clocks manually** after toggling modes in ReverseNX
  - Profile-aware clock override for all games
- System Settings
  - Disable background services, less heat and power consumption in standby mode
  - Game recording and SysDVR streaming @ 60fps with high video bitrate
  - Option to change the threshold for chargers providing enough power
- Remove copyright watermark in screenshots/recordings, courtesy of [HookedBehemoth](https://github.com/HookedBehemoth/exefs_patches)
- **TinyMemBenchNX**: DRAM throughput and latency test based on [tinymembench](https://github.com/ssvb/tinymembench)

#### Details

- **Overclock**
  - **Recommended CPU/GPU clock is 2295.0/1305.6 MHz**, since max clock(2397.0/1344.0 MHz) may not work on some SoCs.
  - **Recommended RAM clock is 1862.4 MHz** for most DRAM chips, **except Hynix ones** (1795.2/1728.0 MHz).
    - **RAM clock is set permanently** via **ptm-patch**, rather than sys-clk, considering the ability to select other clocks than the max one is lost if RAM OC takes into effect, and the impact of added power consumption is negligible.
    - Use Hekate to check out the brand of your RAM chips.
    - Choose RAM clock with care, or your eMMC filesystem will be **corrupted**.
    - Once RAM overvolting is available on Mariko, we may gain more stability and reach higher clock.
  - Mariko variants have much lower power consumption compared to Erista, therefore **GPU clock capping is lifted for Mariko**.
  - For more info, see [README.md](https://github.com/KazushiMe/Switch-OC-Suite/tree/master/Source/sys-clk-OC) in sys-clk-OC.
- **Auto-Boost CPU for faster game loading**
    - When a game launches or is in loading screen, sys-clk will boost CPU to 1963.5 MHz (w/o charger) and 2295.0 MHz (with charger) for ~10 seconds or until the loading screen ends.
    - Some games don't utilize `SetCpuBoostMode` at all, e.g. Overcooked 2, so Auto-Boost will be unavailable to these games.
    - To **disable this feature**, simply remove `boost_start.flag` and `boost.flag` in `/config/sys-clk/ `.
- **Fan Control Optimization** at high load for IcosaMariko/Hoag
  - Higher tolerable temperature and smoother fan curve. Set `holdable_tskin` to 56˚C. Previously it's set to 48˚C, so by default the fan would go crazy (80~100%) easily with a slight degree of OC.
- **Modded sys-clk and ReverseNX**(-Tools and -RT)
  - **No need to change clocks manually** after toggling modes in ReverseNX    
    - Add `/config/sys-clk/downclock_dock.flag` to use handheld clocks in Docked mode when Handheld mode is set in ReverseNX.
    - To **disable this feature**, use original version of ReverseNX-RT and delete `/config/sys-clk/ReverseNX_sync.flag`.
  - Profile-aware clock override for all games
    - Add `[A111111111111111]` title config in `/config/sys-clk/config.ini` to set frequency override globally:
      ```ini
      [A111111111111111]
      docked_cpu=
      docked_gpu=
      handheld_charging_cpu=
      handheld_charging_gpu=
      handheld_charging_usb_cpu=
      handheld_charging_usb_gpu=
      handheld_charging_official_cpu=
      handheld_charging_official_gpu=
      handheld_cpu=
      handheld_gpu=
      ```
- Disable background services, less heat and power consumption in standby mode
  - **Remove** the "Disable Background service" part in `/atmosphere/config/system_settings.ini` if you **use Nintendo Online services**.
- Game recording and SysDVR streaming @ 60fps with high video bitrate (7.5Mbps)
  - (Recommended)[dvr-patches](https://github.com/exelix11/dvr-patches): Allow recording in any games.
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

### Method A: AIO Package

**Contains:**

- Patches for pcv and ptm modules
- Patch tools for pcv module (only for amd64 Windows, build yourself otherwise):
  [hactoolnet](https://github.com/Thealexbarney/LibHac/), [nx2elf](https://github.com/shuffle2/nx2elf), elf2nso from [switch-tools](https://github.com/switchbrew/switch-tools/), [hacPack](https://github.com/The-4n/hacPack), [bsdiff-win](https://github.com/cnSchwarzer/bsdiff-win/) ([bsdiff](http://www.daemonology.net/bsdiff/))
- Prebuilt sys-clk-OC and ReverseNX-RT modified for OC
- `system-settings.ini` with some QoL improvements

**Notice**:

- **Patching SysNAND is NOT recommended**. Since system files are directly altered, you could **NOT** boot to stock(OFW) until you revert the patch.
- **Restoring pcv backup is required before updating** Horizon OS and booting OFW. Launch the `patcher.te` script to restore your backup.
- **Do NOT forget to reapply ptm-patch** after changing RAM OC clock.

**Steps:**
1. Make sure you are running targeted HOS, and have `prod.keys` *with latest master key* (`_0c`) dumped by [Lockpick_RCM](https://github.com/shchmue/Lockpick_RCM).
2. Loader patches for Atmosphere: Grab from the web and apply. I won't provide them here. (Or build AMS with `ValidateAcidSignature()` stubbed.)
3. Place all the files in `SdOut` into SD card.
   **See [Details](#details) section for more info.**
   - Be careful of `/atmosphere/config/system_settings.ini`, **you may want to edit it manually.**
   - Remove all the files in previous OC Suite version before updating to avoid conflicts.
4. Dump your pcv module.
   If you already have the pcv backup of targeted HOS version, jump to Step 5. Otherwise, redump is required.
   - Load [TegraExplorer](https://github.com/suchmememanyskill/TegraExplorer/releases/latest) payload in hekate.
   - Choose `Browse SD` -> `patcher.te` -> `Launch Script`.
   - Select the MMC you'd like to mount and `Dump PCV Module Backup`
   - Wait for `Done!` showing up and transfer the backup `/atmosphere/oc_patches/pcv-backup` to your PC.
5. Extract `PatchTools` folder from the AIO package, put  `pcv-backup` and  `prod.keys` in.
6. Select RAM frequency and prepare the patches: 
   - Copy the `/atmosphere/oc_patches/xx-xxxx.x/ptm-patch` folder ->`/atmosphere/exefs_patches/` on your SD card.
   - Copy `/atmosphere/oc_patches/xx-xxxx.x/pcv-bspatch` -> `PatchTools` on your PC.
7. Run `Patcher.bat` in PatchTools (Do NOT use admin privileges).
8. Move the patched `pcv-module` to `/atmosphere/oc_patches/`.
9. In TegraExplorer, `Browse SD` -> `patcher.te` -> `Launch Script` and then `Apply Patched PCV Module`.
10. Wait for `Done!` and then reboot to enjoy.



### Method B: For Pro-users

Grab necessary patches from the repo, then compile sys-clk, ReverseNX-RT and Atmosphere with devkitpro.
Simply build `loader.kip` from Atmosphere and load it with hekate if you don't feel like wasting time.



## Acknowledgement

- CTCaer for [Hekate-ipl](https://github.com/CTCaer/hekate) bootloader, RE and hardware research
- [devkitPro](https://devkitpro.org/) for All-In-One homebrew toolchains
- HookedBehemoth for am_no_copyright [patch](https://github.com/HookedBehemoth/exefs_patches)
- masagrator for [ReverseNX-RT](https://github.com/masagrator/ReverseNX-RT) and [BatteryChargeInfoNX](https://github.com/masagrator/BatteryChargeInfoNX)
- RetroNX team for [sys-clk](https://github.com/retronx-team/sys-clk)
- SciresM and Reswitched Team for the state-of-the-art [Atmosphere](https://github.com/Atmosphere-NX/Atmosphere) CFW of Switch
- suchmememanyskill for [TegraExplorer](https://github.com/suchmememanyskill/TegraExplorer) and [TegraScript](https://github.com/suchmememanyskill/TegraScript)
- Switchbrew [wiki](http://switchbrew.org/wiki/) for Switch in-depth info
- ZatchyCatGames for RE and original OC loader patches for Atmosphere

