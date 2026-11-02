# Normal-binary AUDIT build-09 (0A3B7D66) — BOUND WITH EXPLICIT EXCEPTION

Lead decision (b): build-09 is the **final normal authoritative target**, bound
**with an explicit exception**. It is **NOT fully green**: the only full-suite
failure is a line2 test-only bodyless test; zero production/agreed-suite
failures. Bodyless-NRD and kill-recovery remain **NOT VERIFIED**. Build-10 is
backlog only if separately approved; no build-10 now.

## 1. Identity (verified; archive/repo/preserved)

| Артефакт | SHA-256 | Size | PE ts |
|---|---|---|---|
| Tests.exe | `0A3B7D66E78F5E70A6D95616F0138D994AEB820796C0C77E98AEC7EAC4F2735B` | 9 335 808 B | `0x6AE91AB1` |
| GrinNode.exe | `F90EEF389D1B35B7A17BD8E3C35F06AECFD9C39410C4F755713E1CD1F448A2F8` | 7 805 440 B | `0x6AE919F2` |

repo == `C:\DHW\.tmp\build-09\Tests.exe` / `GrinNode.exe` (SHA/size совпали);
`build-console.log` → BUILD OK. Freeze: 1289 compiled-root files, aggregate
`832C378CC5F58555B5638F752A74C42DF09A4B32AA5A3B26C009688F7DAC1AFD`;
**0** source changes after build start (06:12:25); production
`src/P2P/MessageProcessor.cpp` = `964FE6D7…` unchanged. Snapshot:
`evidence/line3/build-09-freeze-snapshot-final.json`.

## 2. Normal isolated audit (preserved Tests, isolated HOME/TMP, port 3421 free)

| filter | результат |
|---|---|
| full suite ×2 | **249 cases — 248 passed / 1 failed; assertions 4033 / 4032 passed** (оба прогона идентичны) |
| `Bodyless*` | **FAIL — 1 case / 4 assertions, 3 passed / 1 failed** |
| `Nrd*` | **30 / 428 PASS** |
| `KernelIndex*` | **3 / 48 PASS** |
| `PibdArchiveServer::*` | **8 / 187 PASS** |
| `P2P::AdvertisedTip*` | **2 cases / 15 assertions PASS** |
| `BlockPipe*` | **3 cases / 16 assertions PASS** |

**Единственный провал:** `tests/src/BlockChain/Test_NrdBodylessRestart.cpp(103)`
`REQUIRE(AddBlock(block30) == SUCCESS)` — short-chain header-version
contradiction (BlockHeaderValidator требует v1, NRD gate — v4 на короткой цепи).
**Test-only дефект дизайна теста**, production не затронут. Line2 независимо
подтвердил тот же результат и просит test-only переработку (build-10 backlog).

Kill harness (`Test_NrdKillHarness`, hidden `[.]`) в full-suite не входит; его
recovery-run отдельно **FAIL** (test-only fs restore loop) → **NOT VERIFIED**.

## 3. Line2 build-09 evidence — проверено

- preserved `tools/audit/nrd/evidence/bin/Tests-0A3B7D66E78F.exe` = SHA
  `0A3B7D66…` (+`.sha256.txt`), evidence dirs keep own copies;
- `blockpipe-b09-…`: **3/16 PASS**; `nrd-b09-…`: **30/428 PASS**;
  `kernidx-b09-…`: **3/48 PASS**; `bodyless-b09-…`: **FAIL** (тот же
  test-only дефект); `kill-b09-…`: **FAIL** (test-only recovery).

## 4. ASan refresh (ONE, на frozen build-09 production sources)

- source manifest
  `CF78C188A3F69F242F326B5DEFDA3DFD1D4A1E3A11591DAB8A9CDBF98DB68D85`
  (1290 files); TestsMini
  `12C3D16C21E80F86BC3C736BE5046872454D1A6D7FA26B018D782E89D153778B`;
  build manifest `47869A7E…`; full ASan build PASS (81.7 s).
- subset PASS: `Nrd*` **21/365**, archive **8/187**, Segment **16/68**,
  P2P **7/61** (оба advert-теста), zero-test teardown **PASS**, **0 ASan
  reports**.
- ASan-BLOCKED (normal-only, BlockChain→Database→non-ASan rocksdb):
  `Test_NrdPreCommit`, `Test_NrdFalseBan`, `Test_NrdBatchSafety`,
  `Test_KernelIndex`, `Test_BlockPipeBan`, `Test_NrdBodylessRestart`,
  `Test_NrdKillHarness`.
- ASan остаётся source-bound к build-09 frozen production sources; никаких
  ASan-claims за пределами subset. Build-10 (только tests) не изменит ASan
  libs/TestsMini — при его появлении достаточно re-verify hashes/manifest.

## 5. CI pipeline `ci-0a3-01` (на связанном build-09)

- overall **FAIL** — честно, потому что full-suite содержит 1 **test-only**
  провал; НЕ выдаётся за PASS;
- provenance **PASS**, unit-regressions **PASS**, full-suite **FAIL**
  (249/248/1, тот же bodyless), **asan-subset PASS** (4/4 filters, sha_match),
  fuzz-smoke **PASS (ADVISORY)**, untested-zones REPORTED;
- evidence: `evidence/ci/ci-0a3-01/summary.json`.

## 6. Binding (Lead decision (b))

- **Normal authoritative:** Tests `0A3B7D66…` + GrinNode `F90EEF38…`
  **with explicit exception**: full 249 cases/248 passed/1 failed; only
  test-only `Test_NrdBodylessRestart`; bodyless-NRD + kill-recovery
  **NOT VERIFIED**; zero production/agreed-suite failures. **Не полностью
  зелёный.**
- **Historical:** C8C386BE/F9EF06B4, build-08 7994F191/33258AE7, 930, 8F10, DDC,
  FD7C — не привязаны.
- **ASan:** source-bound `CF78C188…`/TestsMini `12C3D16C…` (build-09 production
  sources only); subset PASS как в §4.
- Build-10 — backlog только по отдельному approval (test-only plain-block
  bodyless + harness restore); без нового production изменения.

## 7. Ограничения

- build-09 НЕ fully green (bodyless test-only; kill NOT VERIFIED);
- ASan не покрывает rocksdb/Database/ChainState; BlockPipe/bodyless/kill/
  PreCommit/FalseBan/BatchSafety/KernelIndex остаются normal-only;
- coverage-guided fuzzing отсутствует; snapshot/NRD fault-injection —
  unit-симуляции; live G-09 NOT VERIFIED; crafted-zip NOT VERIFIED;
  исторический 0xC0000409 root cause NOT VERIFIED
  (teardown-UAF — отдельная находка); hosted CI NOT VERIFIED.
