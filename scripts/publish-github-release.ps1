param(
    [string]$Tag = "",
    [string]$ReleaseNotesFile = "",
    [switch]$SkipTagPush
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$VersionProps = Join-Path $Root "version\Version.props"
$ReleaseDir = Join-Path $Root "Touchpad Shield App\release"
$ConfigCsv = Join-Path $Root "config\TouchpadPhysicalSize.csv"
$NotesTemplate = Join-Path $PSScriptRoot "release-notes-body.template.md"
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

function Write-Utf8BomFile {
    param(
        [string]$Path,
        [string]$Content
    )
    $utf8WithBom = New-Object System.Text.UTF8Encoding $true
    [System.IO.File]::WriteAllText($Path, $Content, $utf8WithBom)

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 3 -or $bytes[0] -ne 0xEF -or $bytes[1] -ne 0xBB -or $bytes[2] -ne 0xBF) {
        throw "Release notes file is missing UTF-8 BOM: $Path"
    }
}

function Get-ReleaseNotesBody {
    param(
        [hashtable]$Version,
        [string]$CustomNotesFile
    )

    if (-not [string]::IsNullOrWhiteSpace($CustomNotesFile)) {
        if (-not (Test-Path -LiteralPath $CustomNotesFile)) {
            throw "Custom release notes file not found: $CustomNotesFile"
        }
        return Get-Content -LiteralPath $CustomNotesFile -Raw -Encoding UTF8
    }

    if (-not (Test-Path -LiteralPath $NotesTemplate)) {
        throw "Release notes template not found: $NotesTemplate"
    }

    $template = Get-Content -LiteralPath $NotesTemplate -Raw -Encoding UTF8
    return $template `
        -replace '\{\{SemVer\}\}', $Version.SemVer `
        -replace '\{\{Build\}\}', $Version.Build
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
    $releaseNotes = Get-ReleaseNotesBody -Version $version -CustomNotesFile $ReleaseNotesFile

    $notesFile = Join-Path $env:TEMP "touchpad-shield-release-$Tag.md"
    # gh on Windows reads --notes-file using system ANSI unless UTF-8 BOM is present
    Write-Utf8BomFile -Path $notesFile -Content $releaseNotes

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

    $bodyPreview = gh release view $Tag --repo $Repo --json body -q .body
    if ($bodyPreview -match '[\u00C3\u00E2\u00E6\u00E7\u00E8\u00E9\u00EF\u00F0\u00F1\u00F2\u00F3\u00F4\u00F5\u00F6\u00F8\u00F9\u00FA\u00FB\u00FC\u00FD\u00FE\u00FF]{3,}') {
        Write-Warning "Release body may contain mojibake. Re-check UTF-8 BOM and run: gh release view $Tag --repo $Repo"
    }

    Write-Host "Done. Release: https://github.com/$Repo/releases/tag/$Tag"
}
finally {
    Pop-Location
}
