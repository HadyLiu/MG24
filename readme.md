# Li-Bat MatterLight (EFR32MG24)

Battery-powered Matter / Thread smart light firmware for **EFR32MG24B010F1536IM40**, based on Silicon Labs Simplicity SDK and Matter extension.

Chinese version: [readme_zh.md](./readme_zh.md)

## Overview

| Item | Value |
|------|--------|
| MCU | Silicon Labs EFR32MG24B010F1536IM40 |
| Protocol | Matter over Thread (BLE commissioning) |
| SDK | Simplicity SDK **2025.12.2** / Matter extension **2.8.1** |
| Product focus | WRGB main light, indicators, battery power, low power |
| App code | Layered C++ under `User/` (HAL → BSP → Middlewares → Service) |

This repository is a Simplicity Studio solution workspace: Matter application (`MatterLightOverThread`), bootloader (`Matter-Bootloader`), solution CMake wrapper, and custom product logic in `User/`.

## Features

- **Light control**: short-press brightness cycle (100% → 35% → OFF), double-press color library, fade timing
- **Matter**: On/Off, Level, Color (HSV / CT / XY), Identify, commissioning window
- **Factory reset**: long-press warning sequence from ~8 s, then NVM write + factory reset; boot flash + fade-in
- **Indicators**: red / white system LEDs for commissioning, battery, and status
- **Power**: battery monitoring, low / critical battery behavior, charge awareness
- **Low power**: ICD-oriented wake coordination via `LowPowerCoordinator`
- **LED driver**: SM15135E (SPI) WRGB channel control

Detailed product requirements: [Doc/设计方案.md](./Doc/设计方案.md)

## Repository Layout

```
.
├── MatterLightOverThread/     # Matter lighting app (Studio / SLC generated + config)
├── Matter-Bootloader/         # Bootloader project
├── Li-Bat_*_cmake/            # Solution CMake (builds app + bootloader + postbuild)
├── User/                      # Product firmware (do not lose on Studio regenerate)
│   ├── entry.cpp              # App orchestration entry
│   ├── hal/                   # L0 hardware access
│   ├── bsp/                   # L1 board support
│   ├── middlewares/           # L2 effects, Matter bridge, sequences
│   └── service/               # L3 domain services (button, light, power, …)
├── Srcipt/                    # Host build / flash scripts
├── docker/                    # Docker build environment
├── artifact/                  # Build outputs (.s37 / .gbl)
├── Doc/                       # Design notes and change logs
└── .vscode/                   # VS Code tasks / launch
```

## Software Architecture

```
APP (entry.cpp)
  └── Service     LightDecisionCenter, ButtonService, PowerServer,
                  IndicatorServer, MatterBridgeServer, LowPowerCoordinator
        └── Middlewares   LightEffectEngine, sequences, MatterBridge
              └── BSP     LEDs, power monitor, sleep, UART log
                    └── HAL     GPIO, PWM, IADC, SPI(SM15135E), NVM, EXTI
```

## Prerequisites (Host Build)

- Linux x86_64 recommended
- Silicon Labs tooling via **slt** / Simplicity Studio 6
- Matching packages under `~/.silabs` (same major versions as above)
- `arm-none-eabi-gcc` 12.2, CMake 3.30+, Ninja, Simplicity Commander

Studio-generated CMake currently hard-codes paths like `/home/hadyliu/.silabs/...`. Keep that layout, or use the Docker image which mirrors it.

## Build & Flash

### VS Code

- **Ctrl+Shift+B** → `一键全自动(编译+下载)` (`Srcipt/CompileDownload.sh`)
- Or task **仅编译代码(不烧录)** (`Srcipt/Compile.sh`)

### Command line

```bash
# Configure + build (solution)
./Srcipt/Compile.sh

# Flash app-only image (edit serial number in script if needed)
./Srcipt/Download.sh

# Clear CMake build caches
./Srcipt/ClearCache.sh
```

Typical outputs after successful postbuild:

- `artifact/Li-Bat_MatterLight_EFR32MG24B010F1536IM40-full.s37`
- `artifact/Li-Bat_MatterLight_EFR32MG24B010F1536IM40-app-only.gbl`

### Docker

See [docker/docker.md](./docker/docker.md) (English) or [docker/docker_zh.md](./docker/docker_zh.md) (Chinese).

```bash
./docker/build-image.sh
docker run --rm -v "$PWD":/workspace -w /workspace \
  li-bat-matterlight:sdk-2025.12.2
```

### Reproduce CI locally

```bash
./Srcipt/CiLocal.sh
```

GitHub setup (ZH): [.github/GITHUB_SETUP_zh.md](./.github/GITHUB_SETUP_zh.md)

## Studio Regenerate Notes

If you re-run **Generate** in Simplicity Studio:

- Preserve custom configs such as `config/sl_matter_icd_config.h` and `config/OpenThreadConfig.h`
- Keep `User/` application sources linked into the project
- Re-check CMake path sync if package hashes under `~/.silabs` change

## Documentation

| Document | Description |
|----------|-------------|
| [develop.md](./Srcipt/develop.md) | Dev guide: `.slcp`, pins, scripts / Docker |
| [develop_zh.md](./Srcipt/develop_zh.md) | 开发指南（中文） |
| [.github/QA_PLAN_zh.md](./.github/QA_PLAN_zh.md) | NonFuncReq QA plan |
| [.github/ReleaseNotes_TEMPLATE_zh.md](./.github/ReleaseNotes_TEMPLATE_zh.md) | Release / test report template |
| [.github/GITHUB_SETUP_zh.md](./.github/GITHUB_SETUP_zh.md) | GitHub Actions / GHCR setup (ZH) |
| [.github/CICD_TODO_zh.md](./.github/CICD_TODO_zh.md) | CI/CD todo list (vs NonFuncReq) |
| [.github/NonFuncReq_zh.md](./.github/NonFuncReq_zh.md) | Non-functional / DevOps requirements |
| [Doc/设计方案.md](./Doc/设计方案.md) | Functional requirements |
| [Doc/修改.md](./Doc/修改.md) | Factory-reset lighting / NVM behavior |
| [Doc/工厂复位后配网问题.md](./Doc/工厂复位后配网问题.md) | Post-reset commissioning / SRP notes |
| [docker/docker.md](./docker/docker.md) | Docker image build (EN) |
| [docker/docker_zh.md](./docker/docker_zh.md) | Docker image build (ZH) |
| [MatterLightOverThread/README.md](./MatterLightOverThread/README.md) | Upstream Silabs lighting example notes |

## License / Attribution

Application logic under `User/` is project-specific. Silicon Labs SDK, Matter extension, and generated Studio content remain under their respective Silicon Labs / CSA licenses.
