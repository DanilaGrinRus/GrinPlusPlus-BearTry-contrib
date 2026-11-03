# Line-3 common regression groups for the integrated Tests binary (no build).
#
# Runs the agreed cross-line regressions on ONE exact binary SHA, each group in its
# own one-shot isolated HOME/TMP:
#   advertised-tip           P2P advertised tip uses header head (line1 fix)
#   false-ban                local NRD exclusion must not classify the peer as bad
#   nrd-rollback-reorg       NRD index rollback/backup after reorg (line2)
#   rebuild-interruption     KernelIndex/NrdRebuild interruption + partial publish
#
# Parallel mode launches groups concurrently; those groups do not bind the fixed
# wallet-manager port 3421, so HOME/TMP isolation is sufficient here. Wallet/full
# suites must stay serialized (fixed 127.0.0.1:3421) - see CI_REPRO.md.
#
# Output: evidence/ci/<run-id>/regressions.json with per-group SHA/result/manifest.

param(
    [string]$Repo = 'C:\DHW\Gplus\src\grinplusplus',
    [string]$TestsExe = 'C:\DHW\Gplus\src\grinplusplus\bin\Release\Release\Tests.exe',
    [string]$ExpectedSha = '',
    [string]$RunId = ('regressions-' + (Get-Date -Format 'yyyyMMdd-HHmmss')),
    [string]$RunRoot = 'C:\DHW\.tmp\ci-iso',
    [string]$EvidenceRoot = 'C:\DHW\Gplus\src\grinplusplus\evidence\ci',
    [switch]$Parallel,
    [switch]$IncludeTask9,
    [string[]]$Only = @()
)

$ErrorActionPreference = 'Stop'
$Repo = (Resolve-Path -LiteralPath $Repo).Path
$TestsExe = (Resolve-Path -LiteralPath $TestsExe).Path
$runner = Join-Path $Repo 'tools\build\run-tests-isolated.ps1'
if (-not (Test-Path -LiteralPath $runner)) { throw "runner missing: $runner" }

$sha = (Get-FileHash -LiteralPath $TestsExe -Algorithm SHA256).Hash
if ($ExpectedSha -and $sha -ne $ExpectedSha) { throw "Tests SHA mismatch: expected $ExpectedSha actual $sha" }

$groups = [ordered]@{
    'advertised-tip'       = @('P2P::AdvertisedTipUsesHeaderHead', 'P2P::AdvertisedTipReadsOneSnapshot')
    'false-ban'            = @('NrdFalseBan*')
    'nrd-rollback-reorg'   = @('NrdRollback*', 'NrdReorgBackup*')
    'rebuild-interruption' = @('KernelIndex*', 'NrdRebuild*')
    # Task-9 (only present in binaries that include the API/index fixes).
    'get-kernel-handler'   = @('GetKernelHandler*')
    'kernel-index-schema'  = @('KernelIndex schema v2*')
    'kernel-duplicate-range' = @('TxHashSet duplicate-excess*')
}
$task9Groups = @('get-kernel-handler', 'kernel-index-schema', 'kernel-duplicate-range')
$selected = if ($Only.Count -gt 0) { $Only } else {
    @($groups.Keys | Where-Object { $IncludeTask9 -or ($task9Groups -notcontains $_) })
}
foreach ($g in $selected) { if (-not $groups.Contains($g)) { throw "unknown group: $g" } }

$runDir = Join-Path $EvidenceRoot $RunId
New-Item -ItemType Directory -Force -Path $runDir | Out-Null
$initial = Join-Path 'C:\DHW\.tmp' ('ci-initial-' + $RunId)
Copy-Item -LiteralPath $TestsExe -Destination (Join-Path 'C:\DHW\.tmp' ('Tests-' + $RunId + '.exe')) -Force -ErrorAction SilentlyContinue

