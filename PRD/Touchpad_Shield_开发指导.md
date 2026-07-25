# Touchpad Shield 开发指导

> 本文档基于当前代码库（**v1.0.0 build 0025**）编写，用于指导开发、构建与需求变更。  
> 最初产品需求见同目录 [`Touchpad_Shield_PRD.md`](Touchpad_Shield_PRD.md)。  
> 构建规范以 [`.cursor/rules/touchpad-shield-build.mdc`](../.cursor/rules/touchpad-shield-build.mdc) 为准，本文第四节与之保持一致并展开说明。

---

## 一、产品设计目的简述

本软件名称为 **Touchpad Shield**，作用是通过调整笔记本电脑触控板的 Click Sensitivity、Touchpad Sensitivity、Tap with a single finger to single-click、Curtains 以及 Super Curtains 的数值，来减少用户在使用笔记本电脑键盘打字时，因手掌、手腕、大拇指误触触控板而造成的鼠标光标漂移以及误击现象。

本软件严格遵循微软 Windows 系统 Precision Touchpad Devices 中关于 Precision touchpad tuning 的相关优化指南进行开发，参考链接：

[https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/touchpad-tuning-guidelines](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/touchpad-tuning-guidelines)

---

## 二、技术栈

| 层级 | 选型 |
|------|------|
| UI 框架 | WinUI 3（**unpackaged**，`WindowsPackageType=None`） |
| 语言 | C++ / C++/WinRT（C++20） |
| 平台 | x64，最低 Windows 10 **17763** |
| 依赖 | Windows App SDK **1.6**、CppWinRT **2.0** |
| 安装包 | NSIS（简体中文 MUI） |
| 构建 | MSBuild + PowerShell 脚本 |
| 版本控制 | 本地 Git（`main` 主干） |

### 2.1 项目目录结构（摘要）

```text
Touchpad Shield/
├── config/                          # 触控板物理尺寸 CSV（唯一权威编辑位置）
│   └── TouchpadPhysicalSize.csv
├── installer/                       # NSIS 脚本与安装器图标
├── Picture/                         # 品牌 LOGO 源文件
├── PRD/                             # 产品需求与开发指导
├── scripts/                         # 构建、签名、版本、图标脚本
├── src/TouchpadShield/              # WinUI 3 应用源码
│   ├── Services/                    # 业务服务层
│   ├── Assets/                      # 运行时图标资源
│   └── MainWindow.xaml / App.xaml …
├── Touchpad Shield App/             # 编译产物（debug / beta / release）
├── version/                         # Version.props、构建号指纹
└── TouchpadShield.sln
```

---

## 三、具体需求

### 1、UI 风格与缩放

（1）前端 UI 样式采用与「Windows 系统设置」一致的风格。界面上主要文字的样式（包括但不限于字体、字号）以及 UI 界面颜色样式，要直接跟随用户在 Windows 操作系统中的设定；各控件的样式以及尺寸也要在 WinUI 3 原生状态下尽量与 Windows 系统设置界面上的控件保持接近。

（2）前端界面必须严格遵循最新 Windows 系统应用的开发要求，必须正确支持 Windows 的缩放功能：
- 应用清单启用 `PerMonitorV2` DPI 感知（`app.manifest`）；
- 窗口最小尺寸、初始客户区尺寸按**逻辑像素**定义，在高 DPI 下通过 `GetDpiForWindow` + `WM_GETMINMAXINFO` 子类化换算物理像素（`WindowBoundsHelper`）；
- 触控板示意图按窗口所在显示器的 **有效 DPI 与显示器逻辑宽度** 估算 `mm/DIP`（`DisplayScaleService`），尽量接近真实物理尺寸渲染；**当前实现未直接读取 EDID**，而是通过 `GetDpiForWindow` / `GetDpiForMonitor` 与 `MONITORINFO` 推算。

（3）区域标题采用**中英双语**格式，例如：`灵敏度设定 (Sensitivity)`、`屏蔽区域设定 (Curtains / Super Curtains)`。

