# Line-3 CI pipeline (runnable locally and from a hosted runner).
#
# Stages are reported SEPARATELY (no combined unconditional PASS):
#   0 provenance      - git HEAD/status, exact binary SHA/size/PE ts, expected-SHA checks
#   1 normal-build    - runs tools/build/build.ps1 unless -SkipBuild (then NOT RUN: provided binary)
#   2 unit-regressions- agreed cross-line groups via ci-regressions.ps1 (SHA-bound, isolated HOME/TMP)
#   3 full-suite      - optional; serialized because wallet manager binds fixed 127.0.0.1:3421
#   4 asan-subset     - only if the ASan source binding matches current compiled sources; otherwise NOT RUN (STALE)
#   5 fuzz-smoke      - optional, advisory if fuzz binaries are source-stale; bounded, no safety claim
#   6 untested-zones  - explicit list of what this pipeline does NOT prove
#
# Output: evidence/ci/<run-id>/{summary.json,summary.md}
# Exit: 0 only if every REQUIRED stage passed and no stage failed. STALE/NOT RUN
# ASan/fuzz stages do not fake a PASS; they are reported as uncovered.

param(
    [string]$Repo = 'C:\DHW\Gplus\src\grinplusplus',
    [string]$TestsExe = '',
    [string]$ExpectedTestsSha = '',
    [string]$GrinNodeExe = '',
    [string]$ExpectedGrinNodeSha = '',
    [string]$RunId = ('ci-' + (Get-Date -Format 'yyyyMMdd-HHmmss')),
    [string]$EvidenceRoot = 'C:\DHW\Gplus\src\grinplusplus\evidence\ci',
    [string]$RunRoot = 'C:\DHW\.tmp\ci-iso',
    [switch]$SkipBuild,
    [switch]$RunFullSuite,
    [switch]$ParallelRegressions,
    [string]$AsanTestsMini = '',
    [switch]$AsanSourceCurrent,
    [string]$AsanTree = 'C:\DHW\.tmp\asan-line3\gpp-tree',
    [string]$FuzzBuildDir = '',
    [int]$FuzzIterations = 100,
    [double]$FuzzMaxSeconds = 20,
    [string[]]$FuzzTargets = @('bytebuffer', 'zip', 'nrd_rules')
)

$ErrorActionPreference = 'Stop'
$Repo = (Resolve-Path -LiteralPath $Repo).Path
if (-not $TestsExe) { $TestsExe = Join-Path $Repo 'bin\Release\Release\Tests.exe' }
$TestsExe = (Resolve-Path -LiteralPath $TestsExe).Path
$evDir = Join-Path $EvidenceRoot $RunId
New-Item -ItemType Directory -Force -Path $evDir | Out-Null
$startUtc = (Get-Date).ToUniversalTime()

function Get-FileSha([string]$Path) { if (Test-Path -LiteralPath $Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash } else { $null } }
function Get-PeTs([string]$Path) {
    $fs = [IO.File]::OpenRead($Path); try { $br = [IO.BinaryReader]::new($fs); $fs.Position = 0x3C; $o = $br.ReadInt32(); $fs.Position = $o + 8; ('0x{0:X8}' -f $br.ReadUInt32()) } finally { $fs.Dispose() }
}

$stages = [ordered]@{}

# 0 - provenance
$testsSha = Get-FileSha $TestsExe
$prov = [ordered]@{
    name = 'provenance'; status = 'PASS'
    git_head = (& git -C $Repo rev-parse HEAD).Trim()
    git_status_porcelain = @(& git -C $Repo status --porcelain)
    tests_exe = $TestsExe; tests_sha256 = $testsSha
    tests_bytes = (Get-Item $TestsExe).Length; tests_pe_ts = Get-PeTs $TestsExe
    expected_tests_sha = $ExpectedTestsSha
    tests_sha_match = ((-not $ExpectedTestsSha) -or ($testsSha -eq $ExpectedTestsSha))
    grin_node_exe = $GrinNodeExe
    grin_node_sha256 = if ($GrinNodeExe) { Get-FileSha $GrinNodeExe } else { $null }
    expected_grin_node_sha = $ExpectedGrinNodeSha
    grin_node_sha_match = ((-not $ExpectedGrinNodeSha) -or ((Get-FileSha $GrinNodeExe) -eq $ExpectedGrinNodeSha))
}
if (-not $prov.tests_sha_match -or -not $prov.grin_node_sha_match) { $prov.status = 'FAIL' }
$stages['provenance'] = $prov

