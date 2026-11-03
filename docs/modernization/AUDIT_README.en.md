# Grin++ 1.2.9 — compatibility audit package (EN, canonical)

## Current pointer — 2026-11-03

Package status does not establish Mainnet readiness. For the updated snapshot and limitations see `AUDIT_DELTA.en.md`; operational tracking and evidence: `THREE_LINE_STATUS.md`. G-09: scenario 7B PASS, full G-09 and public PIBD NOT VERIFIED; old-archive recovery FAIL, root cause NOT VERIFIED. Build-10 full suite 254/253/1; bodyless NRD and kill-recovery NOT VERIFIED. Cross-check the operational board against primary evidence.

## Historical package as of 2026-11-01

The counts, acceptance statuses, results and hashes below belong to an earlier package and do not describe the current build.

Status: **closed by owner acceptance (01.11.2026)**; independent reviewer not assigned.

## What is in this package

- `GAP_MATRIX.md` — 53 entries against grin v5.5.2 (`dbaff34`): 45 closed,
  3 partial (G-09, G-23, G-24), 0 in progress, 0 open, 3 planned
  (G-44, G-45, G-49), 1 deferred (G-07), 1 N/A (G-27).
- `AUDIT_DELTA.en.md` / `.de.md` / `.ru.md` — 29 cards, 8 mandatory fields,
  18 diff blocks each, `check_audit_delta.ps1` PARITY OK, 0 TODO.
- `TEST_RESULTS.md` — E1–E59 evidence, including the live G-52 → G-41 run and the G-51 closure check.
- `REMAINING.md`, `ACCEPTANCE_REPORT.md`, `DB_CONSISTENCY.md`,
  `AUDIT_COMMENTS.md`, `AUDIT_CHECKLIST.md`.
- Release build artifacts (SHA-256; not shipped in this package, verified against `bin/Release/Release`):

```
GrinNode.exe  E16A014EC3AD1766637E06544790AAE815825A9DCFBF18EB1516F512799B6328  7,624,704
Tests.exe     CC6472B1E4A5081DC7770C275FEF927FA876AB257EBD0AD663A56EBCF273457E  8,651,264
```

- Tests: **194 cases / 2227 assertions PASS**.

## Live compatibility evidence (E54)

- PIBD at horizon 3,314,160, validated and activated; archive
  `TxHashSet.3314160.1145e8daa674.zip` = **3,117,206,023 bytes**.
- Legacy client requested it, downloaded the full 3.1 GB, extracted,
  `Successfully validated TxHashSet` (06:19), then block-synced to
  **FULLY_SYNCED at height 3,317,664** (06:53).
- Full sync from genesis headers reached FULLY_SYNCED in ~2h54m on the same
  run (block DB was pre-populated; see E53).

## Known limitations

- G-09 live PIBD serving to a Rust client and full resume completion are not
  verified (unit/live cycle only).
- G-23 hash-level MMR revalidation (`--validate-on-load`) is not implemented.
- G-24 CI wiring and vcpkg pin are open; the offline MSVC build is verified.
- G-51 closed by owner acceptance (01.11.2026); independent reviewer not assigned. License/notices (`D5`) and `dist/audit-*` (`D9`) are complete.
- Server block sync after archive activation stalled at the horizon while the
  client caught up from external peers; a separate diagnosis is pending.
- Arbitrary historical snapshots older than the horizon remain unsupported by
  design (spent-position history gap).
- Fuzz/sanitizers, differential fixtures, reorg and wallet/API/GUI regression
  suites are phases 5–7, outside this matrix.

## Reproduction

```powershell
pwsh tools/build/build.ps1 -Target Tests
$env:HOMEDRIVE='C:'; $env:HOMEPATH='\DHW\.tmp\testhome'
bin\Release\Release\Tests.exe --reporter compact
pwsh tools/audit/check_audit_delta.ps1
```

The node runs testnet/floonet only; no mainnet funds or real keys are used.
