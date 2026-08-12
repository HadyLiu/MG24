# 开发指南

说明如何修改 `.slcp` 组件、调整引脚/外设，以及使用宿主机脚本与 Docker 入口命令。

英文版：[develop.md](./develop.md)

相关文档：

- 项目总览：[readme_zh.md](./readme_zh.md)
- Docker 构建与运行：[docker/docker_zh.md](./docker/docker_zh.md)
- 产品需求：[Doc/设计方案.md](./Doc/设计方案.md)

---

## 1. 开发模型（先选对入口）

| 改动类型 | 改哪里 | 要不要 SLC Generate | 下一步 |
|----------|--------|---------------------|--------|
| 产品业务（灯效、按键策略、Matter 桥接） | `User/` | 否 | `build` |
| SDK / Matter 组件 | `.slcp` | **是** | `generate` → `build` |
| Studio 组件引脚（按键、PWM、SPI…） | `MatterLightOverThread/config/*_config.h` | 一般否（新实例才要） | `build` |
| 板级自定义 GPIO / ADC | `User/bsp/inc/BspIoConfig.h` + HAL/BSP | 否 | `build` |

不要把产品业务写进 `autogen/`，或其它会被 Studio/SLC 随意覆盖的文件。

---

## 2. 查看与修改 `.slcp`

### 2.1 关键文件

| 文件 | 作用 |
|------|------|
| `MatterLightOverThread/MatterLightOverThread.slcp` | Matter 应用组件清单 |
| `Matter-Bootloader/Matter-Bootloader.slcp` | Bootloader 组件 |
| `Li-Bat_MatterLight_EFR32MG24B010F1536IM40.slcw` | 解决方案（应用 + Bootloader） |

### 2.2 `.slcp` 重要字段

| 字段 | 含义 |
|------|------|
| `sdk` / `sdk_extension` | SiSDK + Matter 扩展版本 |
| `component:` | 已选组件（`id:`） |
| `requires:` | 额外强制依赖 |
| `configuration:` | 组件相关宏 |
| `instance:` | 实例名（如 `btn0`、`rgb_data`） |

列出当前已选组件：

```bash
grep -E '^\s+id:' MatterLightOverThread/MatterLightOverThread.slcp
```

### 2.3 如何知道 SDK 里有哪些组件

**Studio（最直观）：** 打开工程 → Software Components → 搜索/安装。

**在磁盘搜 `.slcc`（Matter 扩展 / SiSDK）：**

```bash
# Matter 组件 id
grep -R --include='*.slcc' -n '^id: matter_' \
  "$HOME/.silabs/slt/installs/conan/p/mattef07831835e490/p/slc/component" | head

# 按关键字搜
find "$HOME/.silabs/slt/installs/conan/p/mattef07831835e490/p" \
  -name '*.slcc' | xargs grep -l 'low_power' | head
```

打开对应 `.slcc` 可看 `requires` / `provides` / 源文件。

Docker 内（完整镜像或已挂载 `~/.silabs`）：

```bash
docker run --rm -it \
  -v "$PWD":/workspace \
  -w /workspace \
  --entrypoint /workspace/docker/entrypoint.sh \
  li-bat-matterlight:sdk-2025.12.2 shell

# 容器内
find "$(slt where matter_extension)" -name '*.slcc' | head
```

### 2.4 向 `.slcp` 添加组件

在 `component:` 下增加一项，例如：

```yaml
- package: matter
  vendor: silabs
  id: matter_platform_low_power
```

带实例的 SiSDK 驱动（本工程已有同类写法）：

```yaml
- package: simplicity_sdk
  vendor: silabs
  id: simple_button
  instance:
  - btn0
  - btn_rst0
```

然后执行 Generate + 编译（见第 5 节）。

### 2.5 Generate 后务必保护的内容

若被覆盖，请恢复/保留：

- 整个 `User/` 及工程中的引用关系
- `MatterLightOverThread/config/sl_matter_icd_config.h`
- `MatterLightOverThread/config/OpenThreadConfig.h`
- 若 Generate 重置了引脚，核对 `config/sl_*_config.h` 中的自定义脚位

---

## 3. 引脚与外设功能

本产品有**两层**引脚定义。

### 3.1 Studio / SLC 组件引脚

配置在 `MatterLightOverThread/config/`（部分也会反映到 `pin_config.h`）。

| 功能 | 配置文件 | 当前工程脚位 |
|------|----------|--------------|
| 主按键 `btn0` | `sl_simple_button_btn0_config.h` | PA5 |
| 复位/系统键 `btn_rst0` | `sl_simple_button_btn_rst0_config.h` | PA6 |
| 指示灯 PWM `Indic_led0` | `sl_pwm_init_Indic_led0_config.h` | PA3 / TIMER1 |
| 白光 PWM `w_led0` | `sl_pwm_init_w_led0_config.h` | PD1 / TIMER3 |
| RGB LED 实例 | `sl_simple_rgb_pwm_led_rgb_led0_config.h` | PB（R/G/B） |
| SPI 灯驱 `rgb_data`（SM15135E） | `sl_spidrv_eusart_rgb_data_config.h` | EUSART1（如 TX PD0 …） |

修改方法：

1. 直接改对应 `*_config.h` 的 `_PORT` / `_PIN` / 外设号，或用 Studio Pin Tool 保存。
2. 若 `config/pin_config.h` 也有同一信号，一并核对。
3. 重新 `build`。只有**新增组件实例**时才需要完整 Generate。

原理图 / 引脚图：`Doc/原理图.png`、`Doc/引脚配置.png`。

### 3.2 产品 BSP 自定义引脚