# 1 - normal build (optional)
$stages['normal-build'] = if ($SkipBuild) {
    [ordered]@{ name = 'normal-build'; status = 'NOT RUN'; reason = 'provided binary; build not performed in this invocation' }
} else {
    $log = Join-Path $evDir 'normal-build.log'
    & pwsh -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Repo 'tools\build\build.ps1') -Target 'Tests' *>&1 |
        Tee-Object -FilePath $log | Out-Null
    [ordered]@{ name = 'normal-build'; status = if ($LASTEXITCODE -eq 0) { 'PASS' } else { 'FAIL' }; log = $log; exit_code = $LASTEXITCODE }
}

# 2 - cross-line regression groups
$regArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $Repo 'tools\build\ci-regressions.ps1'),
    '-Repo', $Repo, '-TestsExe', $TestsExe, '-RunId', "$RunId-reg", '-RunRoot', (Join-Path $RunRoot 'reg'),
    '-EvidenceRoot', $evDir)
if ($ExpectedTestsSha) { $regArgs += @('-ExpectedSha', $ExpectedTestsSha) }
if ($ParallelRegressions) { $regArgs += '-Parallel' }
& pwsh @regArgs | Out-Null
$regExit = $LASTEXITCODE
$regPath = Join-Path $evDir ($RunId + '-reg\regressions.json')
$reg = if (Test-Path -LiteralPath $regPath) { Get-Content -LiteralPath $regPath -Raw | ConvertFrom-Json } else { $null }
$stages['unit-regressions'] = [ordered]@{
    name = 'unit-regressions'; status = if ($regExit -eq 0) { 'PASS' } else { 'FAIL' }
    exit_code = $regExit; evidence = $regPath
    groups = if ($reg) { @($reg.groups | ForEach-Object { [ordered]@{ group = $_.group; status = $_.status; sha_match = $_.sha_match } }) } else { @() }
}