（4）全局样式（`App.xaml`）：
- `MmNumberBoxFieldStyle`：NumberBox 隐藏 spin 按钮，右侧预留 mm 单位显示空间；
- `MmNumberBoxUnitStyle`：输入框内右侧显示 `mm` 单位；
- `RightAlignedToggleSwitchStyle`：ToggleSwitch 右对齐，开关列固定宽度 76px。

---

### 2、页面布局与交互

#### 2.1 专用名词中英文对应关系

| 中文 | 英文 | Windows 系统设置对应 | PTP 键值 |
|------|------|---------------------|----------|
| 灵敏度 | Sensitivity | — | — |
| 单击灵敏度 | Click Sensitivity | 触控板 → 单击灵敏度 | `ClickForceSensitivity` |
| 触板灵敏度 | Touchpad Sensitivity | 触控板 → 触控板灵敏度 | `AAPThreshold` |
| 轻拍触控板进行单击 | Tap with a single finger to single-click | 触控板 → 使用单个手指点击即可单击 | `TapsEnabled` |
| 缓冲区域 | Smart Area | Curtains（屏蔽区域） | `CurtainTop/Bottom/Left/Right` |
| 防误触区域 | Smart Edge | Super Curtains（超级屏蔽区域） | `SuperCurtainTop/Bottom/Left/Right` |

#### 2.2 整体布局（上、中、下三区）

页面主要分成上、中、下三个区域：

| 区域 | 内容 |
|------|------|
| **顶部** | 软件名称「Touchpad Shield」、**刷新数据**按钮、右侧超链接 **更多触控板功能设定**（打开 `ms-settings:devices-touchpad`） |
| **中部** | 左右两栏（列宽比 **4:6**）。左栏：灵敏度设定 + 屏蔽区域设定；右栏：笔记本型号、触控板物理尺寸、触控板示意图 |
| **底部** | 左侧（条件显示）：屏蔽区域变更重启提示 + **重启**按钮；右侧：作者信息 + 版本号 |

**窗口与间距：**
- 外层 Grid：`Padding="24,24,24,12"`（底部 12px，其余 24px），`RowSpacing="16"`，`ColumnSpacing="24"`；
- 底部栏固定 `MinHeight="38"`，避免重启提示出现/消失时作者信息与版本号位置跳动。

> **与最初 PRD 的布局差异：** 最初 PRD 将「更多触控板功能设定」放在中左下区域；当前实现已移至**页面顶部右侧**（见第六节对照表）。

#### 2.3 启动流程

（1）软件启动时必须请求 UAC 权限（`app.manifest` 设置 `requireAdministrator`），以正常写入 HKLM 下的屏蔽区域与超级屏蔽区域数值。

（2）启动时显示 **StartupWindow**（ProgressRing +「程序正在启动」），后台完成 `MainWindow.InitializeAsync()` → `LoadAllData()` 后关闭 StartupWindow 并激活主窗口。

（3）启动时应检查注册表中是否有缺失的屏蔽区域以及超级屏蔽区域键值（HKLM），若有则自动补全并将补全值设为 0。补全完成后读取对应数据并反显在控件内，并绘制示意图。

（4）提权后写入 HKCU 类注册表时，通过 `RegistryUserContext::RunAsInteractiveUser` 模拟交互用户上下文执行。

（5）Click/AAP/Taps 等用户设置读写策略（**对称设计**）：
- **读取**：优先 `SystemParametersInfo(SPI_GETTOUCHPADPARAMETERS)`，失败再读 HKCU 注册表；
- **写入**：优先 `SystemParametersInfo(SPI_SETTOUCHPADPARAMETERS)` 即时生效，失败时回退直接写 HKCU 注册表并调用 `NotifyPrecisionTouchPadSettingsChanged()` 广播 `WM_SETTINGCHANGE`。

（6）应用重新启动后，Curtain 重启提示状态重置（`m_curtainRestartPending` 默认 false）。

#### 2.4 中左区域 — 灵敏度设定 (Sensitivity)

从上往下共 4 个功能：

**（1）单击灵敏度 (Click Sensitivity)**
- 滑块控件，表值 `ClickForceSensitivity`（0–100）；
- 采用**方案二（吸附到指定刻度）**：滑块始终 101 档（0–100），在「与 Windows 系统设置保持一致」模式下吸附到 0、50、100，显示「轻 0 / 中 50 / 重 100」；在「自由调节」模式下可自由拖动，仅 0/50/100 处显示文字；
- 滑块右侧显示当前数值标签（`ClickSensitivityValueText`）。

