# Switch OC Suite

[![License: GPL v2](https://img.shields.io/badge/License-GPL_v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Downloads](https://img.shields.io/github/downloads/hanai3Bi/Switch-OC-Suite/total)](https://github.com/hanai3Bi/Switch-OC-Suite/releases)

이 프로젝트는 매우 위험하며 콘솔을 손상시킬 수 있습니다. 따라서 이 프로젝트를 사용하는걸 권장하지 않습니다. 사용할 경우 모든 책임은 본인에게 있습니다.

닌텐도 스위치 Atmosphere 커스텀 펌웨어 용 오버클럭 스위트

[프로젝트 홈페이지](https://hanai3Bi.github.io/Switch-OC-Suite)

**주의 사항: 사용시 모든 책임은 본인에게 있습니다!**

- 일반적으로 오버클럭 시 일부 하드웨어 구성 요소들의 수명이 단축됩니다. sys-clk-oc 에서 안전하지 않은 주파수 활성화 시 **문제나 고장에 대한 모든 책임은 본인에게 있습니다** . 제한 해제에 관한 이슈 등은 무시되거나 답글 없이 닫힐 수 있습니다.

- HorizonOS 의 구조 때문에, 안전하지 않은 RAM 주파수는 파일시스템 손상을 일으킬 수 있습니다. **메모리 오버클럭을 하기 전 반드시 백업을 하세요**

## 기능

- 구형 스위치 (HAC-001)
  - CPU / GPU 오버클럭 (안전한 클럭: 1785 / 921 MHz)
    - 안전하지 않은 클럭
      - 보드 전력 소모 한계나 전원부 IC 때문
      - 2091 / 998 MHz 까지 클럭 해제 가능
      - [README for sys-clk-OC](https://github.com/hanai3Bi/Switch-OC-Suite/blob/master/Source/sys-clk-OC/README.md) 참조

  - 메모리 오버클럭 (안전한 클럭: 1862.4 MHz)

- 신형 스위치 (HAC-001-01, HDH-001, HEG-001)
  - CPU / GPU 오버클럭 (안전한 클럭: 1963 / 998 MHz)
    - 안전하지 않은 클럭
      - 보드 전력 소모 한계나 전원부 IC 때문
      - 2295 / 1267 MHz 까지 클럭 해제 가능
      - [README for sys-clk-OC](https://github.com/hanai3Bi/Switch-OC-Suite/blob/master/Source/sys-clk-OC/README.md) 참조

  - 메모리 오버클럭 (안전한 클럭: 1996.8 MHz)

- 수정된 sys-clk 와 ReverseNX-RT
  - 자동 CPU 부스트
    - 게임 로딩 속도 향상 목적
    - CPU 코어#3 (시스템 코어)가 과부하시 CPU 부스트 (1785 MHz) 활성화 (주로 I/O 작업).
    - 충전기 연결시나 거버너 활성화 했을때만 가능
    - 이 기능은 구형 스위치에서, 특히 높은 GPU 주파수나 거버너랑 같이 사용시, 안전하지 않음.

  - CPU & GPU 주파수 거버너 (실험적 기능)
    - 부하에 따라 주파수를 조정함. 전력 소비를 줄일 수 있지만 렉을 유발할 수 있음. 타이틀 별로 끌 수 있음.
    - 신형 스위치에서 프로필의 CPU 주파수를 1020Mhz 보다 높은 값으로 설정 시, 최소 스케일링 주파수는 1020Mhz로 설정됨.
  - 충전 전류 (100 mA - 2000 mA) 와 충전 제한 (20% - 100%) 설정
    - 충전 제한을 장기간 사용시 배터리 수치가 부정확해 질 수 있음. 완충, 완방 시 재보정에 도움이 될 수 있음. 또는 [battery_desync_fix_nx](https://github.com/CTCaer/battery_desync_fix_nx) 를 사용.

  - 글로벌 프로필
    - 더미 타이틀 아이디 지정 `0xA111111111111111`.
    - 우선 순위: "Temp overrides" > "Application profile" > "Global profile" > "System default".

  - ReverseNX 모드 동기화
    - ReverseNX (-RT) 에서 모드 변경 후 클럭 변경 불필요

- **[System Settings (옵션)](https://github.com/hanai3Bi/Switch-OC-Suite/blob/master/system_settings.md)**


## 설치 방법

1. 최신 [릴리즈](https://github.com/hanai3Bi/Switch-OC-Suite/releases) 파일을 다운로드 한다.

2. `SdOut` 폴더 안의 모든 파일들을 SD카드의 최상단에 복사한다.

3. Atmosphere 버전에 맞는 `x.x.x_loader.kip` 파일을 `loader.kip` 으로 이름을 변경한 후, `/atmosphere/kips/` 로 이동 시킨다.

4. 맞춤 설정 [온라인 loader configurator](https://hanai3Bi.github.io/Switch-OC-Suite/#config):
    <details>

    | Defaults   | Mariko        | Erista        |
    | ---------- | ------------- | ------------- |
    | CPU OC     | 2295 MHz Max  | 2091 MHz Max  |
    | CPU Boost  | 1785 MHz      | N/A           |
    | CPU Volt   | 1235 mV Max   | 1235 mV Max   |
    | GPU OC     | 1267 MHz Max  | N/A           |
    | RAM OC     | 1996 MHz Max  | 1862 MHz Max  |
    | RAM Volt   | Disabled      | Disabled      |
    | RAM Timing | Auto-Adjusted | Auto-Adjusted |
    | CPU UV     | Disabled      | N/A           |
    | GPU UV     | Disabled      | N/A           |

    </details>

5. Hekate 부트로더 전용
   - `bootloader/hekate_ipl.ini`을 연 후, boot entry 항목에 `kip1=atmosphere/kips/loader.kip` 를 추가한다.

## AIO 를 통해 업데이트 하기

1. custom_packs.json 파일을 다운로드 한 후 /config/aio-switch-updater/custom_packs.json 에 복사한다.

2. AIO Switch Updater 를 실행한 후 커스텀 다운로드 탭으로 이동한다.

3. Switch-OC-Suite 를 선택한 후 계속하기를 누른다. 


## 빌드 방법

<details>

Grab necessary patches from the repo, then compile sys-clk, ReverseNX-RT and Atmosphere loader with devkitpro.

Before compiling Atmosphere loader, run `patch.py` in `Atmosphere/stratosphere/loader/source/` to insert oc module into loader sysmodule.

When compilation is done, uncompress the kip to make it work with configurator: `hactool -t kip1 Atmosphere/stratosphere/loader/out/nintendo_nx_arm64_armv8a/release/loader.kip --uncompress=./loader.kip`

</details>


## 크레딧

- CTCaer for [Hekate-ipl](https://github.com/CTCaer/hekate) bootloader, RE and hardware research
- [devkitPro](https://devkitpro.org/) for All-In-One homebrew toolchains
- masagrator for [ReverseNX-RT](https://github.com/masagrator/ReverseNX-RT) and info on BatteryChargeInfoFields in psm module
- Nvidia for [Tegra X1 Technical Reference Manual](https://developer.nvidia.com/embedded/dlc/tegra-x1-technical-reference-manual)
- RetroNX team for [sys-clk](https://github.com/retronx-team/sys-clk)
- SciresM and Reswitched Team for the state-of-the-art [Atmosphere](https://github.com/Atmosphere-NX/Atmosphere) CFW of Switch
- Switchbrew [wiki](http://switchbrew.org/wiki/) for Switch in-depth info
- Switchroot for their [modified L4T kernel and device tree](https://gitlab.com/switchroot/kernel)
- ZatchyCatGames for RE and original OC loader patches for Atmosphere
- KazushiMe for [Switch-OC-Suite](https://github.com/KazushiMe/Switch-OC-Suite)
