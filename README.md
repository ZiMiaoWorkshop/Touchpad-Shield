# Touchpad Shield

**Touchpad Shield** 是一款 Windows 桌面工具，用于在打字时减少手掌、手腕误触触控板导致的光标漂移与误点击。它将 Microsoft Precision Touchpad (PTP) 调优选项集中在一处，并遵循 [Precision Touchpad tuning guidelines](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/touchpad-tuning-guidelines)。

Designed and built by **[ZiMiaoWorkshop](https://github.com/ZiMiaoWorkshop)**.

**Current version:** 1.0.1 build 0033 · [Download latest release](https://github.com/ZiMiaoWorkshop/Touchpad-Shield/releases/latest)

---

## Download

| Asset | Description |
|-------|-------------|
| [**Latest Release**](https://github.com/ZiMiaoWorkshop/Touchpad-Shield/releases/latest) | Official NSIS installer (`TouchpadShield-*-setup.exe`) |
| [`TouchpadPhysicalSize.csv`](https://github.com/ZiMiaoWorkshop/Touchpad-Shield/releases/latest) | Laptop touchpad size presets (also in [`config/`](config/)) |

> **Note:** The installer is Authenticode-signed with a self-signed certificate (publisher **ZiMiaoWorkshop**). Windows SmartScreen may show an unknown publisher warning on first install.

---

## Features

- **Click Sensitivity** — `ClickForceSensitivity` with free adjust or Windows-aligned snapping (0 / 50 / 100)
- **Touchpad Sensitivity** — `AAPThreshold` (4 levels)
- **Tap to click** — `TapsEnabled` toggle
- **Curtains (Smart Area)** — per-edge buffer zones in millimeters (`CurtainTop/Bottom/Left/Right`)
- **Super Curtains (Smart Edge)** — per-edge palm-rejection zones (`SuperCurtainTop/Bottom/Left/Right`)
- **Live diagram** — yellow (Curtains) and red (Super Curtains) overlays with DPI-aware sizing
- **Overlap warnings** — compact two-line hint when Super Curtain ≥ Curtain on any edge
- **Laptop-aware sizing** — BIOS identity matching via `TouchpadPhysicalSize.csv`; apply presets or export your own
- **Instant vs reboot** — HKCU settings apply immediately via `SystemParametersInfo`; HKLM Curtain changes require reboot
- **Native Windows UI** — WinUI 3, system theme, PerMonitorV2 DPI, bilingual labels (Chinese / English)
- **External HID auto toggle** — monitor selected HID devices (VID/PID); disable built-in touchpad when any is connected, re-enable when all are removed (`Status\Enabled` via `Ctrl+Win+F24`)
- **System tray & startup** — optional minimize-to-tray on close and Run-at-logon; forced on when HID monitoring is enabled

---

## External HID & touchpad toggle (v1.0.1)

This is **separate** from PTP tuning keys above. It toggles the Windows touchpad master switch:

| Item | Detail |
|------|--------|
| Registry read | `HKCU\...\PrecisionTouchPad\Status\Enabled` |
| Toggle method | `SendInput`: `Ctrl+Win+F24` (system toggle key) |
| Device detection | `WM_DEVICECHANGE` + `RegisterDeviceNotification` (not polling) |
| Monitored devices | User-selected HID list (VID/PID); built-in touchpad excluded from picker |
| Persistence | `Software\ZiMiaoWorkshop\TouchpadShield` |

When HID monitoring is on, **Run at startup** and **Minimize to tray on close** are forced enabled so background plug/unplug handling keeps working.

---

## PTP settings mapped in the app

| UI (中文 / EN) | Registry key | Scope |
|----------------|--------------|-------|
| 单击灵敏度 / Click Sensitivity | `ClickForceSensitivity` | HKCU |
| 触板灵敏度 / Touchpad Sensitivity | `AAPThreshold` | HKCU |
| 轻拍单击 / Tap to click | `TapsEnabled` | HKCU |
| 缓冲区域 / Curtains (Smart Area) | `CurtainTop/Bottom/Left/Right` | HKLM |
| 防误触区域 / Super Curtains (Smart Edge) | `SuperCurtainTop/Bottom/Left/Right` | HKLM |
| 触控板总开关 / Touchpad on-off | `Status\Enabled` | HKCU (read + F24 toggle; not direct write) |

Registry path: `HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\PrecisionTouchPad` (and HKLM for Curtains).

---

## Requirements

| Item | Detail |
|------|--------|
| OS | Windows 10 version 1809 (build **17763**) or later |
| Architecture | **x64** |
| Privileges | **Administrator (UAC)** — required to write HKLM Curtain / SuperCurtain values |
| Build tools | Visual Studio 2022 · **Desktop development with C++** · **Windows App SDK** |

---

## Build from source

Clone the repository, then from the project root:

```powershell
# Debug build — output: Touchpad Shield App/debug/
.\scripts\build-debug.ps1

# Beta installer (Debug app + NSIS) — output: Touchpad Shield App/beta/
.\scripts\build-beta.ps1

# Release installer — output: Touchpad Shield App/release/
.\scripts\build-release.ps1

# Publish GitHub Release (after gh auth login)
.\scripts\publish-github-release.ps1
```

The first run downloads NuGet packages (Windows App SDK 1.6, CppWinRT 2.0) and may create a self-signed code-signing certificate.

Run locally without installing:

```text
Touchpad Shield App\debug\TouchpadShield.exe
```

---

## Touchpad size config

Authoritative file: [`config/TouchpadPhysicalSize.csv`](config/TouchpadPhysicalSize.csv)

```csv
SystemManufacturer,SystemProductName,SystemSKU,SystemVersion,TouchpadWidth,TouchpadHeight
```

On every build, MSBuild and `build-debug.ps1` sync this file to `{output}/config/` next to the executable.

---

## Architecture

```text
┌─────────────────────────────────────┐
│  Presentation (XAML)                │  MainWindow, StartupWindow, App.xaml
├─────────────────────────────────────┤
│  Application (MainWindow.xaml.cpp)  │  UI events, LoadAllData, orchestration
├─────────────────────────────────────┤
│  Services                           │  Registry, SPI, BIOS, CSV, DPI, diagram
└─────────────────────────────────────┘
```

See [`PRD/Touchpad_Shield_开发指导.md`](PRD/Touchpad_Shield_开发指导.md) for module-level details (Chinese).

---

## Project structure

```text
Touchpad Shield/
├── config/                 # TouchpadPhysicalSize.csv
├── installer/              # NSIS scripts
├── PRD/                    # Developer guide (Chinese)
├── scripts/                # Build, sign, version, release scripts
├── src/TouchpadShield/     # WinUI 3 app (C++/WinRT)
├── Touchpad Shield App/    # Build outputs (gitignored; .gitkeep only)
└── version/                # Version.props, build stamp
```

---

## Versioning

| Part | Rule |
|------|------|
| Semver `MAJOR.MINOR.PATCH` | Manual — `version/Version.props` |
| Build `BUILD` (4 digits) | Auto on source fingerprint change — `scripts/bump-build.ps1` |
| Display | `1.0.0 build 0032` |

Doc-only changes (e.g. README) do **not** increment the build number.

---

## Documentation

| Document | Description |
|----------|-------------|
| [`PRD/Touchpad_Shield_开发指导.md`](PRD/Touchpad_Shield_开发指导.md) | Developer guide — UI spec, architecture, build rules |
| [`.cursor/rules/touchpad-shield-build.mdc`](.cursor/rules/touchpad-shield-build.mdc) | Build policy for automated tooling |

---

## Tech stack

- **UI:** WinUI 3 (unpackaged)
- **Language:** C++ / C++/WinRT (C++20)
- **Installer:** NSIS (Simplified Chinese MUI)
- **Dependencies:** Windows App SDK 1.6, CppWinRT 2.0

---

## License

Copyright 2026 ZiMiaoWorkshop

Licensed under the [Apache License, Version 2.0](LICENSE). See [http://www.apache.org/licenses/LICENSE-2.0](http://www.apache.org/licenses/LICENSE-2.0).
