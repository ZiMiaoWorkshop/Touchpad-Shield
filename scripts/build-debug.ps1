param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$AppOutputRoot = Join-Path $Root "Touchpad Shield App"
$NuGet = Join-Path $Root "tools\nuget.exe"
$MsBuild = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
$Solution = Join-Path $Root "TouchpadShield.sln"

if (-not (Test-Path $NuGet)) {
    New-Item -ItemType Directory -Force -Path (Split-Path $NuGet) | Out-Null
    Invoke-WebRequest -Uri "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" -OutFile $NuGet
}

Push-Location $Root
try {
    if (-not (Test-Path (Join-Path $Root "packages\Microsoft.WindowsAppSDK.1.6.250108002"))) {
        & $NuGet install Microsoft.WindowsAppSDK -Version 1.6.250108002 -OutputDirectory (Join-Path $Root "packages") | Out-Null
    }
    if (-not (Test-Path (Join-Path $Root "packages\Microsoft.Windows.CppWinRT.2.0.240405.15"))) {
        & $NuGet install Microsoft.Windows.CppWinRT -Version 2.0.240405.15 -OutputDirectory (Join-Path $Root "packages") | Out-Null
    }

    & (Join-Path $Root "scripts\generate-icons.ps1")
    & (Join-Path $Root "scripts\bump-build.ps1") -Phase Prepare
    & (Join-Path $Root "scripts\sync-version.ps1")

    $running = Get-Process -Name TouchpadShield -ErrorAction SilentlyContinue
    if ($running) {
        Write-Host "Stopping running TouchpadShield.exe before build..."
        $running | Stop-Process -Force -ErrorAction SilentlyContinue
        Start-Sleep -Seconds 1
    }

    $outputDir = if ($Configuration -eq "Release") {
        Join-Path $AppOutputRoot "release\app"
    } else {
        Join-Path $AppOutputRoot "debug"
    }

    & $MsBuild $Solution /p:Configuration=$Configuration /p:Platform=x64 /m
    if ($LASTEXITCODE -ne 0) {
        & (Join-Path $Root "scripts\bump-build.ps1") -Phase Revert
        throw "Build failed with exit code $LASTEXITCODE"
    }

    & (Join-Path $Root "scripts\bump-build.ps1") -Phase Finalize
    & (Join-Path $Root "scripts\sign-exe.ps1") -ExePath (Join-Path $outputDir "TouchpadShield.exe")
    Write-Host "Build completed. Output: $outputDir"
}
finally {
    Pop-Location
}
