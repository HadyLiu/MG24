# GitHub 操作说明（CI/CD）

仓库：`https://github.com/barryjim/KAJEN_T`  
对应文档：[`CICD_TODO_zh.md`](./CICD_TODO_zh.md)、[`docker/docker_zh.md`](../docker/docker_zh.md)

## 0. 总览

| Workflow | 作用 | 何时跑 |
|----------|------|--------|
| **Docker Publish** | 构建完整编译镜像并推到 GHCR | **首次必跑**；改 `docker/` 时自动 |
| **Lint** | shellcheck + actionlint | 每个 PR / push |
| **CI Firmware** | 拉 GHCR 镜像 → 编固件 → 上传 Artifact | 每个 PR / push |
| **Release Firmware** | Tag `v*` → 构建 → GitHub Release | 打版本标签时 |

```text
本机或 Docker Publish  →  ghcr.io/<owner>/li-bat-matterlight:sdk-2025.12.2
                                    ↓
                         CI Firmware / Release 拉取后编译
```

---

## 1. 推送代码（启用 Actions）

1. 确认远端是 `origin` → `https://github.com/barryjim/KAJEN_T.git`
2. 提交并推送含 `.github/workflows/` 的改动到 `main`（或开 PR）
3. 打开仓库 **Settings → Actions → General**
   - Actions permissions：**Allow all actions and reusable workflows**
   - Workflow permissions：**Read and write permissions**（Release / 推包需要）
   - 勾选 **Allow GitHub Actions to create and approve pull requests**（可选）

---

## 2. 首次准备：把编译镜像放到 GHCR（二选一）

完整镜像约 **9GB**，CI **不能**挂载你电脑的 `~/.silabs`，必须先有 GHCR 镜像。

### 方式 A — 本机推送（推荐，你已有本地镜像时最快）

```bash
# 登录（Token 需含 write:packages）
# GitHub → Settings → Developer settings → Personal access tokens
# Classic: 勾选 write:packages, read:packages, repo
echo YOUR_PAT | docker login ghcr.io -u barryjim --password-stdin

docker tag li-bat-matterlight:sdk-2025.12.2 \
  ghcr.io/barryjim/li-bat-matterlight:sdk-2025.12.2

docker push ghcr.io/barryjim/li-bat-matterlight:sdk-2025.12.2
```

### 方式 B — 用 Actions 构建推送

1. 打开 **Actions** 页
2. 左侧选 **Docker Publish**
3. **Run workflow** → 选 `main` → Run
4. 等待 30～90 分钟（网络下载 SiSDK）

### 包可见性（重要）

1. 打开 `https://github.com/barryjim?tab=packages`
2. 点开 `li-bat-matterlight`
3. **Package settings → Change visibility → Public**  
   （私有包时，其他 fork / 外部协作者拉不到；同仓库 Actions 用 `GITHUB_TOKEN` 一般可读）

---

## 3. 验证 CI Firmware

1. **Actions → CI Firmware → Run workflow**，或随便推一个小提交 / 开 PR
2. 等待 Job 成功
3. 打开该 run → **Artifacts** → 下载 `firmware-<sha>`（内含 `.s37` / `.gbl`）

若失败提示找不到镜像：回到第 2 步。

---

## 4. 发正式版（Release）

```bash
git tag -a v1.1.6 -m "firmware v1.1.6"
git push origin v1.1.6
```

会触发 **Release Firmware**：编译 → 创建 GitHub Release → 挂上产物与 `SHA256SUMS.txt`。

也可在网页：**Releases → Draft a new release → 填 Tag `vX.Y.Z` → Publish**（需已推送 Tag）。

---

## 5. Branch protection（建议）

**Settings → Branches → Add rule**（`main`）：

- Require a pull request before merging
- Require status checks to pass：勾选 **Lint** / **CI Firmware**（跑通一次后才会出现在列表）
- Require approvals：至少 1 人（团队协作时）

---

## 6. 本地复现 CI

```bash
# 与 CI 同路径（需本机已有完整镜像）
./Srcipt/CiLocal.sh

# 仅 lint / 仅编译
./Srcipt/CiLocal.sh --lint-only
./Srcipt/CiLocal.sh --build-only

# 用 slim + 宿主 SDK
IMAGE=li-bat-matterlight:slim ./Srcipt/CiLocal.sh --build-only
```

---

## 7. 尚未在 GitHub 配置的项（阶段 C）

IKEA OTA 发布需 Secrets（等宜家文档后再配）：

- **Settings → Secrets and variables → Actions → New repository secret**
- 例如：`OTA_API_URL`、`OTA_API_TOKEN`（名称以实际 API 为准）

当前 **不要**把 Token 写进仓库文件。
