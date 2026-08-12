# CI/CD 待办清单（DevOps）

依据：[`NonFuncReq_zh.md`](./NonFuncReq_zh.md)（目标规范）  
参考：[`demo/rs-hs-cicd-templates`](./demo/rs-hs-cicd-templates/)（宜家通用模板，**不可整包照搬**）  

本仓库现状：固件构建工具已 Docker 化（`docker/` + `Srcipt/`）。  
**本地 Docker 验证（2026-08-12）：** `li-bat-matterlight:slim` + 挂载宿主 `~/.silabs` 已通过 `check-slc` 与固件 `build`（产物写入 `artifact/`）。完整镜像 `sdk-2025.12.2`（镜像内装 SDK）仍在完善安装依赖。  
根目录 `.github/workflows/` 尚未落地，其余 CI/CD 项待 Docker 路径稳定后再做。

---

## 1. 结论与原则

| 原则 | 说明 |
|------|------|
| 规范优先 | 以 `NonFuncReq_zh.md` 的 DevOps / QA 条款为验收标准 |
| 模板参考 | `rs-hs-cicd-templates` 只借 **lint / Release / 上传 / 合规** 模式，不拷 Node/Go 构建 |
| 流水线要薄 | workflow 每步 1～几行；复杂逻辑进 Docker / Bash（本地 = CI） |
| 固件自建 | Matter / SiSDK / ARM GCC 构建必须本仓库自写，模板库没有 |

