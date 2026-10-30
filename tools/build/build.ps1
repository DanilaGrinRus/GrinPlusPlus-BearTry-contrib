# Build Grin++ against the local dependency sysroot (Windows x64, MSVC, static CRT).
#
# The pinned vcpkg (2020.02) cannot run here: it predates VS2022 and its HTTPS
# downloads fail inside this sandbox. tools/deps/build_sysroot.py builds the same
# pinned dependency versions into .toolchain/sysroot instead, so this script only
# needs CMake + MSBuild.
#
# Usage:
#   pwsh tools/build/build.ps1                 # configure + build GrinNode and Tests
#   pwsh tools/build/build.ps1 -ConfigureOnly
#   pwsh tools/build/build.ps1 -Target GrinNode
param(
    [switch]$ConfigureOnly,
    [string]$Target = "GrinNode;Tests"
)

$ErrorActionPreference = "Stop"

$Repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Toolchain = Join-Path (Split-Path $Repo -Parent) "..\.toolchain"
$Toolchain = (Resolve-Path $Toolchain).Path
$Sysroot = Join-Path $Toolchain "sysroot"
$BuildDir = Join-Path $Toolchain "grinpp-build"
$LogDir = Join-Path $Toolchain "logs"

$Cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$Msbuild = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"

foreach ($tool in @($Cmake, $Msbuild)) {
    if (-not (Test-Path $tool)) { throw "required tool not found: $tool" }
}
if (-not (Test-Path (Join-Path $Sysroot "share\fmt\fmt-config.cmake"))) {
    throw "dependency sysroot missing; run: python tools/deps/build_sysroot.py"
}

New-Item -ItemType Directory -Force -Path $LogDir, "C:\DHW\.tmp\deps" | Out-Null
$env:TMP = "C:\DHW\.tmp\deps"
$env:TEMP = $env:TMP
$env:MSBUILDDISABLENODEREUSE = "1"

Write-Host "repo      : $Repo"
Write-Host "sysroot   : $Sysroot"
Write-Host "build dir : $BuildDir"

$configureArgs = @(
    "-S", $Repo,
    "-B", $BuildDir,
    "-G", "Visual Studio 17 2022",
    "-A", "x64",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_PREFIX_PATH=$Sysroot",
    "-DGRINPP_TESTS=ON",
    "-DGRINPP_TOOLS=ON"
)
& $Cmake @configureArgs
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed ($LASTEXITCODE)" }
if ($ConfigureOnly) { return }

# No /m: parallel MSBuild uses named pipes, which the sandbox blocks.
& $Msbuild (Join-Path $BuildDir "GRIN_PLUS_PLUS.sln") `
    /p:Configuration=Release /p:Platform=x64 "/t:$Target" `
    /p:TrackFileAccess=false /p:UseSharedCompilation=false /nr:false `
    /v:m /nologo "/flp:LogFile=$LogDir\grinpp-build.log;Verbosity=normal"
$exit = $LASTEXITCODE
if ($exit -ne 0) { throw "build failed ($exit), see $LogDir\grinpp-build.log" }
Write-Host "build OK"
