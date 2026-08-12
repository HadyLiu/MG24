# Li-Bat MatterLight（EFR32MG24）

面向 **EFR32MG24B010F1536IM40** 的锂电池 Matter / Thread 智能灯固件，基于 Silicon Labs Simplicity SDK 与 Matter 扩展。

English version: [readme.md](./readme.md)

## 项目简介

| 项目 | 说明 |
|------|------|
| 主控 | Silicon Labs EFR32MG24B010F1536IM40 |
| 协议 | Matter over Thread（BLE 配网） |
| SDK | Simplicity SDK **2025.12.2** / Matter 扩展 **2.8.1** |
| 产品重点 | WRGB 主灯、指示灯、电池供电、低功耗 |
| 应用代码 | `User/` 分层 C++（HAL → BSP → Middlewares → Service） |

本仓库为 Simplicity Studio 解决方案工作区：包含 Matter 应用（`MatterLightOverThread`）、Bootloader（`Matter-Bootloader`）、解决方案级 CMake，以及产品业务逻辑目录 `User/`。

## 主要功能

- **灯光控制**：短按亮度循环（100% → 35% → 关灯）、双击色库切换、渐变时序
- **Matter**：开关 / 亮度 / 色彩（HSV、色温、XY）、Identify、配网窗
- **工厂复位**：约 8s 起预警灯效，结束后写 NVM 并工厂复位；开机快闪 + 淡入
- **指示灯**：红 / 白系统灯，用于配网、电量与状态提示
- **电源管理**：电池监测、低电 / 临界电量策略、充电相关逻辑
- **低功耗**：经 `LowPowerCoordinator` 协调唤醒保持（ICD 相关）
- **灯驱**：SM15135E（SPI）WRGB 通道控制

完整产品需求见：[Doc/设计方案.md](./Doc/设计方案.md)

## 目录结构

```
.
├── MatterLightOverThread/     # Matter 照明应用（Studio / SLC 生成 + 配置）
├── Matter-Bootloader/         # Bootloader 工程
├── Li-Bat_*_cmake/            # 解决方案 CMake（应用 + Bootloader + postbuild）
├── User/                      # 产品固件（Studio 重新 Generate 时务必保留）
│   ├── entry.cpp              # 应用编排入口
│   ├── hal/                   # L0 硬件访问
│   ├── bsp/                   # L1 板级支持
│   ├── middlewares/           # L2 灯效、Matter 桥接、时序调度
│   └── service/               # L3 领域服务（按键、灯光、电源等）
├── Srcipt/                    # 宿主机编译 / 烧录脚本
├── docker/                    # Docker 编译环境
├── artifact/                  # 编译产物（.s37 / .gbl）
├── Doc/                       # 设计与变更说明
└── .vscode/                   # VS Code 任务与调试配置
```

## 软件架构

```
APP（entry.cpp）
  └── Service     LightDecisionCenter、ButtonService、PowerServer、
                  IndicatorServer、MatterBridgeServer、LowPowerCoordinator
        └── Middlewares   LightEffectEngine、时序调度、MatterBridge
              └── BSP     LED、电源监测、休眠、UART 日志
                    └── HAL     GPIO、PWM、IADC、SPI(SM15135E)、NVM、EXTI
```


## 宿主机编译环境

- 推荐 Linux x86_64
- 通过 **slt** / Simplicity Studio 6 安装工具链与 SDK
- `~/.silabs` 中需有与上表一致的主要版本包
- `arm-none-eabi-gcc` 12.2、CMake 3.30+、Ninja、Simplicity Commander

Studio 导出的 CMake 当前硬编码了 `/home/hadyliu/.silabs/...` 路径。请保持该布局，或使用已对齐路径的 Docker 镜像。

## 编译与烧录

### VS Code

- **Ctrl+Shift+B** → `一键全自动(编译+下载)`（`Srcipt/CompileDownload.sh`）
- 或任务 **仅编译代码(不烧录)**（`Srcipt/Compile.sh`）

### 命令行

```bash
# 配置 + 编译（解决方案）
./Srcipt/Compile.sh

# 烧录 app-only 镜像（按需修改脚本中的串口号）
./Srcipt/Download.sh

# 清理 CMake 编译缓存
./Srcipt/ClearCache.sh
```

postbuild 成功后常见产物：

- `artifact/Li-Bat_MatterLight_EFR32MG24B010F1536IM40-full.s37`
- `artifact/Li-Bat_MatterLight_EFR32MG24B010F1536IM40-app-only.gbl`

### Docker

详见 [docker/docker_zh.md](./docker/docker_zh.md)（中文）或 [docker/docker.md](./docker/docker.md)（英文）。

```bash
./docker/build-image.sh
docker run --rm -v "$PWD":/workspace -w /workspace \
  li-bat-matterlight:sdk-2025.12.2
```

### CI 本地复现

与 GitHub Actions 同路径（lint + Docker 编译）：

```bash
./Srcipt/CiLocal.sh
```

GitHub 配置与操作步骤见：[.github/GITHUB_SETUP_zh.md](./.github/GITHUB_SETUP_zh.md)  
CI/CD 待办见：[.github/CICD_TODO_zh.md](./.github/CICD_TODO_zh.md)

## Studio 重新 Generate 注意

在 Simplicity Studio 中重新 **Generate** 时：

- 保留自定义配置，如 `config/sl_matter_icd_config.h`、`config/OpenThreadConfig.h`
- 确保 `User/` 应用源码仍正确加入工程
- 若 `~/.silabs` 包目录 hash 变化，需重新核对 CMake 路径

## 文档索引

| 文档 | 说明 |
|------|------|
| [develop_zh.md](./Srcipt/develop_zh.md) | 开发指南：`.slcp`、引脚、脚本 / Docker |
| [develop.md](./Srcipt/develop.md) | Development guide (English) |
| [.github/GITHUB_SETUP_zh.md](./.github/GITHUB_SETUP_zh.md) | GitHub Actions / GHCR 操作步骤 |
| [.github/CICD_TODO_zh.md](./.github/CICD_TODO_zh.md) | CI/CD 待办（对照 NonFuncReq） |
| [.github/NonFuncReq_zh.md](./.github/NonFuncReq_zh.md) | 非功能 / DevOps 规范 |
| [Doc/设计方案.md](./Doc/设计方案.md) | 功能需求规格 |
| [Doc/修改.md](./Doc/修改.md) | 工厂复位灯效 / NVM 行为 |
| [Doc/工厂复位后配网问题.md](./Doc/工厂复位后配网问题.md) | 复位后配网 / SRP 说明 |
| [docker/docker_zh.md](./docker/docker_zh.md) | Docker 镜像构建（中文） |
| [docker/docker.md](./docker/docker.md) | Docker 镜像构建（英文） |
| [MatterLightOverThread/README.md](./MatterLightOverThread/README.md) | 上游 Silabs Lighting 示例说明 |

## 许可说明

`User/` 下产品业务逻辑为本项目自有代码。Silicon Labs SDK、Matter 扩展及 Studio 生成内容遵循其各自许可（Silicon Labs / CSA 等）。
