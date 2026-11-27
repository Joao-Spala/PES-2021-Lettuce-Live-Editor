# Lettuce Live Editor: assinatura da atualizacao automatica (so para o mantenedor, nao faz parte do app).
#
# O app so instala uma versao nova sozinho quando a release traz update_manifest.txt assinado com a chave
# privada do mantenedor (ECDSA P-256 sobre SHA-256). A chave publica esta compilada no app
# (native/src/engine/self_update.hpp, kUpdatePublicKey). Sem assinatura valida nada e instalado, entao uma conta
# do GitHub comprometida, sozinha, nao consegue empurrar codigo para os usuarios.
#
# A chave e a mesma do mod VAR (um unico backup): %USERPROFILE%\.pes21-var-mod\update_signing_key.txt, fora de
# qualquer repositorio. O manifesto tem cabecalho proprio ("lettuce-live-editor update 1"), entao um manifesto de
# um projeto nao vale no outro. Para usar uma chave so do Lettuce, passe -KeyFile e troque a chave publica no
# self_update.hpp.
#
# Uso (depois de rodar native\build.ps1 com a versao nova):
#   powershell -File native\sign_update.ps1 -Version 14.2.0 -OutDir C:\caminho\da\release
#
# Gera em -OutDir os tres arquivos que sobem para a release junto com o zip:
#   Lettuce_Live_Editor.exe, update_manifest.txt, update_manifest.sig

param(
    [Parameter(Mandatory = $true)][string]$Version,
    [Parameter(Mandatory = $true)][string]$OutDir,
    [string]$KeyFile = "",
    [string]$ExePath = ""
)

$ErrorActionPreference = "Stop"
# Os padroes sao calculados aqui: $PSScriptRoot pode vir vazio na avaliacao dos padroes do param
$ScriptDir = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
if (-not $KeyFile) { $KeyFile = Join-Path $env:USERPROFILE ".pes21-var-mod\update_signing_key.txt" }
if (-not $ExePath) { $ExePath = Join-Path (Split-Path -Parent $ScriptDir) "dist\PES 2021 Workbench Native.exe" }
$Curve = [System.Security.Cryptography.ECCurve+NamedCurves]::nistP256
$Sha256Name = [System.Security.Cryptography.HashAlgorithmName]::SHA256

function ConvertTo-Hex([byte[]]$b) { return -join ($b | ForEach-Object { $_.ToString("x2") }) }
function ConvertFrom-Hex([string]$h) {
    $h = $h.Trim()
    $out = New-Object byte[] ($h.Length / 2)
    for ($i = 0; $i -lt $out.Length; $i++) { $out[$i] = [Convert]::ToByte($h.Substring($i * 2, 2), 16) }
    return $out
}

if (-not ($Version -match '^\d+\.\d+\.\d+$')) { throw "Passe -Version no formato 14.2.0." }
if (-not (Test-Path -LiteralPath $KeyFile)) { throw "Chave de assinatura nao encontrada em $KeyFile." }
if (-not (Test-Path -LiteralPath $ExePath)) { throw "Executavel nao encontrado em $ExePath. Rode native\build.ps1 primeiro." }

# O executavel precisa ser a versao que esta sendo assinada
$fileVersion = (Get-Item -LiteralPath $ExePath).VersionInfo.FileVersion
if (-not ($fileVersion -like "$Version.*")) { throw "O executavel e da versao $fileVersion, nao $Version. Atualize app_version.hpp e app.rc e recompile." }

# Chave privada (arquivo "d = / x = / y = " em hexadecimal)
$vals = @{}
foreach ($ln in [System.IO.File]::ReadAllLines($KeyFile)) {
    if ($ln -match '^\s*([dxy])\s*=\s*([0-9a-f]{64})\s*$') { $vals[$Matches[1]] = ConvertFrom-Hex $Matches[2] }
}
if ($vals.Count -ne 3) { throw "Arquivo de chave malformado: $KeyFile." }

# A chave publica compilada no app precisa ser a desta chave. Senao todo cliente recusaria a atualizacao.
$hdrPath = Join-Path $ScriptDir "src\engine\self_update.hpp"
$hdr = [System.IO.File]::ReadAllText($hdrPath)
$blk = [regex]::Match($hdr, 'kUpdatePublicKey\s*=\s*\{(?<body>[^}]*)\}')
if (-not $blk.Success) { throw "kUpdatePublicKey nao encontrada em $hdrPath." }
$compiled = @([regex]::Matches($blk.Groups['body'].Value, '0x([0-9A-Fa-f]{2})') | ForEach-Object { [Convert]::ToByte($_.Groups[1].Value, 16) })
$mine = [byte[]]($vals["x"] + $vals["y"])
if ($compiled.Count -ne 64 -or (ConvertTo-Hex ([byte[]]$compiled)) -ne (ConvertTo-Hex $mine)) {
    throw "A chave publica compilada no app nao e a desta chave de assinatura. O app recusaria esta atualizacao."
}

$priv = New-Object System.Security.Cryptography.ECParameters
$priv.Curve = $Curve
$priv.D = $vals["d"]
$q = New-Object System.Security.Cryptography.ECPoint
$q.X = $vals["x"]
$q.Y = $vals["y"]
$priv.Q = $q
$signer = [System.Security.Cryptography.ECDsa]::Create()
$signer.ImportParameters($priv)

$exeBytes = [System.IO.File]::ReadAllBytes($ExePath)
if ($exeBytes.Length -lt 4096 -or $exeBytes[0] -ne 0x4D -or $exeBytes[1] -ne 0x5A) { throw "$ExePath nao parece um executavel." }
$sha = (Get-FileHash -LiteralPath $ExePath -Algorithm SHA256).Hash.ToLowerInvariant()

$manifest = "lettuce-live-editor update 1`n" +
            "version = $Version`n" +
            "exe = $sha $($exeBytes.Length)`n"
$manifestBytes = [System.Text.Encoding]::ASCII.GetBytes($manifest)
$sig = $signer.SignData($manifestBytes, $Sha256Name)

# Confere a assinatura com a metade publica antes de gravar
$pub = New-Object System.Security.Cryptography.ECParameters
$pub.Curve = $Curve
$pq = New-Object System.Security.Cryptography.ECPoint
$pq.X = $vals["x"]
$pq.Y = $vals["y"]
$pub.Q = $pq
$verifier = [System.Security.Cryptography.ECDsa]::Create()
$verifier.ImportParameters($pub)
if (-not $verifier.VerifyData($manifestBytes, $sig, $Sha256Name)) { throw "A conferencia da assinatura falhou." }

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
[System.IO.File]::WriteAllBytes((Join-Path $OutDir "update_manifest.txt"), $manifestBytes)
[System.IO.File]::WriteAllText((Join-Path $OutDir "update_manifest.sig"), (ConvertTo-Hex $sig) + "`n", [System.Text.Encoding]::ASCII)
Copy-Item -Force -LiteralPath $ExePath -Destination (Join-Path $OutDir "Lettuce_Live_Editor.exe")
Write-Host "[sign] update_manifest.txt, update_manifest.sig e Lettuce_Live_Editor.exe gravados em $OutDir"
Write-Host $manifest
