# Docker 镜像构建说明

用于构建与运行 Li-Bat MatterLight（EFR32MG24）固件编译环境的 Docker 文档。

英文版：[docker.md](./docker.md)

## 概述

| 项目 | 说明 |
|------|------|
| 基础镜像 | `ubuntu:24.04` |
| Dockerfile | `docker/Dockerfile` |
| 入口脚本 | `docker/entrypoint.sh` |
| 构建脚本 | `docker/build-image.sh` |
| 默认镜像标签 | `li-bat-matterlight:sdk-2025.12.2` |
| 容器工作目录 | `/workspace` |
| 构建用户 / HOME | `hadyliu` / `/home/hadyliu` |

Simplicity Studio 导出的 CMake 工程硬编码了 `/home/hadyliu/.silabs/...` 路径。镜像内固定使用同名用户与 HOME，以保证路径可解析。

## 前置条件

- Docker Engine 20+（推荐 Linux x86_64）
- 完整镜像需要能访问 Docker Hub 与 Silicon Labs 下载服务器
- 所有 `docker build` 命令须在**仓库根目录**执行

## 目录文件

```
docker/
├── Dockerfile                # 镜像定义（完整镜像仅靠网络安装）
├── install-slt-packages.sh   # slt 安装 + 硬编码 hash 软链（Dockerfile 调用）
├── entrypoint.sh             # 容器入口：路径对齐 + build/generate
├── build-image.sh            # 一键构建镜像
├── docker.md                 # 英文说明
└── docker_zh.md              # 本文件（中文说明）
```

## 工具链 / SDK 版本（完整镜像内置）

| 软件包 | 版本 |
|--------|------|
| cmake | 3.30.2 |
| ninja | 1.12.1 |
| commander | 1.23.1 |
| gcc-arm-none-eabi | 12.2.rel1 |
| simplicity-sdk | 2025.12.3（与 matter 2.8.1 在线解析一致；软链到工程硬编码 hash） |
| matter_extension | 2.8.1 |
| slc-cli | 6.0.17 |
| java21 | 21.0.5（SLC 运行必需） |
| zap | 2026.02.26（Matter Generate 必需） |
| slt CLI | 1.1.0 |

## 构建镜像

### 方式 A — 完整镜像（SDK 打进镜像，无需挂载宿主 `~/.silabs`）

仅通过网络用 `slt` 下载工具链 / SiSDK / Matter。**不依赖**宿主机 Simplicity Studio。
首次构建约 20–40 分钟（视网络而定），镜像约 9GB。

```bash
# 在仓库根目录执行
./docker/build-image.sh

# 等价命令
docker build -f docker/Dockerfile -t li-bat-matterlight:sdk-2025.12.2 .
```

> 标签名保留 `sdk-2025.12.2`（与 Studio 工程硬编码 hash 对齐）；镜像内实际安装的是
> `simplicity-sdk/2025.12.3` + `matter_extension/2.8.1`，并对硬编码目录做软链。

### 方式 B — 精简镜像（挂载宿主机 `~/.silabs`）

镜像内不安装 SDK/工具链包。适合宿主机已安装完整 Simplicity Studio / `slt` 的场景。

```bash
INSTALL_SLT_PACKAGES=0 IMAGE_TAG=li-bat-matterlight:slim ./docker/build-image.sh

# 等价命令
docker build -f docker/Dockerfile \
  --build-arg INSTALL_SLT_PACKAGES=0 \
  -t li-bat-matterlight:slim .
```

### 常用构建参数

| 构建参数 | 默认值 | 说明 |
|----------|--------|------|
| `INSTALL_SLT_PACKAGES` | `1` | `1`：经 slt 安装 SDK/工具链；`0`：精简镜像 |
| `BUILD_UID` / `BUILD_GID` | `1000` | 可按需与宿主机 UID/GID 对齐，避免挂载写权限问题 |
| `SIMPLICITY_SDK_VERSION` | `2025.12.3` | SiSDK 版本（须与 Matter 可一起解析） |
| `MATTER_EXTENSION_VERSION` | `2.8.1` | Matter 扩展版本 |
| `CMAKE_VERSION` | `3.30.2` | CMake 版本 |
| `GCC_ARM_VERSION` | `12.2.rel1` | Arm GNU 工具链版本 |