# 3 - optional full suite (serialized; fixed wallet-manager port)
if ($RunFullSuite) {
    $env:ASAN_OPTIONS = ''
    $fullId = "$RunId-full"
    & pwsh -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Repo 'tools\build\run-tests-isolated.ps1') `
        -ExePath $TestsExe -RunId $fullId -RunRoot (Join-Path $RunRoot 'full') -EvidenceRoot $evDir -KeepBinary | Out-Null
    $fullManifestPath = Join-Path (Join-Path $RunRoot 'full') ($fullId + '\manifest.json')
    $full = if (Test-Path -LiteralPath $fullManifestPath) { Get-Content -LiteralPath $fullManifestPath -Raw | ConvertFrom-Json } else { $null }
    $stdout = if ($full) { $full.instances[0].stdout_path } else { $null }
    $failLines = if ($stdout -and (Test-Path -LiteralPath $stdout)) { @(Select-String -Path $stdout -Pattern 'FAILED:' | ForEach-Object { $_.Line.Trim() } | Select-Object -Unique) } else { @() }
    $knownEnvPatterns = 'Test_CreateWallet|Test_Login|Test_EstimateFee|Test_Receive|Test_Slatepack|mg_start|Tor'
    $envLike = ($failLines | Where-Object { $_ -match $knownEnvPatterns }).Count
    $stages['full-suite'] = [ordered]@{
        name = 'full-suite'; status = if ($full -and $full.instances[0].exit_code.decimal -eq 0) { 'PASS' } elseif ($envLike -gt 0) { 'FAIL_ENV_DEPENDENT' } else { 'FAIL' }
        exit_code = if ($full) { $full.instances[0].exit_code.decimal } else { $null }
        summary = if ($full) { @($full.instances[0].summary_lines | Select-Object -Last 2) } else { @() }
        failing_tests = $failLines
        known_wallet_port_or_tor = $envLike
        manifest = $fullManifestPath
        note = 'Full suite serialized. Wallet manager binds fixed 127.0.0.1:3421; concurrent runs/TIME_WAIT cause mg_start failures. Slatepack decrypt paths showed rare full-suite-only failures (line3 stress: 3/5 full PASS, 2/5 FAIL; 8/8 Slatepack-only PASS).'
    }
} else {
    $stages['full-suite'] = [ordered]@{ name = 'full-suite'; status = 'NOT RUN'; reason = '-RunFullSuite not set' }
}

# 4 - ASan subset (never faked)
$asanStale = $true; $mismatch = $null
if (Test-Path -LiteralPath (Join-Path $AsanTree 'include')) {
    $mismatch = 0
    foreach ($root in @('CMakeLists.txt', 'cmake', 'include', 'src', 'tests', 'deps')) {
        $rp = Join-Path $Repo $root; $tp = Join-Path $AsanTree $root
        if (Test-Path -LiteralPath $rp -PathType Leaf) {
            if ((Get-FileSha $rp) -ne (Get-FileSha $tp)) { $mismatch++ }; continue
        }
        if (-not (Test-Path -LiteralPath $tp)) { $mismatch++; continue }
        $rep = @{}; Get-ChildItem -LiteralPath $rp -Recurse -File | ForEach-Object { $rep[$_.FullName.Substring($Repo.Length + 1)] = $_.FullName }
        $tre = @{}; Get-ChildItem -LiteralPath $tp -Recurse -File | ForEach-Object { $tre[$_.FullName.Substring($AsanTree.Length + 1)] = $_.FullName }
        foreach ($k in $rep.Keys) {
            if (-not $tre.ContainsKey($k) -or (Get-FileSha $rep[$k]) -ne (Get-FileSha $tre[$k])) { $mismatch++ }
        }
    }
    $asanStale = ($mismatch -gt 0) -or (-not $AsanSourceCurrent)
}
if (-not $AsanTestsMini) {
    $stages['asan-subset'] = [ordered]@{ name = 'asan-subset'; status = 'NOT RUN'; reason = 'no ASan TestsMini provided' }
} elseif ($asanStale) {
    $stages['asan-subset'] = [ordered]@{ name = 'asan-subset'; status = 'NOT RUN (STALE)'; reason = "ASan source binding does not match current compiled sources (mismatch=$mismatch); rebuild requires explicit build authorization"; asan_testsmini = $AsanTestsMini; source_mismatch_count = $mismatch }
} else {
    $asanId = "$RunId-asan"
    $env:ASAN_OPTIONS = 'detect_leaks=0'; $env:GRINPP_NRD_CASES = Join-Path $Repo 'tools\audit\nrd\cases\nrd_cases.json'
    & pwsh -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Repo 'tools\fuzz\run_asan_teardown_regression.ps1') -ExePath $AsanTestsMini -RunId $asanId -KeepBinary | Out-Null
    $asanExit = $LASTEXITCODE
    $stages['asan-subset'] = [ordered]@{ name = 'asan-subset'; status = if ($asanExit -eq 0) { 'PASS' } else { 'FAIL' }; exit_code = $asanExit; asan_testsmini_sha256 = (Get-FileSha $AsanTestsMini); note = 'teardown regression + subset evidence from prior ASan filtered runs' }
}

# 5 - fuzz smoke (advisory, bounded, never a safety claim)
if ($FuzzBuildDir) {
    $fuzzId = "$RunId-fuzz"
    & pwsh -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Repo 'tools\fuzz\run_fuzz_smoke.ps1') `
        -BuildDir $FuzzBuildDir -Targets ($FuzzTargets -join ',') -Iterations $FuzzIterations -MaxSeconds $FuzzMaxSeconds -RunId $fuzzId | Out-Null
    $fuzzExit = $LASTEXITCODE
    $fuzzManifest = Join-Path 'C:\DHW\.tmp\asan-line3\runs' ($fuzzId + '\manifest.json')
    $stages['fuzz-smoke'] = [ordered]@{
        name = 'fuzz-smoke'; status = if ($fuzzExit -eq 0) { 'PASS (ADVISORY)' } else { 'FAIL' }
        exit_code = $fuzzExit; iterations = $FuzzIterations; max_seconds = $FuzzMaxSeconds; targets = $FuzzTargets
        evidence = $fuzzManifest
        note = 'Bounded mutation smoke; coverage not measured; source binding may be stale (see asan-subset stage). Not a safety proof.'
    }
} else {
    $stages['fuzz-smoke'] = [ordered]@{ name = 'fuzz-smoke'; status = 'NOT RUN'; reason = 'no -FuzzBuildDir provided' }
}

