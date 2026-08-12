# Development Guide

How to modify `.slcp` components, change pin mux / GPIO usage, and run host / Docker scripts for this project.

Chinese version: [develop_zh.md](./develop_zh.md)

Related docs:

- Product overview: [readme.md](./readme.md)
- Docker build/run: [docker/docker.md](./docker/docker.md)
- Requirements: [Doc/设计方案.md](./Doc/设计方案.md)

---

## 1. Development Model

| Change type | Where | Need SLC Generate? | Typical next step |
|-------------|--------|--------------------|-------------------|
| Product logic (light, button policy, Matter bridge) | `User/` | No | `build` |
| SDK / Matter components | `.slcp` | **Yes** | `generate` → `build` |
| Pins for Studio components (button, PWM, SPI…) | `MatterLightOverThread/config/*_config.h` | Usually no (unless new instance) | `build` |
| Board-level custom GPIO / ADC | `User/bsp/inc/BspIoConfig.h` + HAL/BSP | No | `build` |

Do **not** put product business logic into `autogen/` or files that Studio/SLC freely overwrite.

---

## 2. Inspect and Modify `.slcp`

### 2.1 Key files

| File | Role |
|------|------|
| `MatterLightOverThread/MatterLightOverThread.slcp` | Matter app component list |
| `Matter-Bootloader/Matter-Bootloader.slcp` | Bootloader components |
| `Li-Bat_MatterLight_EFR32MG24B010F1536IM40.slcw` | Solution workspace (app + bootloader) |

### 2.2 Important `.slcp` sections

| Section | Meaning |
|---------|---------|
| `sdk` / `sdk_extension` | SiSDK + Matter extension versions |
| `component:` | Selected components (`id:`) |
| `requires:` | Extra required features/groups |
| `configuration:` | Component-related macros |
| `instance:` | Named instances (e.g. `btn0`, `rgb_data`) |

List currently selected component IDs:

```bash
grep -E '^\s+id:' MatterLightOverThread/MatterLightOverThread.slcp
```

### 2.3 Discover available SDK components

**Studio (recommended UI):** open project → Software Components → search/install.

**On disk (Matter extension / SiSDK `.slcc`):**

```bash
# Matter components
grep -R --include='*.slcc' -n '^id: matter_' \
  "$HOME/.silabs/slt/installs/conan/p/mattef07831835e490/p/slc/component" | head

# Search by keyword
find "$HOME/.silabs/slt/installs/conan/p/mattef07831835e490/p" \
  -name '*.slcc' | xargs grep -l 'low_power' | head
```

Open a matching `.slcc` to see `requires` / `provides` / sources.

In Docker (full image or mounted `~/.silabs`):

```bash
docker run --rm -it \
  -v "$PWD":/workspace \
  -w /workspace \
  --entrypoint /workspace/docker/entrypoint.sh \
  li-bat-matterlight:sdk-2025.12.2 shell

# inside container
find "$(slt where matter_extension)" -name '*.slcc' | head
```

### 2.4 Add a component to `.slcp`

Example: add a Matter package component under `component:`:

```yaml
- package: matter
  vendor: silabs
  id: matter_platform_low_power
```

For SiSDK drivers with instances (pattern already used in this project):

```yaml
- package: simplicity_sdk
  vendor: silabs
  id: simple_button
  instance:
  - btn0
  - btn_rst0
```

Then regenerate and build (see §5).

### 2.5 After Generate — files to protect

Keep / restore if overwritten:

- Entire `User/` tree and its CMake linkage
- `MatterLightOverThread/config/sl_matter_icd_config.h`
- `MatterLightOverThread/config/OpenThreadConfig.h`
- Custom pin values in `config/sl_*_config.h` if Generate resets them

---

## 3. Pin and Peripheral Configuration

There are **two** pin layers in this product.

### 3.1 Studio / SLC component pins

Configured in `MatterLightOverThread/config/` (and mirrored in `pin_config.h` for some peripherals).

| Function | Config file | Current pins (project) |
|----------|-------------|-------------------------|
| Main button `btn0` | `sl_simple_button_btn0_config.h` | PA5 |
| Reset / system button `btn_rst0` | `sl_simple_button_btn_rst0_config.h` | PA6 |
| Indicator PWM `Indic_led0` | `sl_pwm_init_Indic_led0_config.h` | PA3 / TIMER1 |
| White PWM `w_led0` | `sl_pwm_init_w_led0_config.h` | PD1 / TIMER3 |
| RGB LED instance | `sl_simple_rgb_pwm_led_rgb_led0_config.h` | PB (R/G/B) |
| SPI to SM15135E `rgb_data` | `sl_spidrv_eusart_rgb_data_config.h` | EUSART1 (e.g. TX PD0 …) |

How to change:

1. Edit the corresponding `*_config.h` (`_PORT` / `_PIN` / peripheral), **or** use Studio Pin Tool and save.
2. Cross-check `config/pin_config.h` if it also defines the same signal.
3. Rebuild (`build`). Full Generate is only needed when adding a **new** component instance.