**（2）单击灵敏度控制方式 (Click Sensitivity Control Mode)**
- 下拉框：`与 Windows 系统设置保持一致` / `自由调节`；
- 模式持久化于本地设置（`Software\ZiMiaoWorkshop\TouchpadShield` → `ClickSensitivityMode`）。

**（3）触板灵敏度 (Touchpad Sensitivity)**
- 下拉框，表值 `AAPThreshold`：0 最高 / 1 高 / 2 中 / 3 低。

**（4）轻拍触控板进行单击 (Tap with a single finger to single-click)**
- ToggleSwitch，表值 `TapsEnabled`（1=开，0=关）。

> 以上 HKCU 类设置修改后**即时生效**，不触发重启提示。

#### 2.5 中左区域 — 屏蔽区域设定 (Curtains / Super Curtains)

（1）从上往下：缓冲区域 (Smart Area) 开关 + 上/下/左/右 4 个 NumberBox；防误触区域 (Smart Edge) 开关 + 上/下/左/右 4 个 NumberBox。

（2）8 个输入框单位为毫米（mm），小数点后限定 2 位；写入注册表时转换为 Himetric（1 mm = 100 Himetric，`UnitConversion`）。

（3）开关为「关」时：对应 4 边数值自动置 0，输入框禁用，并**立即写入 HKLM**；开关为「开」时：从 HKLM **读回**当前 Curtain/SuperCurtain 值并启用输入框。启动时若任一边数值 > 0，则对应开关初始为「开」。

（4）修改 Curtain / SuperCurtain 并成功写入 HKLM 后，底部显示重启提示（见 2.9）。

#### 2.6 中右区域 — 笔记本电脑型号 (Laptop Model)

（1）从 `HKLM\HARDWARE\DESCRIPTION\System\BIOS` 读取 `SystemManufacturer`、`SystemProductName`、`SystemSKU`、`SystemVersion`，组合格式为：

`Manufacturer - ProductName - SKU - Version`

空字段及其前面的 ` - ` 一并隐藏（`BiosService::DisplayName`）。

（2）**导出当前触控板物理尺寸设定**（Accent 按钮，条件显示）：
- CSV 不存在、或无匹配、或当前尺寸与匹配/已有记录不同时显示；
- 点击后将当前 BIOS 身份 + 触控板宽/高 Upsert 到 `TouchpadPhysicalSize.csv`（`CsvConfigService::UpsertEntry`）；
- 成功后弹出 ContentDialog 显示写入路径。

> **与最初 PRD 的差异：** 导出功能为后续实现新增，最初 PRD 未要求。

（3）**配置文件中匹配到的触控板物理尺寸**（条件显示）：
- 以四字段 BIOS 身份为联合主键，在 `config/TouchpadPhysicalSize.csv` 中精确匹配；
- 匹配到且与当前 APP 内尺寸不一致时，显示文案：`配置文件中匹配到的触控板物理尺寸：X mm × Y mm`；
- 同步显示 **应用预设的触控板物理尺寸** 按钮（Accent 样式），点击后将匹配尺寸写入宽/高输入框并保存到本地设置。

#### 2.7 中右区域 — 触控板物理尺寸 (Touchpad Physical Size)

（1）横向宽度 (Width)、纵向高度 (Height) 两个 NumberBox，单位 mm，最小值 1。

（2）启动时读取本地保存数据（`LocalSettingsService`）；若无保存，默认使用 PTP 要求的最小尺寸 **65 mm × 40 mm**（`UnitConversion.h` 常量）。

（3）修改后自动保存到 `Software\ZiMiaoWorkshop\TouchpadShield`，并刷新示意图与匹配/导出按钮状态。

（4）**当前未实现**从 Windows 系统自动读取触控板物理尺寸；尺寸来源为：本地设置 → 默认 65×40 → CSV 匹配/手动应用。

#### 2.8 中右区域 — 触控板示意图 (Touchpad Diagram)

（1）Canvas 高度 **380px**，按最终确定的触控板物理尺寸绘制触控板矩形。

