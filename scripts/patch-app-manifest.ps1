param(
    [Parameter(Mandatory = $true)]
    [string]$ManifestPath,
    [Parameter(Mandatory = $true)]
    [string]$AssemblyVersion
)

$ErrorActionPreference = "Stop"
$content = Get-Content -Raw -LiteralPath $ManifestPath
$content = $content -replace '(<assemblyIdentity\s+version=")[^"]+(")', ('${1}' + $AssemblyVersion + '${2}')
Set-Content -LiteralPath $ManifestPath -Value $content -NoNewline
