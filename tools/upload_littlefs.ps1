param(
  [Parameter(Mandatory = $true)]
  [string]$Port,

  [string]$SketchDir = "CruiseController",
  [string]$ImagePath = "build\littlefs.bin"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$dataDir = Join-Path $repoRoot "$SketchDir\data"
$imageFullPath = Join-Path $repoRoot $ImagePath
$imageDir = Split-Path -Parent $imageFullPath

$mklittlefs = Join-Path $env:LOCALAPPDATA "Arduino15\packages\esp32\tools\mklittlefs\4.0.2-db0513a\mklittlefs.exe"
$esptool = Join-Path $env:LOCALAPPDATA "Arduino15\packages\esp32\tools\esptool_py\5.3.0\esptool.exe"

if (!(Test-Path $mklittlefs)) {
  throw "mklittlefs not found: $mklittlefs"
}

if (!(Test-Path $esptool)) {
  throw "esptool not found: $esptool"
}

if (!(Test-Path $dataDir)) {
  throw "data directory not found: $dataDir"
}

New-Item -ItemType Directory -Force -Path $imageDir | Out-Null

& $mklittlefs -c $dataDir -b 4096 -p 256 -s 0x160000 $imageFullPath
if ($LASTEXITCODE -ne 0) {
  throw "mklittlefs failed: $LASTEXITCODE"
}

& $esptool --chip esp32s3 --port $Port --baud 921600 --before default-reset --after hard-reset write-flash -z --flash-mode keep --flash-freq keep --flash-size keep 0x290000 $imageFullPath
if ($LASTEXITCODE -ne 0) {
  throw "esptool failed: $LASTEXITCODE"
}
