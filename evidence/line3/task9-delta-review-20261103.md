# Task-9 delta review + bind plan (line3, read-only) — 2026-11-03

Line2 task-9 source delta reviewed in the working tree. No build/commit; build-09
freeze/binding unchanged; new tests are registered in their subdir CMakeLists but
not built until build-10.

## Delta scope (verified, uncommitted)

| File | SHA-256 | State |
|---|---|---|
| `src/API/Node/Handlers/GetKernelHandler.h` | `91478B14721BB00A90249BA383E2D672BC8AD1BAFEA236FC62E457E010C0DCCE` | modified (+6/−?) |
| `src/Database/BlockDBImpl.cpp` | `E4EA1853ECDA39587FB924AF3D8FF6D79713E736C655B54F2A76F29B65B80680` | modified (+138/−16) |
| `tests/src/API/Test_GetKernelHandler.cpp` | `F1FBCE97940EC4C598D213BEFF2D9B5BDC3A7ABC6FBF347B7B897A6F62E3BC33` | new, CMake-registered |
| `tests/src/Database/Test_KernelIndexSchema.cpp` | `4FB671378829A900113009FE9D638CAEA67676362A601202FCF2E2B669F4D9EF` | new, CMake-registered |
| `tests/framework/include/TestServer.h` | (modified) | adds `GetTxHashSetManagerShared()` seam |

`git status`: only these + prior uncommitted cross-line work; no production files
beyond the two above.

## Review — accepted

1. **Handler lifetime fix:** `std::optional<TransactionKernel> resolvedKernel`
   is now function-scope (line ~94), direct-scan sets `pKernel = &resolvedKernel.value()`
   (line ~149), dereference at line ~157 survives serialization; indexed path and
   JSON unchanged. Correct.
2. **Schema v2:** `KERNELPOS2/<gen>/P/<BE pos><excess>` primary per leaf;
   `.../E/<excess><BE pos>` secondary per occurrence; `KERNELIDX2/STATE` state.
   `GetKernelPosition` returns the last secondary (latest global position);
   `GetKernelPositionCount/Checksum` iterate **primary** keys, checksum uses the same
   `KernelIndexXorHash(excess, location)` as rebuild; duplicates no longer shortfall.
   `Remove/Clear` adapted to both key sets. No `KernelIndex.cpp` change needed.
3. **Handler range fallback:** when the latest global position is outside
   `[min_index,max_index]`, `inRange` is false → `pKernel` stays null → bounded
   direct scan runs, which finds the **latest in range** (Rust semantics). Code
   path confirmed; test matrix already covers ranges on a non-duplicate chain.
4. **Tests:** `Test_GetKernelHandler` covers direct-scan existing/missing, ranges,
   index-enabled path; `Test_KernelIndexSchema` covers duplicates, latest,
   all-leaf count/checksum, remove/clear. Both CMake-registered (API/Database line 6).

## Review — risks/gaps to address (no blockers for build-10 compile)

1. **`RemoveKernelPositions` removes all occurrences of an excess** in a
   generation (secondary prefix scan + matching primaries), not explicit
   positions. There is currently **no production call site** (only the schema
   test); if future reorg/delta maintenance uses it with duplicate excesses, it
   can delete a surviving earlier occurrence, forcing a fail-closed rebuild.
   Recommendation: take exact positions or restrict/document the API; add a
   partial-removal test if it is intended for reorg.
2. **No handler test for duplicate excess + range fallback.** Schema test proves
   latest-global; handler test uses a non-duplicate TestServer chain. Needed:
   seed two occurrences of the same excess at heights h1<h2 and request a range
   excluding h2 -> must return h1 (direct-scan fallback), and a range including
   both -> h2. This is the remaining end-to-end duplicate-excess regression gap.
3. **Legacy migration:** old `KERNELPOS/`/`KERNELIDX/STATE` entries are ignored
   and orphaned; lazy rebuild starts a new generation. Storage bloat on the live
   fixture (~3.35M old keys) is accepted/documented; add an explicit "legacy
   ignored" test and a cleanup backlog item.
