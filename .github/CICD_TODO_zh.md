# CI/CD 待办清单（DevOps）

依据：[`NonFuncReq_zh.md`](./NonFuncReq_zh.md)（目标规范）  
参考：[`../Doc/demo/rs-hs-cicd-templates`](../Doc/demo/rs-hs-cicd-templates/)（宜家通用模板，**不可整包照搬**）  
GitHub 操作步骤：[`GITHUB_SETUP_zh.md`](./GITHUB_SETUP_zh.md)

本仓库现状：固件构建工具已 Docker 化（`docker/` + `Srcipt/`）。  
**本地 Docker 验证（2026-08-12）：** 完整镜像 `li-bat-matterlight:sdk-2025.12.2` 已通过网络构建，并在**不挂载**宿主 `~/.silabs` 下完成 `check-slc` + `build`。  
**CI 文件（2026-08-12）：** 已落地阶段 A workflows + `CiLocal.sh` + 阶段 B Release workflow；待你在 GitHub 推送代码并推 GHCR 镜像后验收。

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
| 静态检查 / lint 进 CI | **已做** | `lint.yml`：shellcheck + actionlint |
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

---

## 5. 分阶段任务清单

### 阶段 A — 最小可用 CI（优先）

- [x] **A1** `.github/workflows/ci-firmware.yml`（PR/push → pull GHCR → build → Artifacts）
- [x] **A2** `.github/workflows/lint.yml`（shellcheck + actionlint）
- [x] **A3** `Srcipt/CiLocal.sh`
- [x] **A4** 文档：`readme*_zh` / `develop_zh` / `GITHUB_SETUP_zh.md`
- [x] **A5** Runner：`ubuntu-latest` + 预构建镜像推 GHCR（`docker-publish.yml`）

**验收（需在 GitHub 完成）：** 推代码 → 推镜像 → PR 能下载 `artifact/*.s37` / `*.gbl`。

### 阶段 B — 发布 CD（Tag）

- [x] **B1** 约定版本号与 Tag：`vX.Y.Z`（见 GITHUB_SETUP）
- [x] **B2** `.github/workflows/release-firmware.yml`
- [ ] **B3** 发布交付清单模板（Release Notes 必填项）— 可选补 `Doc/ReleaseNotes_TEMPLATE_zh.md`
- [ ] **B4**（可选）SemVer bump 自动化

**验收：** 打 Tag 后自动出 GitHub Release 与完整产物包。

### 阶段 C — IKEA OTA 对接

- [ ] **C1** 向宜家索取 OTA API / 凭证 / 产物格式要求
- [ ] **C2** 编写 `scripts/publish-ota.sh`（或独立工具仓）
- [ ] **C3** 配置 GitHub Secrets（OTA URL、Token 等）
- [ ] **C4** Release workflow 末尾一行调用 publish 脚本
- [ ] **C5** 失败重试与审计日志约定

### 阶段 D — 质量与合规加固

- [ ] **D1** `User/` 单元测试骨架 + CI job
- [ ] **D2** C++ 静态分析（clang-tidy / cppcheck）进 PR
- [ ] **D3** Branch protection：必过 CI + 至少 1 人 Review
- [ ] **D4** Inter IKEA 仓库镜像同步流程
- [ ] **D5** Docker 镜像独立仓库 + GHCR；本仓只 `pull` + `run`
- [ ] **D6**（可选）Self-hosted runner + IaC

---

## 6. 仓库文件（当前）

```text
.github/
  workflows/
    ci-firmware.yml
    lint.yml
    docker-publish.yml
    release-firmware.yml
  GITHUB_SETUP_zh.md
  CICD_TODO_zh.md
  NonFuncReq_zh.md
Srcipt/
  CiLocal.sh
```

---

## 7. 建议优先级（执行顺序）

| 顺序 | 任务 | 原因 |
|------|------|------|
| 1 | 推送 workflows + 推 GHCR 镜像 | 解锁 Actions 验收 |
| 2 | 手动跑 Lint / CI Firmware | 确认 Artifacts |
| 3 | Branch protection（D3） | 保护主干 |
| 4 | Tag 发布（B） | 正式交付物 |
| 5 | C* OTA | 依赖宜家 API |
| 6 | D* 其余质量项 | 长期 |

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

---

## 10. 下一步（你现在要做的）

1. **提交并推送**本批 `.github/workflows/`、`Srcipt/CiLocal.sh`、文档（不要提交 `artifact/` 二进制改动除非有意）。
2. 按 [`GITHUB_SETUP_zh.md`](./GITHUB_SETUP_zh.md)：
   - 打开 Actions 权限
   - **推送 GHCR 镜像**（本机 `docker push` 或跑 **Docker Publish**）
   - 手动跑 **Lint**、**CI Firmware**，下载 Artifacts
3. （可选）给 `main` 加 Branch protection。
4. 发版时：`git tag vX.Y.Z && git push origin vX.Y.Z`。
5. 阶段 C 等宜家 OTA 文档后再做。
