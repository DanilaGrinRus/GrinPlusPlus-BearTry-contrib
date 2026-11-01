# Grin++ 1.2.9 — compatibility audit package (EN, canonical)

Status: **ready for external review**; independent sign-off is still open.

## What is in this package

- `GAP_MATRIX.md` — 53 entries against grin v5.5.2 (`dbaff34`): 44 closed,
  3 partial (G-09, G-23, G-24), 1 in progress (G-51), 0 open, 3 planned
  (G-44, G-45, G-49), 1 deferred (G-07), 1 N/A (G-27).
- `AUDIT_DELTA.en.md` / `.de.md` / `.ru.md` — 29 cards, 8 mandatory fields,
  18 diff blocks each, `check_audit_delta.ps1` PARITY OK, 0 TODO.
- `TEST_RESULTS.md` — E1–E54 evidence, including the live G-52 → G-41 run.
- `REMAINING.md`, `ACCEPTANCE_REPORT.md`, `DB_CONSISTENCY.md`,
  `AUDIT_COMMENTS.md`, `AUDIT_CHECKLIST.md`.
- Artifacts (SHA-256):

```
GrinNode.exe  79812A2845662DA72EBD0E26A334F0274816A09FC1FA50998A180EEA3E41AEEF  7,624,704
Tests.exe     38B9AF99205EA1FC3247305FAE133F0FA8C8F3A6B01E4F564B0818B361DE555F  8,643,072
```

- Tests: **176 cases / 1907 assertions PASS**.

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
- G-51 needs external sign-off, license/notice packaging and `dist/audit-*`.
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