# 6 - untested zones (always reported, never a PASS)
$stages['untested-zones'] = [ordered]@{
    name = 'untested-zones'; status = 'REPORTED'
    zones = @(
        [ordered]@{ zone = 'rocksdb/database/chainstate under ASan'; status = 'NOT COVERED'; close_criterion = 'ASan-rebuilt rocksdb + C++ sysroot and full Tests.exe under ASan' },
        [ordered]@{ zone = 'coverage-guided fuzzing'; status = 'NOT AVAILABLE'; close_criterion = 'clang-cl/libFuzzer or external clang runner with coverage publication' },
        [ordered]@{ zone = 'snapshot activation fault injection'; status = 'NOT COVERED'; close_criterion = 'crash/failure before/after staging, rename/commit, disk-full harness' },
        [ordered]@{ zone = 'NRD transitions fault injection'; status = 'PARTIAL (unit simulations)'; close_criterion = 'read-error during rebuild, interruption before commit, reorg rollback under ASan fault injection' },
        [ordered]@{ zone = 'wallet/Tor environment determinism'; status = 'ENV-DEPENDENT'; close_criterion = 'serialize fixed port 3421 or allocate per-run ports; deterministic full-suite evidence' },
        [ordered]@{ zone = 'ordinary-binary live G-09'; status = 'NOT VERIFIED'; close_criterion = 'live chain/peer acceptance evidence' },
        [ordered]@{ zone = 'historical Tests.exe 0xC0000409'; status = 'NOT VERIFIED'; close_criterion = 'preserved binary/dump + reproducible scenario' },
        [ordered]@{ zone = 'crafted-zip ProcessTxHashSet end-to-end'; status = 'NOT VERIFIED'; close_criterion = 'line2/line1 end-to-end harness evidence' }
    )
}

# overall
$failedStages = @($stages.Values | Where-Object { $_.status -in 'FAIL', 'FAIL_ENV_DEPENDENT' })
$notRun = @($stages.Values | Where-Object { $_.status -match 'NOT RUN' })
$overall = if ($failedStages.Count -gt 0) { 'FAIL' } elseif ($notRun.Count -gt 0) { 'PASS_WITH_UNCOVERED' } else { 'PASS' }
$summary = [ordered]@{
    schema = 'gpp-line3-ci-pipeline/v1'
    run_id = $RunId; created_utc = $startUtc.ToString('o'); repo = $Repo
    overall_result = $overall
    failed_stages = @($failedStages | ForEach-Object { $_.name })
    not_run_stages = @($notRun | ForEach-Object { $_.name })
    stages = $stages
    notes = @(
        'Stages are independent evidence. PASS of normal build/unit/asan/fuzz does not prove live-network compatibility.',
        'ASan/fuzz stages are never converted to PASS when their source binding is stale.',
        'Wallet/full suite must be serialized: fixed 127.0.0.1:3421.'
    )
}
$summaryPath = Join-Path $evDir 'summary.json'
$summary | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $summaryPath -Encoding utf8

$md = @()
$md += "# CI pipeline run $RunId"
$md += ''
$md += "Overall: **$overall**"
$md += ''
$md += '| stage | status | note |'
$md += '|---|---|---|'
foreach ($s in $stages.Values) { $md += ("| {0} | {1} | {2} |" -f $s.name, $s.status, ($s.reason ?? $s.note ?? '')) }
$md += ''
$md += "Evidence: ``$summaryPath``"
$md | Set-Content -LiteralPath (Join-Path $evDir 'summary.md') -Encoding utf8

Write-Host ''
Write-Host "CI pipeline: $overall  (run $RunId)"
foreach ($s in $stages.Values) { Write-Host ("  {0,-20} {1}" -f $s.name, $s.status) }
Write-Host "summary: $summaryPath"
if ($overall -eq 'FAIL') { exit 1 } else { exit 0 }
