## 一、产品设计目的简述

本软件名称为 **Touchpad Shield**，作用是通过调整笔记本电脑触控板的 Click Sensitivity、Touchpad Sensitivity、Tap with a single finger to single-click、Curtains 以及 Super Curtains 的数值，来减少用户在使用笔记本电脑键盘打字时，因手掌、手腕、大拇指误触触控板而造成的鼠标光标漂移以及误击现象。

本软件严格遵循微软 Windows 系统 Precision Touchpad Devices 中关于 Precision touchpad tuning 的相关优化指南进行开发，参考链接：

[https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/touchpad-tuning-guidelines](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/touchpad-tuning-guidelines)

---

## 二、技术栈

| 层级 | 选型 |
|------|------|
| UI 框架 | WinUI 3（unpackaged，`WindowsPackageType=None`） |
| 语言 | C++ / C++/WinRT（C++20） |
| 平台 | x64，最低 Windows 10 17763 |
| 依赖 | Windows App SDK 1.6、CppWinRT 2.0 |
| 安装包 | NSIS（简体中文 MUI） |
| 构建 | MSBuild + PowerShell 脚本 |

---

## 三、具体需求

### 1、UI 风格与缩放

（1）前端 UI 样式采用与「Windows 系统设置」一致的风格。界面上主要文字的样式（包括但不限于字体、字号）以及 UI 界面颜色样式，要直接跟随用户在 Windows 操作系统中的设定；各控件的样式以及尺寸也要在 WinUI 3 原生状态下尽量与 Windows 系统设置界面上的控件保持接近。

（2）前端界面必须严格遵循最新 Windows 系统应用的开发要求，必须正确支持 Windows 的缩放功能：
- 应用清单启用 `PerMonitorV2` DPI 感知；
- 窗口最小尺寸、初始客户区尺寸按逻辑像素定义，在高 DPI 下通过 `GetDpiForWindow` 换算物理像素；
- 触控板示意图按窗口所在显示器的 DPI 与分辨率估算 mm/DIP 比例，尽量接近真实物理尺寸渲染。

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

#### 2.3 启动流程

（1）软件启动时必须请求 UAC 权限（`app.manifest` 设置 `requireAdministrator`），以正常写入 HKLM 下的屏蔽区域与超级屏蔽区域数值。

（2）启动时显示 **StartupWindow**（ProgressRing +「程序正在启动」），后台完成初始化后关闭并激活主窗口。

（3）启动时应检查注册表中是否有缺失的屏蔽区域以及超级屏蔽区域键值，若有则自动补全并将补全值设为 0。补全完成后读取对应数据并反显在控件内，并绘制示意图。

（4）提权后写入 HKCU 类注册表时，通过 `RegistryUserContext` 模拟交互用户上下文；Click/AAP/Taps 等用户设置优先通过 `SystemParametersInfo(SPI_SETTOUCHPADPARAMETERS)` 即时生效，失败时回退直接写注册表并广播 `WM_SETTINGCHANGE`。

#### 2.4 中左区域 — 灵敏度设定 (Sensitivity)

从上往下共 4 个功能：

**（1）单击灵敏度 (Click Sensitivity)**
- 滑块控件，表值 `ClickForceSensitivity`（0–100）；
- 采用**方案二（吸附到指定刻度）**：滑块始终 101 档（0–100），在「与 Windows 系统设置保持一致」模式下吸附到 0、50、100，显示「轻 0 / 中 50 / 重 100」；在「自由调节」模式下可自由拖动，仅 0/50/100 处显示文字；
- 滑块右侧显示当前数值标签。

**（2）单击灵敏度控制方式 (Click Sensitivity Control Mode)**
- 下拉框：`与 Windows 系统设置保持一致` / `自由调节`；
- 模式持久化于本地设置（`Software\ZiMiaoWorkshop\TouchpadShield`）。

**（3）触板灵敏度 (Touchpad Sensitivity)**
- 下拉框，表值 `AAPThreshold`：0 最高 / 1 高 / 2 中 / 3 低。

**（4）轻拍触控板进行单击 (Tap with a single finger to single-click)**
- ToggleSwitch，表值 `TapsEnabled`（1=开，0=关）。

#### 2.5 中左区域 — 屏蔽区域设定 (Curtains / Super Curtains)

（1）从上往下：缓冲区域 (Smart Area) 开关 + 上/下/左/右 4 个 NumberBox；防误触区域 (Smart Edge) 开关 + 上/下/左/右 4 个 NumberBox。