（2）比例依据：`DisplayScaleService` 读取窗口所在显示器的 DPI 与逻辑宽度，计算 `mmPerDip` 传入 `TouchpadDiagramRenderer`；仅当超出 Canvas 可用区域时才等比缩小（margin 8px，居中绘制）。

（3）图示叠加：
- 缓冲区域 (Smart Area)：半透明黄色 `#FFD700`（opacity 0.45）；
- 防误触区域 (Smart Edge)：半透明红色 `#DC143C`（opacity 0.45）；
- 底部显示图例。

（4）校验：若任一边 SuperCurtain ≥ 对应 Curtain 且两者均 > 0，显示红色警告文案，说明该区域将完全以防误触区域逻辑控制。

（5）Canvas `SizeChanged` 或相关参数变更时自动重绘。

#### 2.9 底部区域

（1）修改 Curtain / SuperCurtain 并成功写入 HKLM 后，显示：
- 文案：「屏蔽区域设定已更改，需要重启电脑后生效。」
- **重启**按钮：弹出确认对话框（取消 / 立即重启）后调用 `ExitWindowsEx(EWX_REBOOT)`。

（2）Click/AAP/Taps 等 HKCU 设置修改后**即时生效**，不触发重启提示。

（3）右侧固定显示：
- `Designed and Built by ZiMiaoWorkshop`
- 版本号：`v{MAJOR.MINOR.PATCH build BUILD}`（例如 `v1.0.0 build 0025`）

#### 2.10 窗口尺寸

软件最小窗口尺寸限定为 **100% 缩放下 1280×900**（逻辑像素），通过 XAML `MinWidth/MinHeight` 与 `WindowBoundsHelper` 的 `WM_GETMINMAXINFO` 双重约束；首次 `Activated` 时将客户区调整为 1280×900 逻辑尺寸。

> **与最初 PRD 的差异：** 最初 PRD 要求最小窗口 **1280×720**；当前为 **1280×900**（见第六节）。

#### 2.11 刷新数据

顶部 **刷新数据** 按钮调用 `LoadAllData()`，重新读取注册表、BIOS、CSV 匹配、本地设置，并刷新 UI 与示意图。不会重置 Curtain 重启提示状态（除非重新加载后再次写入 Curtain）。

---

### 3、配置文件及接口设计

#### 3.1 触控板物理尺寸 CSV

| 项 | 说明 |
|----|------|
| **文件名** | `TouchpadPhysicalSize.csv` |
| **权威编辑路径** | 项目根目录 `config/TouchpadPhysicalSize.csv` |
| **运行时路径** | `{exe 目录}/config/TouchpadPhysicalSize.csv` |
| **表头** | `SystemManufacturer,SystemProductName,SystemSKU,SystemVersion,TouchpadWidth,TouchpadHeight` |
| **匹配规则** | 四字段 BIOS 身份完全一致 |
| **Upsert** | 同身份覆盖更新，保留其他行；尺寸格式化为两位小数 |

**编译时 config 同步（强制）：**

为确保维护者只需更新 `config/` 目录，终端用户自行编译也能获得最新机型数据，当前采用**双重保障**：

1. **MSBuild**（`TouchpadShield.vcxproj`）  
   - `CopyConfigFile` 设为 `InitialTargets`；  
   - 通过 `Inputs` / `Outputs` 跟踪 `config/TouchpadPhysicalSize.csv` → `$(OutDir)config/`；  
   - config 文件变更时即使 C++ 源码未改也会触发复制。

2. **构建脚本**（`scripts/build-debug.ps1`）  
   - MSBuild 完成后**强制**从 `config/TouchpadPhysicalSize.csv` 复制到输出目录；  
   - 源文件不存在则**构建失败并报错**。

Beta / Release 均调用 `build-debug.ps1`，因此安装包内 config 与项目 `config/` 保持一致（在对应 Configuration 编译完成后）。

#### 3.2 注册表路径与接口命名

**PTP 用户设置（HKCU）**  
路径：`SOFTWARE\Microsoft\Windows\CurrentVersion\PrecisionTouchPad`

