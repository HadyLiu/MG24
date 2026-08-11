# Docker Image Build Guide

Build and run the Li-Bat MatterLight (EFR32MG24) firmware compile environment with Docker.

Chinese version: [docker_ZN.md](./docker_ZN.md)

## Overview

| Item | Value |
|------|--------|
| Base image | `ubuntu:24.04` |
| Dockerfile | `docker/Dockerfile` |
| Entry script | `docker/entrypoint.sh` |
| Helper script | `docker/build-image.sh` |
| Default image tag | `li-bat-matterlight:sdk-2025.12.2` |
| Container workdir | `/workspace` |
| Build user / HOME | `hadyliu` / `/home/hadyliu` |

Studio-exported CMake files hard-code paths under `/home/hadyliu/.silabs/...`. The image uses the same user and home directory so those paths resolve correctly.

## Prerequisites

- Docker Engine 20+ (Linux x86_64 recommended)
- Network access to Docker Hub and Silicon Labs download servers (full image)
- Run all `docker build` commands from the **repository root**

## Files

```
docker/
├── Dockerfile         # Image definition
├── entrypoint.sh      # Container entry: path sync + build
├── build-image.sh     # One-shot image build helper
├── docker.md          # This document (English)
└── docker_ZN.md       # Chinese document
```

## Toolchain / SDK Versions (baked into full image)

| Package | Version |
|---------|---------|
| cmake | 3.30.2 |
| ninja | 1.12.1 |
| commander | 1.23.1 |
| gcc-arm-none-eabi | 12.2.rel1 |
| simplicity-sdk | 2025.12.2 |
| matter_extension | 2.8.1 |
| slc-cli | 6.0.17 |
| java21 | 21.0.5 (required by SLC) |
| zap | 2026.02.26 (required for Matter generate) |
| slt CLI | 1.1.0 |

## Build the Image

### Option A — Full image (SDK inside the image)

Downloads Silicon Labs packages via `slt`. First build can take a long time and produce a large image.

```bash
# From repository root
./docker/build-image.sh

# Equivalent
docker build -f docker/Dockerfile -t li-bat-matterlight:sdk-2025.12.2 .
```

### Option B — Slim image (mount host `~/.silabs`)

Skip package install in the image. Use when the host already has a complete Simplicity Studio / `slt` install.

```bash
INSTALL_SLT_PACKAGES=0 IMAGE_TAG=li-bat-matterlight:slim ./docker/build-image.sh

# Equivalent
docker build -f docker/Dockerfile \
  --build-arg INSTALL_SLT_PACKAGES=0 \
  -t li-bat-matterlight:slim .
```

### Useful build arguments

| Build arg | Default | Description |
|-----------|---------|-------------|
| `INSTALL_SLT_PACKAGES` | `1` | `1` = install SDK/toolchain via slt; `0` = slim |
| `BUILD_UID` / `BUILD_GID` | `1000` | Match host UID/GID if needed for volume writes |
| `SIMPLICITY_SDK_VERSION` | `2025.12.2` | SiSDK version |
| `MATTER_EXTENSION_VERSION` | `2.8.1` | Matter extension version |
| `CMAKE_VERSION` | `3.30.2` | CMake version |
| `GCC_ARM_VERSION` | `12.2.rel1` | Arm GNU toolchain version |

Example:

```bash
docker build -f docker/Dockerfile \
  --build-arg BUILD_UID=$(id -u) \
  --build-arg BUILD_GID=$(id -g) \
  -t li-bat-matterlight:sdk-2025.12.2 .
```

## Run Firmware Build

### Full image

```bash
docker run --rm \
  -v "$PWD":/workspace \
  -w /workspace \
  li-bat-matterlight:sdk-2025.12.2
```

### Slim image (mount host SDK)

```bash
docker run --rm \
  -v "$PWD":/workspace \
  -v "$HOME/.silabs":/home/hadyliu/.silabs:ro \
  -w /workspace \
  li-bat-matterlight:slim
```

Default command is `build` (configure + compile, same flow as `Srcipt/Compile.sh`). Artifacts are written back to the host under `artifact/` when post-build succeeds.

### Container commands

| Command | Description |
|---------|-------------|
| `build` | Configure and build (default) |
| `generate [target]` | SLC regenerate project files; `target` = `all` / `app` / `bootloader` |
| `check-slc` | Verify java / slc / zap-cli |
| `shell` | Open interactive bash |
| `help` | Show entrypoint help |
| `<cmd...>` | Run an arbitrary command |

## How to use `generate`