（2）8 个输入框单位为毫米（mm），小数点后限定 2 位；写入注册表时转换为 Himetric（1 mm = 100 Himetric）。

（3）开关为「关」时：对应 4 边数值自动置 0，输入框禁用，并立即写入注册表；开关为「开」时：输入框可编辑。启动时若任一边数值 > 0，则对应开关初始为「开」。

（4）**更多触控板功能设定** 入口位于**页面顶部右侧**（非中左区域）。

#### 2.6 中右区域 — 笔记本电脑型号 (Laptop Model)

（1）从 `HKLM\HARDWARE\DESCRIPTION\System\BIOS` 读取 `SystemManufacturer`、`SystemProductName`、`SystemSKU`、`SystemVersion`，组合格式为：

`Manufacturer - ProductName - SKU - Version`

空字段及其前面的 ` - ` 一并隐藏。

（2）**导出当前触控板物理尺寸设定**（Accent 按钮，条件显示）：
- CSV 不存在、或无匹配、或当前尺寸与匹配/已有记录不同时显示；
- 点击后将当前 BIOS 身份 + 触控板宽/高 Upsert 到 `TouchpadPhysicalSize.csv`。

（3）**配置文件中匹配到的触控板物理尺寸**（条件显示）：
- 以四字段 BIOS 身份为联合主键，在 `config/TouchpadPhysicalSize.csv` 中精确匹配；
- 匹配到且与当前 APP 内尺寸不一致时，显示文案：`配置文件中匹配到的触控板物理尺寸：X mm × Y mm`；
- 同步显示 **应用预设的触控板物理尺寸** 按钮（Accent 样式），点击后将匹配尺寸写入宽/高输入框并保存到本地设置。

#### 2.7 中右区域 — 触控板物理尺寸 (Touchpad Physical Size)

（1）横向宽度 (Width)、纵向高度 (Height) 两个 NumberBox，单位 mm，最小值 1。

（2）启动时读取本地保存数据；若无保存，默认使用 PTP 要求的最小尺寸 **65 mm × 40 mm**。

（3）修改后自动保存到 `Software\ZiMiaoWorkshop\TouchpadShield`，并刷新示意图与匹配/导出按钮状态。

#### 2.8 中右区域 — 触控板示意图 (Touchpad Diagram)

（1）Canvas 高度 **380px**，按最终确定的触控板物理尺寸绘制触控板矩形。

（2）比例依据：读取窗口所在显示器的 **DPI 与分辨率**估算 `mm/DIP`（`DisplayScaleService`），尽量接近真实物理尺寸；仅当超出 Canvas 可用区域时才等比缩小（无硬编码上限）。

（3）图示叠加：
- 缓冲区域 (Smart Area)：半透明黄色 `#80FFD700`；
- 防误触区域 (Smart Edge)：半透明红色 `#80DC143C`；
- 底部显示图例。

（4）校验：若任一边 SuperCurtain ≥ 对应 Curtain，显示红色警告，说明该区域将完全以防误触区域逻辑控制。

#### 2.9 底部区域

（1）修改 Curtain / SuperCurtain 并成功写入 HKLM 后，显示：
- 文案：「屏蔽区域设定已更改，需要重启电脑后生效。」
- **重启**按钮：弹出确认对话框后调用 `ExitWindowsEx(EWX_REBOOT)`。

（2）Click/AAP/Taps 等 HKCU 设置修改后**即时生效**，不触发重启提示。

（3）右侧固定显示：
- `Designed and Built by ZiMiaoWorkshop`
- 版本号：`v{MAJOR.MINOR.PATCH build BUILD}`（例如 `v1.0.0 build 0019`）

#### 2.10 窗口尺寸

软件最小窗口尺寸限定为 **100% 缩放下 1280×900**（逻辑像素，含内容区 MinWidth/MinHeight 与 `WM_GETMINMAXINFO` 限制）。

---

### 3、配置文件及接口设计

#### 3.1 触控板物理尺寸 CSV

- **文件名：** `TouchpadPhysicalSize.csv`
- **部署路径：** `{exe 目录}/config/TouchpadPhysicalSize.csv`（构建时从项目 `config/` 复制）
- **表头：**

```
SystemManufacturer,SystemProductName,SystemSKU,SystemVersion,TouchpadWidth,TouchpadHeight
```

- **匹配规则：** 四字段 BIOS 身份完全一致；
- **Upsert：** 同身份覆盖更新，保留其他行；尺寸格式化为两位小数。

#### 3.2 注册表路径与接口命名

