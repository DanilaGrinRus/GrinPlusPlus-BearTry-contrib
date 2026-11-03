# Normal-binary AUDIT build-11 (02DC3D9C / 565BC2DB) — bound with explicit exception

Status: **FAIL overall, only known test-only bodyless**; zero
production/agreed-suite failures. Bound as the build-11 normal candidate **with
explicit exception**, not fully green. No build/commit by line3.

## 1. Identity

| Artifact | SHA-256 | Size | PE timestamp |
|---|---|---|---|
| Tests-retry.exe | `02DC3D9C00BB1E1B7256771A158F04F1DB3B739839A7AFF2391AEAA5DAAFC121` | 9,438,720 B | `0x6AE94C3F` |
| GrinNode-retry.exe | `565BC2DBF30C638F84B458012F1F07D091B8EF6DB9C933E821A58EF3CF6021A4` | 7,843,328 B | `0x6AE94B43` |

Preserved: `C:\DHW\.tmp\build-11\Tests-retry.exe` / `GrinNode-retry.exe`.
Retry build log `build-console-retry.log` SHA
`B274B34A8D197B356D0EB9C9FCDA3B53C5E60122ADD39AF7A5A45448CA54C0DE`, ends
`build OK`. The failed first attempt (`build-console.log`, SHA
`ACED40F9391B86698A27F39DB8D73AC5B183222B03CDEDAC262486F8661116A4`, C1083)
is **excluded and not audited**.

Build-11 freeze snapshot:
`evidence/line3/build-11-freeze-snapshot-final.json`, SHA
`E5F7519C0690D8B30327A07437CE66D2B1B93C5ECC77EE38607AE1349A259351`;
source HEAD `6dbf8a7…`; 9-file uncommitted set (line1 archive/include-fix +
line2 diagnostics + inert FakeBlockChain stub); no source changes during the
successful retry. ASan copied tree matches 1003/1003 entries.

## 2. Normal audit

Command pattern: isolated runner, per-run HOME/TMP, `-KeepBinary`,
`-FailOnNoTests`, quoted `-TestArgs`, serialized (no concurrent tests; port
3421 free at start).

### Full suite

- **255 cases — 254 passed / 1 failed**
- **4201 assertions — 4200 passed / 1 failed**
- sole failure: `tests/src/BlockChain/Test_NrdBodylessRestart.cpp(103)`
  (`AddBlock(block30) == SUCCESS`) — known test-only bodyless exception.
- zero `mg_start failed`; user profile unchanged.

### Pre-registered groups — all PASS

| Filter | Cases | Assertions | Run id |
|---|---:|---:|---|
| `PibdArchiveServer::*` | 8 | 187 | `b11-archive-01` |
| `Nrd*` | 30 | 428 | `b11-nrd-01` |
| `KernelIndex*` | 5 | 74 | `b11-kernidx-01` |
| `GetKernelHandler*` | 2 | 49 | `b11-getkernel-01` |
| `KernelIndex schema v2*` | 2 | 26 | `b11-schema-01` |
| `TxHashSet duplicate-excess*` | 1 | 10 | `b11-dup-01` |
| `P2P::ArchivePublication*` | 1 | 83 | `b11-p2p-archive-01` |
| `P2P::AdvertisedTip*` | 2 | 15 | `b11-advert-01` |

Every run retained source SHA `02DC3D9C…`, had no test-less result, and left
the real user profile unchanged. Manifests:
`evidence/line3/b11-<group>-01/manifest.json`.

The new `P2P::ArchivePublicationValidatedAtomicPublish` test (1/83) passes,
covering staged publish, readiness rejection, overwrite and recovery-target
helpers at the test level.

## 3. Classification / caveats

- Normal overall: **FAIL (bodyless-only)**; production/agreed-suite failures 0.
- Binding: Tests `02DC3D9C…` + GrinNode `565BC2DB…`, **with explicit exception**.
- Bodyless-NRD/kill recovery remain NOT VERIFIED.
- ArchivePublication tests are unit-level and do not by themselves prove live
  recovery, crash interruption, or Mainnet readiness.
- Build-11 is not a full G-09 PASS; existing 7B PASS remains valid only for its
  fixed-H scenario.
- ASan refresh runs separately from this normal binding; no ASan claim here.

## 3.1 ASan refresh (separate, subset PASS)

- Source manifest `69B01EE6…` (1293 files), full ASan build
  `full-asan-20261103-095356` **BUILD_PASS** (101.1 s); copied tree matches
  build-11 freeze 1003/1003 (0 mismatch).
- TestsMini `C76CCCF2F8FB5F2B3154ABC5676AE0D877B20CFA3C3845E167CABC0C7D82C344`.
- PASS: Nrd* 21/365, archive 8/187, Segment 16/68, P2P+advert 7/61,
  `P2P::ArchivePublication*` 1/83; teardown PASS; **0 ASan reports**.
- ASan-BLOCKED normal-only: PreCommit/FalseBan/BatchSafety/KernelIndex/
  BlockPipe/bodyless/kill; rocksdb/sysroot uninstrumented.
- `tests_mini_prefix` not built (missing legacy source); teardown uses TestsMini.
- **Post-audit root cause (2026-11-03):** build-11 contains the production
  Release `SpentOutputs::Deserialize` NDEBUG assert-side-effect bug (old
  `SpentOutput.h F3F38B44…`). This normal/ASan binding does **not** validate the
  fix. Build-12 audit is required for `SpentOutput.h D27A607D…` + regression
  `Test_SpentOutputs.cpp 6157AC56…` and live H=3313440/H=3315600 recovery. See
  `task10-11-spent-deserialize-rootcause-20261103/` and
  `build12-preaudit-20261103.md`.
- Evidence: `asan-build11-refresh-20261103.md`,
  `build11-asan-refresh-20261103.json`, `evidence/line3/b11-asan-*-01/`.

## 4. Evidence

- `evidence/line3/b11-full-01/` — full manifest/stdout (255/254/1)
- `evidence/line3/b11-archive-01/`, `b11-nrd-01/`, `b11-kernidx-01/`,
  `b11-getkernel-01/`, `b11-schema-01/`, `b11-dup-01/`,
  `b11-p2p-archive-01/`, `b11-advert-01/`
- `evidence/line3/build-11-freeze-snapshot-final.json`
- this document