| 键值 | 接口（`RegistryService`） |
|------|--------------------------|
| `ClickForceSensitivity` | `APP_ClickForceSensitivity` / `APP_SetClickForceSensitivity` |
| `AAPThreshold` | `APP_AAPThreshold` / `APP_SetAAPThreshold` |
| `TapsEnabled` | `APP_TapsEnabled` / `APP_SetTapsEnabled` |

底层 SPI 封装见 `TouchpadParametersService`（`SPI_GET/SETTOUCHPADPARAMETERS`）。

**PTP 系统设置（HKLM，需管理员）**  
同路径：

| 键值 | 接口 |
|------|------|
| `CurtainTop/Bottom/Left/Right` | `APP_CurtainMm` / `APP_SetCurtainMm` |
| `SuperCurtainTop/Bottom/Left/Right` | `APP_SuperCurtainMm` / `APP_SetSuperCurtainMm` |

**应用本地设置（HKCU）**  
路径：`Software\ZiMiaoWorkshop\TouchpadShield`

| 键值 | 内容 |
|------|------|
| `TouchpadWidthMm` / `TouchpadHeightMm` | 触控板尺寸（REG_SZ，两位小数 mm） |
| `ClickSensitivityMode` | `FreeAdjust` 或 `MatchWindowsSettings` |

接口命名原则：针对 PTP 调优指南涉及的注册表键值，接口名称尽量直接体现键值名称。

#### 3.3 Debug 日志

Debug 构建定义 `TOUCHPAD_SHIELD_DEBUG`，日志写入：

`%LocalAppData%\TouchpadShield\logs\touchpad-shield.log`

Release 构建不写入文件日志（`Logger` 在 Release 下为空操作）。

---

### 4、品牌资源

`Picture/` 文件夹中放置：
- **Touchpad Shield LOGO**（PNG）→ `scripts/generate-icons.ps1` 生成 `Assets/TouchpadShield.ico` 及安装程序图标；
- **ZiMiaoWorkshop LOGO**（JPG）→ `scripts/generate-installer-images.ps1` 生成 NSIS 安装向导侧边图（`installer/welcome-finish.bmp`，164×314）设计参考。

图标缩放需注意精度与色深，确保 icon 与安装图片清晰。`welcome-finish.bmp` 为构建时生成物，已加入 `.gitignore`。

---

### 5、代码签名与 UAC 发布者

（1）编译成功后对可执行文件进行 Authenticode 签名（`scripts/sign-exe.ps1`）。

（2）使用主题名为 `CN=ZiMiaoWorkshop` 的代码签名证书；首次构建时自动创建自签证书并注册到当前用户 `TrustedPublisher`。

（3）UAC 弹窗「发布者」应显示 **ZiMiaoWorkshop**。正式对外分发时建议使用商业代码签名证书以获得 SmartScreen 信任。

（4）Windows 资源（`TouchpadShield.rc`）中 `CompanyName`、`LegalCopyright` 均为 ZiMiaoWorkshop。

---

## 四、版本管理及编译要求

> 本节与 `.cursor/rules/touchpad-shield-build.mdc` 保持一致，作为强制执行规范。

### 1、版本号

| 项 | 规则 |
|----|------|
| 语义化版本 | `MAJOR.MINOR.PATCH`，人工维护于 `version/Version.props` |
| 构建号 | 4 位数字 `BUILD`，**源码变动时自动递增**（`scripts/bump-build.ps1`），不在 CI/CD 空跑时递增 |
| UI 展示格式 | `MAJOR.MINOR.PATCH build BUILD`（例如 `1.0.0 build 0025`） |
| Manifest | `assemblyIdentity` 使用四段数字 `MAJOR.MINOR.PATCH.buildInt`（例如 `1.0.0.25`） |
| 同步 | `scripts/sync-version.ps1` 将版本同步至 NSIS 安装脚本 |

**构建号指纹范围：** `src/`、`scripts/`、`installer/`、`config/`、`Picture/`、`TouchpadShield.sln`、`version/Version.props`、`version/Version.targets`（排除 `build-stamp.json`、`.build-pending.json` 及 NSIS/图标等衍生产物）。

**构建号流程：** `bump-build.ps1 -Phase Prepare`（编译前计算指纹、必要时递增）→ 编译 → 成功则 `Finalize`，失败则 `Revert` 回滚 Version.props。