**PTP 用户设置（HKCU）**  
路径：`SOFTWARE\Microsoft\Windows\CurrentVersion\PrecisionTouchPad`

| 键值 | 接口示例 |
|------|----------|
| `ClickForceSensitivity` | `APP_ClickForceSensitivity` / `APP_SetClickForceSensitivity` |
| `AAPThreshold` | `APP_AAPThreshold` / `APP_SetAAPThreshold` |
| `TapsEnabled` | `APP_TapsEnabled` / `APP_SetTapsEnabled` |

**PTP 系统设置（HKLM，需管理员）**  
同路径：

| 键值 | 接口示例 |
|------|----------|
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

Debug 构建启用 `TOUCHPAD_SHIELD_DEBUG`，日志写入：

`%LocalAppData%\TouchpadShield\logs\touchpad-shield.log`

---

### 4、品牌资源

`Picture/` 文件夹中放置：
- **Touchpad Shield LOGO**（PNG）→ 生成应用图标（`Assets/TouchpadShield.ico`）及安装程序图标；
- **ZiMiaoWorkshop LOGO**（JPG）→ NSIS 安装向导侧边图（`installer/welcome-finish.bmp`，164×314）设计参考。

图标缩放需注意精度与色深，确保 icon 与安装图片清晰。

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
| UI 展示格式 | `MAJOR.MINOR.PATCH build BUILD`（例如 `1.0.0 build 0019`） |
| Manifest | `assemblyIdentity` 使用四段数字 `MAJOR.MINOR.PATCH.buildInt`（例如 `1.0.0.19`） |
| 同步 | `scripts/sync-version.ps1` 将版本同步至 NSIS 安装脚本 |

**构建号指纹范围：** `src/`、`scripts/`、`installer/`、`config/`、`Picture/`、`TouchpadShield.sln`、`version/Version.props`、`version/Version.targets`（排除 stamp 文件及 NSIS/图标等衍生产物）。

### 2、产物目录

| 目录 | 内容 |
|------|------|
| `Touchpad Shield App/debug/` | Debug 可执行文件及依赖，启用 debug 日志 |
| `Touchpad Shield App/beta/` | NSIS 打包的 Debug 版安装包（`*-beta-setup.exe`） |
| `Touchpad Shield App/release/` | 正式发布 NSIS 安装包（`*-setup.exe`）及 `release/app/` 应用文件 |

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

### 4、构建脚本一览

| 脚本 | 用途 |
|------|------|
| `scripts/build-debug.ps1` | 主构建：NuGet、图标、构建号 bump、MSBuild、签名 |
| `scripts/build-beta.ps1` | Debug 构建 + Beta 安装包 |
| `scripts/build-release.ps1` | Release 构建 + Release 安装包 |
| `scripts/bump-build.ps1` | 源码指纹对比，变动时 BUILD+1；编译失败回滚 |
| `scripts/sync-version.ps1` | 同步版本号至 NSIS |
| `scripts/sign-exe.ps1` | Authenticode 签名 |
| `scripts/generate-icons.ps1` | LOGO → 多尺寸 ICO |
| `scripts/generate-installer-images.ps1` | 安装向导侧边图 |
| `scripts/patch-app-manifest.ps1` | 写入 manifest 四段版本 |

### 5、安装包行为（NSIS）

- 安装目录：`$PROGRAMFILES64\Touchpad Shield`
- 请求管理员权限（`RequestExecutionLevel admin`）
- 快捷方式：桌面 + 开始菜单 `Programs\ZiMiaoWorkshop\Touchpad Shield`
- 卸载信息注册于 `HKLM\...\Uninstall\Touchpad Shield`，Publisher 为 ZiMiaoWorkshop

### 6、需求变更原则

需求理解阶段有任何疑问必须与产品负责人确认，**不可自行决定解决方案**。

---

## 五、实现状态说明（v1.0.0）

当前已实现 v1.0.0 全部核心功能，包括：

- WinUI 3 原生风格 UI、PerMonitorV2 缩放、1280×900 最小窗口；
- 灵敏度四件套（含单击灵敏度吸附方案二）；
- Curtain / SuperCurtain 开关与 mm 输入、重启提示；
- BIOS 型号识别、CSV 匹配/应用/导出；
- DPI 驱动的触控板示意图与 SuperCurtain 校验警告；
- UAC 提权、Curtain 键补全、StartupWindow 启动态；
- 构建号自动递增、Debug/Beta/Release 分包、ZiMiaoWorkshop 代码签名。

---