# CI/CD 待办清单（DevOps）

依据：[`NonFuncReq_zh.md`](./NonFuncReq_zh.md)（目标规范）  
参考 1：[`../Doc/demo/rs-hs-cicd-templates`](../Doc/demo/rs-hs-cicd-templates/)（宜家通用模板，**不可整包照搬**）  
参考 2：[`../Doc/CICD_总结.md`](../Doc/CICD_总结.md)（豪庭 MG301 **固件** CI 对照；借产物/质量报告，不借 clone SDK）  
GitHub 操作步骤：[`GITHUB_SETUP_zh.md`](./GITHUB_SETUP_zh.md)

本仓库现状：固件构建工具已 Docker 化（`docker/` + `Srcipt/`）。  
**本地 Docker 验证（2026-08-12）：** 完整镜像 `li-bat-matterlight:sdk-2025.12.2` 已通过网络构建，并在**不挂载**宿主 `~/.silabs` 下完成 `check-slc` + `build`。  
**CI 文件（2026-08-12）：** 已落地阶段 A workflows + `CiLocal.sh` + 阶段 B Release workflow；待你在 GitHub 推送代码并推 GHCR 镜像后验收。

---

## 1. 结论与原则

| 原则 | 说明 |
|------|------|
| 规范优先 | 以 `NonFuncReq_zh.md` 的 DevOps / QA 条款为验收标准 |
| 模板参考 | 通用仓只借 **lint / Release / 上传**；MG301 固件仓只借 **产物形态 / 质量报告进 Release**，不拷 Node/Go，也不改成 CI 现场 clone SDK |
| 流水线要薄 | workflow 每步 1～几行；复杂逻辑进 Docker / Bash（本地 = CI） |
| 固件自建 | Matter / SiSDK / ARM GCC 构建必须本仓库自写，模板库没有 |

```text
GitHub（主干 + 短分支）
  ├─ PR / push / Tag → CI：CodeAnalysis → UnitTest → Build → Release（仅 Tag）
  └─ 手动            → Docker Publish（GHCR 镜像）
                    ↑
         docker/entrypoint.sh + Srcipt/*.sh
```

---

## 2. 已具备（可复用）

| 项 | 位置 | 说明 |
|----|------|------|
| Docker 构建镜像 | `docker/Dockerfile` | Ubuntu 24.04 + slt / slc / java21 / zap / SDK |
| 容器入口 | `docker/entrypoint.sh` | `build` / `generate` / `check-slc` / `shell` |
| 本地编译脚本 | `Srcipt/Compile.sh` | 与 CI 同路径的本地副本基础 |
| 本地 = CI | `Srcipt/CiLocal.sh` | lint + `docker run ... build` |
| 烧录脚本 | `Srcipt/Download.sh` 等 | 设备侧，不进 CI |
| 镜像说明 | `docker/docker_zh.md` | 构建与运行文档 |
| 开发说明 | `Srcipt/develop_zh.md` | `.slcp` / 引脚 / 脚本 |
| GitHub 操作 | `.github/GITHUB_SETUP_zh.md` | Actions / GHCR / Tag / 保护分支 |

---

## 3. 与 NonFuncReq 对照（缺口）

### 3.1 源代码 / 协作

| 要求 | 状态 | 待办 |
|------|------|------|
| 使用 GitHub | 远端 `barryjim/KAJEN_T` | 推送 workflows；配置 branch protection |
| 主干开发 + 短分支 | 未制度化 | 写贡献约定：`main` + 短生命周期 feature 分支 |
| 镜像同步 Inter IKEA GitHub | 未做 | 交付/发版时同步；可选 mirror workflow |
| Tag = 发布版本号 | workflow 已备 | 约定 `vX.Y.Z`；打 Tag 触发 Release |
| 可集成宜家组件 API | 设计预留 | 等宜家 API 文档后再接 |

### 3.2 构建流水线（CI）

