# One-shot isolated runner for Grin++ Tests.exe (Line 3 evidence harness).
#
# Why: TestMain.cpp runs
#     FileUtil::RemoveFile(Config::DefaultDataDir(Environment::AUTOMATED_TESTING))
# where RemoveFile is recursive (std::filesystem::remove_all) and
# Config::DefaultDataDir is FileUtil::GetHomeDirectory()/.GrinPP/<ENV>, with
# GetHomeDirectory() built from the HOMEDRIVE/HOMEPATH environment variables.
# The tests also use fs::temp_directory_path() with FIXED names that collide
# between concurrent runs. Therefore every run must get its own fresh HOME and
# TMP/TEMP inside a disposable workspace directory.
#
# This script:
#   - refuses to run unless the run directory is inside an allowed disposable base;
#   - refuses run directories that already exist (one-shot) or contain reparse points;
#   - copies the tested binary and verifies source/copy SHA-256 are identical;
#   - redirects HOMEDRIVE/HOMEPATH/USERPROFILE/HOME and TMP/TEMP into the run dir;
#   - snapshots the real user profile .GrinPP before/after and proves it unchanged;
#   - records exact SHA-256, PE timestamp, cwd, env, command line, stdout/stderr,
#     exit code (decimal + hex), duration, Windows Application Error events;
#   - writes a JSON evidence manifest and copies logs to evidence/line3/<run-id>.
#
# Usage:
#   pwsh tools/build/run-tests-isolated.ps1 -RunId baseline
#   pwsh tools/build/run-tests-isolated.ps1 -RunId race -HomeMode Shared -Instances 2
#
# This script never builds anything and does not touch production P2P/NRD code.

param(
    [string]$ExePath = 'C:\DHW\Gplus\src\grinplusplus\bin\Release\Release\Tests.exe',
    [string]$Repo = 'C:\DHW\Gplus\src\grinplusplus',
    [string]$RunRoot = 'C:\DHW\.tmp\line3-iso',
    [string]$EvidenceRoot = 'C:\DHW\Gplus\src\grinplusplus\evidence\line3',
    [string]$RunId = (Get-Date -Format 'yyyyMMdd-HHmmss'),
    [int]$TimeoutSec = 900,
    [string[]]$TestArgs = @(),
    [string]$ExeArgs = '',
    [ValidateSet('PerRun', 'Shared')][string]$HomeMode = 'PerRun',
    [int]$Instances = 1,
    [switch]$KeepBinary,
    [switch]$NoEvidenceCopy,
    [switch]$FailOnNoTests
)

$ErrorActionPreference = 'Stop'

