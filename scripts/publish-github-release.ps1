param(
    [string]$Tag = "",
    [switch]$SkipTagPush
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$VersionProps = Join-Path $Root "version\Version.props"
$ReleaseDir = Join-Path $Root "Touchpad Shield App\release"
$ConfigCsv = Join-Path $Root "config\TouchpadPhysicalSize.csv"
$Repo = "ZiMiaoWorkshop/Touchpad-Shield"

function Get-VersionFromProps {
    param([string]$Path)
    [xml]$xml = Get-Content -LiteralPath $Path
    $major = $xml.Project.PropertyGroup.TouchpadShieldMajorVersion
    $minor = $xml.Project.PropertyGroup.TouchpadShieldMinorVersion
    $patch = $xml.Project.PropertyGroup.TouchpadShieldPatchVersion
    $build = $xml.Project.PropertyGroup.TouchpadShieldBuildNumber
    return @{
        SemVer = "$major.$minor.$patch"
        Build  = $build
        Tag    = "v$major.$minor.$patch-build$build"
    }
}

$version = Get-VersionFromProps -Path $VersionProps
if ([string]::IsNullOrWhiteSpace($Tag)) {
    $Tag = $version.Tag
}

$installer = Get-ChildItem -Path $ReleaseDir -Filter "TouchpadShield-*-setup.exe" |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

if (-not $installer) {
    throw "Release installer not found under: $ReleaseDir. Run .\scripts\build-release.ps1 first."
}

if (-not (Test-Path -LiteralPath $ConfigCsv)) {
    throw "Config CSV not found: $ConfigCsv"
}

$gh = Get-Command gh -ErrorAction SilentlyContinue
if (-not $gh) {
    throw "GitHub CLI (gh) not found. Install from https://cli.github.com/ and run: gh auth login"
}

gh auth status --hostname github.com 2>$null | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "GitHub CLI is not authenticated. Run: gh auth login"
}

Push-Location $Root
try {
    $tagExists = git tag -l $Tag
    if (-not $tagExists) {
        Write-Host "Creating tag $Tag ..."
        git tag -a $Tag -m "Touchpad Shield $($version.SemVer) build $($version.Build)"
    }

    if (-not $SkipTagPush) {
        Write-Host "Pushing tag $Tag to origin ..."
        git push origin $Tag
    }

    $releaseTitle = "Touchpad Shield $($version.SemVer) build $($version.Build)"
    $releaseNotes = @"
## Touchpad Shield $($version.SemVer) build $($version.Build)

正式版 Release 安装包与触控板物理尺寸配置文件。

- 需要 Windows 10 17763+ / x64 / 管理员权限
- 发布者：ZiMiaoWorkshop（自签证书）
- ``TouchpadPhysicalSize.csv`` 也可在仓库 ``config/`` 目录获取
"@

    $notesFile = Join-Path $env:TEMP "touchpad-shield-release-$Tag.md"
    # gh on Windows reads --notes-file using system ANSI unless UTF-8 BOM is present
    $utf8WithBom = New-Object System.Text.UTF8Encoding $true
    [System.IO.File]::WriteAllText($notesFile, $releaseNotes, $utf8WithBom)

    $previousErrorAction = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    gh release view $Tag --repo $Repo 2>$null | Out-Null
    $releaseExists = ($LASTEXITCODE -eq 0)
    $ErrorActionPreference = $previousErrorAction

    if ($releaseExists) {
        Write-Host "Release $Tag already exists. Updating notes and uploading assets ..."
        gh release edit $Tag `
            --repo $Repo `
            --notes-file $notesFile
        gh release upload $Tag `
            --repo $Repo `
            --clobber `
            $installer.FullName `
            $ConfigCsv
    }
    else {
        Write-Host "Creating release $Tag ..."
        gh release create $Tag `
            --repo $Repo `
            --title $releaseTitle `
            --notes-file $notesFile `
            $installer.FullName `
            $ConfigCsv
    }

    if ($LASTEXITCODE -ne 0) {
        throw "gh release command failed with exit code $LASTEXITCODE"
    }

    Write-Host "Done. Release: https://github.com/$Repo/releases/tag/$Tag"
}
finally {
    Pop-Location
}