function Invoke-Group([string]$name, [string]$id) {
    $filter = ($groups[$name] -join ',')
    $start = (Get-Date).ToUniversalTime()
    $output = & pwsh -NoProfile -ExecutionPolicy Bypass -File $runner -ExePath $TestsExe -RunId $id `
        -RunRoot $RunRoot -EvidenceRoot (Join-Path $runDir 'runs') -KeepBinary -TestArgs @($filter) -FailOnNoTests *>&1
    $exit = $LASTEXITCODE
    $manifestPath = Join-Path $RunRoot ($id + '\manifest.json')
    $manifest = if (Test-Path -LiteralPath $manifestPath) { Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json } else { $null }
    $inst = if ($manifest) { $manifest.instances[0] } else { $null }
    $instPass = ($manifest -and $inst -and $inst.status -eq 'PASS' -and $inst.exit_code.decimal -eq 0)
    return [ordered]@{
        group        = $name
        filter       = $filter
        run_id       = $id
        started_utc  = $start.ToString('o')
        exit_code    = $exit
        status       = if ($exit -eq 0 -and $instPass) { 'PASS' } else { 'FAIL' }
        no_tests_ran = [bool]($manifest -and $manifest.no_tests_ran)
        tests_sha256 = $sha
        sha_match    = [bool]($manifest -and $manifest.binary.source_sha256 -eq $sha)
        summary      = if ($inst) { @($inst.summary_lines | Select-Object -Last 3) } else { @() }
        manifest     = $manifestPath
        manifest_sha256 = if (Test-Path -LiteralPath $manifestPath) { (Get-FileHash $manifestPath -Algorithm SHA256).Hash } else { $null }
        stderr       = if ($inst) { $inst.stderr_path } else { $null }
        output_tail  = @($output | Select-Object -Last 4)
    }
}

$results = @()
if ($Parallel) {
    $jobs = @()
    foreach ($g in $selected) {
        $id = "$RunId-$g"
        $jobs += Start-Job -ScriptBlock {
            param($name, $id, $TestsExe, $runner, $RunRoot, $runDir, $sha, $groups)
            $filter = ($groups[$name] -join ',')
            & pwsh -NoProfile -ExecutionPolicy Bypass -File $runner -ExePath $TestsExe -RunId $id `
                -RunRoot $RunRoot -EvidenceRoot (Join-Path $runDir 'runs') -KeepBinary -TestArgs @($filter) -FailOnNoTests *>&1 | Out-Null
            $exit = $LASTEXITCODE
            $mp = Join-Path $RunRoot ($id + '\manifest.json')
            $m = if (Test-Path -LiteralPath $mp) { Get-Content -LiteralPath $mp -Raw | ConvertFrom-Json } else { $null }
            $instPass = ($m -and $m.instances[0].status -eq 'PASS' -and $m.instances[0].exit_code.decimal -eq 0)
            [ordered]@{
                group = $name; filter = $filter; run_id = $id; exit_code = $exit
                status = if ($exit -eq 0 -and $instPass) { 'PASS' } else { 'FAIL' }
                no_tests_ran = [bool]($m -and $m.no_tests_ran)
                tests_sha256 = $sha
                sha_match = [bool]($m -and $m.binary.source_sha256 -eq $sha)
                summary = if ($m) { @($m.instances[0].summary_lines | Select-Object -Last 3) } else { @() }
                manifest = $mp
                manifest_sha256 = if (Test-Path -LiteralPath $mp) { (Get-FileHash $mp -Algorithm SHA256).Hash } else { $null }
                stderr = if ($m) { $m.instances[0].stderr_path } else { $null }
            }
        } -ArgumentList $g, $id, $TestsExe, $runner, $RunRoot, $runDir, $sha, $groups
    }
    $results = @($jobs | ForEach-Object { Wait-Job $_ | Out-Null; Receive-Job $_ })
    $jobs | Remove-Job -Force -ErrorAction SilentlyContinue
} else {
    foreach ($g in $selected) { $results += Invoke-Group -name $g -id "$RunId-$g" }
}

$failed = @($results | Where-Object { $_.status -ne 'PASS' -or -not $_.sha_match })
$summary = [ordered]@{
    schema       = 'gpp-line3-ci-regressions/v1'
    run_id       = $RunId
    created_utc  = (Get-Date).ToUniversalTime().ToString('o')
    repo         = $Repo
    git_head     = (& git -C $Repo rev-parse HEAD).Trim()
    tests_exe    = $TestsExe
    tests_sha256 = $sha
    expected_sha = $ExpectedSha
    parallel     = [bool]$Parallel
    result       = if ($failed.Count -eq 0) { 'PASS' } else { 'FAIL' }
    failed_groups = @($failed | ForEach-Object { $_.group })
    groups       = $results
    note         = 'Groups are serialized by default; parallel mode is only for non-port-binding regression groups. Wallet/full suite binds fixed 127.0.0.1:3421 and must be serialized. Filters are passed via -TestArgs (quoted per argument) and -FailOnNoTests, so a misquoted/empty filter fails instead of producing a false PASS. Task-9 groups require -IncludeTask9 (or explicit -Only).'
}
$summaryPath = Join-Path $runDir 'regressions.json'
$summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $summaryPath -Encoding utf8
Copy-Item -LiteralPath $summaryPath -Destination (Join-Path $runDir ('regressions-' + $RunId + '.json')) -Force

Write-Host ''
Write-Host ("regressions: {0}  tests_sha={1}" -f $summary.result, $sha)
foreach ($r in $results) { Write-Host ("  {0,-22} {1} exit={2} sha_match={3}" -f $r.group, $r.status, $r.exit_code, $r.sha_match) }
Write-Host "evidence: $summaryPath"
if ($failed.Count -gt 0) { exit 1 } else { exit 0 }
