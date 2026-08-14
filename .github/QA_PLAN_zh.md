# 质量保证（QA）落地方案

依据：[`NonFuncReq_zh.md`](./NonFuncReq_zh.md)「质量保证」段  
借用：[`Doc/demo/rs-hs-cicd-templates/.github`](../Doc/demo/rs-hs-cicd-templates/.github)
（**只借模式，不整包照搬**；无 Matter/固件构建）  
固件 CI 对照：[`Doc/demo/rs-hs-external-haoting-fw-dev-support-code-mg301/CICD_总结.md`](../Doc/demo/rs-hs-external-haoting-fw-dev-support-code-mg301/CICD_总结.md)

GitHub 操作见：[`GITHUB_SETUP_zh.md`](./GITHUB_SETUP_zh.md)

---

## 0. 结论（先看这）

| 层级 | 含义 | 本仓库状态 |
|------|------|------------|
| **现在就能做** | 代码 + GitHub 流程即可验收 | 本方案已落文件 / workflow |
| **需要板子 / Matter 环境** | 实验室自测 | 清单模板已写，需人工勾选 |
| **需要宜家 / 正式流程** | 规范、Hub、审计、OTA | 等对方文档与权限 |

模板 `.github` **不包含**固件编译、C++ 单元测试、宜家产品测试；本仓自建补齐。

---

## 1. 从 rs-hs-cicd-templates 借用什么

| 模板内容 | 本仓用法 |
|----------|----------|
| `PULL_REQUEST_TEMPLATE.md` | → `.github/PULL_REQUEST_TEMPLATE.md`（改成固件清单） |
| `rs-hs-lint.yaml` 的 yamllint / shellcheck / actionlint | → `lint.yml` 已有 shell+actionlint；**补 yamllint（仅 workflows）** |
| `rs-hs-release-service` 的 Release Notes 思路 | → `.github/ReleaseNotes_TEMPLATE_zh.md` |
| Node/Go CI、Phrase、Splunk、S3、compliance NPM | **不借用** |
| 整库 `.github/workflows` 复制 | **禁止** |
| 豪庭组件旁 `test/` + 自动发现 | → 阶段 D10：扩 `qa/host/`，不上 cmocka |
| 豪庭 cppcheck 报告进 Release | → 阶段 D9 |

---

## 2. NonFuncReq QA 条款 ↔ 落地

### 2.1 现在就能做（已落地 / 可马上用）

| NonFuncReq 要求 | 落地 |
|-----------------|------|
| Lint 进 CI | `lint.yml`（push/PR）：shellcheck + actionlint + yamllint |
| 静态代码分析 | `qa.yml`（push/PR）：`cppcheck` 扫描 `User/` |
| 单元测试 | `qa/host/`：host 侧测 `LightDimmingSpec`；`qa.yml` 编译运行 |
| 人工 Code Review | PR 模板强制自检；Settings 开「Require PR + approvals」 |
| 每版测试报告 | `ReleaseNotes_TEMPLATE_zh.md`：发 Tag 前/后填并贴进 Release |
| 设计描述审查材料 | 发版勾选：已对照 `Doc/设计方案.md`（本地 Doc） |

本地一键：

```bash
./Srcipt/CiLocal.sh              # 三块全跑
./Srcipt/CiLocal.sh --lint-only
./Srcipt/CiLocal.sh --qa-only    # host 单测 +（若已装）cppcheck
./Srcipt/CiLocal.sh --build-only
IMAGE=li-bat-matterlight:slim ./Srcipt/CiLocal.sh --build-only
```

### 2.2 需要板子 / Matter 环境

| NonFuncReq 要求 | 落地 |
|-----------------|------|
| 产品功能验证 | Release 模板「板级自测」勾选：按键/灯效/配网/复位/低电/充电 |
| 性能粗测 | 同模板：休眠电流、配网时长（自建指标，先记数） |
| 诊断：Assert / 复位原因 | 差距表见下节；板上核对日志与复位原因 |
| Fabric 数量 | 控制器 / 芯片工具读 FabricCount；与 `MatterBridge` 行为对照 |
| Diagnostic cluster | 控制器读 General Diagnostics 等；记版本差异 |
| 与主流 Matter Hub 互操作 | 自测表（Apple/Google/Amazon/IKEA Dirigera 等）— 有设备再勾 |

### 2.3 需要宜家 / 流程文档（暂不能关单）

| NonFuncReq 要求 | 状态 |
|-----------------|------|
| 宜家产品测试规范全套 + 合格报告 | 等规范 |
| 系统/集成/互操作（宜家系统） | 等实验室与账号 |
| 安全分解展示（安全审计） | 生命周期至少 1 次 |
| EDS 后维护 3 年 | 商务/流程 |
| 生产关调试日志、熔丝、Secure Boot 专项 | 见 NonFuncReq「安全性」；另开安全清单 |

---

## 3. 诊断能力差距表（产品侧待填）

| 能力 | 代码侧线索 | 验收方式 | 状态 |
|------|------------|----------|------|
| Assert | `User/` 多处 `assert` | 调试构建触发后是否留复位原因 | 待板上确认 |
| 复位原因 | SDK / Matter 诊断 | 读 Diagnostic / 启动日志 | 待确认 |
| Fabric 数量 | `MatterBridge` / FabricTable | Hub 或 `FabricCount` | 待确认 |
| Diagnostic cluster | Matter 栈默认簇 | 控制器读属性 | 待确认 |

发版时把本表结论贴进 Release Notes「已知问题 / 诊断」小节。

---

## 4. 仓库文件一览

```text
.github/
  QA_PLAN_zh.md                 # 本文件
  PULL_REQUEST_TEMPLATE.md      # 借自模板，固件化
  ReleaseNotes_TEMPLATE_zh.md   # 发版 + 测试报告 + 板测
  workflows/
    lint.yml                    # push/PR（+ yamllint）
    qa.yml                      # push/PR：host 单测 + cppcheck
    ci-firmware.yml             # push/PR：编固件
    release-firmware.yml        # 仅 v* Tag
    docker-publish.yml          # 仅手动
qa/
  host/
    test_light_dimming_spec.cpp # 无硬件依赖单测
    run_host_tests.sh
Srcipt/CiLocal.sh               # --qa-only
```

---

## 5. 建议执行顺序

1. Push 本批 QA 文件；Tag 或手动跑 **Lint** / **QA**  
2. GitHub：`main` 开 **Require pull request + 1 approval**  
3. 每次发版：按 `ReleaseNotes_TEMPLATE_zh.md` 填报告再打 `v*`  
4. 有板后：补齐诊断差距表与板测勾选  
5. 宜家规范/OTA 到手后：再开阶段 C 与正式互操作报告  

---

## 6. 刻意不做（避免假合规）

- 不把 `rs-hs-nodejs` / `rs-hs-go-ci` 拷进固件仓  
- 不把「有 lint」说成「宜家测试规范已满足」  
- 不在无 API 时伪造 OTA / Inter IKEA 同步 workflow  
