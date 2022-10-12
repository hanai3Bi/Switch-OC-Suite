# `system_settings.ini`

## Cherry-pick from below and add them manually.

  - Fan Control Optimization (Mariko only)
    - `[tc]`
    - Set `holdable_tskin` to 52˚C (default 48˚C).
    - Replacing stock thermal paste and adding thermal pad on Wi-Fi/BT module(shielded, adjacent to antennas) is recommended.
    - Beware that Aula (OLED model) has worse cooling compared to all previous models.

  - Disable background services (For power saving in standby mode)
    - Do NOT add this if online service is in use.

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
