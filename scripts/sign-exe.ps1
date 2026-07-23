param(
    [Parameter(Mandatory = $true)]
    [string]$ExePath
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $ExePath)) {
    throw "Executable not found: $ExePath"
}

$CertSubject = "CN=ZiMiaoWorkshop"
$CertFriendlyName = "Touchpad Shield Code Signing"

function Get-SignToolPath {
    $kitRoot = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin"
    if (-not (Test-Path -LiteralPath $kitRoot)) {
        return $null
    }

    $signtool = Get-ChildItem -LiteralPath $kitRoot -Recurse -Filter "signtool.exe" -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match "\\x64\\signtool\.exe$" } |
        Sort-Object { [version]($_.Directory.Parent.Name) } -Descending |
        Select-Object -First 1

    if ($signtool) {
        return $signtool.FullName
    }

    return $null
}

$signtool = Get-SignToolPath
if (-not $signtool) {
    Write-Warning "signtool.exe not found. Skipping Authenticode signing for: $ExePath"
    return
}

$cert = Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert -ErrorAction SilentlyContinue |
    Where-Object { $_.Subject -eq $CertSubject -and $_.HasPrivateKey } |
    Sort-Object NotAfter -Descending |
    Select-Object -First 1

if (-not $cert) {
    $cert = New-SelfSignedCertificate `
        -Type CodeSigningCert `
        -Subject $CertSubject `
        -FriendlyName $CertFriendlyName `
        -CertStoreLocation Cert:\CurrentUser\My `
        -KeyExportPolicy Exportable `
        -KeyUsage DigitalSignature `
        -KeyAlgorithm RSA `
        -KeyLength 2048 `
        -NotAfter (Get-Date).AddYears(10)
    Write-Host "Created code signing certificate: $CertSubject"
}

$trusted = Get-ChildItem Cert:\CurrentUser\TrustedPublisher -ErrorAction SilentlyContinue |
    Where-Object { $_.Thumbprint -eq $cert.Thumbprint } |
    Select-Object -First 1

if (-not $trusted) {
    $tempCert = Join-Path $env:TEMP "TouchpadShield-signing.cer"
    Export-Certificate -Cert $cert -FilePath $tempCert | Out-Null
    Import-Certificate -FilePath $tempCert -CertStoreLocation Cert:\CurrentUser\TrustedPublisher | Out-Null
    Remove-Item -LiteralPath $tempCert -Force -ErrorAction SilentlyContinue
    Write-Host "Registered signing certificate in CurrentUser\TrustedPublisher."
}

Write-Host "Signing $ExePath ..."
& $signtool sign /fd SHA256 /sha1 $cert.Thumbprint /tr http://timestamp.digicert.com /td SHA256 $ExePath
if ($LASTEXITCODE -ne 0) {
    Write-Host "Timestamp server unavailable; signing without timestamp..."
    & $signtool sign /fd SHA256 /sha1 $cert.Thumbprint $ExePath
    if ($LASTEXITCODE -ne 0) {
        throw "signtool failed with exit code $LASTEXITCODE"
    }
}

Write-Host "Signed successfully. UAC publisher should display ZiMiaoWorkshop."
