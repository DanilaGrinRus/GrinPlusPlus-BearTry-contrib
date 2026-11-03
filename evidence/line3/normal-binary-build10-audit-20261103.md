# Normal-binary AUDIT build-10 (C17C3BC0 / 60D7F7A6) — bound with explicit exception

Status: **FAIL overall, only known test-only bodyless**; zero
production/agreed-suite failures. Bound as final normal candidate **with explicit
exception**, not fully green.

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
(SHA `E84E31657CED042E7963AAF3FFF401FD95C6D4DC36D203DD5A8B53A3BD4488BE`,
1002 compiled-root files, aggregate
`F1E47615D024E566CFF6EB2E908326364529813CF85111F7280B41EC6ADD869C`,
repo HEAD `ee843f059b573159c79b0be14fff84440741d989`). Only docs changed after
the build; no compiled-root source file changed after build start.

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

## 3. ASan status — NOT RUN (STALE)

Existing ASan binding remains build-09:

- source manifest `CF78C188A3F69F242F326B5DEFDA3DFD1D4A1E3A11591DAB8A9CDBF98DB68D85`
- TestsMini `12C3D16C21E80F86BC3C736BE5046872454D1A6D7FA26B018D782E89D153778B`
- build-09 git_head `cb510302f5bebc0d64c5d6a1c0571046a63593ef`

The copied ASan tree is **stale** against build-10 sources: `ci-pipeline`
comparison reports **mismatch=13** (changed/missing task-9 source, test and
CMake files), including
`GetKernelHandler.h`, `BlockDb.h`, `BlockDBImpl.*`, `TestServer.h`, task-9 test
files/CMakeLists. Therefore:

- build-10 ASan refresh: **NOT RUN (STALE, mismatch=13)**.
- No rebuild was performed under the instruction **No build/commit**; a rebuild
  requires explicit ASan build authorization.
- Do not bind build-09 ASan `CF78C188…`/`12C3D16C…` to build-10. No ASan claim
  is made for build-10.

## 4. Classification and caveats

- Normal **overall**: FAIL (bodyless-only); production/agreed-suite failures = 0.
- Binding: build-10 Tests `C17C3BC0…` + GrinNode `60D7F7A6…`, **with explicit
  exception**, analogously to build-09.
- Bodyless-NRD/kill-recovery remain NOT VERIFIED; bodyless is a known test-only
  exception, not a production regression.
- `mg_start` failures are environment/port-collision only; clean rerun had zero.
- ASan/fuzz not refreshed; no ASan PASS claim for build-10.
- Build-10 remains the final normal candidate; no build/commit performed by
  line3.

## 5. Evidence paths

- `evidence/line3/b10-full-02/` — clean full manifest + stdout
- `evidence/line3/b10-archive-02/`, `b10-nrd-02/`, `b10-kernidx-02/`,
  `b10-getkernel-02/`, `b10-schema-02/`, `b10-dup-02/`, `b10-advert-02/`
- `evidence/line3/b10-full-01/` — INVALID port-collision run (not counted)
- `evidence/line3/build-10-freeze-snapshot-final.json` — source freeze/binary
  binding
- `evidence/line3/normal-binary-build10-audit-20261103.md` — this document
