# sys-clk-OC

Switch sysmodule allowing you to set cpu/gpu clocks according to the running application and docked state.


## Clock table (MHz)

official means HOS official, unless specified

### CPU clocks

* 2397 → OC max for Mariko (with CPU UV)
* 2295 → OC max for Mariko
* 2193
* 2091 → OC max for Erista
* 1963 → official and safe max for Mariko
* 1887
* 1785 → official boost mode, safe max for Erista
* 1683
* 1581
* 1428
* 1326
* 1224 → sdev OC
* 1122
* 1020 → official docked & handheld
* 918
* 816
* 714
* 612

### GPU clocks

* 1305 → N/A
* 1267 → official max for Mariko (Tegra X1+ official max)
* 1228 → recommended max for Hiopt
* 1152
* 1075 → recommended max for SLT
* 998 → safe max for Mariko, max for Erista (Tegra X1 official max)
* 921 → safe max for Erista
* 844
* 768 → official docked
* 691 
* 614 → recommended Mariko max for handheld
* 537
* 460 → max handheld
* 384 → official handheld
* 307 → official handheld
* 230
* 153
* 76 → boost mode

### MEM clocks

From Hekate Minerva module [sys_sdrammtc.c](https://github.com/CTCaer/hekate/blob/master/modules/hekate_libsys_minerva/sys_sdrammtc.c#L65)

- 3200 → l4t and closed version max for Mariko
- 2931
- 2665
- 2502 → max for Mariko
- 2400
- 2361 → l4t and closed version max for Erista
- 2131 → JEDEC. max for Erista and official max for Mariko. lpddr4(x) official max
- 2099
- 2064
- 1996 → OCS mariko default
- 1932
- 1894
- 1862 → JEDEC. official max for Erista; OCS erista default
- 1795
- 1728
- 1600 → official docked & official boost mode
- 1331 → JEDEC. official handheld
- 1065
- 800
- 665

## Capping

To protect the battery from excessive strain, clocks requested from config may be capped before applying, depending on your current profile:

### Erista (Safe)
|         | Handheld | Charging (USB) | Charging (Official) | Docked |
|:-------:|:--------:|:--------------:|:-------------------:|:------:|
| **MEM** | -        | -              | -                   | -      |
| **CPU** | 1785     | 1785           | 1785                | 1785   |
| **GPU** | 460      | 768            | -                   | -      |


### Erista (Unsafe allowed)
|         | Handheld | Charging (USB) | Charging (Official) | Docked |
|:-------:|:--------:|:--------------:|:-------------------:|:------:|
| **MEM** | -        | -              | -                   | -      |
| **CPU** | 1785     | -              | -                   | -      |
| **GPU** | 460      | 768            | -                   | -      |


### Mariko (Safe)
|         | Handheld | Charging (USB) | Charging (Official) | Docked |
|:-------:|:--------:|:--------------:|:-------------------:|:------:|
| **MEM** | -        | -              | -                   | -      |
| **CPU** | 1963     | 1963           | 1963                | 1963   |
| **GPU** | 998      | 998            | 998                 | 998    |


### Mariko (Unsafe allowed)
|         | Handheld | Charging (USB) | Charging (Official) | Docked |
|:-------:|:--------:|:--------------:|:-------------------:|:------:|
| **MEM** | -        | -              | -                   | -      |
| **CPU** | -        | -              | -                   | -      |
| **GPU** | 998      | -              | -                   | -      |


## Installation

The following instructions assumes you have a Nintendo Switch running Atmosphère, updated to at least the latest stable version.
Copy the `atmosphere`, and `switch` folders at the root of your sdcard, overwriting files if prompted. Also copy the `config` folder if you're not updating, to include default settings.

**Note:** sys-clk-overlay requires to have [Tesla](https://gbatemp.net/threads/tesla-the-nintendo-switch-overlay-menu.557362/) installed and running

## Relevant files

* Config file allows one to set custom clocks per docked state and title id, described below

	`/config/sys-clk-oc/config.ini`

* Log file where the logs are written if enabled

	`/config/sys-clk-oc/log.txt`

* Log flag file enables log writing if file exists

	`/config/sys-clk-oc/log.flag`

* CSV file where the title id, profile, clocks and temperatures are written if enabled

	`/config/sys-clk-oc/context.csv`

* sys-clk overlay (accessible from anywhere by invoking the [Tesla menu](https://gbatemp.net/threads/tesla-the-nintendo-switch-overlay-menu.557362/))

	`/switch/.overlays/sys-clk-overlay.ovl`

* sys-clk core sysmodule

	`/atmosphere/contents/00FF0000636C6BFF/exefs.nsp`
	`/atmosphere/contents/00FF0000636C6BFF/flags/boot2.flag`

## Config

Presets can be customized by adding them to the ini config file located at `/config/sys-clk/config.ini`, using the following template for each app

```
[Application Title ID]
docked_cpu=
docked_gpu=
docked_mem=
handheld_charging_cpu=
handheld_charging_gpu=
handheld_charging_mem=
handheld_charging_usb_cpu=
handheld_charging_usb_gpu=
handheld_charging_usb_mem=
handheld_charging_official_cpu=
handheld_charging_official_gpu=
handheld_charging_official_mem=
handheld_cpu=
handheld_gpu=
handheld_mem=
governor_config=
```

* Replace `Application Title ID` with the title id of the game/application you're interested in customizing.
A list of games title id can be found in the [Switchbrew wiki](https://switchbrew.org/wiki/Title_list/Games).
* Frequencies are expressed in mhz, and will be scaled to the nearest possible values, described in the clock table below.
* If any key is omitted, value is empty or set to 0, it will be ignored, and stock clocks will apply.
* If charging, sys-clk will look for the frequencies in that order, picking the first found
	1. Charger specific config (USB or Official) `handheld_charging_usb_X` or `handheld_charging_official_X`
	2. Non specific charging config `handheld_charging_X`
	3. Handheld config `handheld_X`

### Example 1: Zelda BOTW

* Overclock CPU when docked or charging

Leads to a smoother framerate overall (ex: in the korok forest)

```
[01007EF00011E000]
docked_cpu=1224
handheld_charging_cpu=1224
handheld_mem=1600
```

### Example 2: Picross

* Underclocks on handheld to save battery

```
[0100BA0003EEA000]
handheld_cpu=816
handheld_gpu=153
```

### Advanced

The `[values]` section allows you to alter timings in sys-clk, you should not need to edit any of these unless you know what you are doing. Possible values are:

| Key                      | Desc                                                                          | Default   |
|:------------------------:|-------------------------------------------------------------------------------|:---------:|
|**allow_unsafe_freq**     | Allow unsafe frequencies (CPU > 1963.5 MHz, GPU > 921.6 MHz)                  | OFF       |
|**auto_cpu_boost**        | Auto-boost CPU when system Core #3 utilization ≥ 95%                          | OFF       |
|**sync_reversenx_mode**   | Sync nominal profile (mode) with ReverseNX (-Tool and -RT)                    | ON        |
|**charging_current**      | Charging current limit (100 mA - 2000 mA)                                     | 2000 mA   |
|**charging_limit_perc**   | Charging limit (20% - 100%)                                                   | 100%(OFF) |
|**governor_experimental** | CPU & GPU frequency governor (Experimental)                                   | OFF       |
|**governor_handheld_only**| Use governor only on Handheld Profile		                                   | OFF       |
|**temp_log_interval_ms**  | Defines how often sys-clk log temperatures, in milliseconds (`0` to disable)  | 0 ms      |
|**csv_write_interval_ms** | Defines how often sys-clk writes to the CSV, in milliseconds (`0` to disable) | 0 ms      |
|**poll_interval_ms**      | Defines how fast sys-clk checks and applies profiles, in milliseconds         | 500 ms    |
