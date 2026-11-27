# Compila e roda os testes nativos: motor (test_engine) e servicos da v14.1 (test_services).
# Uso: powershell -ExecutionPolicy Bypass -File native\tests\build_tests.ps1
$ErrorActionPreference = "Stop"
$NativeDir = Split-Path -Parent $PSScriptRoot
$Src = Join-Path $NativeDir "src"
$Out = Join-Path $NativeDir "build_tmp\tests"
New-Item -ItemType Directory -Force -Path $Out | Out-Null

$Clang = Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW*" -Recurse -Filter "x86_64-w64-mingw32-clang++.exe" -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
if (-not $Clang) { $c = Get-Command "x86_64-w64-mingw32-clang++" -ErrorAction SilentlyContinue; if ($c) { $Clang = $c.Source } }
if (-not $Clang) { throw "x86_64-w64-mingw32-clang++ not found." }

$Common = @("-std=c++20", "-O2", "-Wall", "-Wextra", "-Wno-unused-parameter", "-Wno-missing-field-initializers", "-DUNICODE", "-D_UNICODE", "-I$Src", "-I$NativeDir\third_party\json", "-I$NativeDir\third_party\imgui")
$ImGui = @("imgui", "imgui_draw", "imgui_tables", "imgui_widgets") | ForEach-Object { Join-Path $NativeDir "build_tmp\obj\imgui_$_.o" }
foreach ($o in $ImGui) { if (-not (Test-Path $o)) { throw "Objeto do ImGui ausente ($o). Rode native\build.ps1 primeiro." } }

Write-Host "[engine] test_engine"
& $Clang @Common "$NativeDir\tests\test_engine.cpp" "$Src\engine\ovr_engine.cpp" "$Src\engine\pes_engine.cpp" "$Src\engine\hooks.cpp" "$Src\engine\win32_mem.cpp" "$Src\engine\backup_retention.cpp" "$Src\engine\action_log.cpp" -lole32 -lshell32 -luser32 -o "$Out\test_engine.exe"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& "$Out\test_engine.exe"
if ($LASTEXITCODE -ne 0) { Write-Host "test_engine FALHOU"; exit $LASTEXITCODE }

Write-Host "[services] test_services"
& $Clang @Common "$NativeDir\tests\test_services.cpp" "$Src\engine\backup_retention.cpp" "$Src\engine\action_log.cpp" "$Src\engine\update_check.cpp" "$Src\ui\theme.cpp" "$Src\ui\i18n_es.cpp" "$Src\engine\win32_mem.cpp" @ImGui -lwinhttp -lole32 -lshell32 -luser32 -lgdi32 -o "$Out\test_services.exe"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& "$Out\test_services.exe"
if ($LASTEXITCODE -ne 0) { Write-Host "test_services FALHOU"; exit $LASTEXITCODE }
Write-Host "Todos os testes passaram."
