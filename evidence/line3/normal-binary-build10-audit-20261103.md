# Normal-binary AUDIT build-10 (C17C3BC0 / 60D7F7A6) — bound with explicit exception

Status: **FAIL overall, only known test-only bodyless**; zero
production/agreed-suite failures. Bound as final normal candidate **with explicit
exception**, not fully green.

Scope note (2026-11-03 09:43): this binding is **only** for the build-10
snapshot (`C17C3BC0…` / `60D7F7A6…` / source freeze `0E51D632…`). Current HEAD
`6dbf8a7` contains unbuilt archive/recovery and task-11 diagnostic source; it
is **not** covered by this audit and must not be cited as final. Build-11 is
awaiting Lead order and a consistent frozen source set; no build-11 binary
exists.

Date: 2026-11-03. No source edits, no build, no commit. Tests were run with the
isolated runner, quoted `-TestArgs`, `-FailOnNoTests` on filtered runs,
`-KeepBinary`, and per-run HOME/TMP.

## 1. Identity

| Artifact | SHA-256 | Size | PE timestamp |
|---|---|---|---|
| Tests.exe | `C17C3BC04BE04C501DB425E3617A71E61985E9257EF41D879E130E28F8B621A4` | 9,410,560 B | `0x6AE93F14` |
| GrinNode.exe | `60D7F7A6B3FFCC702B260E80036B7143A06DB9B036AD3DF3754B3C7B9438CB23` | 7,809,024 B | `0x6AE93E33` |

