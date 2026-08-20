param(

    [string]$VersionPropsPath = (Join-Path (Split-Path -Parent $PSScriptRoot) "version\Version.props"),

    [ValidateSet("alpha", "beta", "release")]

    [string]$VersionChannel = "release"

)



$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot



[xml]$props = Get-Content -LiteralPath $VersionPropsPath

$versionGroup = $props.Project.PropertyGroup | Where-Object { $_.Label -eq "Version" } | Select-Object -First 1



$major = $versionGroup.TouchpadShieldMajorVersion

$minor = $versionGroup.TouchpadShieldMinorVersion

$patch = $versionGroup.TouchpadShieldPatchVersion

$build = $versionGroup.TouchpadShieldBuildNumber



$semVer = "$major.$minor.$patch"

$displaySuffix = switch ($VersionChannel) {

    "alpha" { " (alpha)" }

    "beta" { " (beta)" }

    default { "" }

}

$display = "$semVer build $build$displaySuffix"

$buildInt = [int]$build

$assemblyVersion = "$semVer.$buildInt"



Write-Host "Syncing version: $display (assembly: $assemblyVersion)"



$nsisFiles = @(

    (Join-Path $Root "installer\TouchpadShield-beta.nsi"),

    (Join-Path $Root "installer\TouchpadShield-release.nsi")

)



foreach ($file in $nsisFiles) {

    if (-not (Test-Path $file)) { continue }

    $content = Get-Content -LiteralPath $file -Raw

    $content = $content -replace '!define APP_SEMVER "[^"]*"', "!define APP_SEMVER `"$semVer`""

    $content = $content -replace '!define APP_BUILD "[^"]*"', "!define APP_BUILD `"$build`""

    $content = $content -replace '!define APP_VERSION "[^"]*"', "!define APP_VERSION `"$display`""

    Set-Content -LiteralPath $file -Value $content -NoNewline

    Write-Host "  Updated $(Split-Path $file -Leaf)"

}



Write-Host "Version sync completed."

