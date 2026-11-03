# Normal-binary AUDIT build-12 (D1103F34 / 833CCBFD) — bound with explicit exception

Status: **FAIL overall, only known test-only bodyless**; zero
production/agreed-suite failures. Build-12 includes the Release SpentOutputs
fix; bound with explicit exception, not fully green. No build/commit by line3.

## 1. Identity

| Artifact | SHA-256 | Size | PE timestamp |
|---|---|---|---|
| Tests.exe | `D1103F3470C1E7E2507093418392CC69CF24E52137AEB5FC81CB7CE8A06A38CD` | 9,449,472 B | `0x6AE95151` |
| GrinNode.exe | `833CCBFD8C344D79AAC748107F60492B055D995BE4A2C70D6F19115C8A01751C` | 7,843,328 B | `0x6AE95056` |

Preserved: `C:\DHW\.tmp\build-12\Tests.exe` / `GrinNode.exe`.
Build log `build-console.log` SHA
`76BC0E49F66D81CF231DE4F41D220DF27D10930B0B6E48ACDFC20D3144434BC4`, ends
`build OK`.

Source freeze: `evidence/line3/build-12-freeze-snapshot-final.json` SHA
`64BE17E5EA0617D426ABFED13CEFEFE03537D7544EF5FFC581703227566DD00D`;
HEAD `6dbf8a7…`, 11-file set. Release fix `SpentOutput.h D27A607D…`;
regression `Test_SpentOutputs.cpp 6157AC56…`; old build-11 hash `F3F38B44…`.
No source changes during the successful build.

## 2. Normal audit

Serialized isolated runner, per-run HOME/TMP, `-KeepBinary`, quoted filters.
Port 3421 free at start; no concurrent Tests.

### Full suite

- **256 cases — 255 passed / 1 failed**
- **4211 assertions — 4210 passed / 1 failed**
- sole failure: `tests/src/BlockChain/Test_NrdBodylessRestart.cpp(103)`
  (`AddBlock(block30) == SUCCESS`, `Invalid version for header`) — known
  test-only bodyless exception.
- zero `mg_start`; user profile unchanged.

### Targeted / pre-registered groups

| Filter | Result | Cases / assertions | Run id |
|---|---|---|---|
| `SpentOutputs*` | **PASS** | 1 / 6 | `b12-spent-01` |
| `BlockChain preflight requires every input in spent positions` | **PASS** | exact targeted run | `b12-preflight-02` |
| `P2P::ArchivePublication*` | **PASS** | 1 / 83 | `b12-p2p-archive-01` |
| `PibdArchiveServer::*` | **PASS** | 8 / 187 | `b12-archive-01` |
| `GetKernelHandler*` | **PASS** | 2 / 49 | `b12-getkernel-01` |
| `KernelIndex schema v2*` | **PASS** | 2 / 26 | `b12-schema-01` |
| `TxHashSet duplicate-excess*` | **PASS** | 1 / 10 | `b12-dup-01` |
| `Nrd*` | **PASS** | 30 / 428 | `b12-nrd-01` |
| `KernelIndex*` | **PASS** | 5 / 74 | `b12-kernidx-01` |
| `P2P::AdvertisedTip*` | **PASS** | 2 / 15 | `b12-advert-01` |

`b12-preflight-01` (`BlockChain*preflight*`) returned FAIL_NO_TESTS due
wildcard syntax and is **excluded**; `b12-preflight-02` with the exact test
name PASSes. This was not a product failure.

The Release-mode regression
`SpentOutputs Release-mode deserialize consumes version byte` PASSes in the
normal Release binary (1 case / 6 assertions), including version rejection and
map size 1. The old build-11 artifact cannot run this test and is not a
pre-fix PASS.

## 3. Classification / caveats

- Normal overall: **FAIL (bodyless-only)**; production/agreed-suite failures 0.
- Binding: Tests `D1103F34…` + GrinNode `833CCBFD…`, **with explicit exception**.
- Build-11 normal/ASan PASS did not cover the root-cause fix; this build does
  compile/run the Release regression successfully.
- Live H=3313440/H=3315600 recovery, atomic interruption and Mainnet readiness
  remain separate acceptance work; no live recovery claim here.
- ASan refresh runs separately; no ASan claim in this normal binding.

## 3.1 ASan refresh (separate, subset PASS)

- Source manifest `3DB12364…` (1293 files), full ASan
  `full-asan-20261103-101529` **BUILD_PASS** (96.4 s); copied tree matches
  build-12 freeze **1003/1003, 0 mismatch**.
- TestsMini `CB09CEE89699DF0B35ABB3FD81C25470953A4C1B885C20DABC0AD930A92E7EBD`.
- PASS: Nrd* 21/365, archive 8/187, Segment 16/68, P2P+advert 7/61,
  ArchivePublication 1/83; teardown PASS; **0 ASan reports**.
- `Test_SpentOutputs.cpp` is not in TestsMini, so the Release regression was
  not rerun under ASan; it PASSed in normal Release (`SpentOutputs*` 1/6).
  ASan-BLOCKED normal-only: PreCommit/FalseBan/BatchSafety/KernelIndex/
  BlockPipe/bodyless/kill; rocksdb/sysroot uninstrumented.
- Evidence: `asan-build12-refresh-20261103.md`,
  `build12-asan-refresh-20261103.json`, `evidence/line3/b12-asan-*-01/`.

## 4. Evidence

- `evidence/line3/b12-full-01/` — full manifest/stdout (256/255/1)
- `evidence/line3/b12-spent-01/`, `b12-preflight-02/`,
  `b12-p2p-archive-01/`, `b12-archive-01/`, `b12-getkernel-01/`,
  `b12-schema-01/`, `b12-dup-01/`, `b12-nrd-01/`, `b12-kernidx-01/`,
  `b12-advert-01/`
- `evidence/line3/build-12-freeze-snapshot-final.json`
- this document