4. **TestServer seam** exposes a mutable shared `TxHashSetManager`; test-only and
   documented ("must not replace"), acceptable. Keep an eye on accidental
   mutation across tests.
5. **ASan:** both new tests use `TestServer` real DB → normal-only
   (BlockChain→Database→rocksdb boundary). The handler lifetime pattern is
   covered by the task-9 pattern reproducer; real-handler ASan is **NOT
   APPLICABLE** unless a fake-only DB seam is introduced. Record honestly in CI.

## Bind plan (after Lead orders build-10; no build now)

1. Verify preserved build-10 Tests SHA == repo, freeze, production delta.
2. Run isolated with `-KeepBinary`: `GetKernelHandler*`, `KernelIndex schema v2*`,
   plus the agreed regressions (Nrd*/archive/advert/KernelIndex) on the same SHA.
3. Add two CI groups to `tools/build/ci-regressions.ps1`:
   `get-kernel-handler` = `GetKernelHandler*`; `kernel-index-schema` =
   `KernelIndex schema v2*` (append to existing groups; no parallel wallet).
4. Bind line2's evidence + these runs in EVIDENCE/CI_REPRO; keep ASan stage
   source-bound; mark these two tests normal-only.
5. Keep task-9 draft harness unregistered; retire its placeholder once line2's
   real tests are compiled and green (do not duplicate coverage).

## Update 2 — revised delta after review (2026-11-03, read-only)

New hashes verified:
- `GetKernelHandler.h` `91478B14…DCCE` (unchanged);
- `BlockDBImpl.cpp` `8B4460B7…975B`; `BlockDb.h` `CF709B51…8630`; `BlockDBImpl.h` `4D9B7E6F…248B`;
- `Test_GetKernelHandler.cpp` `10C7891C…20B4`;
- `Test_KernelIndexSchema.cpp` `0FA18D19…2DAA`;
- `Test_NrdKernelRange.cpp` `CE682138…B77D` (CMake-registered).
Exact `RemoveKernelPosition(gen,excess,location)` replaces excess-only removal;
only BlockDB/FakeBlockDB implementations exist; no stale callers. Handler E2E
duplicate-range case added; legacy-ignored case added; PMMR latest-in-range case
added. Expected counts: KernelIndex* 3→5, Nrd* 30 unchanged, full suite +5 cases
(249→254).

### Concrete issues found in revised tests

1. **Legacy test will throw at runtime:** `Test_KernelIndexSchema.cpp` writes
   `pRawDB->Put("", DBEntry<...>)`, but `RocksDB::GetTable` matches
   `RocksDBTable::GetName()`; RocksDB's default column family is named
   `"default"` (`kDefaultColumnFamilyName`; verified in the toolchain header),
   and production uses `Put("default", ...)`. `Put("")` → "Database table  not
   found" exception. Use `"default"` (or a table handle).
2. **Legacy test does not force a rebuild:** it mines 3 blocks (v2 state + v2
   positions already committed), then adds legacy keys and reopens. The reopen
   reads the existing v2 state, so the assertions pass without proving "v2
   ignores legacy and rebuilds correct". Must clear v2 state/positions (raw
   `KERNELIDX2/STATE` + `KERNELPOS2/...` keys) before reopen, or start from a
   DB containing only legacy keys; then assert new generation/count/checksum.
3. **Handler duplicate-range test** builds two real blocks sharing one plain
   kernel excess (h30/h32) and is the right E2E shape; it depends on real
   validation accepting the reused kernel signature/excess — build-10 runtime
   will confirm. If validation rejects it, fall back to a fixture-level handler
   test.
4. **PMMR range test** needs real `ChainBuilder::ApplyBlock` to accept duplicate
   NRD kernels at h1/h3; build-10 runtime will confirm. Expectations use
   `GetKernelMMRSize` node-count semantics; if that assumption is wrong the test
   will show it.
5. **Bodyless/kill unchanged since build-09** (`63BE69E0…`, `EA289E86…`): full
   build-10 suite is expected to remain 254 cases with the known test-only
   bodyless failure (253 pass/1 fail) unless a separate fix is added. Do not
   bind build-10 as fully green on count alone.