### 2、产物目录

| 目录 | 内容 |
|------|------|
| `Touchpad Shield App/debug/` | Debug 可执行文件及依赖（含 `config/`、`Assets/`），启用 debug 日志 |
| `Touchpad Shield App/beta/` | NSIS 打包的 Debug 版安装包（`*-beta-setup.exe`） |
| `Touchpad Shield App/release/` | 正式发布 NSIS 安装包（`*-setup.exe`）及 `release/app/` Release 应用文件 |

> **与最初 PRD 的差异：** 最初 PRD 要求项目根下 `debug/`、`beta/`、`release/` 三个文件夹；当前统一为 **`Touchpad Shield App/`** 子目录（见第六节）。

### 3、编译策略

| 触发条件 | 动作 |
|----------|------|
| 每次修改源码 | 自动编译 **debug** 版本，输出到 `Touchpad Shield App/debug/` |
| 用户明确指令 | 编译 **beta**（`scripts/build-beta.ps1`）或 **release**（`scripts/build-release.ps1`） |

**Beta vs Release：**

| | Beta | Release |
|---|------|---------|
| 应用构建配置 | Debug（含日志） | Release（无 debug 日志） |
| NSIS 打包源 | `Touchpad Shield App/debug/*` | `Touchpad Shield App/release/app/*` |
| 安装包命名 | `TouchpadShield-{semver}-build{BUILD}-beta-setup.exe` | `TouchpadShield-{semver}-build{BUILD}-setup.exe` |
| 签名对象 | `debug/TouchpadShield.exe` | `release/app/TouchpadShield.exe` |

**build-debug.ps1 主要步骤：**
1. 确保 NuGet 包（Windows App SDK、CppWinRT）  
2. `generate-icons.ps1`  
3. `bump-build.ps1 -Phase Prepare` + `sync-version.ps1`  
4. 若 TouchpadShield.exe 正在运行则终止进程  
5. MSBuild（Debug 或 Release）  
6. **强制同步 `config/TouchpadPhysicalSize.csv` 到输出目录**  
7. `bump-build.ps1 -Phase Finalize`  
8. `sign-exe.ps1` 签名  

### 4、构建脚本一览

| 脚本 | 用途 |
|------|------|
| `scripts/build-debug.ps1` | 主构建：NuGet、图标、构建号 bump、MSBuild、config 同步、签名 |
| `scripts/build-beta.ps1` | Debug 构建 + 生成安装器图片 + Beta NSIS 安装包 |
| `scripts/build-release.ps1` | Release 构建 + 生成安装器图片 + Release NSIS 安装包 |
| `scripts/bump-build.ps1` | 源码 SHA256 指纹对比，变动时 BUILD+1；编译失败回滚 |
| `scripts/sync-version.ps1` | 同步版本号至 NSIS 脚本中的 `APP_BUILD` / `APP_VERSION` |
| `scripts/sign-exe.ps1` | Authenticode 自签（CN=ZiMiaoWorkshop） |
| `scripts/generate-icons.ps1` | LOGO → 多尺寸 ICO |
| `scripts/generate-installer-images.ps1` | 安装向导侧边图 |
| `scripts/patch-app-manifest.ps1` | 写入 manifest 四段版本 |

### 5、安装包行为（NSIS）

- 安装目录：`$PROGRAMFILES64\Touchpad Shield`
- 请求管理员权限（`RequestExecutionLevel admin`）
- 安装内容：递归复制 debug 或 release/app 目录（**含 `config/`、`Assets/`、WinAppSDK 运行时依赖**）
- 快捷方式：桌面 + 开始菜单 `Programs\ZiMiaoWorkshop\Touchpad Shield`
- 卸载信息注册于 `HKLM\...\Uninstall\Touchpad Shield`，Publisher 为 ZiMiaoWorkshop

### 6、需求变更原则

需求理解阶段有任何疑问必须与产品负责人确认，**不可自行决定解决方案**。

---

## 五、核心模块与源码结构