定义在 `User/bsp/inc/BspIoConfig.h`（供 HAL/BSP 使用，不完全等同 Studio 组件）：

| 宏 | 端口/脚 | 作用 |
|----|---------|------|
| `POWER_IN_DETECT` / `USB_IN` | PA8 | USB/电源检测、EXTI 唤醒 |
| `INDIC_R_LED` | PA4 | 红色指示灯 |
| `BAT_VOLTAGE` | PC4 | 电池电压 ADC |
| `BAT_NTC` | PC5 | NTC ADC |
| `BAT_EN` | PC0 | 电池使能 |
| `LAMP_STATUS` | PC1 | 灯状态 |
| `CHARGE_SPEED` | PC2 | 充电速度 |
| `CHARGE_EN` | PC6 | 充电使能 |

改宏后，若别处写死了方向/模式，需同步改对应 BSP/HAL。

### 3.3 改脚检查清单

1. Studio 组件脚与 `BspIoConfig.h` 无冲突  
2. TIMER / EUSART / ADC 通道在 MG24 上对该脚仍合法  
3. 低功耗 / EXTI 唤醒脚若用于休眠，仍正确  
4. 编译后实测：按键、指示灯、SPI 主灯、充电检测  

---

## 4. 宿主机脚本使用

脚本在 `Srcipt/`（目录名按仓库现状保留）。请在**仓库根目录**执行。

| 脚本 | 作用 |
|------|------|
| `Srcipt/Compile.sh` | CMake 配置 + 并行编译（解决方案） |
| `Srcipt/Download.sh` | 用 Commander 烧录 `artifact/...-app-only.gbl` |
| `Srcipt/CompileDownload.sh` | 编译成功后自动烧录 |
| `Srcipt/ClearCache.sh` | 删除各工程 `build/` 缓存 |

示例：

```bash
./Srcipt/Compile.sh
./Srcipt/Download.sh
./Srcipt/CompileDownload.sh
./Srcipt/ClearCache.sh
```

说明：

- 脚本内 CMake 路径指向 `~/.silabs/.../cmake.../bin/cmake`（Studio/slt 安装）。
- `Download.sh` / `CompileDownload.sh` 里写死了 `--serialno`（如 `440350761`），请改成你的调试器序列号。
- 器件型号：`EFR32MG24B010F1536IM40`。
- VS Code：**Ctrl+Shift+B** 默认跑 `CompileDownload.sh`（见 `.vscode/tasks.json`）。

---

## 5. Docker 入口命令使用

详见 [docker/docker_zh.md](./docker/docker_zh.md)。

镜像入口：`docker/entrypoint.sh`。

| 命令 | 含义 |
|------|------|
| `build`（默认） | 对齐 SDK 路径 + CMake 编译 |
| `generate [all\|app\|bootloader]` | 对 `.slcp` 执行 `slc generate` |
| `check-slc` | 检查 java / slc / zap-cli |
| `shell` | 交互 bash |
| `help` | 入口帮助 |

完整镜像已包含 **slc-cli + java21 + zap**，可用于 SLC CLI 开发。修改 Dockerfile 后需 `./docker/build-image.sh` 重建。

### 5.1 编译固件

```bash
docker run --rm \
  -v "$PWD":/workspace \
  -w /workspace \
  li-bat-matterlight:sdk-2025.12.2
```

### 5.2 修改 `.slcp` 后 Generate

```bash
# 若镜像较旧，建议显式用仓库里的 entrypoint
docker run --rm \
  -v "$PWD":/workspace \
  -w /workspace \
  --entrypoint /workspace/docker/entrypoint.sh \
  li-bat-matterlight:sdk-2025.12.2 \
  generate app

docker run --rm \
  -v "$PWD":/workspace \
  -w /workspace \
  --entrypoint /workspace/docker/entrypoint.sh \
  li-bat-matterlight:sdk-2025.12.2 \
  build
```

可选环境变量：`SLC_PART`、`GENERATOR_TIMEOUT`。

### 5.3 重建 Docker 镜像

```bash
./docker/build-image.sh
# 精简镜像（运行时再挂宿主 SDK）：
INSTALL_SLT_PACKAGES=0 IMAGE_TAG=li-bat-matterlight:slim ./docker/build-image.sh
```

---

## 6. 推荐工作流

### A. 只改 `User/` 功能

```text
改 User/ → ./Srcipt/Compile.sh（或 Docker build）→ 烧录
```

### B. 增加/更换 SDK 组件

```text
改 .slcp → generate → 核对受保护配置 → build → 烧录
```

### C. 改引脚

```text
改 config/*_config.h 和/或 BspIoConfig.h → build → 上板验证
```

### D. 干净重编

```bash
./Srcipt/ClearCache.sh
./Srcipt/Compile.sh
```

### E. 本地复现 CI（lint + Docker build）

```bash
./Srcipt/CiLocal.sh
# 或
./Srcipt/CiLocal.sh --build-only
```

GitHub 上启用 Actions / 推 GHCR / 打 Tag 发布：见
[`.github/GITHUB_SETUP_zh.md`](../.github/GITHUB_SETUP_zh.md)。

---

## 7. 新代码放哪里

| 层级 | 路径 | 职责 |
|------|------|------|
| HAL | `User/hal/` | 寄存器、SPI/IADC/GPIO/NVM |
| BSP | `User/bsp/` | 板级引脚、LED、电源监测 |
| Middlewares | `User/middlewares/` | 灯效引擎、Matter 桥接胶水 |
| Service | `User/service/` | 按键/灯光/电源/指示灯策略 |
| Entry | `User/entry.cpp` | 组装服务与回调 |

