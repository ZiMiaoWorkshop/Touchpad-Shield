# Touchpad Shield

**Touchpad Shield** 是一款 Windows 桌面工具，用于在打字时减少手掌、手腕误触触控板导致的光标漂移与误点击。它将 Microsoft Precision Touchpad (PTP) 调优选项集中在一处，并遵循 [Precision Touchpad tuning guidelines](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/touchpad-tuning-guidelines)；同时可根据您监控的外接输入设备（如鼠标、键盘、手柄等）连接状态，自动关闭或恢复内置触控板，避免外设在线时误触。

设计与开发：**[ZiMiaoWorkshop](https://github.com/ZiMiaoWorkshop)**

**当前版本：** 1.1.1 build 0108 · [下载最新发行版](https://github.com/ZiMiaoWorkshop/Touchpad-Shield/releases/latest)

**v1.1.1 变更：** 修复重启后登录自启被误跳过；单实例 Mutex 改为 Session 级（`Local\`）；移除 `AutostartHandledSessionId`；统一 `ApplyInitialClientBounds` 窗口尺寸/最小约束；二次打开 exe 经 `ShowFromTray` 显示已有实例（修复窗口过小问题）。

---

## 下载

| 资源 | 说明 |
|------|------|
| [**最新 Release**](https://github.com/ZiMiaoWorkshop/Touchpad-Shield/releases/latest) | 正式 NSIS 安装包（`TouchpadShield-1.1.1-build0108-setup.exe`） |
| [`TouchpadPhysicalSize.csv`](https://github.com/ZiMiaoWorkshop/Touchpad-Shield/releases/latest) | 笔记本触控板物理尺寸预设（仓库内见 [`config/`](config/)） |

> **说明：** 安装包使用自签名 Authenticode 证书（发布者 **ZiMiaoWorkshop**）。首次安装时 Windows SmartScreen 可能提示「未知发布者」。

---

## 功能概览

- **单击灵敏度** — `ClickForceSensitivity`，支持自由调节或与 Windows 对齐的三档吸附（0 / 50 / 100）
- **触板灵敏度** — `AAPThreshold`（4 档）
- **轻拍单击** — `TapsEnabled` 开关
- **缓冲区域（Smart Area）** — 四边毫米级缓冲区（`CurtainTop/Bottom/Left/Right`）
- **防误触区域（Smart Edge）** — 四边防误触区（`SuperCurtainTop/Bottom/Left/Right`）
- **实时示意图** — 黄色（缓冲）与红色（防误触）叠加，按 DPI 缩放
- **重叠提示** — 任一边 Super Curtain ≥ Curtain 时显示两行紧凑警告
- **机型尺寸匹配** — 通过 `TouchpadPhysicalSize.csv` 与 BIOS 信息匹配；可应用预设或导出自定义条目
- **即时生效 vs 重启** — HKCU 项经 `SystemParametersInfo` 即时生效；HKLM Curtain 变更需重启
- **原生 Windows UI** — WinUI 3、跟随系统主题、PerMonitorV2 DPI、中英双语标签
- **外接输入设备自动启停触控板** — 监控所选输入设备容器（与 **设置 → 蓝牙和其他设备 → 设备 → 输入** 同源）；任一监控设备在线时关闭内置触控板，全部离线后恢复（经 `Status\Enabled` + `Ctrl+Win+F24`）
- **系统托盘与开机自启** — 可选关闭时最小化到托盘、登录时运行；启用自动启停时会强制开启上述两项

---

## 外接输入设备与触控板开关

此功能与上文 PTP 调参键**独立**，切换的是 Windows 触控板总开关（自 **v1.1.0** 引入；自启/单实例行为以 **v1.1.1** 为准）：

| 项目 | 说明 |
|------|------|
| 注册表读取 | `HKCU\...\PrecisionTouchPad\Status\Enabled` |
| 切换方式 | `SendInput`：`Ctrl+Win+F24`（系统切换键） |
| 设备检测 | `PnpObjectWatcher` 监听 `DeviceContainer`（非轮询） |
| 设备列表 | 所有输入类容器（已配对或已连接）；排除内置触控板 |
| 匹配键 | 设备容器 ID（`ContainerId`） |
| 持久化 | `Software\ZiMiaoWorkshop\TouchpadShield`（见下表） |

启用触控板自动启停时，**登录时运行**与**关闭时最小化到托盘**会被强制开启，以保证后台能处理插拔事件。

**注册表键（每 Windows 用户，HKCU `Software\ZiMiaoWorkshop\TouchpadShield`）：**

| 键名 | 用途 |
|------|------|
| `InputAutoTouchpadEnabled` | 触控板自动启停开/关 |
| `MonitoredInputDevices` | JSON：`containerId`、`label`、可选 `matchKey` |
| `RunAtStartup` | 登录时运行（计划任务） |
| `MinimizeToTrayOnClose` | 关闭窗口时最小化到托盘 |

（已废弃）`AutostartHandledSessionId` — v1.1.0 内部键；**v1.1.1 起**不再读写，启动时 best-effort 删除。

**登录自启与单实例：** 计划任务 `\TouchpadShield`，登录触发器绑定当前用户 SAM、`RunLevel=Highest`，参数 `"<exe>" --startup`；同时移除遗留 HKCU Run 项。各 Windows 用户设置与任务独立。

| 场景 | 行为 |
|------|------|
| 首次 `--startup`（本 Session 无实例） | 静默进托盘，初始化设备监听 |
| 重复 `--startup`（本 Session 已有实例） | 第二进程静默退出 |
| 手动再次打开 exe | `PostMessage(ShowMainWindow)` → `ShowFromTray()` 显示已有实例（不启动第二进程） |

单实例 Mutex：`Local\TouchpadShield_SingleInstance_v2`（每 Session 独立；多用户各 Session 可各跑一份）。

**PnP 监听（有意设计）：** 应用进程运行期间，即使关闭「启用自动启停」，`PnpObjectWatcher` 仍保持注册；仅 F24 切换与 reconcile 受 `InputAutoTouchpadEnabled` 控制，插拔仍会刷新设备列表以便编辑监控列表。关闭自动启停时，若触控板仍被关着，会发送 F24 尝试恢复。

---

## 应用内 PTP 参数对照

| 界面（中文 / EN） | 注册表键 | 作用域 |
|-------------------|----------|--------|
| 单击灵敏度 / Click Sensitivity | `ClickForceSensitivity` | HKCU |
| 触板灵敏度 / Touchpad Sensitivity | `AAPThreshold` | HKCU |
| 轻拍单击 / Tap to click | `TapsEnabled` | HKCU |
| 缓冲区域 / Curtains (Smart Area) | `CurtainTop/Bottom/Left/Right` | HKLM |
| 防误触区域 / Super Curtains (Smart Edge) | `SuperCurtainTop/Bottom/Left/Right` | HKLM |
| 触控板总开关 / Touchpad on-off | `Status\Enabled` | HKCU（读取 + F24 切换，不直接写入） |

注册表路径：`HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\PrecisionTouchPad`（Curtains 在 HKLM）。

---

## 系统要求

| 项目 | 要求 |
|------|------|
| 操作系统 | Windows 10 1809（内部版本 **17763**）或更高 |
| 架构 | **x64** |
| 权限 | **管理员（UAC）** — 写入 HKLM Curtain / SuperCurtain 需要 |
| 编译环境 | Visual Studio 2022 · **使用 C++ 的桌面开发** · **Windows App SDK** |

---

## 从源码构建

克隆仓库后，在项目根目录执行：

```powershell
# Debug 构建 — 输出：Touchpad Shield App/debug/（版本展示含 (alpha)）
.\scripts\build-debug.ps1

# Beta 安装包（Debug 应用 + NSIS）— 输出：Touchpad Shield App/beta/（含 (beta)）
.\scripts\build-beta.ps1

# Release 安装包 — 输出：Touchpad Shield App/release/（无渠道后缀）
.\scripts\build-release.ps1

# 发布 GitHub Release（需先 gh auth login）
.\scripts\publish-github-release.ps1
```

首次运行会下载 NuGet 包（Windows App SDK 1.6、CppWinRT 2.0），并可能创建自签名代码签名证书。

本地直接运行（无需安装）：

```text
Touchpad Shield App\debug\TouchpadShield.exe
```

---

## 触控板尺寸配置

权威文件：[`config/TouchpadPhysicalSize.csv`](config/TouchpadPhysicalSize.csv)

```csv
SystemManufacturer,SystemProductName,SystemSKU,SystemVersion,TouchpadWidth,TouchpadHeight
```

每次构建时，MSBuild 与 `build-debug.ps1` 会将该文件同步到可执行文件旁的 `{output}/config/`。

---

## 架构概览

```text
┌─────────────────────────────────────┐
│  表现层 (XAML)                      │  MainWindow、StartupWindow、App.xaml
├─────────────────────────────────────┤
│  应用层 (MainWindow.xaml.cpp)       │  UI 事件、LoadAllData、编排
├─────────────────────────────────────┤
│  服务层 (Services)                  │  注册表、SPI、BIOS、CSV、DPI、示意图
└─────────────────────────────────────┘
```

模块级说明见 [`PRD/Touchpad_Shield_开发指导.md`](PRD/Touchpad_Shield_开发指导.md)，含 **§6.1 代码维护约定**（有意保留的设计与已完成清理项）。

---

## 项目结构

```text
Touchpad Shield/
├── config/                 # TouchpadPhysicalSize.csv
├── installer/              # NSIS 脚本
├── PRD/                    # 产品与开发文档（中文）
├── scripts/                # 构建、签名、版本、发布脚本
├── src/TouchpadShield/     # WinUI 3 应用（C++/WinRT）
├── Touchpad Shield App/    # 构建产物（gitignore；仅 .gitkeep）
└── version/                # Version.props、build stamp
```

---

## 版本号规则

| 部分 | 规则 |
|------|------|
| 语义化版本 `MAJOR.MINOR.PATCH` | 人工维护 — `version/Version.props` |
| 构建号 `BUILD`（4 位） | 源码指纹变动时自动递增 — `scripts/bump-build.ps1` |
| 界面展示 | `1.1.1 build 0108`；debug 追加 ` (alpha)`，beta 追加 ` (beta)`，release 无后缀 |

仅文档变更（如 README）**不会**递增构建号。

---

## 文档索引

| 文档 | 说明 |
|------|------|
| [`PRD/Touchpad_Shield_开发指导.md`](PRD/Touchpad_Shield_开发指导.md) | 开发指导 — UI 规范、架构、构建规则、**§6.1 维护约定**（**v1.1.1 build 0108**） |
| [`PRD/Touchpad_Shield_v1.0.0_to_v1.1.0_变更说明.md`](PRD/Touchpad_Shield_v1.0.0_to_v1.1.0_变更说明.md) | **v1.0.0 → v1.1.0** 功能变更 + **§七附 v1.1.1 patch** |
| [`.cursor/rules/touchpad-shield-build.mdc`](.cursor/rules/touchpad-shield-build.mdc) | 自动化工具构建规范 |
| [`.cursor/rules/touchpad-shield-code.mdc`](.cursor/rules/touchpad-shield-code.mdc) | 代码维护 — 勿重复提议的重构项 |

---

## 技术栈

- **UI：** WinUI 3（unpackaged）
- **语言：** C++ / C++/WinRT（C++20）
- **安装包：** NSIS（简体中文 MUI）
- **依赖：** Windows App SDK 1.6、CppWinRT 2.0

---

## 许可证

Copyright 2026 ZiMiaoWorkshop

基于 [Apache License, Version 2.0](LICENSE) 授权。详见 [http://www.apache.org/licenses/LICENSE-2.0](http://www.apache.org/licenses/LICENSE-2.0)。
