# Tests-only build-10 audit (4F6319EB) — NOT GREEN, not final

Lead-declared "tests-only build with API/index fixes is GREEN". Independent line3
audit of the preserved binary shows **3 full-suite failures** (all test-only;
no production failure observed). This is **NOT** the final build-10 (GrinNode is
still F90EEF38) and must not be bound as fully green.

## Identity (verified)

| Поле | Значение |
|---|---|
| Tests SHA | `4F6319EBD8A399DEEAEB7C20E43470A06ECC17746CB9017341D2DB8F5F079CB1` |
| size | 9 410 560 B |
| PE ts | `0x6AE92A6E` |
| preserved | `C:\DHW\.tmp\build-10-tests\Tests-retry.exe`; repo == preserved |
| build | `build-console-retry.log` → BUILD OK (B00M 07:27:26); first attempt log failed |
| GrinNode | `F90EEF389D1B35B7A17BD8E3C35F06AECFD9C39410C4F755713E1CD1F448A2F8` (build-09 value, **not final**) |
| source freeze | 0 compiled-root changes after Tests mtime 07:27:26; test files mtime 07:19 (included in build) |

Current task-9 source hashes at build time: `GetKernelHandler.h 91478B14…`,
`BlockDBImpl.cpp 8B4460B7…`, `Test_GetKernelHandler.cpp A8F9BE55…`,
`Test_KernelIndexSchema.cpp BAC9D193…`, `Test_NrdKernelRange.cpp CE682138…`.
(Line2's last message quoted older F4F976F0/10C7891C hashes; the current tests
are the 07:19 versions and **are** in this binary.)

## Independent isolated runs (preserved, -KeepBinary)

| filter | результат |
|---|---|
| GetKernelHandler* (2) | **PASS — 49 assertions / 2 cases** |
| KernelIndex schema v2* (2) — quoted filter | **PASS — 26 assertions / 2 cases** |
| TxHashSet duplicate-excess* (1) | **FAIL — 7 assertions, 6 passed / 1 failed**, deterministic (2 runs) |
| KernelIndex* (5) | **FAIL — 5 cases, 4 passed / 1 failed; assertions 68 / 67** |
| Nrd* | **PASS — 30 cases / 428 assertions** |
| PibdArchiveServer::* | **PASS — 8 / 187** |
| P2P::AdvertisedTip* | **PASS — 2 / 15** |
| full suite | **FAIL — 254 cases / 251 passed / 3 failed; assertions 4109 / 4106** |

### Full-suite failures (all test-only)

1. `tests/src/BlockChain/Test_KernelIndex.cpp(78)`
   `REQUIRE(GetKernelPositionCount(gen) == pState->GetKernelCount())` → **9 == 8**.
   The corruption fixture calls `AddKernelPosition(gen, tipKernel.excess,
   OutputLocation(LeafIndex::At(1),1))` expecting the old schema's
   overwrite-by-excess ("entry count unchanged"). Schema v2 keys by
   position+excess, so this adds a **new primary leaf** (count 9); the assertion
   is obsolete. **Test-only**, needs schema-v2 update (corrupt an existing
   primary value via raw DB, or expect count+1 and verify Ensure repairs).
2. `tests/src/BlockChain/Test_NrdBodylessRestart.cpp(103)` — known bodyless
   test-only failure (unchanged since build-09 `63BE69E0…`). NOT VERIFIED.
3. `tests/src/PMMR/Test_NrdKernelRange.cpp(50)`
   `REQUIRE(found->second == firstIndex)` → **2 == 3**. New PMMR duplicate-range
   test: `firstIndex = pHeader1->GetKernelMMRSize()` while
   `FindKernelByExcess` returns 2. Index-unit/expectation mismatch in the test;
   the handler-level duplicate E2E test **passes** (latest-in-range works on a
   real duplicate chain), so this is **test-only pending line2 analysis**, not a
   confirmed production regression.
4. GetKernelHandler duplicate E2E (h30/h32 shared plain kernel) **PASS**;
   schema v2 exact-remove/legacy-rebuild **PASS**.

## Filter-quoting pitfall (important for CI)

`run-tests-isolated.ps1 -ExeArgs 'KernelIndex schema v2*'` loses quoting through
Start-Process: Catch2 receives `KernelIndexschemav2*` and prints "No tests ran"
(exit 0 → false PASS). Same for `TxHashSet duplicate-excess*`. Correct runs need
quoted filters (`'"KernelIndex schema v2*"'`), as used above. Regression groups
must never treat "No tests ran" as PASS; add a guard in CI wrappers.

## Status / binding

- **NOT final build-10, NOT green.** Do not bind as normal authoritative; build-09
  remains bound (with its documented exception). GrinNode is still F90EEF38.
- No ASan claims; task-9 tests are normal-only.
- Required before final build-10: line2 fixes Test_KernelIndex corruption
  fixture and Test_NrdKernelRange index expectations; bodyless still open.
  Then rebuild (Tests+GrinNode) and re-audit.
