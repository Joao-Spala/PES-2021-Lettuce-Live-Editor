# Builds standalone native C++20 + DirectX 11 + Dear ImGui executable:
#   dist\PES 2021 Workbench Native.exe
# Separate from the Python build (dist\PES 2021 Workbench.exe).

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$NativeDir = $PSScriptRoot
$DistDir = Join-Path $Root "dist"
$ObjDir = Join-Path $NativeDir "build_tmp\obj"
New-Item -ItemType Directory -Force -Path $DistDir, $ObjDir | Out-Null

# Locate LLVM-MinGW 64-bit UCRT clang++
$Clang = Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW*" -Recurse -Filter "x86_64-w64-mingw32-clang++.exe" -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
if (-not $Clang) {
    $cmd = Get-Command "x86_64-w64-mingw32-clang++" -ErrorAction SilentlyContinue
    if ($cmd) { $Clang = $cmd.Source }
}
if (-not $Clang) {
    throw "x86_64-w64-mingw32-clang++ not found. Install MartinStorsjo.LLVM-MinGW.UCRT via winget."
}

Write-Host "Compiler: $Clang"

# Step 1: Regenerate game_data.hpp from verified Python tables
$VenvPy = Join-Path $Root "app\.venv\Scripts\python.exe"
if (Test-Path $VenvPy) {
    & $VenvPy (Join-Path $NativeDir "gen_data.py")
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$CommonFlags = @(
    "-std=c++20",
    "-O3",
    "-flto",
    "-DNDEBUG",
    "-DUNICODE",
    "-D_UNICODE",
    "-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS",
    "-I$NativeDir\third_party\imgui",
    "-I$NativeDir\third_party\json",
    "-I$NativeDir\src"
)

$ImGuiSources = @(
    "$NativeDir\third_party\imgui\imgui.cpp",
    "$NativeDir\third_party\imgui\imgui_draw.cpp",
    "$NativeDir\third_party\imgui\imgui_tables.cpp",
    "$NativeDir\third_party\imgui\imgui_widgets.cpp",
    "$NativeDir\third_party\imgui\imgui_impl_win32.cpp",
    "$NativeDir\third_party\imgui\imgui_impl_dx11.cpp",
    "$NativeDir\third_party\imgui\imgui_stdlib.cpp"
)

$AppSources = @(
    "$NativeDir\src\engine\win32_mem.cpp",
    "$NativeDir\src\engine\backup_retention.cpp",
    "$NativeDir\src\engine\action_log.cpp",
    "$NativeDir\src\engine\update_check.cpp",
    "$NativeDir\src\engine\hooks.cpp",
    "$NativeDir\src\engine\ovr_engine.cpp",
    "$NativeDir\src\engine\pes_engine.cpp",
    "$NativeDir\src\ui\i18n_es.cpp",
    "$NativeDir\src\ui\theme.cpp",
    "$NativeDir\src\ui\icons.cpp",
    "$NativeDir\src\ui\widgets.cpp",
    "$NativeDir\src\ui\workbench_ui.cpp",
    "$NativeDir\src\main.cpp"
)

$Objects = @()

# Compile Win32 icon & version resource
$BinDir = Split-Path -Parent $Clang
$Windres = Join-Path $BinDir "x86_64-w64-mingw32-windres.exe"
$RcFile = Join-Path $NativeDir "assets\app.rc"
if ((Test-Path $Windres) -and (Test-Path $RcFile)) {
    $ResObj = Join-Path $ObjDir "app_res.o"
    Write-Host "  [RC] app.rc (app_icon.ico + version info)"
    & $Windres "-I$NativeDir\assets" $RcFile -O coff -o $ResObj
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    $Objects += $ResObj
}

# Compile ImGui object files (cached if source unchanged)
foreach ($src in $ImGuiSources) {
    $name = [System.IO.Path]::GetFileNameWithoutExtension($src)
    $obj = Join-Path $ObjDir "imgui_$name.o"
    $Objects += $obj
    if (-not (Test-Path $obj) -or (Get-Item $src).LastWriteTimeUtc -gt (Get-Item $obj).LastWriteTimeUtc) {
        Write-Host "  [CC] $name.cpp"
        & $Clang @CommonFlags -c $src -o $obj
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
}

# Compile Workbench sources with strict warnings
$AppWarnings = @("-Wall", "-Wextra", "-Wno-unused-parameter", "-Wno-missing-field-initializers")
foreach ($src in $AppSources) {
    $name = [System.IO.Path]::GetFileNameWithoutExtension($src)
    $obj = Join-Path $ObjDir "app_$name.o"
    $Objects += $obj
    Write-Host "  [CC] $name.cpp"
    & $Clang @CommonFlags @AppWarnings -c $src -o $obj
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

# Link standalone stripped static 64-bit Windows GUI EXE
$OutExe = Join-Path $DistDir "PES 2021 Workbench Native.exe"
Write-Host "  [LD] $OutExe"
& $Clang -std=c++20 -O3 -flto -s -mwindows -municode -static `
    @Objects `
    -o $OutExe `
    -ld3d11 -ldxgi -ld3dcompiler -ldwmapi -limm32 -lole32 -lshell32 -lcomdlg32 -lgdi32 -luser32 -lwinhttp
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$szMB = [math]::Round((Get-Item $OutExe).Length / 1MB, 2)
$sha = (Get-FileHash $OutExe -Algorithm SHA256).Hash
Write-Host ""
Write-Host "SUCCESS: $OutExe ($szMB MB)"
Write-Host "SHA256:  $sha"
