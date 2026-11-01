param(
    [switch]$SkipBuild,
    [string]$TestHome = 'C:\DHW\.tmp\ci-home'
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Set-Location $repo

$shortTemp = 'C:\DHW\.tmp\deps'
New-Item -ItemType Directory -Force $shortTemp | Out-Null
$env:TMP = $shortTemp
$env:TEMP = $shortTemp

if (-not $SkipBuild) {
    & pwsh -NoProfile -ExecutionPolicy Bypass -File (Join-Path $repo 'tools\build\build.ps1') -Target GrinNode
    if ($LASTEXITCODE -ne 0) { throw 'GrinNode build failed' }

    & pwsh -NoProfile -ExecutionPolicy Bypass -File (Join-Path $repo 'tools\build\build.ps1') -Target Tests
    if ($LASTEXITCODE -ne 0) { throw 'Tests build failed' }
}

New-Item -ItemType Directory -Force $TestHome | Out-Null
$env:HOMEDRIVE = 'C:'
$env:HOMEPATH = $TestHome

$tests = Join-Path $repo 'bin\Release\Release\Tests.exe'
$node = Join-Path $repo 'bin\Release\Release\GrinNode.exe'
if (-not (Test-Path $tests)) { throw "Missing tests binary: $tests" }
if (-not (Test-Path $node)) { throw "Missing node binary: $node" }

& $tests --reporter compact
$testExit = $LASTEXITCODE
if ($testExit -ne 0) { throw "Tests failed with exit code $testExit" }

$rev = (& git rev-parse HEAD).Trim()
$evidenceDir = Join-Path $repo "evidence\ci\$rev"
New-Item -ItemType Directory -Force $evidenceDir | Out-Null

$manifest = [ordered]@{
    git_rev = $rev
    generated_at = (Get-Date).ToString('o')
    test_home = $TestHome
    temp = $shortTemp
    GrinNode = (Get-FileHash $node -Algorithm SHA256).Hash
    Tests = (Get-FileHash $tests -Algorithm SHA256).Hash
    test_exit_code = $testExit
}
$manifest | ConvertTo-Json | Set-Content (Join-Path $evidenceDir 'manifest.json') -Encoding UTF8
$manifest | Format-List
Write-Host "Evidence written to $evidenceDir"
