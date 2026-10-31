# AUDIT_DELTA parity check (EN/DE/RU)
#
# Verifies that the three language versions of the audit delta contain the same
# card IDs (headings "### G-xx ...") and reports how many TODO placeholders are
# still open in each file. Run before handover; non-zero exit means the package
# is not ready.
param(
    [string]$Dir = (Join-Path $PSScriptRoot '..\..\docs\modernization')
)

$files = [ordered]@{
    en = Join-Path $Dir 'AUDIT_DELTA.en.md'
    de = Join-Path $Dir 'AUDIT_DELTA.de.md'
    ru = Join-Path $Dir 'AUDIT_DELTA.ru.md'
}

$cards = @{}
foreach ($lang in $files.Keys) {
    $path = $files[$lang]
    if (-not (Test-Path -LiteralPath $path)) {
        Write-Output "MISSING FILE: $path"
        exit 2
    }
    $cards[$lang] = @(
        Select-String -Path $path -Pattern '^### (G-[0-9]{2}[a-z]?(?: / G-[0-9]{2}[a-z]?)*)' |
            ForEach-Object { $_.Matches[0].Groups[1].Value }
    )
}

$all = @($cards.Values | ForEach-Object { $_ } | Select-Object -Unique)
$ok = $true

foreach ($id in $all) {
    foreach ($lang in $files.Keys) {
        if ($cards[$lang] -notcontains $id) {
            Write-Output "MISSING in ${lang}: $id"
            $ok = $false
        }
    }
}

foreach ($lang in $files.Keys) {
    $todo = (Select-String -Path $files[$lang] -Pattern 'TODO' -AllMatches | Measure-Object).Count
    $count = $cards[$lang].Count
    Write-Output ("{0}: {1} cards, {2} lines with TODO" -f $lang, $count, $todo)
}

if (-not $ok) {
    Write-Output 'PARITY FAILED'
    exit 1
}

Write-Output ("PARITY OK: {0} card IDs" -f $all.Count)