The full image now includes **slc-cli + java21 + zap** for SLC CLI workflows (edit `.slcp` → `generate` → `build`).

After rebuilding the image, smoke-test:

```bash
./docker/build-image.sh
docker run --rm -v "$PWD":/workspace -w /workspace \
  li-bat-matterlight:sdk-2025.12.2 check-slc
```

`entrypoint.sh` is the first process in the container. The image defaults to `build`. Pass an extra argument at the end of `docker run` to select another action:

```text
docker run ... IMAGE           → entrypoint → build
docker run ... IMAGE generate  → entrypoint → generate
docker run ... IMAGE shell     → entrypoint → bash
```

### When to use which command

| Goal | Command |
|------|---------|
| Compile after editing `User/` only | `build` (or omit the command) |
| Refresh project after editing `.slcp` / SDK components | `generate`, then `build` |
| Manual `slc` / `cmake` inside the container | `shell` |

### Concrete commands (from repository root)

```bash
# A. Regenerate Matter app + Bootloader
docker run --rm \
  -v "$PWD":/workspace \
  -w /workspace \
  li-bat-matterlight:sdk-2025.12.2 \
  generate

# B. Matter app only
docker run --rm \
  -v "$PWD":/workspace \
  -w /workspace \
  li-bat-matterlight:sdk-2025.12.2 \
  generate app

# C. Bootloader only
docker run --rm \
  -v "$PWD":/workspace \
  -w /workspace \
  li-bat-matterlight:sdk-2025.12.2 \
  generate bootloader

# D. Compile after generate / after code changes
docker run --rm \
  -v "$PWD":/workspace \
  -w /workspace \
  li-bat-matterlight:sdk-2025.12.2 \
  build
```

Generated files land in the bind-mounted host tree (e.g. `MatterLightOverThread/cmake_gcc/`, `autogen/`).

### After editing `docker/entrypoint.sh`

The script is copied into the image at build time. After changing it:

```bash
# Rebuild image (recommended)
./docker/build-image.sh

# Or use the repo script without rebuilding
docker run --rm \
  -v "$PWD":/workspace \
  -w /workspace \
  --entrypoint /workspace/docker/entrypoint.sh \
  li-bat-matterlight:sdk-2025.12.2 \
  generate
```

Examples:

```bash
# Interactive shell
docker run --rm -it \
  -v "$PWD":/workspace \
  -w /workspace \
  li-bat-matterlight:sdk-2025.12.2 shell

# Check toolchain
docker run --rm \
  li-bat-matterlight:sdk-2025.12.2 \
  bash -lc 'slt where cmake; arm-none-eabi-gcc --version'
```

## How the Entry Script Works

**`build`:** sync SDK paths → CMake configure → CMake build.

**`generate`:** sync SDK paths → `slc generate` on app/bootloader `.slcp` → remind you to run `build`.

## Notes and Limitations

- Target architecture is **linux/amd64**. Arm hosts need an amd64 emulator or a rebuilt toolchain set.
- Full image build requires outbound access to Silicon Labs package servers.
- Flashing is not part of `build`; use host `Srcipt/Download.sh` / Commander.
- Keep container HOME as `/home/hadyliu` unless you rewrite Studio hard-coded paths.
- `generate` may overwrite auto-generated files; keep `User/` and custom ICD/OpenThread configs.

## Troubleshooting

| Symptom | Likely cause | Action |
|---------|--------------|--------|
| Cannot pull `ubuntu:24.04` | Docker Hub / network | Configure mirror or retry later |
| `~/.silabs/slt/installs not found` | Slim image without mount | Mount `$HOME/.silabs`, or rebuild with `INSTALL_SLT_PACKAGES=1` |
| CMake cannot find SDK sources | Hash / package mismatch | Ensure matching SiSDK + Matter versions; check entrypoint symlink logs |
| Permission denied on build outputs | UID mismatch | Rebuild with `BUILD_UID=$(id -u)` and `BUILD_GID=$(id -g)` |
| `slt install` fails | Network or package name/version | Verify versions in Dockerfile; check Silicon Labs connectivity |
| `generate` unknown command | Old entrypoint in image | Rebuild image, or `--entrypoint /workspace/docker/entrypoint.sh` |
| `slc not found` | Slim image without slc-cli | Use full image, or mount host `~/.silabs` that includes slc |
| `java not found` / SLC fails | Old image without java21 | Rebuild full image with the updated Dockerfile |
| Matter `generate` missing ZAP | Old image without zap | Rebuild full image (includes `zap/2026.02.26`) |