### Bind plan delta (same conditions: after Lead orders build-10)

- Filters: `GetKernelHandler*` (2 cases), `KernelIndex schema v2*` (2),
  `TxHashSet duplicate-excess*` (1; PMMR), plus existing regressions.
- Expected normal counts: KernelIndex* 5 (was 3), Nrd* 30 unchanged, full +5
  cases; bodyless test-only failure likely persists.
- Legacy test must be fixed (CF name + forced rebuild) before it can be accepted
  as evidence; otherwise mark it FAIL/not evidence.
- ASan: normal-only for all three tests (real DB/API); pattern-level handler
  evidence remains separate. Task-9 draft stays unregistered.

## Update 3 — legacy-test fixes verified (2026-11-03, read-only)

`Test_KernelIndexSchema.cpp` sha256
`F4F976F07A65FBBB8AAC9F65DCEFAE98E641FA5977DD8BC1A5847E45F225DBE7`; all
other task-9 hashes unchanged from Update 2.

Both reported issues are fixed in code:
1. raw DB access now uses CF `"default"` for Put/Delete/GetIterator (matches
   `RocksDBTable::GetName()` and production `KERNEL_POS_TABLE="default"`);
2. the test deletes `KERNELIDX2/STATE` and every `KERNELPOS2/...` key, asserts
   legacy `KERNELIDX/STATE` + `KERNELPOS/...` remain and v2 state/positions are
   absent, then `TestServer::OpenExisting()` and asserts fresh rebuild: non-null
   state, tip hash/height, kernelCount==entryCount==tip numKernels, position
   count, and read-back checksum == published state checksum. This is a genuine
   rebuild proof.

Remaining pre-build-10 unknowns are runtime-only: real duplicate plain-kernel
chain acceptance (handler test) and duplicate NRD `ApplyBlock` (PMMR test);
counts expected KernelIndex* 5, Nrd* 30, full 254 with bodyless known-fail.
No build/commit; bind after build-10 SHA.

## Update 4 — line2 test fixes verified; runner quoting fixed (2026-11-03)

Hashes verified:
- `Test_KernelIndex.cpp` `5F911DBC…E137`: schema-v2 corruption fixture now expects
  `count == kernelCount + 1` (extra primary leaf) and then `EnsureKernelIndex()`
  detects mismatch/rebuilds — matches the audit finding.
- `Test_NrdKernelRange.cpp` `BA2B36A6…1D6E`: root cause documented (genesis
  kernel seeded by KernelMMR::Load = leaf0; h1 = leaf1; h2 header size=3; h3 =
  leaf2). Expectations use 1-based PMMR node position: `firstIndex = 2`,
  `h2MaxIndex = 3`, `latestIndex = 4`; consistent with `LeafIndex::At(leaf)`
  (`position = 2*leaf - popcount(leaf)`, +1 for 1-based index). The earlier
  `2 == 3` failure was exactly firstIndex vs header node-count mismatch.

Runner/CI fix (verified with tests-only 4F6319EB, no build):
- `run-tests-isolated.ps1` sha `2A842CE1…5BA9`: `-TestArgs` per-element quoting;
  `-FailOnNoTests` → `FAIL_NO_TESTS`/exit1; zero-test teardown without the flag
  remains PASS.
- `ci-regressions.ps1` sha `AA378EDC…1853`: quoted args + no-tests guard; new
  groups `get-kernel-handler`, `kernel-index-schema`, `kernel-duplicate-range`
  (opt-in via `-IncludeTask9`/`-Only`); pipeline got `-IncludeTask9`.
- Verification artifact: `runner-quoting-guard-20261103.json` sha `32AE57F9…3E48`;
  quoted schema filter PASS 2/26; bogus filter FAIL_NO_TESTS; CI groups handler
  PASS, schema PASS, duplicate-range FAIL (expected until new build).

Expected after final Tests+GrinNode build-10: `KernelIndex*` 5 cases/68 assertions
PASS; full 254 cases with only the known bodyless test-only failure; tests-only
4F6319EB stays evidence-only, not bound. ASan normal-only for these tests.
