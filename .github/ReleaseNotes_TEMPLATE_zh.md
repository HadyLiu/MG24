# 发布说明模板（Release Notes + 测试报告）

> 打 `vX.Y.Z` Tag 前填写；可贴到 GitHub Release 正文。  
> 对应 NonFuncReq：每次发布须有产物 + Release Notes；审查需「每个版本的测试报告」。

## 版本

- **Tag / 版本号：** vX.Y.Z
- **Git commit：**
- **编译镜像：** `ghcr.io/<owner>/li-bat-matterlight:sdk-2025.12.2`
- **日期：**

## 变更摘要

-
-

## 产物清单

- [ ] `*-full.s37`
- [ ] `*-app-only.gbl`
- [ ] `SHA256SUMS.txt`（Release 自动生成则可勾）

## 软件侧验证（可无板）

- [ ] `./Srcipt/CiLocal.sh --lint-only` 或 Actions **Lint** 通过
- [ ] `./Srcipt/CiLocal.sh --qa-only` 或 Actions **QA** 通过
- [ ] Actions **Release Firmware** 编译成功

## 板级自测（需要板子）

| 项 | 通过 | 备注 |
|----|------|------|
| 烧录 full / OTA gbl | ☐ | |
| 短按亮度 100%→35%→关 | ☐ | |
| 双击色库切换 | ☐ | |
| Matter 配网 / 开关 / 调光 / 颜色 | ☐ | Hub： |
| 工厂复位灯效与重配网 | ☐ | |
| 低电 / 临界电量策略 | ☐ | |
| 充电指示 | ☐ | |
| 休眠 / 唤醒观感 | ☐ | 电流若可测： |

## 诊断核对（需要板子 / 控制器）

| 项 | 结果 | 备注 |
|----|------|------|
| Assert / 复位原因可读 | | |
| Fabric 数量正确 | | |
| Diagnostic cluster 可读 | | |

## 已知问题

-

## 测试报告结论

- **结论：** 通过 / 带风险通过 / 不通过
- **测试人：**
- **设计描述是否已对照：** 是 / 否（`Doc/设计方案.md`）

## 宜家正式项（暂缓，有规范再填）

- [ ] 宜家产品测试规范条目
- [ ] 宜家系统 / 主流 Hub 互操作报告链接
- [ ] 安全审计材料链接