```text
GitHub（主干 + 短分支）
  ├─ PR / push  → lint + 固件编译 → Artifacts
  └─ Tag        → 正式构建 → Release Notes + 产物包 → IKEA OTA API
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
| 烧录脚本 | `Srcipt/Download.sh` 等 | 设备侧，不进 CI |
| 镜像说明 | `docker/docker_zh.md` | 构建与运行文档 |
| 开发说明 | `Srcipt/develop_zh.md` | `.slcp` / 引脚 / 脚本 |

---

## 3. 与 NonFuncReq 对照（缺口）

### 3.1 源代码 / 协作

| 要求 | 状态 | 待办 |
|------|------|------|
| 使用 GitHub | 部分（本地有 `.git`，需确认远端） | 确认推送到组织仓库；配置 branch protection |
| 主干开发 + 短分支 | 未制度化 | 写贡献约定：`main` + 短生命周期 feature 分支 |
| 镜像同步 Inter IKEA GitHub | 未做 | 交付/发版时同步；可选 mirror workflow |
| Tag = 发布版本号 | 未做 | 约定 `vX.Y.Z`；Release workflow 打 Tag/Release |
| 可集成宜家组件 API | 设计预留 | 等宜家 API 文档后再接 |

### 3.2 构建流水线（CI）

| 要求 | 状态 | 待办 |
|------|------|------|
| GitHub Actions | **未做**（`.github` 空） | 新增 workflow |
| 配置检入 VCS | 待做 | 提交 `.github/workflows/*` |
| 工具进 Docker + 自定义 Action | Docker 有；Action 未独立仓库 | 阶段 1 用本仓 Docker；后期镜像/Action 拆独立仓 |
| 流水线逻辑极简 | 待做 | workflow 只调脚本/`docker run` |
| 本地可复现 CI | 部分 | 补 `Srcipt/CiLocal.sh`（一键 lint+build，与 CI 同命令） |

### 3.3 交付（CD）

| 要求 | 状态 | 待办 |
|------|------|------|
| 发布至 IKEA OTA API | 未做 | 等 API；写 `scripts/publish-ota.sh` + Secrets |
| 每次发布：全产物 + Release Notes | 未做 | Tag 触发打包 `artifact/` + `gh release` |
| Secrets 存凭据 | 未做 | 配置 GitHub Secrets（禁止写进仓库） |

### 3.4 基础设施

| 要求 | 状态 | 待办 |
|------|------|------|
| Linux 构建 | 已有（Ubuntu 镜像） | 保持 LTS |
| 软件更新策略 | 未文档化 | Dockerfile 固定版本号 + 定期升版记录 |
| Self-hosted runner + IaC | 未做 | 构建慢/镜像大时再议；参考模板库 runners + Terraform/Ansible |

### 3.5 质量保证（与 CI 相关）

| 要求 | 状态 | 待办 |
|------|------|------|
| 静态检查 / lint 进 CI | 未做 | shellcheck、actionlint、可选 clang-tidy/cppcheck |
| 单元测试 / 集成测试 | 未做 | `User/` 可 host 测的用例 + CI job |
| 人工 Code Review | 流程未固化 | PR + required reviewers |
| 诊断簇 / Assert / Fabric 数等 | 产品侧 | 不在本 CI 清单展开；测试报告单独管 |

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

### 正确引用方式（有 org 权限时）

```yaml
jobs:
  lint:
    uses: inter-ikea/rs-hs-cicd-templates/.github/workflows/rs-hs-lint.yaml@main
```

无权限时：在本仓仿写精简版 lint job 即可。

---

## 5. 分阶段任务清单

### 阶段 A — 最小可用 CI（优先）

- [ ] **A1** 新增 `.github/workflows/ci-firmware.yml`
  - 触发：`pull_request`、`push`（`main` / `develop`）
  - Job：checkout → 构建/拉取 Docker 镜像 → `docker run ... build` → 上传 `artifact/`
- [ ] **A2** 新增 `.github/workflows/lint.yml`
  - `shellcheck`：`Srcipt/*.sh`、`docker/entrypoint.sh`、`docker/build-image.sh`
  - `actionlint`：校验 workflow YAML
  - 可选：`yamllint`
- [ ] **A3** 新增 `Srcipt/CiLocal.sh`
  - 本地一键执行与 CI 相同的 lint + build 命令
- [ ] **A4** 文档：在 `readme_zh.md` / `Srcipt/develop_zh.md` 增加「CI 本地复现」小节
- [ ] **A5** 确认 Runner 策略
  - 短期：`ubuntu-latest` + 预构建镜像（GHCR）或 CI 内 `docker build`
  - 注意：完整 SiSDK 镜像体积大、首次构建慢，需评估缓存 / 预推镜像

**验收：** PR 能自动编出 `artifact/*.s37` / `*.gbl` 并下载 Artifacts。

---

### 阶段 B — 发布 CD（Tag）

- [ ] **B1** 约定版本号与 Tag（如 `v1.1.6`）
- [ ] **B2** 新增 `.github/workflows/release-firmware.yml`
  - 触发：`push` tags `v*`
  - 构建 → 打包全产物 → `gh release create` + Release Notes
- [ ] **B3** 发布交付清单模板（Release Notes 必填项）
  - 版本、变更摘要、产物列表、已知问题、测试报告链接
- [ ] **B4**（可选）仿 `rs-hs-release-service` 做 SemVer bump

**验收：** 打 Tag 后自动出 GitHub Release 与完整产物包。

---

### 阶段 C — IKEA OTA 对接

- [ ] **C1** 向宜家索取 OTA API / 凭证 / 产物格式要求
- [ ] **C2** 编写 `scripts/publish-ota.sh`（或独立工具仓）
- [ ] **C3** 配置 GitHub Secrets（OTA URL、Token 等）
- [ ] **C4** Release workflow 末尾一行调用 publish 脚本
- [ ] **C5** 失败重试与审计日志约定

**验收：** Tag 发布后固件自动进入 HomeSmart OTA（或 staging）。

---

### 阶段 D — 质量与合规加固

- [ ] **D1** `User/` 单元测试骨架 + CI job
- [ ] **D2** C++ 静态分析（clang-tidy / cppcheck）进 PR
- [ ] **D3** Branch protection：必过 CI + 至少 1 人 Review
- [ ] **D4** Inter IKEA 仓库镜像同步流程（文档 + 可选 workflow）
- [ ] **D5** Docker 镜像独立仓库 + GHCR；本仓 workflow 只 `docker pull` + `run`
- [ ] **D6**（可选）Self-hosted runner + IaC（Terraform/Ansible）入库，供宜家复现

---

## 6. 建议新增的仓库文件（规划）

```text
.github/
  workflows/
    ci-firmware.yml          # 阶段 A
    lint.yml                 # 阶段 A
    release-firmware.yml     # 阶段 B
  PULL_REQUEST_TEMPLATE.md   # 可选，从模板库抄
Srcipt/
  CiLocal.sh                 # 阶段 A：本地 = CI
scripts/                     # 或继续放 Srcipt/
  publish-ota.sh             # 阶段 C
Doc/
  CICD_TODO_zh.md            # 本文件
  ReleaseNotes_TEMPLATE_zh.md # 阶段 B（可选）
```

---

## 7. 建议优先级（执行顺序）

| 顺序 | 任务 | 原因 |
|------|------|------|
| 1 | A1 + A2 + A3 | 满足「CI 检入 + Docker 构建 + 本地复现」硬要求 |
| 2 | A5 镜像预构建/缓存 | 避免 CI 每次从零装 SiSDK |
| 3 | B1～B3 | Tag 发布与交付物 |
| 4 | C* | 依赖宜家 API，可并行等文档 |
| 5 | D* | 质量与长期运维 |

---

## 8. 风险与注意

| 风险 | 应对 |
|------|------|
| Docker 完整镜像巨大、CI 超时 | 预构建推 GHCR；Runner 加大磁盘/超时；或挂载缓存的 `~/.silabs`（需自托管） |
| Studio 硬编码 `/home/hadyliu/.silabs` | 镜像内用户已对齐；CI 勿改 HOME |
| Generate 覆盖自定义配置 | CI 默认只 `build`，不自动 `generate`（除非单独 job） |
| `rs-hs-cicd-templates` 路径/组织权限 | 无 `inter-ikea` 权限则本地仿写，不阻塞阶段 A |
| Secrets 泄露 | 只用 GitHub Secrets；禁止提交凭据 |

---

## 9. 相关文档

| 文档 | 说明 |
|------|------|
| [NonFuncReq_zh.md](./NonFuncReq_zh.md) | 非功能 / DevOps 原文 |
| [demo/rs-hs-cicd-templates/README_CN.MD](./demo/rs-hs-cicd-templates/README_CN.MD) | 模板库说明与局限 |
| [../docker/docker_zh.md](../docker/docker_zh.md) | Docker 镜像构建与 `generate`/`build` |
| [../Srcipt/develop_zh.md](../Srcipt/develop_zh.md) | 开发：`.slcp`、引脚、脚本 |

---

## 10. 下一步（ Immediate ）

### 已完成（本地 Docker）

- [x] 构建 `li-bat-matterlight:slim`
- [x] `check-slc`（java / slc / zap-cli）
- [x] `docker run ... build` 编出 `artifact/*.s37` / `*.gbl`
- [x] entrypoint 自动清理宿主机/容器路径不一致的 CMakeCache

### 本地常用命令（当前推荐）

```bash
# 编译固件（挂载本机 SDK）
docker run --rm \
  -v "$PWD":/workspace \
  -v "$HOME/.silabs":/home/hadyliu/.silabs \
  -w /workspace \
  li-bat-matterlight:slim build

# SLC 自检
docker run --rm \
  -v "$PWD":/workspace \
  -v "$HOME/.silabs":/home/hadyliu/.silabs \
  -w /workspace \
  li-bat-matterlight:slim check-slc
```

注意：若刚在宿主机编过，容器路径是 `/workspace`，entrypoint 会清掉不匹配的 `build/` 缓存。

### 待继续

1. 完整镜像 `li-bat-matterlight:sdk-2025.12.2`：上次失败为下载中断（`unexpected EOF` / conan remote），可重试 `./docker/build-image.sh`  
2. 阶段 A：`.github/workflows/ci-firmware.yml` + `lint.yml` + `Srcipt/CiLocal.sh`（**等 Docker 路径稳定后再做**）