示例：

```bash
docker build -f docker/Dockerfile \
  --build-arg BUILD_UID=$(id -u) \
  --build-arg BUILD_GID=$(id -g) \
  -t li-bat-matterlight:sdk-2025.12.2 .
```

## 运行固件编译

### 完整镜像

```bash
docker run --rm \
  -v "$PWD":/workspace \
  -w /workspace \
  li-bat-matterlight:sdk-2025.12.2
```

### 精简镜像（挂载宿主机 SDK）

```bash
docker run --rm \
  -v "$PWD":/workspace \
  -v "$HOME/.silabs":/home/hadyliu/.silabs:ro \
  -w /workspace \
  li-bat-matterlight:slim
```

默认命令为 `build`（配置 + 编译，流程与 `Srcipt/Compile.sh` 一致）。postbuild 成功后，产物会写回宿主机 `artifact/` 目录。

### 容器命令

| 命令 | 说明 |
|------|------|
| `build` | 配置并编译（默认） |
| `generate [target]` | 用 SLC 重新 Generate 工程；`target` 为 `all` / `app` / `bootloader` |
| `check-slc` | 检查 java / slc / zap-cli 是否可用 |
| `shell` | 进入交互式 bash |
| `help` | 显示入口帮助 |
| `<cmd...>` | 执行任意命令 |

## 如何使用 `generate`（重点）

完整镜像已内置 **slc-cli + java21 + zap**，可用于 SLC CLI 开发（改 `.slcp` → `generate` → `build`）。

重建镜像后先自检：

```bash
./docker/build-image.sh
docker run --rm -v "$PWD":/workspace -w /workspace \
  li-bat-matterlight:sdk-2025.12.2 check-slc
```

`entrypoint.sh` 是容器启动后**第一个执行的脚本**。镜像默认跑 `build`；你在 `docker run` 末尾多写一个参数，就会走对应分支：

```text
docker run ... 镜像名          → 执行 entrypoint → build（编译）
docker run ... 镜像名 generate → 执行 entrypoint → generate（生成工程）
docker run ... 镜像名 shell    → 执行 entrypoint → 打开 bash
```

### 和「改功能」的关系

| 你想做的事 | 用哪个命令 |
|------------|------------|
| 只改 `User/` 业务代码后编译 | `build`（或不写命令） |
| 改了 `.slcp` / 加了 SDK 组件，要刷新工程 | 先 `generate`，再 `build` |
| 临时进容器手动敲 `slc` / `cmake` | `shell` |

### 具体命令（在仓库根目录执行）

```bash
# A. 刷新整个解决方案（Matter 应用 + Bootloader）
docker run --rm \
  -v "$PWD":/workspace \
  -w /workspace \
  li-bat-matterlight:sdk-2025.12.2 \
  generate

# B. 只刷新 Matter 应用
docker run --rm \
  -v "$PWD":/workspace \
  -w /workspace \
  li-bat-matterlight:sdk-2025.12.2 \
  generate app

# C. 只刷新 Bootloader
docker run --rm \
  -v "$PWD":/workspace \
  -w /workspace \
  li-bat-matterlight:sdk-2025.12.2 \
  generate bootloader

# D. Generate 完成后再编译
docker run --rm \
  -v "$PWD":/workspace \
  -w /workspace \
  li-bat-matterlight:sdk-2025.12.2 \
  build
```

生成结果直接写在宿主机挂载目录里（例如 `MatterLightOverThread/cmake_gcc/`、`autogen/`），不是留在容器里。

### 可选环境变量