| 要求 | 状态 | 待办 |
|------|------|------|
| GitHub Actions | **已落文件** | 推送到远端后在 Actions 页验收 |
| 配置检入 VCS | 待推送 | 提交 `.github/workflows/*` |
| 工具进 Docker + 自定义 Action | Docker + GHCR | 阶段 D 可拆独立镜像仓 |
| 流水线逻辑极简 | 已做 | workflow 只 `docker pull` / `run` |
| 本地可复现 CI | **已做** | `Srcipt/CiLocal.sh` |

### 3.3 交付（CD）

| 要求 | 状态 | 待办 |
|------|------|------|
| 发布至 IKEA OTA API | 未做 | 等 API；写 `scripts/publish-ota.sh` + Secrets |
| 每次发布：全产物 + Release Notes | workflow 已备 | 打 `v*` Tag 验收 |
| Secrets 存凭据 | 未做 | 配置 GitHub Secrets（禁止写进仓库） |

### 3.4 基础设施

| 要求 | 状态 | 待办 |
|------|------|------|
| Linux 构建 | 已有（Ubuntu 镜像） | 保持 LTS |
| 软件更新策略 | 部分 | Dockerfile 固定版本号；升版记入 docker 文档 |
| Self-hosted runner + IaC | 未做 | 构建慢/镜像大时再议 |

### 3.5 质量保证（与 CI 相关）

| 要求 | 状态 | 待办 |
|------|------|------|
| 静态检查 / lint 进 CI | **已做** | `ci.yml` CodeAnalysis：shellcheck + actionlint + yamllint |
| 单元测试 / 集成测试 | 未做 | `User/` 可 host 测的用例 + CI job |
| 人工 Code Review | 流程未固化 | PR + required reviewers |
| 诊断簇 / Assert / Fabric 数等 | 产品侧 | 不在本 CI 清单展开 |

---

## 4. 对 `rs-hs-cicd-templates` 的用法

### 可借鉴

| 模板内容 | 用途 |
|----------|------|
| `rs-hs-lint.yaml` | YAML / Shell / JSON / actionlint |
| `rs-hs-release-service.yaml` | SemVer / Tag / GitHub Release / Notes |
| `rs-hs-publish-to-s3.yaml` | 产物上传模式（若 OTA 走 S3） |
| `github-actions/upload-artifact` 等 | 产物上传细节 |
| `templates/.github/PULL_REQUEST_TEMPLATE.md` | PR 模板 |
| `self-hosted-runners/` | 自托管 runner（后期） |

### 不要照搬

| 模板内容 | 原因 |
|----------|------|
| `rs-hs-nodejs.yaml` / `rs-hs-go-ci.yaml` | 与本固件工程无关 |
| `rs-hs-compliance.yaml` 的 NPM 更新 | 无 Node 包管理需求时跳过 |
| 整库复制进固件仓库根目录 | 污染仓库；且无 Matter 构建 |

无 `inter-ikea` 权限：本仓已仿写精简版 lint / release，不阻塞阶段 A。

### 4.1 对豪庭 MG301 固件 CI 的用法

详细对照见 [`../Doc/CICD_总结.md`](../Doc/CICD_总结.md)。源码说明书：[`Read.md`](../Doc/demo/rs-hs-external-haoting-fw-dev-support-code-mg301/Read.md)、[`Unit_test.md`](../Doc/demo/rs-hs-external-haoting-fw-dev-support-code-mg301/Unit_test.md)。

| 可借鉴 | 本仓用法 |
|--------|----------|
| 薄 YAML + 厚脚本 | 已用 Docker / `Srcipt/`；保持 |
| debug / dev / release | 阶段 B6：Tag 发 release，日常可只编一种 |
| App + BL 合体 + Matter OTA | 阶段 B5：对齐交付物，不只 `.s37`/`.gbl` |
| 版本双轨（hex + 字符串） | 已有 `sl_matter_config.h`（`17` / `"1.1.7"`）；Tag 必须与宏一致 |
| cppcheck/lint 报告进 Release | 阶段 D9 |
| Google clang-format / cpplint | **已落地**：`User/`；CI `--check` + cpplint 进 `code_quality_report` |
| 组件旁 `test/` + CI 自动发现 | 阶段 D10；host 测即可，不上 cmocka |
| `update_version.sh` 升版对包 | 阶段 C，等 OTA API |