function Get-Sha256([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

function Get-PeTimestamp([string]$Path) {
    $fs = [System.IO.File]::OpenRead($Path)
    try {
        $br = [System.IO.BinaryReader]::new($fs)
        $fs.Position = 0x3C
        $peOffset = $br.ReadInt32()
        $fs.Position = $peOffset + 8
        $ts = $br.ReadUInt32()
        $dt = [DateTimeOffset]::FromUnixTimeSeconds([int64]$ts).UtcDateTime.ToString('o')
        return [ordered]@{ raw_hex = ('0x{0:X8}' -f $ts); utc = $dt }
    } finally {
        $fs.Dispose()
    }
}

function Assert-NoReparsePoint([string]$Path) {
    # Walk every existing component from the drive root down to $Path.
    $full = [System.IO.Path]::GetFullPath($Path)
    $parts = $full -split '\\'
    $cur = $parts[0] + '\'
    for ($i = 1; $i -lt $parts.Count; $i++) {
        if ([string]::IsNullOrEmpty($parts[$i])) { continue }
        $cur = Join-Path $cur $parts[$i]
        if (Test-Path -LiteralPath $cur) {
            $item = Get-Item -LiteralPath $cur -Force
            if ($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
                throw "isolation check failed: reparse point on path chain: $cur"
            }
        }
    }
    return $true
}

function Assert-InsideDisposableBase([string]$Path, [string[]]$Bases) {
    $full = [System.IO.Path]::GetFullPath($Path).TrimEnd('\')
    foreach ($b in $Bases) {
        $bf = [System.IO.Path]::GetFullPath($b).TrimEnd('\')
        if ($full -eq $bf -or $full.StartsWith($bf + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
            return $full
        }
    }
    throw "isolation check failed: '$full' is not inside an allowed disposable base ($($Bases -join ', '))"
}

function Get-UserProfileSnapshot([string]$ProfilePath) {
    $grinpp = Join-Path $ProfilePath '.GrinPP'
    if (-not (Test-Path -LiteralPath $grinpp)) {
        return [ordered]@{ path = $grinpp; exists = $false; recursive_entries = 0; total_bytes = 0; latest_write_utc = $null; listing_sha256 = $null }
    }
    $entries = Get-ChildItem -LiteralPath $grinpp -Recurse -Force -ErrorAction SilentlyContinue
    $listing = ($entries | ForEach-Object {
            $rel = $_.FullName.Substring($grinpp.Length).TrimStart('\')
            "$rel|$($_.PSIsContainer)|$($_.Length)|$($_.LastWriteTimeUtc.ToString('o'))"
        } | Sort-Object) -join "`n"
    $latest = ($entries | Measure-Object -Property LastWriteTimeUtc -Maximum).Maximum
    $sb = [System.Text.StringBuilder]::new()
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = $sha.ComputeHash([System.Text.Encoding]::UTF8.GetBytes($listing))
        [void]$sb.Append(($bytes | ForEach-Object { $_.ToString('x2') }) -join '')
    } finally { $sha.Dispose() }
    return [ordered]@{
        path                = $grinpp
        exists              = $true
        recursive_entries   = @($entries).Count
        total_bytes         = [int64](($entries | Measure-Object -Property Length -Sum).Sum)
        latest_write_utc    = if ($latest) { ([datetime]$latest).ToString('o') } else { $null }
        listing_sha256      = $sb.ToString()
    }
}

function Get-TreeListing([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) { return @() }
    return @(Get-ChildItem -LiteralPath $Path -Recurse -Force -ErrorAction SilentlyContinue | ForEach-Object {
            $rel = $_.FullName.Substring($Path.Length).TrimStart('\')
            [ordered]@{ rel = $rel; dir = $_.PSIsContainer; bytes = $_.Length }
        } | Sort-Object { $_.rel })
}

function Get-NewEvents([datetime]$SinceUtc, [datetime]$UntilUtc, [string]$PathNeedle) {
    try {
        $ev = Get-WinEvent -FilterHashtable @{ LogName = 'Application'; StartTime = $SinceUtc; EndTime = $UntilUtc } -ErrorAction Stop
        return @($ev | Where-Object { $_.Message -match [regex]::Escape($PathNeedle) } | ForEach-Object {
                [ordered]@{
                    time_utc     = $_.TimeCreated.ToUniversalTime().ToString('o')
                    id           = $_.Id
                    provider     = $_.ProviderName
                    level        = $_.LevelDisplayName
                    message_head = (($_.Message -split "`r?`n") | Select-Object -First 6) -join ' | '
                }
            })
    } catch {
        return @()
    }
}

# ---------------- preconditions ----------------
$Repo = (Resolve-Path -LiteralPath $Repo).Path
$ExePath = (Resolve-Path -LiteralPath $ExePath).Path
$disposableBases = @('C:\DHW\.tmp')
$RunRootFull = Assert-InsideDisposableBase -Path $RunRoot -Bases $disposableBases
if ($Instances -lt 1 -or $Instances -gt 8) { throw "Instances must be 1..8" }

$runDir = Join-Path $RunRootFull $RunId
if (Test-Path -LiteralPath $runDir) { throw "run directory already exists (one-shot): $runDir" }

# The target test data dir child must not exist anywhere in the user profile
# unless we create it ourselves inside the run dir.
$realProfile = $env:USERPROFILE
if (-not $realProfile) { $realProfile = Join-Path $env:HOMEDRIVE $env:HOMEPATH }
$userSnapshotBefore = Get-UserProfileSnapshot -ProfilePath $realProfile

if ($HomeMode -eq 'PerRun') {
    $homeDir = Join-Path $runDir 'home'
} else {
    $homeDir = Join-Path $runDir 'shared-home'
}
$tmpDir = Join-Path $runDir 'tmp'
$binDir = Join-Path $runDir 'bin'
$outDir = Join-Path $runDir 'out'
$cwdDir = Join-Path $runDir 'cwd'
Assert-NoReparsePoint -Path $RunRootFull | Out-Null
Assert-NoReparsePoint -Path $runDir | Out-Null

New-Item -ItemType Directory -Force -Path $runDir, $homeDir, $tmpDir, $binDir, $outDir, $cwdDir | Out-Null

# ---------------- tested binary ----------------
$sourceSha = Get-Sha256 $ExePath
$sourcePe = Get-PeTimestamp $ExePath
$sourceSize = (Get-Item -LiteralPath $ExePath).Length
$copyPath = Join-Path $binDir 'Tests.exe'
Copy-Item -LiteralPath $ExePath -Destination $copyPath -Force
$copySha = Get-Sha256 $copyPath
if ($copySha -ne $sourceSha) { throw "binary copy SHA mismatch: source=$sourceSha copy=$copySha" }
$copyPe = Get-PeTimestamp $copyPath

# ASan-profile executables need the dynamic runtime next to them. Copy the
# runtime DLL from the source exe directory when present (hashed for evidence).
$copiedRuntime = @()
$sourceDir = Split-Path -Parent $ExePath
foreach ($pattern in 'clang_rt.asan_dynamic-x86_64.dll', 'clang_rt.asan_dbg_dynamic-x86_64.dll') {
    $runtime = Join-Path $sourceDir $pattern
    if (Test-Path -LiteralPath $runtime) {
        Copy-Item -LiteralPath $runtime -Destination $binDir -Force
        $copiedRuntime += [ordered]@{ name = $pattern; sha256 = (Get-Sha256 (Join-Path $binDir $pattern)) }
    }
}

# ---------------- env isolation ----------------
$homeAbs = [System.IO.Path]::GetFullPath($homeDir)
$tmpAbs = [System.IO.Path]::GetFullPath($tmpDir)
$cwdAbs = [System.IO.Path]::GetFullPath($cwdDir)
$drive = [System.IO.Path]::GetPathRoot($homeAbs).TrimEnd('\')
$homePathForEnv = $homeAbs
if ($homePathForEnv.StartsWith($drive + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
    $homeDrive = $drive
} else {
    throw "home dir '$homeAbs' is not on drive '$drive'"
}

$envBefore = [ordered]@{}
foreach ($k in 'HOMEDRIVE', 'HOMEPATH', 'USERPROFILE', 'HOME', 'TMP', 'TEMP', 'TMPDIR') {
    $envBefore[$k] = [Environment]::GetEnvironmentVariable($k, 'Process')
}
$env:HOMEDRIVE = $homeDrive
$env:HOMEPATH = $homePathForEnv
$env:USERPROFILE = $homeAbs
$env:HOME = $homeAbs
$env:TMP = $tmpAbs
$env:TEMP = $tmpAbs
$env:TMPDIR = $tmpAbs
# Same value GetHomeDirectory() composes: <drive>/<path> == HOMEPATH here.
$testDataDir = Join-Path $homeAbs '.GrinPP\AUTOMATED_TESTING'
$testLogDir = Join-Path $testDataDir 'LOGS'

# Capture git provenance (read-only).
function Invoke-Git([string[]]$GitArgs) {
    try {
        $out = & git -C $Repo @GitArgs 2>&1
        return (($out | Out-String).Trim())
    } catch { return "git-error: $($_.Exception.Message)" }
}
$gitRev = Invoke-Git @('rev-parse', 'HEAD')
$gitStatus = Invoke-Git @('status', '--porcelain')
$gitDiffStat = Invoke-Git @('diff', '--stat')

$provenanceFiles = @(
    'tests/src/TestMain.cpp',
    'tests/src/PMMR/CMakeLists.txt',
    'tests/src/PMMR/Test_ZipFile.cpp',
    'include/P2P/Capabilities.h',
    'src/PMMR/Zip/ZipFile.cpp',
    # Line 2 NRD handoff: hash these so a run covering NRD/index rebuild is
    # traceable to the exact (possibly uncommitted) sources.
    'include/PMMR/NrdRules.h',
    'include/PMMR/KernelIndex.h',
    'src/PMMR/KernelIndex.cpp',
    'tests/src/PMMR/Test_NrdRules.cpp',
    'tests/src/PMMR/Test_NrdDifferential.cpp',
    'tests/src/PMMR/Test_NrdIndexRebuild.cpp',
    'tests/src/PMMR/Test_NrdSupport.h'
)
$sourceHashes = [ordered]@{}
foreach ($rel in $provenanceFiles) {
    $p = Join-Path $Repo $rel
    if (Test-Path -LiteralPath $p) { $sourceHashes[$rel] = Get-Sha256 $p }
    else { $sourceHashes[$rel] = $null }
}

# ---------------- run ----------------
# Argument handling:
#   -TestArgs is the safe path: every element is quoted for the Windows
#   command line, so filters containing spaces (e.g. "KernelIndex schema v2*")
#   reach Catch2 as ONE argument. The old space-joined form silently turned such
#   filters into "KernelIndexschemav2*" ("No tests ran", exit 0 => false PASS).
#   -ExeArgs remains a raw command-line fragment for backward compatibility;
#   callers using it are responsible for quoting.
function ConvertTo-QuotedArg([string]$Value) {
    if ($null -eq $Value) { return '""' }
    if ($Value -eq '') { return '""' }
    if ($Value -notmatch '[\s"]') { return $Value }
    $escaped = $Value -replace '(\\*)"', '$1$1\"'
    $escaped = $escaped -replace '(\\+)$', '$1$1'
    return '"' + $escaped + '"'
}

$argList = if (@($TestArgs).Count -gt 0) {
    (@($TestArgs) | ForEach-Object { ConvertTo-QuotedArg $_ }) -join ' '
} elseif ($ExeArgs) { $ExeArgs } else { '' }
$instancesInfo = @()
$startUtc = (Get-Date).ToUniversalTime()
$globalStopwatch = [System.Diagnostics.Stopwatch]::StartNew()
$procs = @()
for ($i = 1; $i -le $Instances; $i++) {
    $stdout = Join-Path $outDir ("instance-$i.stdout.log")
    $stderr = Join-Path $outDir ("instance-$i.stderr.log")
    $p = Start-Process -FilePath $copyPath -ArgumentList $argList -WorkingDirectory $cwdAbs `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru -NoNewWindow
    $procs += [pscustomobject]@{ index = $i; proc = $p; stdout = $stdout; stderr = $stderr; startedUtc = (Get-Date).ToUniversalTime() }
}

$timeoutMs = $TimeoutSec * 1000
$anyTimeout = $false
foreach ($pi in $procs) {
    $remaining = [Math]::Max(1000, $timeoutMs - [int]$globalStopwatch.ElapsedMilliseconds)
    if (-not $pi.proc.WaitForExit($remaining)) {
        $anyTimeout = $true
        try { & taskkill /PID $pi.proc.Id /T /F 2>&1 | Out-Null } catch { }
    }
}
foreach ($pi in $procs) {
    try { $pi.proc.WaitForExit() } catch { }
}
$globalStopwatch.Stop()
$endUtc = (Get-Date).ToUniversalTime()

# ---------------- results ----------------
function Format-ExitCode([int]$Code) {
    return [ordered]@{ decimal = $Code; hex = ('0x{0:X8}' -f ($Code -band 0xFFFFFFFF)) }
}

foreach ($pi in $procs) {
    $exit = $null
    try { $exit = $pi.proc.ExitCode } catch { }
    $exitKnown = ($null -ne $exit)
    $stdoutText = if (Test-Path -LiteralPath $pi.stdout) { Get-Content -LiteralPath $pi.stdout -Raw -ErrorAction SilentlyContinue } else { $null }
    $stderrText = if (Test-Path -LiteralPath $pi.stderr) { Get-Content -LiteralPath $pi.stderr -Raw -ErrorAction SilentlyContinue } else { $null }
    $summary = @()
    if ($stdoutText) {
        $summary = @($stdoutText -split "`r?`n" | Where-Object { $_ -match 'Passed all \d+ test cases|All tests passed|Failed \d+ test cases|test cases:|assertions:|Test Runtime Initialized|Running Tests' } | Select-Object -Last 8)
    }
    $noTests = ($null -ne $stdoutText) -and ($stdoutText -match 'No tests ran|No test cases matched')
    $crash = ($exit -ne $null -and (($exit -band 0xFFFFFFFF) -eq 0xC0000409))
    $instancesInfo += [ordered]@{
        index             = $pi.index
        pid               = $pi.proc.Id
        started_utc       = $pi.startedUtc.ToString('o')
        duration_sec      = [Math]::Round(((Get-Date).ToUniversalTime() - $pi.startedUtc).TotalSeconds, 3)
        exit_code         = if ($exitKnown) { Format-ExitCode ([int]$exit) } else { [ordered]@{ decimal = $null; hex = $null } }
        status            = if (-not $exitKnown) { 'FAIL_NO_EXIT_CODE' } elseif ($crash) { 'FAIL_CRASH_0xC0000409' } elseif ($FailOnNoTests -and $noTests) { 'FAIL_NO_TESTS' } elseif ($exit -eq 0) { 'PASS' } elseif ($anyTimeout) { 'TIMEOUT_OR_FAIL' } else { 'FAIL' }
        no_tests_ran      = [bool]$noTests
        stdout_path       = $pi.stdout
        stdout_sha256     = if (Test-Path -LiteralPath $pi.stdout) { Get-Sha256 $pi.stdout } else { $null }
        stdout_bytes      = if (Test-Path -LiteralPath $pi.stdout) { (Get-Item -LiteralPath $pi.stdout).Length } else { 0 }
        stderr_path       = $pi.stderr
        stderr_sha256     = if (Test-Path -LiteralPath $pi.stderr) { Get-Sha256 $pi.stderr } else { $null }
        stderr_bytes      = if (Test-Path -LiteralPath $pi.stderr) { (Get-Item -LiteralPath $pi.stderr).Length } else { 0 }
        summary_lines     = $summary
    }
}

$copyShaAfter = Get-Sha256 $copyPath
$userSnapshotAfter = Get-UserProfileSnapshot -ProfilePath $realProfile
$userUnchanged = ($userSnapshotBefore.listing_sha256 -eq $userSnapshotAfter.listing_sha256) -and
                 ($userSnapshotBefore.exists -eq $userSnapshotAfter.exists)
$homeTree = Get-TreeListing -Path $homeDir
$dataDirCreated = Test-Path -LiteralPath $testDataDir
$logDirCreated = Test-Path -LiteralPath $testLogDir
$cwdTree = Get-TreeListing -Path $cwdDir
$cwdGrinpp = Test-Path -LiteralPath (Join-Path $cwdAbs '.GrinPP')
$events = Get-NewEvents -SinceUtc $startUtc.AddMinutes(-2) -UntilUtc $endUtc.AddMinutes(1) -PathNeedle $copyPath

$allExitZero = ($instancesInfo | Where-Object { $_.exit_code.decimal -ne 0 }).Count -eq 0
$allInstancesPass = ($instancesInfo | Where-Object { $_.status -ne 'PASS' }).Count -eq 0
$anyNoTests = ($instancesInfo | Where-Object { $_.no_tests_ran }).Count -gt 0
$isolationVerified = $userUnchanged -and $dataDirCreated -and (-not $cwdGrinpp) -and (-not $anyTimeout)
$status = if (-not $userUnchanged) { 'FAIL_ISOLATION_USER_PROFILE_TOUCHED' } elseif ($anyTimeout) { 'TIMEOUT' } elseif ($anyNoTests -and $FailOnNoTests) { 'FAIL_NO_TESTS' } elseif ($allExitZero -and $allInstancesPass -and $isolationVerified) { 'PASS' } else { 'FAIL' }

$manifest = [ordered]@{
    schema                = 'gpp-line3-test-run/v1'
    run_id                = $RunId
    status                = $status
    no_tests_ran          = $anyNoTests
    fail_on_no_tests      = [bool]$FailOnNoTests
    created_utc           = (Get-Date).ToUniversalTime().ToString('o')
    repo                  = $Repo
    git_head              = $gitRev
    git_status_porcelain  = if ($gitStatus) { $gitStatus -split "`r?`n" } else { @() }
    git_diff_stat         = $gitDiffStat
    source_hashes         = $sourceHashes
    host                  = [ordered]@{
        os              = [Environment]::OSVersion.VersionString
        pwsh            = $PSVersionTable.PSVersion.ToString()
        architecture    = $env:PROCESSOR_ARCHITECTURE
        user_profile    = $realProfile
        utc_offset_min  = [int][TimeZoneInfo]::Local.GetUtcOffset((Get-Date)).TotalMinutes
    }
    binary                = [ordered]@{
        source_path       = $ExePath
        source_sha256     = $sourceSha
        copy_path         = $copyPath
        copy_sha256       = $copySha
        copy_sha256_after = $copyShaAfter
        copy_unchanged    = ($copySha -eq $copyShaAfter)
        bytes             = $sourceSize
        pe_timestamp      = $sourcePe
        pe_timestamp_copy = $copyPe
        copied_asan_runtime = @($copiedRuntime)
    }
    isolation             = [ordered]@{
        mode                 = $HomeMode
        run_dir              = $runDir
        cwd                  = $cwdAbs
        home_dir             = $homeAbs
        tmp_dir              = $tmpAbs
        env                  = [ordered]@{
            HOMEDRIVE         = $env:HOMEDRIVE
            HOMEPATH          = $homePathForEnv
            USERPROFILE       = $env:USERPROFILE
            HOME              = $env:HOME
            TMP               = $env:TMP
            TEMP              = $env:TEMP
            TMPDIR            = $env:TMPDIR
        }
        env_before           = $envBefore
        expected_test_data_dir = $testDataDir
        test_data_dir_created  = $dataDirCreated
        test_log_dir_created   = $logDirCreated
        cwd_tree             = $cwdTree
        home_tree_sample     = @($homeTree | Select-Object -First 60)
        home_tree_entries    = $homeTree.Count
        user_profile_before  = $userSnapshotBefore
        user_profile_after   = $userSnapshotAfter
        user_profile_unchanged = $userUnchanged
        reparse_point_check  = 'passed'
        disposable_base      = $RunRootFull
    }
    command               = [ordered]@{
        exe           = $copyPath
        args          = $argList
        test_args     = @($TestArgs)
        raw_exe_args  = $ExeArgs
        fail_on_no_tests = [bool]$FailOnNoTests
        working_dir   = $cwdAbs
        instances     = $Instances
        timeout_sec   = $TimeoutSec
        started_utc   = $startUtc.ToString('o')
        finished_utc  = $endUtc.ToString('o')
        wall_sec      = [Math]::Round($globalStopwatch.Elapsed.TotalSeconds, 3)
    }
    instances             = $instancesInfo
    windows_application_events = $events
    notes                 = @(
        'Non-instrumented build: no sanitizer. Evidence covers test-run isolation and runtime behavior only.',
        'stdout redirected to a file is block-buffered; an empty log does not prove the process died before printing.',
        'Windows Application Error events are matched by the unique copy path inside this run directory.'
    )
}

$manifestPath = Join-Path $runDir 'manifest.json'
$manifest | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $manifestPath -Encoding utf8

if (-not $NoEvidenceCopy) {
    $evDir = Join-Path $EvidenceRoot $RunId
    New-Item -ItemType Directory -Force -Path $evDir | Out-Null
    Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $evDir 'manifest.json') -Force
    Copy-Item -Path (Join-Path $runDir 'out\*') -Destination $evDir -Force -ErrorAction SilentlyContinue
    if ($KeepBinary) { Copy-Item -LiteralPath $copyPath -Destination (Join-Path $evDir 'Tests.exe') -Force }
    $manifestPath = Join-Path $evDir 'manifest.json'
}

Write-Host ''
Write-Host "run id        : $RunId"
Write-Host "status        : $status"
Write-Host "exe sha256    : $sourceSha"
Write-Host "pe timestamp  : $($sourcePe.raw_hex) ($($sourcePe.utc))"
foreach ($inst in $instancesInfo) {
    Write-Host ("instance {0,2} : pid={1} exit={2} ({3}) bytes_stdout={4}" -f $inst.index, $inst.pid, $inst.exit_code.decimal, $inst.exit_code.hex, $inst.stdout_bytes)
}
Write-Host "isolated HOME : $homeAbs"
Write-Host "test data dir : $testDataDir (created=$dataDirCreated)"
Write-Host "user profile  : unchanged=$userUnchanged"
Write-Host "events        : $($events.Count)"
Write-Host "manifest      : $manifestPath"

if ($status -eq 'PASS') { exit 0 } else { exit 1 }
