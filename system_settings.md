## `/atmosphere/config/system_settings.ini`

  - Default Fan Control Parameters
    - [Info on tskin coefficients](https://github.com/masagrator/Status-Monitor-Overlay/blob/master/docs/modes.md#additional-info)
    - If (tskin > holdable_tskin and handheld mode) or (tskin > touchable_tskin and dock mode), then fan speed is set to 100% regardlessly.
    - https://switchbrew.org/wiki/System_Settings#tc

  - Disable background services (For power saving in standby mode)
    - For power saving in standby mode
    - Do NOT add this if online service is in use.
    ```ini
    [bgtc]
    enable_halfawake = u32!0x0
    minimum_interval_normal = u32!0x7FFFFFFF
    minimum_interval_save = u32!0x7FFFFFFF
    battery_threshold_save = u32!0x64
    battery_threshold_stop = u32!0x64

    [npns]
    background_processing = u8!0x0
    sleep_periodic_interval = u32!0x7FFFFFFF
    sleep_processing_timeout = u32!0x0
    sleep_max_try_count = u32!0x0

    [ns.notification]
    enable_download_task_list = u8!0x0
    enable_download_ticket = u8!0x0
    enable_network_update = u8!0x0
    enable_random_wait = u8!0x0
    enable_request_on_cold_boot = u8!0x0
    enable_send_rights_usage_status_request = u8!0x0
    enable_sync_elicense_request = u8!0x0
    enable_version_list = u8!0x0
    retry_interval_min = u32!0x7FFFFFFF
    retry_interval_max = u32!0x7FFFFFFF
    version_list_waiting_limit_bias = u32!0x7FFFFFFF
    version_list_waiting_limit_min = u32!0x7FFFFFFF

    [account]
    na_required_for_network_service = u8!0x0
    na_license_verification_enabled = u8!0x0

    [account.daemon]
    background_awaking_periodicity = u32!0x7FFFFFFF
    initial_schedule_delay = u32!0x7FFFFFFF
    profile_sync_interval = u32!0x7FFFFFFF
    na_info_refresh_interval = u32!0x7FFFFFFF

    [capsrv]
    enable_album_screenshot_filedata_verification = u8!0x0
    enable_album_movie_filehash_verification = u8!0x0
    enable_album_movie_filesign_verification = u8!0x0

    [friends]
    background_processing = u8!0x0

    [notification.presenter]
    snooze_interval_in_seconds = u32!0x7FFFFFFF
    connection_retry_count = u32!0x0
    alarm_pattern_total_repeat_count = u32!0x0
    alarm_pattern_with_vibration_repeat_count = u32!0x0

    [prepo]
    ;background_processing = u8!0x0 (shutdown directly when entering sleep mode)
    transmission_interval_min = u32!0x7FFFFFFF
    transmission_retry_interval_min = u32!0x7FFFFFFF
    transmission_retry_interval_max = u32!0x7FFFFFFF
    transmission_interval_in_sleep = u32!0x7FFFFFFF
    statistics_save_interval_min = u32!0x7FFFFFFF
    statistics_post_interval = u32!0x7FFFFFFF
    save_system_report = u8!0x0

    [olsc]
    default_auto_upload_global_setting = u8!0x0
    default_auto_download_global_setting = u8!0x0
    autonomy_registration_interval_seconds = u32!0x7FFFFFFF
    network_service_license_info_cache_expiration_seconds = u32!0x7FFFFFFF
    postponed_transfer_task_processing_interval_seconds = u32!0x7FFFFFFF
    retry_offset_seconds = u32!0x7FFFFFFF
    network_trouble_detection_span_seconds = u32!0x7FFFFFFF
    network_connection_polling_interval_seconds = u32!0x7FFFFFFF
    is_save_data_backup_policy_check_required = u8!0x0
    is_global_transfer_task_autonomy_registration_enabled = u8!0x0
    is_on_event_transfer_task_registration_enabled = u8!0x0
    is_periodic_transfer_task_registration_enabled = u8!0x0

    [ntc]
    is_autonomic_correction_enabled = u8!0x0
    autonomic_correction_interval_seconds = u32!0x7FFFFFFF
    autonomic_correction_failed_retry_interval_seconds = u32!0x7FFFFFFF
    autonomic_correction_immediate_try_count_max = u32!0x0
    autonomic_correction_immediate_try_interval_milliseconds = u32!0x7FFFFFFF

    [systemupdate]
    bgnup_retry_seconds = u32!0x7FFFFFFF

    [ns.rights]
    skip_account_validation_on_rights_check = u8!0x1
    next_available_time_of_unexpected_error = u32!0x7FFFFFFF

    [pctl]
    intermittent_task_interval_seconds = u32!0x7FFFFFFF

    [sprofile]
    adjust_polling_interval_by_profile = u8!0x0
    polling_interval_sec_max = u32!0x7FFFFFFF
    polling_interval_sec_min = u32!0x7FFFFFFF
    ```

  - Parameters of game recording and streaming
    - ```ini
      [am.debug]
      continuous_recording_fps = u32!60 ; 30 or 60 FPS, default: 30
      continuous_recording_video_bit_rate = u32!0x780000 ; 7.5Mbps(0x780000 = 7,864,320), default: ~5Mbps(0x4C4B40), VBR(Variable Bitrate)
      continuous_recording_key_frame_count = u32!15 ; One I-frame in 15 frames (with other 14 P-frames), default: 15
      ```
    - Recommended: [dvr-patches](https://github.com/exelix11/dvr-patches): Allow screenshot/recording in any games and remove overlay image (copyright notice or logo).
      - For optimal streaming experience, SysDVR via USB interface is recommended.
    - Known Issues (won't fix)
      - Game recordings may be less than 30 seconds if higher bitrate is used.
      - It has noticeable performance impacts in demanding games.
      - Video duration shown in album will be doubled, while the playback speed or mp4 file itself are not affected.

  - Charging
    - https://switchbrew.org/wiki/System_Settings#psm
    - `enough_power_threshold_mw`
    - `cdp_dcp_input_current_limit_in_ma`: 5V CDP/DCP (BC1.2/QC?) Charger Current Limit