| 不要照搬 | 原因 |
|----------|------|
| CI 每次 clone SiSDK / Matter | 本仓已 Docker + GHCR |
| 打开 MG301 的 UnitTest Job | demo 缺 `homebrew_static`，会挂 |
| MG301 VID/PID、板卡、kt_components | 产品不同 |

---

## 5. 分阶段任务清单

### 阶段 A — 最小可用 CI（优先）

- [x] **A1** `.github/workflows/ci.yml` + `ci-template.yml`（PR/push/Tag：四段主任务）
- [x] **A2** CodeAnalysis：shellcheck + actionlint + yamllint
- [x] **A3** `Srcipt/CiLocal.sh`
- [x] **A4** 文档：`readme*_zh` / `develop_zh` / `GITHUB_SETUP_zh.md`
- [x] **A5** Runner：`ubuntu-latest` + 预构建镜像推 GHCR（`docker-publish.yml`）

**验收（需在 GitHub 完成）：** 推代码 → 推镜像 → PR 能下载 `artifact/*.s37` / `*.gbl`。

### 阶段 B — 发布 CD（Tag）

- [x] **B1** 约定版本号与 Tag：`vX.Y.Z`（见 GITHUB_SETUP）
- [x] **B2** `ci-template.yml` Release Job（仅 `v*` Tag）
- [x] **B3** 发布交付清单模板：`.github/ReleaseNotes_TEMPLATE_zh.md`
- [x] **B4** Tag `vX.Y.Z` 必须等于 `CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING`（`Srcipt/CheckReleaseTag.sh`）
- [x] **B5** 产物清单：`Srcipt/PackRelease.sh` → App / BL+App / `.gbl` / 可选 `.ota` / `config.json` / `SHA256SUMS.txt` / `VERSION.txt`  
      宜家 `config.json`：PID←`CHIPProjectConfig.h`；version←`sl_matter_config.h`；min/max←`Srcipt/OtaUpgradeRange.conf`（说明见 `GITHUB_SETUP_zh.md` 第 4 节）
- [x] **B6** dev / release：`LI_BAT_BUILD_VARIANT`；CI=dev，Tag=release（关 APP/Matter 调试日志与 CLI）

**验收：** 打 Tag 后自动出 GitHub Release 与完整产物包。

### 阶段 C — IKEA OTA 对接

- [ ] **C1** 向宜家索取 OTA API / 凭证 / 产物格式要求
- [x] **C2** `Srcipt/PublishOta.sh` 骨架（无 Secrets 则跳过；有 URL 无协议则失败）
- [ ] **C3** 配置 GitHub Secrets（`IKEA_OTA_URL`、`IKEA_OTA_TOKEN`）
- [x] **C4** Release Job 调用 publish 脚本
- [ ] **C5** 失败重试与审计日志约定（等 API）

### 阶段 D — 质量与合规加固

- [x] **D1** `qa/host/` 单元测试骨架 + CI UnitTest Job
- [x] **D2** cppcheck（`User/`）进 CodeAnalysis；clang-tidy 可选后续
- [ ] **D3** Branch protection：Require PR + ≥1 Review（网页配置）
- [ ] **D4** Inter IKEA 仓库镜像同步流程
- [ ] **D5** Docker 镜像独立仓库 + GHCR；本仓只 `pull` + `run`
- [ ] **D6**（可选）Self-hosted runner + IaC
- [x] **D7** QA 方案文档：`.github/QA_PLAN_zh.md`（借模板 + NonFuncReq 对照）
- [x] **D8** PR 模板：`.github/PULL_REQUEST_TEMPLATE.md`
- [x] **D9** 豪庭式质量报告：cppcheck XML + `cppcheck-htmlreport` HTML、gcovr 覆盖率；
      `code_quality_report.tar.gz` / `unit_test_report.tar.gz` 随 QA Artifact 与 Release 挂上
