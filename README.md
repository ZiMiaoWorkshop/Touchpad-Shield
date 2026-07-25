# Touchpad Shield

**Touchpad Shield** is a Windows desktop utility that reduces accidental touchpad input while typing. It exposes Precision Touchpad (PTP) tuning options—click sensitivity, touchpad sensitivity, tap-to-click, **Curtains** (edge buffer zones), and **Super Curtains** (stronger palm-rejection zones)—in one place, following Microsoft's [Precision Touchpad tuning guidelines](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/touchpad-tuning-guidelines).

Designed and built by **ZiMiaoWorkshop**.

**Current version:** 1.0.0 build 0025

---

## Features

- **Sensitivity controls** — Click Force Sensitivity (free adjust or Windows-aligned snapping at 0 / 50 / 100), AAP threshold (touchpad sensitivity), and tap-to-click toggle
- **Smart Area & Smart Edge** — Configure Curtains and Super Curtains per edge in millimeters; live diagram with yellow (buffer) and red (palm-rejection) overlays
- **Laptop-aware sizing** — Read BIOS identity, match touchpad physical size from `TouchpadPhysicalSize.csv`, apply presets, or export your own entry
- **Instant vs reboot settings** — HKCU settings (click / tap / AAP) apply immediately via `SystemParametersInfo`; HKLM Curtain changes prompt for reboot
- **Native Windows UI** — WinUI 3, system theme integration, PerMonitorV2 DPI scaling, bilingual section labels (Chinese / English)
- **Installer & signing** — NSIS packages for beta (Debug) and release; Authenticode signing with publisher **ZiMiaoWorkshop**

---

## Requirements

| Item | Detail |
|------|--------|
| OS | Windows 10 version 1809 (build **17763**) or later |
| Architecture | **x64** |
| Privileges | **Administrator (UAC)** — required to write HKLM Curtain / SuperCurtain registry values |
| Build tools | Visual Studio 2022 with **Desktop development with C++** and **Windows App SDK** workload |

---

## Quick Start (Build)

Clone the repository, then from the project root:

```powershell
# Debug build (default) — output: Touchpad Shield App/debug/
.\scripts\build-debug.ps1

# Beta installer (Debug app + NSIS) — output: Touchpad Shield App/beta/
.\scripts\build-beta.ps1

# Release installer — output: Touchpad Shield App/release/
.\scripts\build-release.ps1
```

The first run downloads NuGet packages (Windows App SDK 1.6, CppWinRT 2.0) and may create a self-signed code-signing certificate.

Run the app:

```text
Touchpad Shield App\debug\TouchpadShield.exe
```

---

## Touchpad Size Config

The authoritative config file lives at:

```text
config/TouchpadPhysicalSize.csv
```

CSV header:

```csv
SystemManufacturer,SystemProductName,SystemSKU,SystemVersion,TouchpadWidth,TouchpadHeight
```

On every build, this file is **always synced** to the output directory (`config/` next to the executable) via MSBuild and the build script, so editing the repo copy is enough for local and packaged builds.

---

## Project Structure

```text
Touchpad Shield/
├── config/                 # TouchpadPhysicalSize.csv (edit here)
├── installer/              # NSIS scripts
├── PRD/                    # Product requirements & developer guide (Chinese)
├── scripts/                # Build, sign, version, icon scripts
├── src/TouchpadShield/     # WinUI 3 app (C++/WinRT)
├── Touchpad Shield App/    # Build outputs (debug / beta / release)
└── version/                # Version.props, build stamp
```

---

## Versioning

- **Semantic version** (`MAJOR.MINOR.PATCH`) — maintained manually in `version/Version.props`
- **Build number** (4 digits) — auto-incremented when source fingerprint changes (`scripts/bump-build.ps1`)
- **Display format** — `1.0.0 build 0025`

---

## Documentation

| Document | Description |
|----------|-------------|
| [`PRD/Touchpad_Shield_开发指导.md`](PRD/Touchpad_Shield_开发指导.md) | Developer guide (implementation details, build rules, PRD diff) |
| [`PRD/Touchpad_Shield_PRD.md`](PRD/Touchpad_Shield_PRD.md) | Original product requirements (archive) |
| [`.cursor/rules/touchpad-shield-build.mdc`](.cursor/rules/touchpad-shield-build.mdc) | Build policy for automated tooling |

---

## Tech Stack

- **UI:** WinUI 3 (unpackaged)
- **Language:** C++ / C++/WinRT (C++20)
- **Installer:** NSIS (Simplified Chinese MUI)
- **Dependencies:** Windows App SDK 1.6, CppWinRT 2.0

---

## License

No license file is included yet. All rights reserved by ZiMiaoWorkshop unless stated otherwise.
