# GitHub 操作说明（CI/CD）

仓库：`https://github.com/barryjim/KAJEN_T`  
对应文档：[`CICD_TODO_zh.md`](./CICD_TODO_zh.md)、[`QA_PLAN_zh.md`](./QA_PLAN_zh.md)、[`docker/docker_zh.md`](../docker/docker_zh.md)、[CI/CD 总结](../Doc/CICD_总结.md)

## 0. 总览

| Workflow | 作用 | 何时跑 |
|----------|------|--------|
| **Lint** | shellcheck + yamllint + actionlint | **每次** push/PR（`main`/`develop`） |
| **QA** | host 单测 + cppcheck | **每次** push/PR |
| **CI Firmware** | 编固件 → Artifact | **每次** push/PR |
| **Release Firmware** | 对 Tag → 编固件 → `pack` 带版本产物 + cppcheck → GitHub Release | **仅 `v*` Tag** |
| **Docker Publish** | Dockerfile → GHCR | **仅手动**（镜像太大） |

```text
push / PR → Lint + QA + CI Firmware
tag v*    → Release Firmware（+ 上面若同推也会跑）
手动      → Docker Publish（首次 / 改 docker 时）
```

---

## 1. 推送代码（启用 Actions）

1. 远端：`origin` → `https://github.com/barryjim/KAJEN_T.git`
2. 推送含 `.github/workflows/` 的改动
3. **Settings → Actions → General**
   - Allow all actions
   - Workflow permissions：**Read and write**

---

## 2. 首次：GitHub 上编 Docker 镜像（不必本机 push）

1. **Actions → Docker Publish → Run workflow**
2. 等 30～90+ 分钟
3. Packages 中确认 `li-bat-matterlight:sdk-2025.12.2`

之后每次提交会自动跑 **CI Firmware**（依赖此镜像）。

---

## 3. 日常提交

`git push` 到 `main`/`develop` 或开 PR → 自动跑 **Lint / QA / CI Firmware**。

`Permission denied` 建 `build/`：确认含「Allow container user to write workspace」。

---

## 4. 发正式版

发版前按 [`ReleaseNotes_TEMPLATE_zh.md`](./ReleaseNotes_TEMPLATE_zh.md) 填测试报告。

**Tag 必须等于** `MatterLightOverThread/config/sl_matter_config.h` 里的  
`CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING`（当前 `"1.1.7"` → Tag `v1.1.7`）。  
数字宏 `CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION`（当前 `17`）一并递增。  
本地可先校验：

```bash
eval "$(./Srcipt/ReadFirmwareVersion.sh)"
./Srcipt/CheckReleaseTag.sh "v${FW_VERSION_STRING}"
git tag -a "v${FW_VERSION_STRING}" -m "firmware v${FW_VERSION_STRING}"
git push origin "v${FW_VERSION_STRING}"
```

触发 **Release Firmware**：先对 Tag，再以 **release** 变体编固件（关调试日志），再 `pack` 出 `dist/`。

CI Firmware 用 **dev** 变体（可留 UART 日志）。本地：

```bash
LI_BAT_BUILD_VARIANT=release ./Srcipt/CiLocal.sh --build-only
```

---

## 5. Branch protection（建议）

**Settings → Branches → Add rule**（`main`）：

- Require a pull request before merging
- Require status checks：勾选 **Lint** / **QA** / **CI Firmware**（跑通一次后出现在列表）
- Require approvals：至少 1 人

---

## 6. 本地复现

```bash
./Srcipt/CiLocal.sh
./Srcipt/CiLocal.sh --lint-only
./Srcipt/CiLocal.sh --qa-only
./Srcipt/CiLocal.sh --build-only
```

---

## 7. 阶段 C（IKEA OTA）

等宜家 API 后再配 Secrets。禁止把 Token 写进仓库。
