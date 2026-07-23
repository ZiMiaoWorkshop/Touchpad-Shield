param(
    [ValidateSet("Prepare", "Finalize", "Revert")]
    [string]$Phase = "Prepare"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$VersionPropsPath = Join-Path $Root "version\Version.props"
$StampPath = Join-Path $Root "version\build-stamp.json"
$PendingPath = Join-Path $Root "version\.build-pending.json"

$DerivedRelativePaths = @(
    "installer/TouchpadShield-beta.nsi",
    "installer/TouchpadShield-release.nsi",
    "src/TouchpadShield/Assets/TouchpadShield.ico",
    "src/TouchpadShield/Assets/TouchpadShieldLogo.png",
    "installer/TouchpadShield.ico"
)

function Test-IsDerivedPath {
    param([string]$RelativePath)

    $normalized = $RelativePath.Replace('\', '/')
    return $DerivedRelativePaths -contains $normalized
}

function Get-NormalizedFileHash {
    param(
        [string]$FilePath,
        [string]$RelativePath
    )

    if ($RelativePath.Replace('\', '/') -eq 'version/Version.props') {
        $content = Get-Content -LiteralPath $FilePath -Raw
        $content = [regex]::Replace(
            $content,
            '(?m)^\s*<TouchpadShieldBuildNumber>.*?</TouchpadShieldBuildNumber>\s*\r?\n?',
            '')
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($content)
        $stream = New-Object System.IO.MemoryStream(,$bytes)
        return (Get-FileHash -InputStream $stream -Algorithm SHA256).Hash.ToLowerInvariant()
    }

    return (Get-FileHash -LiteralPath $FilePath -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Get-SourceFingerprint {
    $relativePaths = @(
        "src",
        "scripts",
        "installer",
        "config",
        "Picture",
        "TouchpadShield.sln",
        (Join-Path "version" "Version.props"),
        (Join-Path "version" "Version.targets")
    )

    $entries = New-Object System.Collections.Generic.List[string]

    foreach ($relativePath in $relativePaths) {
        $fullPath = Join-Path $Root $relativePath
        if (-not (Test-Path -LiteralPath $fullPath)) {
            continue
        }

        if (Test-Path -LiteralPath $fullPath -PathType Leaf) {
            $normalizedRelative = $relativePath.Replace('\', '/')
            $hash = Get-NormalizedFileHash -FilePath $fullPath -RelativePath $normalizedRelative
            $entries.Add("$normalizedRelative|$hash")
            continue
        }

        Get-ChildItem -LiteralPath $fullPath -Recurse -File |
            Sort-Object FullName |
            ForEach-Object {
                $normalizedRelative = $_.FullName.Substring($Root.Length).TrimStart('\', '/').Replace('\', '/')
                if ($normalizedRelative -eq 'version/build-stamp.json' -or
                    $normalizedRelative -eq 'version/.build-pending.json' -or
                    (Test-IsDerivedPath -RelativePath $normalizedRelative)) {
                    return
                }

                $hash = Get-NormalizedFileHash -FilePath $_.FullName -RelativePath $normalizedRelative
                $entries.Add("$normalizedRelative|$hash")
            }
    }

    $payload = ($entries | Sort-Object) -join "`n"
    $payloadBytes = [System.Text.Encoding]::UTF8.GetBytes($payload)
    $stream = New-Object System.IO.MemoryStream(,$payloadBytes)
    return (Get-FileHash -InputStream $stream -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Get-BuildNumber {
    [xml]$props = Get-Content -LiteralPath $VersionPropsPath
    $versionGroup = $props.Project.PropertyGroup | Where-Object { $_.Label -eq "Version" } | Select-Object -First 1
    return [string]$versionGroup.TouchpadShieldBuildNumber
}

function Set-BuildNumber {
    param([string]$BuildNumber)

    $content = [System.IO.File]::ReadAllText($VersionPropsPath)
    $updated = [regex]::Replace(
        $content,
        '(?<=\<TouchpadShieldBuildNumber\>)[^<]+(?=\</TouchpadShieldBuildNumber\>)',
        $BuildNumber)
    if ($updated -eq $content) {
        throw "TouchpadShieldBuildNumber not found in $VersionPropsPath"
    }

    $utf8WithBom = New-Object System.Text.UTF8Encoding $true
    [System.IO.File]::WriteAllText($VersionPropsPath, $updated, $utf8WithBom)
}

function Write-PendingState {
    param(
        [string]$PreviousBuild,
        [string]$NewBuild,
        [string]$Fingerprint,
        [bool]$IsInitial = $false
    )

    $pending = [ordered]@{
        previousBuild = $PreviousBuild
        newBuild      = $NewBuild
        fingerprint   = $Fingerprint
        isInitial     = $IsInitial
    }
    ($pending | ConvertTo-Json -Compress) | Set-Content -LiteralPath $PendingPath -Encoding UTF8 -NoNewline
}

function Write-BuildStamp {
    param(
        [string]$Fingerprint,
        [string]$BuildNumber
    )

    $stamp = [ordered]@{
        fingerprint = $Fingerprint
        buildNumber = $BuildNumber
        updatedAt   = (Get-Date).ToString("o")
    }
    ($stamp | ConvertTo-Json -Compress) | Set-Content -LiteralPath $StampPath -Encoding UTF8 -NoNewline
}

switch ($Phase) {
    "Prepare" {
        $fingerprint = Get-SourceFingerprint
        $currentBuild = Get-BuildNumber

        if (-not (Test-Path -LiteralPath $StampPath)) {
            Write-PendingState -PreviousBuild $currentBuild -NewBuild $currentBuild -Fingerprint $fingerprint -IsInitial $true
            Write-Host "First successful build will record baseline stamp at build $currentBuild."
            return
        }

        $stamp = Get-Content -LiteralPath $StampPath -Raw | ConvertFrom-Json
        if ($stamp.fingerprint -eq $fingerprint) {
            Write-Host "Source unchanged since last successful build; keeping build $currentBuild."
            return
        }

        $previousBuild = $currentBuild
        $nextBuild = "{0:D4}" -f ([int]$previousBuild + 1)
        Set-BuildNumber -BuildNumber $nextBuild
        Write-PendingState -PreviousBuild $previousBuild -NewBuild $nextBuild -Fingerprint $fingerprint

        Write-Host "Source changed; build number incremented: $previousBuild -> $nextBuild"
    }

    "Finalize" {
        if (-not (Test-Path -LiteralPath $PendingPath)) {
            return
        }

        $pending = Get-Content -LiteralPath $PendingPath -Raw | ConvertFrom-Json
        $fingerprint = Get-SourceFingerprint
        Write-BuildStamp -Fingerprint $fingerprint -BuildNumber $pending.newBuild
        Remove-Item -LiteralPath $PendingPath -Force

        if ($pending.isInitial) {
            Write-Host "Recorded baseline build stamp at $($pending.newBuild)."
        } else {
            Write-Host "Build stamp updated to $($pending.newBuild)."
        }
    }

    "Revert" {
        if (-not (Test-Path -LiteralPath $PendingPath)) {
            return
        }

        $pending = Get-Content -LiteralPath $PendingPath -Raw | ConvertFrom-Json
        if ($pending.newBuild -ne $pending.previousBuild) {
            Set-BuildNumber -BuildNumber $pending.previousBuild
            Write-Host "Build failed; reverted build number to $($pending.previousBuild)."
        } else {
            Write-Host "Build failed before baseline stamp was recorded."
        }

        Remove-Item -LiteralPath $PendingPath -Force
    }
}