Schematic reference images: `Doc/原理图.png`, `Doc/引脚配置.png`.

### 3.2 Product BSP pins (custom board logic)

Defined in `User/bsp/inc/BspIoConfig.h` (used by HAL/BSP, not only Studio components):

| Macro | Port/Pin | Role |
|-------|----------|------|
| `POWER_IN_DETECT` / `USB_IN` | PA8 | USB/power detect, EXTI wake |
| `INDIC_R_LED` | PA4 | Red indicator |
| `BAT_VOLTAGE` | PC4 | Battery ADC |
| `BAT_NTC` | PC5 | NTC ADC |
| `BAT_EN` | PC0 | Battery enable |
| `LAMP_STATUS` | PC1 | Lamp status |
| `CHARGE_SPEED` | PC2 | Charge speed |
| `CHARGE_EN` | PC6 | Charge enable |

Change these macros, then update the matching BSP/HAL usage if direction/mode logic is hard-coded elsewhere.

### 3.3 Checklist when remapping pins

1. No conflict between Studio component pins and `BspIoConfig.h`
2. Peripheral instance (TIMER / EUSART / ADC channel) still valid for that pin on MG24
3. Low-power / EXTI wake pins still correct if used for sleep
4. Rebuild and smoke-test button, LED, SPI light, charging detect

---

## 4. Script Usage (Host)

All scripts live under `Srcipt/` (typo kept as in repo). Run from **repository root**.

| Script | Purpose |
|--------|---------|
| `Srcipt/Compile.sh` | CMake configure + parallel build (solution) |
| `Srcipt/Download.sh` | Flash `artifact/...-app-only.gbl` via Commander |
| `Srcipt/CompileDownload.sh` | Compile, then flash if success |
| `Srcipt/ClearCache.sh` | Delete CMake `build/` dirs |

Examples:

```bash
./Srcipt/Compile.sh
./Srcipt/Download.sh
./Srcipt/CompileDownload.sh
./Srcipt/ClearCache.sh
```

Notes:

- CMake path inside scripts points at `~/.silabs/.../cmake.../bin/cmake` (Studio/slt install).
- `Download.sh` / `CompileDownload.sh` hard-code `--serialno` (e.g. `440350761`). Change it to your adapter serial.
- Device part: `EFR32MG24B010F1536IM40`.
- VS Code: **Ctrl+Shift+B** runs `CompileDownload.sh` by default (see `.vscode/tasks.json`).

---

## 5. Docker Script / Entrypoint Usage

See also [docker/docker.md](./docker/docker.md).

Image entrypoint: `docker/entrypoint.sh`.

| Command | Meaning |
|---------|---------|
| `build` (default) | Align SDK paths + CMake build |
| `generate [all\|app\|bootloader]` | `slc generate` on `.slcp` |
| `check-slc` | Verify java / slc / zap-cli |
| `shell` | Interactive bash |
| `help` | Entrypoint help |

The full image includes **slc-cli + java21 + zap** for SLC CLI work. Rebuild with `./docker/build-image.sh` after Dockerfile changes.

### 5.1 Build firmware

```bash
docker run --rm \
  -v "$PWD":/workspace \
  -w /workspace \
  li-bat-matterlight:sdk-2025.12.2
```

### 5.2 Generate after `.slcp` change

```bash
# Prefer host entrypoint if image was built before generate support
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

Optional env: `SLC_PART`, `GENERATOR_TIMEOUT`.

### 5.3 Rebuild Docker image

```bash
./docker/build-image.sh
# slim (mount host SDK later):
INSTALL_SLT_PACKAGES=0 IMAGE_TAG=li-bat-matterlight:slim ./docker/build-image.sh
```

---

## 6. Recommended Workflows

### A. Feature in `User/` only

```text
Edit User/ → ./Srcipt/Compile.sh  (or Docker build) → flash
```

### B. Add/change SDK component

```text
Edit .slcp → generate → verify protected configs → build → flash
```

### C. Change pin mux

```text
Edit config/*_config.h and/or BspIoConfig.h → build → hardware test
```

### D. Clean rebuild

```bash
./Srcipt/ClearCache.sh
./Srcipt/Compile.sh
```

### E. Reproduce CI locally

```bash
./Srcipt/CiLocal.sh
```

GitHub Actions / GHCR setup (Chinese):
[`.github/GITHUB_SETUP_zh.md`](../.github/GITHUB_SETUP_zh.md).

---

## 7. Where to Put New Code

| Layer | Path | Responsibility |
|-------|------|----------------|
| HAL | `User/hal/` | Registers, SPI/IADC/GPIO/NVM |
| BSP | `User/bsp/` | Board pins, LEDs, power monitor |
| Middlewares | `User/middlewares/` | Light engine, Matter bridge glue |
| Service | `User/service/` | Button/light/power/indicator policy |
| Entry | `User/entry.cpp` | Wire services / callbacks |

