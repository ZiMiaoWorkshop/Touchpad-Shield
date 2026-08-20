param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$AppOutputRoot = Join-Path $Root "Touchpad Shield App"
$ReleaseDir = Join-Path $AppOutputRoot "release"
$AppDir = Join-Path $ReleaseDir "app"
$NsisScript = Join-Path $Root "installer\TouchpadShield-release.nsi"
$MakeNsis = "C:\Program Files (x86)\NSIS\makensis.exe"

if (-not (Test-Path $MakeNsis)) {
    throw "NSIS makensis.exe not found: $MakeNsis"
}

Push-Location $Root
try {
    & (Join-Path $Root "scripts\build-debug.ps1") -Configuration $Configuration -VersionChannel release

    if (-not (Test-Path $AppDir)) {
        throw "Release output not found: $AppDir"
    }

    if (-not (Test-Path $ReleaseDir)) {
        New-Item -ItemType Directory -Force -Path $ReleaseDir | Out-Null
    }

    & (Join-Path $Root "scripts\generate-installer-images.ps1")

    Write-Host "Building release installer..."
    & $MakeNsis $NsisScript
    if ($LASTEXITCODE -ne 0) {
        throw "NSIS build failed with exit code $LASTEXITCODE"
    }

    $installer = Get-ChildItem -Path $ReleaseDir -Filter "*-setup.exe" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($installer) {
        Write-Host "Release installer: $($installer.FullName)"
    } else {
        Write-Host "Release installer output: $ReleaseDir"
    }
}
finally {
    Pop-Location
}
