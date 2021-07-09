# Switch OC Suite

Overclocking suite for Switch(Erista and Mariko) running on Atmosphere CFW.



## Notice

### Reserved and intended for personal use ONLY. It will NOT be actively maintained.

### Failure to read this README carefully or Doing CPU/GPU Overclocking on Erista will brick or fry your device (in the short term or long term, who knows).

- Erista support will be dropped later.




## Features

- CPU/GPU Overclock up to 2397.0/1344.0 MHz for Mariko and up to 2295.0/1075.2 MHz for Erista

- Auto-Boost CPU when a game starts or is loading (Mariko Only)

- Optimization for fan control at high load (Mariko Only)

- RAM Overclock up to 2131.2 MHz for Erista and 1996.8 MHz for Mariko

- Disable some background services, less power consumption in standby mode (Optional)

- Sync sys-clk profiles with ReverseNX(-Tools and -RT), no need to change clocks after toggling modes

- Profile-aware frequency override for all games

- Game recording and SysDVR streaming @ 60fps with high video bitrate (Optional)

- Remove copyright watermark in screenshots/recordings and bypass connection test, courtesy of [HookedBehemoth](https://github.com/HookedBehemoth/exefs_patches)

### Details

- Bump CPU/GPU frequencies up to 2397.0/1344.0 MHz for Mariko and 2295.0/1075.2 MHz for Erista, bypassing Horizon OS limit.

  - Some SoCs may not reach MAX clock, or be unstable at/near MAX clock.
  - CPU/GPU Overclocking on Erista will consume more power than what the charger provides, and generates much more heat. **You have been WARNED AGAIN!**
  - Mariko is still functioning w/o charger under MAX OC(Your Mileage May Vary), therefore limit posed by sys-clk is lifted only for Mariko, but don't overdo it on battery.
  
- Auto-Boost CPU when a game starts or is in loading screen (Mariko Only, Optional). 

    - 1963.5 MHz w/o charger and 2295.0 MHz with charger

    - Some games don't utilize SetCpuBoostMode, e.g. Overcooked 2, so Auto-Boost would be invalid in loading screens.

- RAM Overclock for both Erista and Mariko consoles. For now, Erista can reach up to 2131.2 MHz with overvolting, and up to 1996.8 MHz for Mariko without overvolting.

  - RAM frequencies other than the only one you've chosen can NOT be used, but the impact of power consumption is negligible. So the ability to set RAM frequencies is removed in favor of ptm RAM patches, which could set RAM at specific clock permanently.

  - For Mariko:
  
    - Recommended frequency for Hynix RAM is 1731.2/1862.4 MHz(fk Hynix), but for Samsung and Micron ones you may use higher frequencies like 1996.8 MHz.
      - Use Hekate to check out the brand of your RAM chips.
    - Choose RAM clock with care, or your eMMC filesystem will be **corrupted**.

  - For Erista:

    - If you boot via Hekate, Minerva module(`/bootloader/sys/libsys_minerva.bso`) should be removed or recompiled with [changes](https://github.com/CTCaer/hekate/blob/master/modules/hekate_libsys_minerva/sys_sdrammtc.c#L31) applied.
    - RAM voltage is set at 1125mV @ 1731.2 MHz, 1150 mV @ 1862.4 MHz, 1175 mV @ 1996.8 MHz and 1200 mV @ 2131.2 MHz.

- Game recording and SysDVR streaming @ 60fps with high video bitrate (Optional).

  - Video duration shown in album will be 2x than the actual value, but playback speed is not affected.

  - Recordings may be less than 30sec if higher bitrate is used.

  - It has noticeable performance impacts in demanding games.



## Installation

### Method A: AIO Package

**Contains:**

- Patches for pcv and ptm modules

- Precompiled patch tools for pcv module (only for amd64 Windows, build yourself otherwise):

  [hactool](https://github.com/SciresM/hactool), [nx2elf](https://github.com/shuffle2/nx2elf), elf2nso from [switch-tools](https://github.com/switchbrew/switch-tools/), [hacPack](https://github.com/The-4n/hacPack), [bsdiff-win](https://github.com/cnSchwarzer/bsdiff-win/) ([bsdiff](http://www.daemonology.net/bsdiff/))

- Prebuilt sys-clk and ReverseNX-RT modified for OC

- `system-settings.ini` for Horizon OS

⚠️**Warnings**:⚠️

- Since system files are altered, you could **NOT** boot to stock(OFW) with patch applied to SysNAND until you revert the patch, and ban risks exist (?). Therefore, patching SysNAND is **NOT encouraged**.

- Restoring pcv module backup is required before updating Horizon OS and booting OFW. Launch the `patcher.te` script to restore your backup.

**Steps:**

1. Make sure you are running targeted HOS (12.0.x), and have `prod.keys` dumped by [Lockpick_RCM](https://github.com/shchmue/Lockpick_RCM).

2. Loader patches for Atmosphere: Grab from the web and apply. I won't provide them here. (Or build AMS with `ValidateAcidSignature()` stubbed.)

3. Place all the files in `SdOut` into SD card.
   See [Usage and customization](#usage and customization) and [Details](#details) sections for more info.

   - Be careful of `/atmosphere/config/system_settings.ini`, you may want to edit it manually. 

   - Remove all the files in previous OC Suite version before updating to avoid conflicts.

4. Dump your pcv module.

   If you already have the pcv backup of targeted HOS version, jump to Step 4. Otherwise, redump is required.

   - Load [TegraExplorer](https://github.com/suchmememanyskill/TegraExplorer/releases/latest) payload in hekate.

   - Choose `Browse SD` -> `patcher.te` -> `Launch Script`.

   - Select the MMC you'd like to mount and `Dump PCV Module Backup`

   - Wait for `Done!` showing up and transfer the backup `/atmosphere/oc_patches/pcv-backup` to your PC.

5. Extract `PatchTools` folder from the AIO package, put  `pcv-backup` and  `prod.keys` in.

6. Select RAM frequency and prepare the patches: 

   - Copy the `/atmosphere/oc_patches/xx-xxxx.x/ptm-patch` folder ->`/atmosphere/exefs_patches/` on your SD card.

   - Copy `/atmosphere/oc_patches/xx-xxxx.x/pcv-bspatch` -> `PatchTools` on your PC.

7. Open `cmd.exe`, change working directory to the `PatchTools` folder and type in the following commands.

   Assuming that you put the folder on your Desktop:

   ```cmd
   cd %USERPROFILE%\Desktop\PatchTools
   mkdir .\temp
   mkdir .\temp\pcv_exefs
   hactool -k prod.keys --disablekeywarns -t -nca .\pcv-backup --exefsdir .\temp
   nx2elf .\temp\main
   bspatch .\temp\main.elf .\temp\main-mod.elf .\pcv-bspatch
   elf2nso .\temp\main-mod.elf .\temp\pcv_exefs\main
   copy .\temp\main.npdm .\temp\pcv_exefs\
   hacpack -k prod.keys -o .\ --type nca --ncatype program --titleid 010000000000001A --exefsdir .\temp\pcv_exefs\
   ren *.nca pcv-module
   rd /S /Q .\temp\
   rd /S /Q .\hacpack_backup\
   
   ```

8. Move the patched `pcv-module` to `/atmosphere/oc_patches/`.

9. In TegraExplorer, `Browse SD` -> `patcher.te` -> `Launch Script` and then `Apply Patched PCV Module`.

10. Wait for `Done!` and then reboot to enjoy.



### Method B: For Pro-users

```bash
git clone https://github.com/KazushiMe/Switch-OC-Suite.git ~/Switch-OC-Suite

cd $YOUR_ATMOSPHERE_REPO
cp -R ~/Switch-OC-Suite/Atmosphere/*pp stratosphere/loader/source/
patch < ~/Switch-OC-Suite/Atmosphere/ldr_patcher.diff

cd $YOUR_SYS_CLK_REPO
git apply ~/Switch-OC-Suite/sys-clk.diff

cd $YOUR_REVERSENX_RT_REPO
git apply ~/Switch-OC-Suite/ReverseNX-RT.diff
```

Then compile sys-clk, ReverseNX-RT and Atmosphere with devkitpro, and don't forget to grab necessary patches in the repo.

Simply build `loader.kip` from Atmosphere and load it with hekate if you don't feel like wasting time.



## Usage and customization

**system_settings.ini** in `/atmosphere/config/`

- **For Erista:**
  - Remove the "Fan Control for Mariko" section.

- Remove the "disable services" part if you use Nintendo Online services like friends, cloud saving and notifications, or use Tesla overlays in Docked mode.
  - **Known Issue**: Tesla Menu and its overlays will sometimes crash atmosphere in Docked mode when some services are disabled.

- For "Game Recording FPS and Bitrate", if you play demanding games or don't care about streaming/framerate/bitrate, comment out this section.

**sys-clk**

- **For Mariko:**

  - Remove `/config/sys-clk/boost.flag` if you like longer waiting time in loading screens.

  - Remove `/config/sys-clk/boost_start.flag` if you don't want games to boot faster.

- Add `/config/sys-clk/downclock_dock.flag` to use handheld clocks in Docked mode when Handheld flag is set in ReverseNX.

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



## Credit

- CTCaer for modded Hekate bootloader, RE and hardware research

- HookedBehemoth for nifm_ctest and am_no_copyright [patches](https://github.com/HookedBehemoth/exefs_patches)

- masagrator for [ReverseNX-RT](https://github.com/masagrator/ReverseNX-RT)

- RetroNX team for [sys-clk](https://github.com/retronx-team/sys-clk) and a [place](https://*discord*.*gg*/erNC4FB) to discuss

- SciresM and Reswitched Team for the state-of-the-art [Atmosphere](https://github.com/Atmosphere-NX/Atmosphere) CFW of Switch

- suchmememanyskill for [TegraExplorer](https://github.com/suchmememanyskill/TegraExplorer) and [TegraScript](https://github.com/suchmememanyskill/TegraScript)

- Switchbrew [wiki](http://switchbrew.org/wiki/) for Switch in-depth info

- ZatchyCatGames for RE and original OC patches