```bash
docker run --rm \
  -e SLC_PART=efr32mg24b010f1536im40 \
  -e GENERATOR_TIMEOUT=800 \
  -v "$PWD":/workspace \
  -w /workspace \
  li-bat-matterlight:sdk-2025.12.2 \
  generate app
```

### 更新了 `docker/entrypoint.sh` 之后

`entrypoint.sh` 在**构建镜像时**被拷进镜像。改脚本后任选其一：

```bash
# 方式 1：重新构建镜像（推荐）
./docker/build-image.sh

# 方式 2：不重建，临时用仓库里的脚本作入口
docker run --rm \
  -v "$PWD":/workspace \
  -w /workspace \
  --entrypoint /workspace/docker/entrypoint.sh \
  li-bat-matterlight:sdk-2025.12.2 \
  generate
```

### 典型开发循环

```text
1. 宿主机改 User/ 或 .slcp
2. 若改了 .slcp / 组件 → docker run ... generate
3. docker run ... build
4. 宿主机 ./Srcipt/Download.sh 烧录
```

示例：

```bash
# 进入交互终端
docker run --rm -it \
  -v "$PWD":/workspace \
  -w /workspace \
  li-bat-matterlight:sdk-2025.12.2 shell

# 检查工具链
docker run --rm \
  li-bat-matterlight:sdk-2025.12.2 \
  bash -lc 'slt where cmake; arm-none-eabi-gcc --version'
```

## 入口脚本工作流程

**`build`：**

1. 检查 `/home/hadyliu/.silabs/slt/installs`
2. 若 Conan 包目录 hash 与 Studio 导出名称不一致，则软链到期望 hash
3. 执行 CMake `--preset project`，再执行 `--build --preset default_config`

**`generate`：**

1. 同样先做 SDK 路径对齐
2. 调用 `slc generate` 处理 `MatterLightOverThread.slcp` / `Matter-Bootloader.slcp`
3. 提示接下来执行 `build`

## 注意事项与限制

- 目标架构为 **linux/amd64**。Arm 主机需 amd64 模拟，或另行适配工具链。
- 完整镜像构建需要能访问 Silicon Labs 软件包服务器。
- 默认 `build` 命令不包含烧录；产物生成后请在宿主机使用 `Srcipt/Download.sh` / Commander 烧录。
- 不要随意更改容器 HOME（须保持 `/home/hadyliu`），除非同步修改工程中 Studio 生成的硬编码路径。
- `generate` 可能覆盖部分自动生成文件；请保留 `User/`，以及 `config/sl_matter_icd_config.h`、`config/OpenThreadConfig.h` 等自定义配置。

## 常见问题

| 现象 | 可能原因 | 处理建议 |
|------|----------|----------|
| 无法拉取 `ubuntu:24.04` | Docker Hub / 网络异常 | 配置镜像源或稍后重试 |
| `~/.silabs/slt/installs not found` | 精简镜像未挂载 SDK | 挂载 `$HOME/.silabs`，或以 `INSTALL_SLT_PACKAGES=1` 重建完整镜像 |
| CMake 找不到 SDK 源码 | 包 hash / 版本不匹配 | 确认 SiSDK、Matter 版本一致；查看入口脚本软链日志 |
| 编译产物权限不足 | UID 不一致 | 使用 `BUILD_UID=$(id -u)`、`BUILD_GID=$(id -g)` 重建 |
| `slt install` 失败 | 网络或包名/版本问题 | 核对 Dockerfile 中版本，并检查 Silicon Labs 网络连通性 |
| `generate` 提示未知命令 | 镜像仍是旧 entrypoint | 重建镜像，或用 `--entrypoint /workspace/docker/entrypoint.sh` |
| `slc not found` | 精简镜像未装 slc-cli | 使用完整镜像，或挂载含 slc 的宿主 `~/.silabs` |
| `java not found` / SLC 起不来 | 旧镜像未装 java21 | 用新 Dockerfile 重建完整镜像 |
| Matter `generate` 缺 ZAP | 旧镜像未装 zap | 重建完整镜像（含 `zap/2026.02.26`） |