- [x] **D10** `User/**/test/*.cpp` 自动发现（host 测，不搬 cmocka）

---

## 6. 仓库文件（当前）

```text
.github/
  workflows/
    ci.yml                 # 主入口（调度器）
    ci-template.yml        # CodeAnalysis → UnitTest → Build → Release
    docker-publish.yml     # 仅手动
  PULL_REQUEST_TEMPLATE.md
  ReleaseNotes_TEMPLATE_zh.md
  QA_PLAN_zh.md
  GITHUB_SETUP_zh.md
  CICD_TODO_zh.md
  NonFuncReq_zh.md
qa/host/
  run_host_tests.sh
  setup.sh
qa/cppcheck/
  run_cppcheck.sh
  setup.sh
qa/cpplint/
  run_cpplint.sh
  setup.sh
qa/format/
  run_clang_format.sh
qa/PackQualityReports.sh
.clang-format
CPPLINT.cfg
User/middlewares/test/
  test_light_dimming_spec.cpp
Srcipt/
  CiLocal.sh
  ReadFirmwareVersion.sh
  CheckReleaseTag.sh
  PackRelease.sh
  OtaUpgradeRange.conf
  UpdateFirmwareVersion.sh
  PublishOta.sh
```

---

## 7. 建议优先级（执行顺序）

| 顺序 | 任务 | 原因 |
|------|------|------|
| 1 | 推送 workflows + 推 GHCR 镜像 | 解锁 Actions 验收 |
| 2 | 手动跑 CI | 确认四段 Job 与 Artifacts |
| 3 | Branch protection（D3） | 保护主干 |
| 4 | Tag 发布（B） | 正式交付物 |
| 5 | B5 产物清单（OTA / BL 合体） | 对齐宜家/豪庭交付 |
| 6 | C* OTA | 依赖宜家 API |
| 7 | D9 / D10 质量报告与组件单测 | 长期 |

---

## 8. 风险与注意

| 风险 | 应对 |
|------|------|
| Docker 完整镜像巨大、CI 超时 | 预构建推 GHCR；`free-disk-space`；timeout 120/180 min |
| Studio 硬编码 `/home/hadyliu/.silabs` | 镜像内用户已对齐；CI 勿改 HOME |
| Generate 覆盖自定义配置 | CI 默认只 `build`，不自动 `generate` |
| GHCR 包私有导致 pull 失败 | Package 设 Public，或确保 `packages: read` |
| Secrets 泄露 | 只用 GitHub Secrets；禁止提交凭据 |

---

## 9. 相关文档

| 文档 | 说明 |
|------|------|
| [GITHUB_SETUP_zh.md](./GITHUB_SETUP_zh.md) | **你在网页上怎么点** |
| [NonFuncReq_zh.md](./NonFuncReq_zh.md) | 非功能 / DevOps 原文 |
| [../docker/docker_zh.md](../docker/docker_zh.md) | Docker 镜像构建与 `generate`/`build` |
| [../Srcipt/develop_zh.md](../Srcipt/develop_zh.md) | 开发：`.slcp`、引脚、脚本 |
| [../Doc/CICD_总结.md](../Doc/CICD_总结.md) | 豪庭 MG301 固件 CI 对照与本仓落地 |

---

## 10. 下一步（你现在要做的）

1. **提交并推送**本批 `.github/workflows/`、`Srcipt/CiLocal.sh`、文档（不要提交 `artifact/` 二进制改动除非有意）。
2. 按 [`GITHUB_SETUP_zh.md`](./GITHUB_SETUP_zh.md)：
   - 打开 Actions 权限
   - **推送 GHCR 镜像**（本机 `docker push` 或跑 **Docker Publish**）
   - 手动跑 **CI**，下载 Artifacts
3. （可选）给 `main` 加 Branch protection。
4. 发版时：`git tag vX.Y.Z && git push origin vX.Y.Z`。
5. 阶段 C 等宜家 OTA 文档后再做。
