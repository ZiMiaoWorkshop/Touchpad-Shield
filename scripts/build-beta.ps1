param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$AppOutputRoot = Join-Path $Root "Touchpad Shield App"
$BetaDir = Join-Path $AppOutputRoot "beta"
$DebugDir = Join-Path $AppOutputRoot "debug"
$NsisScript = Join-Path $Root "installer\TouchpadShield-beta.nsi"
$MakeNsis = "C:\Program Files (x86)\NSIS\makensis.exe"

if (-not (Test-Path $MakeNsis)) {
    throw "NSIS makensis.exe not found: $MakeNsis"
}

Push-Location $Root
try {
    & (Join-Path $Root "scripts\build-debug.ps1") -Configuration $Configuration -VersionChannel beta

    if (-not (Test-Path $DebugDir)) {
        throw "Debug output not found: $DebugDir"
    }

    if (-not (Test-Path $BetaDir)) {
        New-Item -ItemType Directory -Force -Path $BetaDir | Out-Null
    }

    & (Join-Path $Root "scripts\generate-installer-images.ps1")

    Write-Host "Building beta installer..."
    & $MakeNsis $NsisScript
    if ($LASTEXITCODE -ne 0) {
        throw "NSIS build failed with exit code $LASTEXITCODE"
    }

    $installer = Get-ChildItem -Path $BetaDir -Filter "*-beta-setup.exe" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($installer) {
        Write-Host "Beta installer: $($installer.FullName)"
    } else {
        Write-Host "Beta installer output: $BetaDir"
    }
}
finally {
    Pop-Location
}
