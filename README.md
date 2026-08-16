# Touchpad Shield

**Touchpad Shield** 是一款 Windows 桌面工具，用于在打字时减少手掌、手腕误触触控板导致的光标漂移与误点击。它将 Microsoft Precision Touchpad (PTP) 调优选项集中在一处，并遵循 [Precision Touchpad tuning guidelines](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/touchpad-tuning-guidelines)。

Designed and built by **[ZiMiaoWorkshop](https://github.com/ZiMiaoWorkshop)**.

**Current version:** 1.1.0 build 0081 · [Download latest release](https://github.com/ZiMiaoWorkshop/Touchpad-Shield/releases/latest)

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
- **External input auto toggle** — monitor selected input device containers (same source as Windows Settings → Bluetooth & devices → Devices → Input); disable built-in touchpad when any monitored device is connected, re-enable when all are offline (`Status\Enabled` via `Ctrl+Win+F24`)
- **System tray & startup** — optional minimize-to-tray on close and Run-at-logon; forced on when input device monitoring is enabled

---

## External input devices & touchpad toggle (v1.1.0)

This is **separate** from PTP tuning keys above. It toggles the Windows touchpad master switch:

| Item | Detail |
|------|--------|
| Registry read | `HKCU\...\PrecisionTouchPad\Status\Enabled` |
| Toggle method | `SendInput`: `Ctrl+Win+F24` (system toggle key) |
| Device detection | `PnpObjectWatcher` on `DeviceContainer` (not polling) |
| Device list | All input-category containers (paired or connected); built-in touchpad excluded |
| Match key | Device Container ID (`ContainerId`) |
| Persistence | `Software\ZiMiaoWorkshop\TouchpadShield` — see keys below |

When input device monitoring is on, **Run at startup** and **Minimize to tray on close** are forced enabled so background connect/disconnect handling keeps working.

**Registry keys (per Windows user, HKCU):**

| Key | Purpose |
|-----|---------|
| `InputAutoTouchpadEnabled` | Touchpad auto toggle on/off |
| `MonitoredInputDevices` | JSON: `containerId`, `label`, optional `matchKey` |
| `RunAtStartup` | Run at logon (scheduled task) |
| `MinimizeToTrayOnClose` | Minimize to tray when closing window |
| `AutostartHandledSessionId` | DWORD — session ID where `--startup` already ran (internal) |

**Run at logon:** Task Scheduler `\TouchpadShield`, logon trigger bound to the current user, `RunLevel=Highest`, `"<exe>" --startup`. Legacy HKCU Run entries are removed. Each Windows user has independent settings and task registration. After a successful `--startup`, the app records the current session ID so duplicate `--startup` in the same session is ignored (switching back to an already-logged-in user does not log in again, so the task normally does not re-fire).

**PnP watcher (intentional):** While the app process is running, `PnpObjectWatcher` stays registered even when **Enable auto toggle** is off. Only reconcile / F24 toggling is gated by `InputAutoTouchpadEnabled`; device plug/unplug still refreshes the device lists so you can edit the monitored list anytime. Disabling auto toggle also sends F24 to re-enable the touchpad if it was left off.

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

See [`PRD/Touchpad_Shield_开发指导.md`](PRD/Touchpad_Shield_开发指导.md) for module-level details (Chinese), including **§6.1 Code maintenance conventions** (intentional non-refactors and completed cleanups).

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
| [`PRD/Touchpad_Shield_开发指导.md`](PRD/Touchpad_Shield_开发指导.md) | Developer guide — UI spec, architecture, build rules, **§6.1 maintenance conventions** |
| [`PRD/Touchpad_Shield_v1.0.0_to_v1.1.0_变更说明.md`](PRD/Touchpad_Shield_v1.0.0_to_v1.1.0_变更说明.md) | **v1.0.0 → v1.1.0** feature and UI changelog |
| [`.cursor/rules/touchpad-shield-build.mdc`](.cursor/rules/touchpad-shield-build.mdc) | Build policy for automated tooling |
| [`.cursor/rules/touchpad-shield-code.mdc`](.cursor/rules/touchpad-shield-code.mdc) | Code maintenance — refactors not to re-propose |

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