Preserved paths: `C:\DHW\.tmp\build-10\Tests.exe` / `GrinNode.exe`; line1
preserved copies in `C:\DHW\.tmp\line1-build-10\`. Build console
(`C:\DHW\.tmp\build-10\build-console.log`) ends with `build OK`.

Build-10 source freeze snapshot:
`evidence/line3/build-10-freeze-snapshot-final.json`
(SHA `0E51D63287AF8F96B3094A9F24B120C2F663736CAA1E9B4DF5E8C2B6EB6CCA5E`,
1002 compiled-root files, aggregate
`F1E47615D024E566CFF6EB2E908326364529813CF85111F7280B41EC6ADD869C`,
repo HEAD `77b397032bb48317c6b944582ae9722dec5e85da`; snapshot was first
captured at `ee843f05`, then HEAD advanced to `77b3970` with identical
compiled-root contents). Only docs changed after the build; no compiled-root
source file changed after build start.

## 2. Normal audit results

### Full suite

Command: `run-tests-isolated.ps1 -ExePath build-10\Tests.exe -RunId b10-full-02
-RunRoot C:\DHW\.tmp\line3-iso -KeepBinary -FailOnNoTests` (no filter).

- **254 test cases — 253 passed / 1 failed**
- **4118 assertions — 4117 passed / 1 failed**
- Sole failure: `tests/src/BlockChain/Test_NrdBodylessRestart.cpp(103)`
  (`AddBlock(block30)`; short-chain header-version contradiction) — known
  test-only exception.
- Zero `mg_start failed`; user profile unchanged; isolated HOME/TMP.
- Evidence: `evidence/line3/b10-full-02/`.

The first full run `b10-full-01` is **INVALID / not counted**: a parallel line2
Tests process held fixed `127.0.0.1:3421`, producing 15 wallet/Tor
`mg_start failed` failures plus bodyless (254/238/16). It is preserved but must
not be used for build-10 classification. After line2 paused and the port was
free, `b10-full-02` was rerun cleanly.

### Pre-registered filters (all PASS)

| Filter | Test cases | Assertions | Run id |
|---|---:|---:|---|
| `PibdArchiveServer::*` | 8 | 187 | `b10-archive-02` |
| `Nrd*` | 30 | 428 | `b10-nrd-02` |
| `KernelIndex*` | 5 | 74 | `b10-kernidx-02` |
| `GetKernelHandler*` | 2 | 49 | `b10-getkernel-02` |
| `KernelIndex schema v2*` | 2 | 26 | `b10-schema-02` |
| `TxHashSet duplicate-excess*` | 1 | 10 | `b10-dup-02` |
| `P2P::AdvertisedTip*` | 2 | 15 | `b10-advert-02` |

Each run used `-TestArgs @('<filter>') -FailOnNoTests -KeepBinary`; all retained
the tested source SHA `C17C3BC0…`, had zero test-less runs, and left the real
user profile unchanged. Manifests are in `evidence/line3/<run-id>/manifest.json`.

## 3. ASan refresh — PASS (subset), copied tree

Authorized refresh from frozen build-10 sources in the copied tree
(`C:\DHW\.tmp\asan-line3\gpp-tree`; no shared build):

- source manifest `776B0DB3356FE953930E91B464F9BD6C5C8D8B30CACA9D53EED22AB785A37E7F`
  (1292 files), build BUILD_PASS (98.649 s), build manifest
  `full-asan-20261103-090921`; ASan copied tree matches all 1002 build-10
  freeze entries (**0 mismatch / 0 missing**).
- TestsMini `497353ECC81E3F88E414773F29FE0618C59707DB45FD9643418A89CA4758007B`.
- PASS: Nrd* 21/365, archive 8/187, Segment 16/68, P2P+advert 7/61; teardown
  regression PASS (zero tests, exit 0, no ASan, runtime DLL copied);
  **0 ASan reports**.
- ASan-BLOCKED normal-only: `Test_NrdPreCommit`, `Test_NrdFalseBan`,
  `Test_NrdBatchSafety`, `Test_KernelIndex`, BlockPipe, bodyless, kill;
  Database/ChainState/rocksdb and sysroot C libs uninstrumented.
- Evidence: `asan-build10-refresh-20261103.md`,
  `build10-asan-refresh-20261103.json`,
  `evidence/line3/full-asan-20261103-090921/`, `b10-asan-*`.
- Note: optional `tests_mini_prefix` target was not built because
  `TestMain_legacy_static.cpp` is absent in the build-10 tree; teardown uses
  TestsMini, so the required teardown check is PASS.

## 4. Classification and caveats

- Normal **overall**: FAIL (bodyless-only); production/agreed-suite failures = 0.
- Binding: build-10 Tests `C17C3BC0…` + GrinNode `60D7F7A6…`, **with explicit
  exception**, analogously to build-09.
- Bodyless-NRD/kill-recovery remain NOT VERIFIED; bodyless is a known test-only
  exception, not a production regression.
- `mg_start` failures are environment/port-collision only; clean rerun had zero.
- ASan refresh PASS covers only the subset above; no full Tests.exe ASan, no
  fuzz/coverage, no hosted-CI claim.
- Build-10 remains the final normal candidate; no build/commit performed by
  line3.

## 5. Evidence paths

- `evidence/line3/b10-full-02/` — clean full manifest + stdout
- `evidence/line3/b10-archive-02/`, `b10-nrd-02/`, `b10-kernidx-02/`,
  `b10-getkernel-02/`, `b10-schema-02/`, `b10-dup-02/`, `b10-advert-02/`
- `evidence/line3/b10-full-01/` — INVALID port-collision run (not counted)
- `evidence/line3/build-10-freeze-snapshot-final.json` — source freeze/binary
  binding
- `evidence/line3/asan-build10-refresh-20261103.md` — ASan refresh report
- `evidence/line3/build10-asan-refresh-20261103.json` + `... /build10-asan-refresh-20261103/`
  — ASan manifest/copy
- `evidence/line3/full-asan-20261103-090921/` — full ASan build manifest/logs
- `evidence/line3/b10-asan-nrd-01/`, `b10-asan-archive-01/`,
  `b10-asan-segment-01/`, `b10-asan-p2p-01/`, `b10-asan-teardown-01/`
- `evidence/line3/normal-binary-build10-audit-20261103.md` — this document