| 模块 | 路径 | 职责 |
|------|------|------|
| 主窗口 UI | `MainWindow.xaml(.cpp/.h)` | 布局、事件、LoadAllData、示意图刷新 |
| 启动窗口 | `StartupWindow.xaml(.cpp/.h)` | 启动等待 UI |
| 注册表服务 | `Services/RegistryService.*` | PTP HKCU/HKLM 读写、Curtain 键补全 |
| SPI 服务 | `Services/TouchpadParametersService.*` | SPI_GET/SETTOUCHPADPARAMETERS |
| 用户上下文 | `Services/RegistryUserContext.*` | 提权进程下模拟交互用户写 HKCU |
| BIOS | `Services/BiosService.*` | 读取四字段身份并格式化显示名 |
| CSV | `Services/CsvConfigService.*` | 匹配、Upsert、CSV 解析 |
| 本地设置 | `Services/LocalSettingsService.*` | 触控板尺寸、单击模式持久化 |
| 显示缩放 | `Services/DisplayScaleService.*` | mm/DIP 估算 |
| 示意图 | `Services/TouchpadDiagramRenderer.*` | Canvas 绘制与 SuperCurtain 警告 |
| 单位换算 | `Services/UnitConversion.*` | mm ↔ Himetric、尺寸比较 |
| 窗口边界 | `Services/WindowBoundsHelper.*` | 最小尺寸、初始客户区、DPI 换算 |
| 窗口图标 | `Services/WindowIconHelper.*` | 从 Assets 或嵌入资源加载 ICO |
| 日志 | `Services/Logger.*` | Debug 文件日志 |

**Git 版本库：** 跟踪源码、脚本、PRD、`config/`、安装器脚本等；忽略 `packages/`、`obj/`、`Generated Files/`、构建产物目录内容（保留 `.gitkeep`）。

---

## 六、与最初 PRD（`Touchpad_Shield_PRD.md`）的差异对照

下表对比**最初 PRD 原文要求**与**当前 v1.0.0 实现**。「一致」表示已按 PRD 实现；「变更/新增/未实现」需特别关注。

| 编号 | 最初 PRD 要求 | 当前实现 | 类别 |
|------|--------------|----------|------|
| 1 | 技术栈：WinUI 3 + C++ + NSIS，可提更好方案 | 确定为 WinUI 3 **unpackaged** + C++/WinRT + NSIS + PowerShell 构建链 | **细化** |
| 2 | UI 跟随系统主题与缩放 | 已实现 PerMonitorV2、WindowBoundsHelper、DisplayScaleService | 一致 |
| 3 | 区域标题 | 增加**中英双语**区域标题（PRD 未明确要求格式） | **新增** |
| 4 | 「更多触控板功能设定」在中左下区域 | 移至**页面顶部右侧** HyperlinkButton | **变更** |
| 5 | 单击灵敏度：方案一（改刻度）或方案二（吸附），推荐方案二 | 已实现**方案二（吸附 0/50/100）** | 一致 |
| 6 | 触板灵敏度 4 档下拉 | 已实现 | 一致 |
| 7 | Curtain/SuperCurtain 开关与 mm 输入、关=置 0 并禁用 | 已实现；**开=从 HKLM 读回**再启用编辑 | **增强** |
| 8 | 启动 UAC + 缺失 Curtain 键补 0 + 反显 + 绘图 | 已实现 | 一致 |
| 9 | 最小窗口 **1280×720** | 最小窗口 **1280×900**，初始客户区同为 1280×900 | **变更** |
| 10 | 示意图：结合 **EDID** 物理尺寸、分辨率、缩放比 | 使用 **DPI + 显示器逻辑宽度** 估算 mm/DIP，**未直接读 EDID** | **变更** |
| 11 | 默认触控板尺寸：PTP 最小尺寸（未写具体数值） | 明确默认 **65 mm × 40 mm** | **细化** |
| 12 | CSV 匹配与应用预设按钮 | 已实现；匹配文案为「**配置文件中匹配到的触控板物理尺寸**」 | **细化** |
| 13 | CSV 表结构六字段 | 一致；部署路径明确为 `{exe}/config/` | **细化** |
| 14 | 导出触控板尺寸到 CSV | **已实现「导出当前触控板物理尺寸设定」** | **新增** |
| 15 | 刷新数据按钮 | PRD 顶部有刷新；当前为 **「刷新数据」** 按钮，可重载全部数据 | 一致 |
| 16 | Curtain 修改后重启提示 | PRD **未要求**；当前修改 HKLM Curtain 后显示重启栏 | **新增** |
| 17 | HKCU 设置即时生效（SPI） | PRD **未要求**；当前优先 SPI，失败回退注册表 + 广播 | **新增** |
| 18 | 提权下 HKCU 写交互用户上下文 | PRD **未要求**；`RegistryUserContext` | **新增** |
| 19 | StartupWindow 启动等待 | PRD **未要求** | **新增** |
| 20 | 代码签名与 UAC 发布者 ZiMiaoWorkshop | PRD **未要求** | **新增** |
| 21 | 版本格式 `MAJOR.MINOR.PATCH[-prerelease][+BUILD]` | 展示为 `MAJOR.MINOR.PATCH build BUILD`（空格 + 4 位 BUILD）；BUILD 独立维护 | **变更** |
| 22 | 产物目录：根目录 `debug/`、`beta/`、`release/` | **`Touchpad Shield App/debug|beta|release/`** | **变更** |
| 23 | 构建号：源码变动时递增，非每次 CI 空跑 | `bump-build.ps1` 指纹 + `build-stamp.json` | 一致 |
| 24 | 改源码自动编 debug；beta/release 听指令 | 与 `.cursor/rules` 一致 | 一致 |
| 25 | config 部署与编译同步 | PRD 仅描述表结构；当前 **MSBuild InitialTargets + 脚本强制复制** | **新增** |
| 26 | 从 Windows 自动读取触控板物理尺寸 | **未实现**；依赖本地设置 / 默认 / CSV | **未实现** |
| 27 | 从 GitHub 自动拉取最新 config | **未实现**（曾讨论，待产品决策） | **未实现** |
| 28 | Picture LOGO → 图标与安装图 | 已实现 generate-icons / generate-installer-images | 一致 |

