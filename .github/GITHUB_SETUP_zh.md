# GitHub 操作说明（CI/CD）

仓库：`https://github.com/barryjim/KAJEN_T`  
对应文档：[`CICD_TODO_zh.md`](./CICD_TODO_zh.md)、[`QA_PLAN_zh.md`](./QA_PLAN_zh.md)、[`docker/docker_zh.md`](../docker/docker_zh.md)、[CI/CD 总结](../Doc/CICD_总结.md)

## 0. 总览

对齐豪庭 MG301：一条主任务（入口 + 四段模板），Docker 镜像单独、仅手动。

| Workflow | 作用 | 何时跑 |
|----------|------|--------|
| **CI** | `ci.yml` 入口，调用 `ci-template.yml`：CodeAnalysis → UnitTest → Build → Release | push/PR（`main`/`develop`）、`v*` Tag、手动 |
| **Docker Publish** | Dockerfile → GHCR | **仅手动**（镜像太大） |

```text
push / PR / 手动
        │
        ▼
  CI（ci.yml 入口）──调用──▶ ci-template.yml
        │
        │   CodeAnalysis → UnitTest → Build → Release（仅 v* Tag）
        │   • shellcheck / yamllint / actionlint
        │   • clang-format / cpplint / cppcheck
        │   • host 单测 + gcovr
        │   • docker pull GHCR → 编固件（dev；Tag 为 release）
        │   • Tag：pack + GitHub Release
        │
手动    → Docker Publish（首次 / 改 docker/ 时）
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

之后每次提交会自动跑 **CI** 的 Build（依赖此镜像）。

---

## 3. 日常提交

`git push` 到 `main`/`develop` 或开 PR → 自动跑 **CI**（CodeAnalysis → UnitTest → Build）。  
`v*` Tag 同一条流水线末尾多跑 **Release**。

`Permission denied` 建 `build/`：确认含「Allow container user to write workspace」。

---

## 4. 发正式版

发版前按 [`ReleaseNotes_TEMPLATE_zh.md`](./ReleaseNotes_TEMPLATE_zh.md) 填测试报告。

### 4.1 升版本与 Tag

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

触发 **CI → Release**：先对 Tag，再以 **release** 变体编固件（关调试日志），再 `pack` 出 `dist/`。  
push/PR 的 Build 用 **dev** 变体（可留 UART 日志）。本地：

```bash
LI_BAT_BUILD_VARIANT=release ./Srcipt/CiLocal.sh --build-only
```

`pack` 由 `Srcipt/PackRelease.sh` 完成（容器命令 `docker run ... IMAGE pack`）。

### 4.2 宜家 OTA `config.json`

`pack` 会在 `dist/config.json` 写出宜家 HomeSmart OTA 元数据，**不要手改产物里的 JSON**，改下面这些源：

| JSON 字段 | 含义 | 设置位置 |
|-----------|------|----------|
| `productId` | 产品 PID | `MatterLightOverThread/include/CHIPProjectConfig.h` 的 `CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID` |
| `version` | 本包版本 | `sl_matter_config.h` 的 `CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING` |
| `minVersion` | 可升级下限 | [`Srcipt/OtaUpgradeRange.conf`](../Srcipt/OtaUpgradeRange.conf) 的 `OTA_MIN_VERSION` |
| `maxVersion` | 可升级上限 | 同上 `OTA_MAX_VERSION`（留空则自动 = 本包 version − 1） |

版本编码：`A.B.C` → `0xAABB00CC`。例如 `1.1.5` → `0x01010005`，`1.1.7` → `0x01010007`。

`productId` 与 Matter PID 是同一值。当前头文件仍是示例 `0x8005`；若宜家产品 ID 为 `0x5000`，改 `CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID` 后重新 pack。

生成示例（本包 `1.1.5`，区间 1.1.0～1.1.4）：

```json
{
    "productId": "0x8005",
    "version": "0x01010005",
    "minVersion": "0x01010000",
    "maxVersion": "0x01010004"
}
```

### 4.3 升级上下限怎么设

设备**当前版本**落在闭区间 `[minVersion, maxVersion]` 时，才允许升到本包 `version`。  
本包 `version` **必须大于** `maxVersion`，否则 `pack` 失败。

编辑 [`Srcipt/OtaUpgradeRange.conf`](../Srcipt/OtaUpgradeRange.conf)：

```text
OTA_MIN_VERSION=1.1.0
OTA_MAX_VERSION=1.1.4
```

也可写十六进制（`0x01010004`）。某一项留空则 pack 自动推算：

- min 空 → 本系列起点（`1.1.7` → `1.1.0` / `0x01010000`）
- max 空 → 本包 version − 1（`1.1.7` → `1.1.6` / `0x01010006`）

临时覆盖（优先于配置文件）：

```bash
OTA_MIN_VERSION=1.1.0 OTA_MAX_VERSION=1.1.4 \
  docker run --rm -e OTA_MIN_VERSION -e OTA_MAX_VERSION \
    -v "$PWD":/workspace -w /workspace IMAGE pack
```

---

## 5. Branch protection（建议）

**Settings → Branches → Add rule**（`main`）：

- Require a pull request before merging
- Require status checks：勾选 **Firmware / CodeAnalysis**、**Firmware / UnitTest**、**Firmware / Build**（跑通一次后出现）
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