### 6.1 差异说明摘要

**有意变更（产品/体验优化）：**
- 最小窗口由 720 增至 **900** 高度，容纳右侧示意图与型号信息；
- 「更多触控板功能设定」上移至顶栏，与中左参数区分离；
- 示意图比例算法由「EDID」改为更易获取的 **DPI + 分辨率** 方案。

**实现增强（PRD 未写但已交付）：**
- SPI 即时应用 HKCU 触控板设置；
- Curtain 变更重启提示与确认重启；
- CSV 导出、StartupWindow、代码签名、构建号指纹与 config 强制同步；
- 开关重新打开时从 HKLM 读回 Curtain 值。

**尚未实现（后续可规划）：**
- 自动从系统读取触控板物理尺寸；
- 从 GitHub 远程更新 `TouchpadPhysicalSize.csv`。

---

## 七、实现状态（v1.0.0 build 0025）

当前已实现 v1.0.0 全部核心功能，包括：

- WinUI 3 原生风格 UI、PerMonitorV2 缩放、1280×900 最小窗口；
- 灵敏度四件套（含单击灵敏度吸附方案二）；
- Curtain / SuperCurtain 开关与 mm 输入、HKLM 写入、重启提示；
- BIOS 型号识别、CSV 匹配/应用/导出；
- DPI 驱动的触控板示意图与 SuperCurtain 校验警告；
- UAC 提权、Curtain 键补全、StartupWindow 启动态；
- SPI 优先的 HKCU 读写、RegistryUserContext；
- 构建号自动递增、Debug/Beta/Release 分包、config 强制同步、ZiMiaoWorkshop 代码签名；
- 本地 Git 版本管理（`main` 主干）。

---

## 八、文档维护

| 文档 | 用途 |
|------|------|
| `Touchpad_Shield_PRD.md` | 最初产品需求（只读归档，变更以开发指导为准） |
| `Touchpad_Shield_开发指导.md` | 本文档：实现细节、构建方案、与 PRD 差异 |
| `.cursor/rules/touchpad-shield-build.mdc` | Cursor 构建规则（精简版） |

功能或构建流程变更时，应同步更新**开发指导**与 **build 规则**；若涉及产品行为变更，需与产品负责人确认后再改 PRD 归档或本文档。
