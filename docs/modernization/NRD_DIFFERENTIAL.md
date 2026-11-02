# NRD differential and PIBD-without-bodies rebuild (Line 2)

Status: **PASS for the Rust oracle, the C++ differential suite, the synthetic
PIBD-without-bodies rebuild/reorg/restart suite, the fail-closed txhashset
callers (task-5), the D2 mainnet NRD gate on the block consensus path, the
task-6 false-ban fix, the round-2 reorg-backup/rebuild-exception fixes and the
PAIRED Rust/C++ differential**
(executed on build-07 final candidate Tests SHA-256
`C8C386BE576A62604404E0FA02E56EE9E8F06B50EA0B15AD109F36F5AFF5B7FE`:
paired 11 computed / 3 not_computed / 11 paired cases / **0 paired mismatches**;
`Nrd*` 30 cases / 428 assertions PASS (incl. 2 batch-safety cases);
`NrdFalseBan*` 5 / 17 PASS; `KernelIndex*` 3 / 48 PASS; fixture negative/
positive/report-write-error checks as expected). Earlier builds
`930148B6…BB6E` and `8F10BFD6…D73D6` remain bound to their own evidence.
Full suite on C8C386BE (independent line2 rerun after line3-fuzz completed its
serialized x3): run `full-c8c386-20261103-053707`, **245 cases / 4013
assertions PASS**, exit 0, `-KeepBinary`, isolated HOME/TMP, profile unchanged,
0 crash events — reproduces the Lead/line1/line3 245/4013 PASS.
Remaining non-PASS items (section 8): live end-to-end peer-ban reproduction
(NOT VERIFIED — code/units/static trace PASS), D2 txpool gate + NRD-index policy
early-return (partial), end-to-end reorg over a genuinely bodyless PIBD state
(NOT VERIFIED/BLOCKED), crafted-zip `ProcessTxHashSet` end-to-end harness
(NOT VERIFIED, blocker documented), real PIBD state / rebuild interruption
(NOT VERIFIED), and commit-SHA provenance (NOT VERIFIED). `not_computed`
differential cases are listed explicitly and are not paired evidence.

Round-2 auditor findings (section 9): reorg-backup and rebuild-exception fixes
are **verified on C8C386BE**; the real-Rust `verify_kernel_pos_index` path is
exercised for all 8 computed rebuild cases; paired v3 on C8C386BE is
**11 paired / 0 mismatches / 3 not_computed**; missing-fixture negative,
positive and report-write-error checks all behave as required.

Scope: Grin++ 1.2.9 (`C:\DHW\Gplus\src\grinplusplus`, HEAD `e0a5d2d`) versus
Rust grin 5.5.2 (canonical reference `C:\DHW\Gplus\reference\grin-5.5.2`,
claimed commit `dbaff34`). This line changed only:

- `src/BlockChain/Validators/BlockValidator.h/.cpp` (D1 header-version gate; D2
  mainnet NRD network gate with `VerifyNrdNetworkGate` test seam)
- `src/PMMR/TxHashSetImpl.h`, `src/PMMR/TxHashSetImpl.cpp` (D3/D4, diagnostics)
- `src/BlockChain/Processors/TxHashSetProcessor.{h,cpp}` (task-5 pre-commit NRD
  gate on staged/new state; `EFailureKind` classification)
- `src/BlockChain/BlockChainImpl.cpp` (task-5 fail-closed startup/callers)
- `include/Core/Config.h`, `src/Core/Config.cpp` (D2
  `Config::IsNrdKernelEnabled(Environment)` helper)
- `tests/src/PMMR/Test_NrdRules.cpp` (extended)
- `tests/src/PMMR/Test_NrdDifferential.cpp` (new)
- `tests/src/PMMR/Test_NrdIndexRebuild.cpp` (new)
- `tests/src/PMMR/Test_NrdSupport.h` (new)
- `tests/src/PMMR/Test_NrdPreCommit.cpp` (new, task-5 gate/classification tests)
- `tests/src/PMMR/Test_NrdPolicy.cpp` (new, D2 network gate tests)
- `tests/src/PMMR/CMakeLists.txt` (additive test registrations; auditor ZipFile
  line preserved)
- `tools/audit/nrd/**` (cases, README, evidence)
- this document

Nothing is committed. Line 1's P2P files and the auditor's `ZipFile.cpp` /
`Test_ZipFile.cpp` were not touched.

## 1. Reference and oracle provenance

| item | value | status |
| --- | --- | --- |
| Rust version | `Cargo.toml` `grin 5.5.2`; `Cargo.lock` in the canonical tree | PASS (content) |
| claimed commit `dbaff34` | no `.git` in the reference tree | **NOT VERIFIED** |
| canonical tree hash | 391-file sorted `path\0sha256` manifest digest `0677A6D981525A4B8D5FD1C5A49ECE404DF13D5067F78B0484BEF5E32044000E` (`tools/audit/nrd/evidence/grin-5.5.2-reference-sha256.txt`) | PASS (content) |
| oracle copy | `C:\Users\nurli\AppData\Local\Temp\grin55-minpeers` | byte-identical to canonical for `chain/`, `core/`, `pool/`, `store/`, `util/`, `keychain/`; only `servers/src/grin/sync/syncer.rs` (another line's min-peers patch) and `Cargo.lock` differ |
| toolchain | rustc 1.99.1 (58cf8f9c3 2026-10-13), cargo 1.99.1 (5f94df478 2026-08-27), MSVC x64, static CRT (`-Ctarget-feature=+crt-static`) | PASS |
| oracle binaries | SHA-256 list in `tools/audit/nrd/evidence/binaries-sha256.txt`; `grin.exe` `2023847F…A34CB` (line 1 live node, not rebuilt here) | PASS |
| source integrity | pre/post manifests of the oracle copy identical (`rust55-source-sha256-{pre,post}.txt`); only the copied `chain/.grin*` test data dirs were created and removed | PASS |

Oracle test runs (release profile reused from `Temp\grin55-target`; logs in
`tools/audit/nrd/evidence/`):

```powershell
$env:CARGO_TARGET_DIR = 'C:\Users\nurli\AppData\Local\Temp\grin55-target'
cd C:\Users\nurli\AppData\Local\Temp\grin55-minpeers
cargo test --release -p grin_chain --test nrd_validation_rules        # 3 passed
cargo test --release -p grin_chain --test store_kernel_pos_index     # 5 passed
cargo test --release -p grin_core  --test block                      # 27 passed
cargo test --release -p grin_pool  --test nrd_kernel_relative_height # 1 passed
cargo test --release -p grin_pool  --test nrd_kernels_disabled       # 1 passed
cargo test --release -p grin_pool  --test nrd_kernels_enabled        # 1 passed
cargo test --release -p grin_core  --lib nrd                         # 3 passed (36 filtered)
```

These are the real Rust 5.5.2 code paths, not a reimplementation:

- `nrd_validation_rules` builds real blocks with real NRD kernel signatures and
  asserts: same-block duplicate invalid; `relative_height=2` at `+1` invalid and
  `+2` valid; `relative_height=1` at `+1` valid; fork/rewind case with index
  rewind and re-application (`Rewind extension ... checking NRD index ...` log
  lines show the real `apply_kernel_rules` decisions);
- `store_kernel_pos_index` pins `MultiIndex` semantics: `peek_pos` = latest,
  positions must increase, rewind/pop/clear;
- `block` pins the header-version/feature gates (`NRDKernelPreHF3` at
  `version < 4`, `NRDKernelNotEnabled` when the flag is off);
- `nrd_kernel_relative_height` pins pool/stempool handling of repeated NRD
  excesses across blocks.

## 2. Differential format and categories

Machine-readable cases: `tools/audit/nrd/cases/nrd_cases.json`
(format documented in `tools/audit/nrd/README.md`). Each case stores network,
header version, chain blocks, NRD/non-NRD kernels with full features, excess,
relative height, prior occurrences with heights/positions (derived from the
block list and the real MMR), pre-state (`incremental` / `rebuild` / `no_index`)
and the expected decision category.

Categories distinguish the reasons for `false`:
`accepted`, `accepted_non_nrd`, `invalid_nrd_relative_height`,
`invalid_nrd_duplicate_in_block`, `missing_local_history`, `malformed_input`.

The C++ runner (`Test_NrdDifferential.cpp`) executes every case through the real
`TxHashSet::ApplyBlock` / `TxHashSet::RebuildRecentKernelIndex` on a real
file-backed `KernelMMR` and records
`TxHashSet::GetLastNrdValidationResult()`. A decision is only compared against
the category, never only against the bool.

> **Correction (2026-11-03, auditor finding accepted):** the historical
> "14/14" claim is **C++ actual vs expected metadata**, not a paired Rust/C++
> comparison. `expected_rust_category` is derived from cited Rust test sources
> and is NOT computed by executing Rust on the same input. The report therefore
> carries `rust_result_source = expectation_metadata_not_computed`. A real
> paired oracle now exists in source (`tools/audit/nrd/rust-oracle`) and the C++
> runner compares actual Rust vs actual C++ when `GRINPP_NRD_RUST_ORACLE` is
> provided; paired N/N may only be claimed for the computed subset after that
> run. A missing fixture/report now FAILS the C++ test instead of skipping.

Case matrix (14 chain cases + 4 header/network cases):

| case | mode | Rust oracle | category |
| --- | --- | --- | --- |
| first-occurrence-accepted | rebuild | nrd_validation_rules block_valid_9 | accepted |
| exact-boundary-accepted | rebuild | nrd_validation_rules height 11 | accepted |
| one-before-boundary-invalid | rebuild | nrd_validation_rules height 10 | invalid_nrd_relative_height |
| relative-height-1-next-block-accepted | rebuild | nrd_validation_rules rel=1 | accepted |
| three-occurrences-boundary-accepted | rebuild | pool nrd_kernel_relative_height | accepted |
| duplicate-nrd-in-one-block-invalid | rebuild | verify_no_nrd_duplicates | invalid_nrd_duplicate_in_block |
| plain-excess-not-indexed | rebuild | apply_kernel_rules indexes NRD only | accepted |
| plain-repeat-accepted | incremental | NRD rules are NRD-only | accepted_non_nrd |
| multi-kernel-mixed-block-attribution | rebuild | verify_kernel_pos_index attribution | accepted |
| missing-local-history | no_index | local incompleteness is not peer invalidity | missing_local_history |
| rebuild-rejects-historical-violation | rebuild (must not publish) | verify_kernel_pos_index replays rules | missing_local_history |
| rebuild-rejects-duplicate-history-block | rebuild (must not publish) | same | missing_local_history |
| incremental-one-before-boundary-invalid | incremental | nrd_validation_rules | invalid_nrd_relative_height |
| incremental-exact-boundary-accepted | incremental | nrd_validation_rules | accepted |

Plus `header_version_cases`: version 3 (Rust `NRDKernelPreHF3`, C++ rejected
after D1 fix), version 4 (passes the gate), mainnet (Rust disables NRD, C++ has
no flag — documented divergence D2), testnet (rules enforced).

## 3. Divergences found

### D1 — header version gate was one hard fork too early (FIXED)

`BlockValidator::VerifyKernelFeatures` rejected NRD kernels only for
`header.version < 3`. Rust 5.5.2 `verify_nrd_kernels_for_header_version`
(`core/src/core/block.rs`) requires `HeaderVersion(4)` (HF3):
`core/tests/block.rs::block_with_nrd_kernel_pre_post_hf3` pins version 3 to
`Error::NRDKernelPreHF3` and version 4 to valid. A version-3 block carrying an
NRD kernel would have been accepted by Grin++ and rejected by Rust.

Fix: `version < 4`, message "not valid before header version 4". Regression:
`NrdHeaderVersionGateMatchesRustHf3Rule` (C++), oracle: `block` test suite.

### D2 — NRD feature flag (DOCUMENTED, not implemented)

Rust disables NRD entirely on mainnet (`src/bin/grin.rs`:
`init_global_nrd_enabled(false)` for `ChainTypes::Mainnet`, true otherwise) and
`block_with_nrd_kernel_nrd_not_enabled` pins `NRDKernelNotEnabled`. Grin++ has
no NRD feature flag: it enforces NRD rules on every network. On mainnet a
NRD-bearing block would be rejected by Rust and evaluated by Grin++ on its
relative-height rules. Recommendation: add a network feature gate at chain
startup; not implemented here because it is network policy outside this line's
verified live scope.

### D3 — rebuild published an index without replaying NRD history (FIXED, fail-closed)

Rust `TxHashSet::verify_kernel_pos_index` calls `apply_kernel_rules` for every
NRD kernel while rebuilding; a historical violation returns `NRDRelativeHeight`
and the txhashset write fails. `TxHashSet::RebuildRecentKernelIndex` only
populated the index, so a PIBD state with a violating kernel history would be
published and accepted by Grin++.

Fix: the rebuild now replays the same relative-height check (including
same-excess duplicates inside one historical block), and on violation clears
the index and leaves it **incomplete**, so any NRD block is rejected
`missing_local_history` (fail-closed). The rebuild also now requires the kernel
MMR leaf count to equal the tip header's cumulative kernel count, so a short or
over-long kernel MMR is never published.

Residual difference: Rust errors the whole txhashset write; Grin++
(`RebuildRecentKernelIndex` returns `void`, callers in `BlockChainImpl.cpp`
outside this line's write scope) only refuses NRD blocks afterwards. Making the
caller invalidate the received state on an incomplete rebuild is the recommended
follow-up.

### D4 — rollback did not restore the in-memory NRD index (FIXED)

`TxHashSet::Rollback` restored the three MMRs and the flushed header but not
`m_recentNrdKernels`/`m_recentNrdIndexComplete`. A failed block (`ValidateRoots`
or kernel-sum failure after `ApplyBlock`) or a reorg that ends with
`pBatch->Rollback()` (alternative branch with lower total difficulty) left stale
NRD entries, so re-applying the same valid block could be falsely rejected as a
duplicate. Fix: `Commit()` snapshots the index and `Rollback()` restores it
(fail-closed if the restore throws); a failed rebuild invalidates the snapshot.
Test: `NrdRollbackRestoresIndexSoReappliedBlockIsAccepted`.

### D5 — rewind over bodyless blocks (PARTLY PARITY, NOT VERIFIED end-to-end)

Both implementations obtain the rewound block body from the DB
(Rust `txhashset.rs:1613 batch.get_block`, C++ `TxHashSet::Rewind`
`pBlockDB->GetBlock`) and use per-block spent positions. So "PIBD without bodies,
then reorg over those bodyless blocks" is not supported by either client through
this path; it is **NOT VERIFIED** here and cannot be claimed PASS. Grin++ also
silently skipped the NRD cut when the previous header was missing; that case now
clears the index and leaves it incomplete (fail-closed). No end-to-end
valid/invalid-apply + reorg over a genuinely bodyless PIBD chain was executed in
this line (would need a live/PIBD-produced state); marked BLOCKED below.

### Missing local history is not invalid-peer proof

`missing_local_history` is a distinct category. The fail-closed rule prevents a
false accept but is not evidence against a peer. Historical-rule violations are
reported separately (`missing_local_history` after a refused rebuild) and are
never conflated with an invalid remote block.

## 4. Rebuild test matrix (`Test_NrdIndexRebuild.cpp`)

All tests use a synthetic file-backed kernel MMR + header chain + fake block DB
and the real `TxHashSet`; bodies are absent unless explicitly stored.

| test | property |
| --- | --- |
| `NrdRebuildPibdWithoutBodiesPublishesKernelMmrPlusHeaders` | PIBD state (0 bodies), rebuild at tip 3, entries at exact positions/heights, then valid/invalid/boundary applies (`rel=4`: +2/+3 invalid, +4 valid) |
| `NrdRebuildHonoursTwoWeekWindowCutoff` | tip 2*WEEK+2: kernel at height 1 excluded, kernel at cutoff height 2 included, 0 bodies, 3 kernel leaves |
| `NrdRebuildRejectsHistoricalRelativeHeightViolation` | cross-block violation and same-block historical duplicate leave the index incomplete and unpublishable; exact-boundary valid history publishes |
| `NrdRebuildNeverPublishesPartialIndex` | missing header, short kernel MMR vs claimed tip, old-tip replacement via real rewind: index stays incomplete/no stale entries |
| `NrdIndexMissingIsNotAcceptedAndRebuildFixesIt` | no index => `missing_local_history`; rebuild => accepted |
| `NrdRollbackRestoresIndexSoReappliedBlockIsAccepted` | D4 |
| `NrdRewindRemovesEntriesOfRewoundBlocks` | reorg rewind removes rewound NRD entries, keeps earlier ones, alternative branch applies |
| `NrdMultiKernelBlocksKeepEveryPosition` | multi-kernel blocks, plain+Nrd in one block, correct positions (`LeafIndex::At`), plain excess not indexed |
| `NrdIndexSurvivesRestartFromKernelMmrAndHeaders` | reopen KernelMMR/header DB (restart), rebuild, identical entries |

## 5. C++ runtime evidence

| item | value |
| --- | --- |
| baseline (pre-change binary) | `NrdRules*` 3 cases / 13 assertions PASS, `Tests.exe` SHA-256 `4F75DCD4…636C7B`, log `C:\DHW\.tmp\line2-nrd\cpp-nrd-baseline-20261103-022651\nrd-rules.log` |
| new build | `pwsh tools/build/build.ps1 -Target Tests`, BUILD OK 2026-11-03T02:56:09+07:00; `bin\Release\Release\Tests.exe` 9012736 bytes, SHA-256 `0EDC0F810E7D9BB864F27266EEB0872DC12288B6F08A9E79293B44D912235FC7`; build log `tools/audit/nrd/evidence/cpp-build-20261103-024953.log` |
| Nrd* isolated run | run id `nrd-20261103-025613`, **17 test cases / 350 assertions, all passed**, exit 0; filters `Nrd*`; differential runner executed **14 chain cases**; isolated HOME/TMP; real user profile unchanged; `tools/audit/nrd/evidence/isolated/nrd-20261103-025613/{manifest.json,instance-1.stdout.log}` |
| KernelIndex* isolated run | run id `kernidx-20261103-025622`, **3 test cases / 48 assertions, all passed**, exit 0; same binary SHA; `tools/audit/nrd/evidence/isolated/kernidx-20261103-025622/` |
| differential JSON report | `tools/audit/nrd/evidence/cpp-nrd-differential-report.json` (per-case actual vs expected category, index completeness/count, Rust oracle reference) |
| binary ↔ log binding | exact tested binary preserved as `tools/audit/nrd/evidence/bin/Tests-0EDC0F810E7D.exe` (SHA-256 `0EDC0F81…235FC7`, 9012736 bytes, PE ts `0x6AE8EAD8`) with a `.sha256.txt` manifest; line3-fuzz independently re-ran both suites on this SHA with `-KeepBinary` (`evidence/line3/nrd-audit-20261103-01/02`, audit memo `evidence/line3/nrd-handoff-audit-20261103.md`) |

Build/run commands (approved slot):

```powershell
pwsh tools/build/build.ps1 -Target Tests
pwsh tools/build/run-tests-isolated.ps1 -RunId nrd-<ts> `
    -RunRoot 'C:\DHW\.tmp\line2-nrd\iso' `
    -EvidenceRoot '<repo>\tools\audit\nrd\evidence\isolated' -TestArgs 'Nrd*'
pwsh tools/build/run-tests-isolated.ps1 -RunId kernidx-<ts> ... -TestArgs 'KernelIndex*'
```

### Fail-before / pass-after

Three fixture bugs in the first isolated attempt were fixed in the tests
(height arithmetic for one incremental case, 1-byte header-hash collisions after
256 synthetic headers, and an unflushed kernel MMR in the restart test). No
production behavior or expected Rust category was weakened to make them pass;
the failing assertions were exactly the fixture/index-contract signals the Lead
identified.

The new tests were **not executed against a HEAD production build**: that would
need a second Tests build with reverted production files (fail-before run not
performed, another build slot would be required). Fail-before is instead
deterministic from HEAD source:

* D1: `git show HEAD:src/BlockChain/Validators/BlockValidator.cpp` has
  `version < 3`, so a version-3 NRD block would not raise the version error and
  `NrdHeaderVersionGateMatchesRustHf3Rule` would fail on that binary.
* D3: HEAD `RebuildRecentKernelIndex` has no relative-height replay, so
  `NrdRebuildRejectsHistoricalRelativeHeightViolation` would see a published
  complete index and fail.
* D4: HEAD `TxHashSet::Rollback` has no NRD restore, so
  `NrdRollbackRestoresIndexSoReappliedBlockIsAccepted` would see the stale entry
  and reject the re-applied block.

Executed fail-before remains **NOT VERIFIED** and is listed as such below.

## 6. Honest status summary

| item | status |
| --- | --- |
| Rust oracle provenance/version | PASS for content; commit SHA **NOT VERIFIED** |
| Rust oracle runs (7 suites) | PASS (counts above, logs saved) |
| Differential case format + 14 cases | PASS — executed, 14/14 categories match (`nrd-20261103-025613`) |
| D1 fix + regression test | PASS (executed; version 3 rejected, version 4 not rejected by the NRD gate) |
| D3 rebuild history validation + negatives | PASS (executed; violating history leaves the index incomplete, no false accept) |
| D4 rollback restore | PASS (executed; re-applied block accepted after rollback) |
| Partial-index-not-published (C++ NRD index) | PASS (executed: missing header, short kernel MMR, refused historical rebuild; index never complete/carrying stale entries) |
| PIBD-without-bodies -> rebuild -> valid/invalid apply | PASS for the synthetic kernel-MMR+headers state (0 bodies); **NOT VERIFIED on a real PIBD-produced state** |
| reorg sequence via real Rewind | PASS for stored-body blocks (rewind removes NRD entries, alternative branch applies); reorg over genuinely bodyless blocks **NOT VERIFIED/BLOCKED** (both clients read block bodies in rewind) |
| restart equivalence | PASS (executed; reopened kernel MMR + headers rebuild to identical entries) |
| fail-before execution of D1/D3/D4 tests on HEAD | **NOT VERIFIED** (would need a second production build; HEAD source shown to differ) |
| rebuild interrupted before publication (process kill/crash injection) | **NOT VERIFIED** (all refusal paths are covered, but no crash-injection run was executed) |
| disk-full / write-error during rebuild | **NOT VERIFIED** (no fault injection in this line; the in-memory index is only published after successful reads/verification) |
| D2 mainnet NRD flag | **FIXED for the block consensus path**: `BlockValidator::VerifyNrdNetworkGate` wired via `Config::IsNrdKernelEnabled(Global::GetEnv())` (mainnet rejects NRD kernels, Rust parity; tests PASS on SHA `DDC21F45…F714`); txpool gate and NRD-index policy early-return remain **unwired proposal** items (section 7.2) |
| live Floonet differential / real PIBD state | **NOT VERIFIED** (out of this line's executed scope) |

## 7. Follow-up: fail-closed txhashset callers + D2 mainnet gate proposal

### 7.1 Atomic pre-commit NRD gate (implemented, unit tests executed)

Rust grin 5.5.2 validates the kernel-pos index inside `txhashset_write` and only
then commits the store batch (`chain/src/chain.rs`: `verify_kernel_pos_index`
before `batch.commit()`). Grin++ previously committed the new state first and
rebuilt the in-memory index afterwards, so a rejected history left a committed
state (old files deleted, no rollback possible) and a later startup throw could
make the node unbootable.

Implemented:

* `src/BlockChain/Processors/TxHashSetProcessor.cpp`
  * new `TxHashSetProcessor::VerifyNrdIndexBeforeCommit()`: rebuilds the real
    `TxHashSet` NRD index and classifies the outcome:
    * `None` — index rebuilt completely;
    * `InvalidState` — every prerequisite was locally available (kernel count
      matches the header, header chain reaches the 2×WEEK window, kernel leaves
      are readable) yet the rebuild found a historical relative-height violation
      (definite invalid NRD history, Rust would error the write);
    * `LocalUnverifiable` — missing header/kernel data or count mismatch, i.e.
      local incompleteness that is **not** peer-ban evidence.
  * `ProcessTxHashSet`: the gate runs on the **staged** state after
    `ValidateTxHashSet` and before the live directory is swapped and before the
    ChainState batch commit. On refusal the staging dir is removed, the live
    directory is reopened untouched, and the previous working state is kept.
  * `ProcessExistingTxHashSet`: the gate runs after open+validate and **before**
    `pChainStateBatch->Commit()`; on refusal the confirmed chain is not updated.
    (Pre-existing PIBD limitation: `PibdActivation::Activate` already replaced
    the live files before this call; the incomplete index keeps NRD fail-closed.
    Restoring the pre-PIBD files would require a P2P/PIBD scope change.)
  * `TxHashSetProcessor::GetLastFailureKind()` exposes the classification.
* `src/BlockChain/BlockChainImpl.cpp`
  * startup: an incomplete rebuild now throws (matches Rust `Chain::init`
    propagating `init_recent_kernel_pos_index` errors) instead of starting with
    an unvalidated NRD history;
  * `ProcessTransactionHashSet` / `ProcessExistingTxHashSet`: defense-in-depth
    rebuild after processor success must also be complete (returns
    `STORE_ERROR`, never `SUCCESS`), and a processor `LocalUnverifiable`
    refusal maps to `EBlockChainStatus::STORE_ERROR` so `TxHashSetPipe` does not
    ban the peer solely on local incompleteness. `InvalidState` remains
    `INVALID` (peer-ban path), matching a proven invalid NRD history.

Tests (executed on Tests.exe SHA-256
`2032536D810F9DD78E57658FDEC39DCA06B71368551B2B1C6CAB08B79592140F`,
build 2026-11-03T03:33:55+07:00, binary preserved in
`tools/audit/nrd/evidence/bin/`):

* `tests/src/PMMR/Test_NrdPreCommit.cpp` — complete history ⇒ `None`;
  historical violation ⇒ `InvalidState` + unpublished index; missing middle
  header and kernel-count mismatch ⇒ `LocalUnverifiable`; a refused staged state
  leaves a previous "live" txhashset directory byte-identical (content hash
  before/after) and still usable.
* Executed: `nrd-precommit-20261103-033401`, filter `Nrd*` —
  **20 test cases / 375 assertions, all PASS**, isolated HOME/TMP, real user
  profile unchanged, exit 0 (evidence
  `tools/audit/nrd/evidence/isolated/nrd-precommit-20261103-033401/`).
* `KernelIndex*`: `kernidx-20261103-033401` — **3 cases / 48 assertions PASS**.
* Full suite (`full-20261103-033401`, `-KeepBinary`): 231 cases / 3917
  assertions — 230 pass, **1 external failure** in line 1's
  `tests/src/PMMR/Test_PibdServing.cpp::PibdArchiveServer::LeaseOutlivesHorizonSwitch`
  (`REQUIRE(destroyed == 1)` → `0 == 1`, deterministic on a single-test rerun
  `lease-recheck-20261103-033526`); no NRD test failed.
* Remaining: end-to-end `ProcessTxHashSet`/`ProcessExistingTxHashSet`
  integration test with a crafted txhashset zip would need a full node harness
  and is **NOT VERIFIED** in this iteration.
  **Blocker (documented, not pursued):** a rejecting fixture must be a
  roots-valid txhashset whose kernel history violates NRD relative-height rules.
  The node cannot produce such a state (`TxHashSet::ApplyBlock` rejects it), and
  `SnapshotTxHashSet`/TestServer only snapshot valid chains. Building one by hand
  requires writing all three PMMR backends (kernel/output/rangeproof) plus a
  matching header chain, `BlockSums`, output positions and a full `ChainState`
  (LMDB store, confirmed chain, `TxHashSetManager`) so `TxHashSetProcessor` can
  run, then wrapping it in the zip format and driving `BlockChain::Process*`.
  That is a separate full-node fixture project with low marginal value over the
  existing `TxHashSet`-level gate/classification tests, so it is left NOT
  VERIFIED.
* Per-run binary binding (line3-fuzz recommendation): the preserved binary was
  re-run with `-KeepBinary`, so `nrd-final-keepbin-20261103-033742` and
  `kernidx-final-keepbin-20261103-033742` each keep a `Tests.exe` copy with the
  same SHA-256 (`2032536D…2140F`), not only the shared `evidence/bin/` copy.
* **Integrated build binding (line1 final green build, no line2 rebuild):**
  line1/Lead reported the final normal build green (full suite 232 / 3926,
  `PibdArchiveServer::*` 7 / 168, lease 1 / 16) for `Tests.exe` SHA-256
  `FD7C04922A2E016723531EC389E21D3AC1E8C0F99134F003595DB18149AB16E5`
  (9154048 B, PE ts `0x6AE8F63F`). Isolated reruns against that exact binary
  with `-KeepBinary`:
  `nrd-bind-FD7C0492-20261103-034652` — 20 / 375 PASS (14/14 differential) and
  `kernidx-bind-FD7C0492-20261103-034652` — 3 / 48 PASS, each keeping its own
  `Tests.exe` copy with the same SHA-256. Differential report
  `cpp-nrd-differential-report-FD7C0492.json` SHA-256 `EA44A390…5FCB`.

### 7.2 D2 mainnet NRD gate — block consensus path **wired**; txpool still unwired

Rust grin 5.5.2 `src/bin/grin.rs`:

```rust
match global::get_chain_type() {
    ChainTypes::Mainnet => global::init_global_nrd_enabled(false),
    _ => global::init_global_nrd_enabled(true),
}
```

and `core/src/core/block.rs` rejects NRD kernels with
`NRDKernelNotEnabled` when the flag is off (pinned by
`core/tests/block.rs::block_with_nrd_kernel_nrd_not_enabled`), *before* the HF3
header-version rule. Previously Grin++ evaluated NRD relative-height rules on
every network, so on mainnet it could accept a version-4 NRD block that Rust
rejects.

Implemented (Lead-approved, no other policy changes):

* `include/Core/Config.h` / `src/Core/Config.cpp`:
  `Config::IsNrdKernelEnabled(Environment)` returns false only for `MAINNET`,
  true for `FLOONET` and `AUTOMATED_TESTING` (mirrors grin.rs).
* `src/BlockChain/Validators/BlockValidator.h/.cpp`:
  `BlockValidator::VerifyNrdNetworkGate(block, environment)` throws
  `BadDataException` (`EBanReason::BadBlock`,
  "NRD kernel … is not enabled on this network") for any NRD kernel when the
  policy is off. `VerifyKernelFeatures` calls it with `Global::GetEnv()` before
  the HF3 version check, matching Rust error precedence
  (`NRDKernelNotEnabled` before `NRDKernelPreHF3`). The gate is public so the
  mainnet/testnet decision is unit-testable without switching the process-wide
  `Global` environment; plain kernels are unaffected.
* `tests/src/PMMR/Test_NrdPolicy.cpp` now covers: network decision table;
  mainnet rejects an NRD block while FLOONET/AUTOMATED_TESTING accept it at the
  gate; mainnet leaves plain-kernel blocks alone; network-error precedence over
  the HF3 version error; and the wired `VerifySelfConsistent` path under the
  test process network (AUTOMATED_TESTING => no network-gate error).
  Runtime status: **PASS** on Tests.exe SHA-256
  `DDC21F45EBFE6FA4846FFFB0FA5E376297EED5BFFEFAA5583E3A6B04B0BCF714`
  (built 2026-11-03T03:59:57+07:00, binary preserved in
  `tools/audit/nrd/evidence/bin/`): `nrd-d2-DDC21F45-20261103-040003` —
  **23 test cases / 382 assertions PASS** (includes the 4 `Test_NrdPolicy`
  cases), `KernelIndex*` 3 / 48 PASS, and the **full suite 236 cases /
  3952 assertions ALL PASS** (`full-d2-DDC21F45-20261103-040003`,
  `-KeepBinary`); differential 14/14.
  **Provenance nuance (line3-fuzz audit):** the DDC21F45 tree also contains
  line1's uncommitted `tests/src/PMMR/Test_PibdServing.cpp` update
  (mtime 2026-11-03T03:54:55+07:00, SHA-256
  `EDB1BA612B18B5DA167F00EC6A7A7F6EDBCCC2DAE024C055E28FA1572FD799A3`), which
  line2 did not edit. Count attribution: Nrd* 20/375 -> 23/382 is entirely the
  line2 D2 tests; full suite 232/3926 -> 236/3952 is +3 line2 cases/+7
  assertions plus +1 line1 Test_PibdServing case/+19 assertions; line3's
  archive subset 7/168 -> 8/187 is entirely line1's update.
  **Run-only rebind on the current integrated binary (Lead-approved, no rebuild):**
  after line1 rebuilt the shared Tests binary, the same isolated filters were run
  against `Tests.exe` SHA-256
  `8F10BFD67047DD25FAB3C201C29FA125096EF4DD7E8EFFFE33B61627CF1D73D6`
  (9,172,480 B, PE ts `0x6AE8FC0B`, pre/post SHA stable; preserved as
  `evidence/bin/Tests-8F10BFD67047.exe`): `nrd-bind-8F10BFD6-20261103-041147`
  — 23 / 382 PASS, `kernidx-bind-8F10BFD6-20261103-041147` — 3 / 48 PASS, each
  with its own kept `Tests.exe` copy; differential 14/14
  (`cpp-nrd-differential-report-8F10BFD6.json`, same sha `EA44A390…5FCB`). The
  full suite on this SHA was reported green by Lead/line1 (236 / 3952) and was
  not re-run by line2; line3-fuzz independently verified everything (Nrd* 23/382,
  KernelIndex* 3/48, archive 8/187, full 236/3952 PASS) and found that
  DDC21F45 → 8F10BFD6 changed **zero** compiled sources (source-identical
  rebuild; only docs/tooling changed), which explains the identical counts and
  binary size. **8F10BFD6…D73D6 is therefore the current authoritative normal SHA
  for NRD/archive evidence; DDC21F45 remains preserved historical evidence.**

Still unwired (proposal, needs separate approval):

1. `src/TxPool/TransactionPoolImpl.cpp` `AddTransaction`: reject NRD kernels on
   mainnet (`EAddTransactionStatus::TX_INVALID`, Rust `PoolError::NRDKernelNotEnabled`).
2. `src/PMMR/TxHashSetImpl.cpp`: when the policy is off, skip NRD indexing and
   the NRD pre-check exactly like Rust `global::is_nrd_enabled()` early-returns
   (`apply_kernel_rules` / `verify_kernel_pos_index`). Not needed for block
   rejection after the block gate above; a non-standard mainnet state containing
   NRD kernels would still be treated fail-closed (stricter than Rust) by the
   rebuild path.

No mainnet network or user data was used; the tests use pure functions and
synthetic blocks only.

## 8. Open auditor findings (2026-11-03) — line 2

These are accepted as open; none is claimed as PASS in this document.

### 8.1 Possible false peer ban when local NRD history is incomplete (OPEN)

Confirmed code path: `TxHashSet::ApplyBlock` returns `false` with category
`missing_local_history` when `m_recentNrdIndexComplete == false`
(`src/PMMR/TxHashSetImpl.cpp`, the `MISSING_LOCAL_HISTORY` branch), and
`BlockProcessor::ValidateAndAddBlock` (`src/BlockChain/Processors/BlockProcessor.cpp:198`)
still converts any `ApplyBlock == false` into
`BAD_DATA_EXCEPTION_F(EBanReason::BadBlock, ...)`. `BlockPipe::ProcessNewBlock`
bans on both `EBlockChainStatus::INVALID` and any exception, so local
incompleteness can ban a peer that sent a well-formed next block.

The task-5 classification change fixed only the txhashset-snapshot paths
(`TxHashSetProcessor` / `BlockChainImpl`); it does not cover this block-
application path.

**Fix implemented in working tree (task-6, Lead-approved, runtime pending):**
`BlockProcessor::ApplyBlockNrdAware()` retries once after
`RebuildRecentKernelIndex(pBlockDB, previousHeader)` when the only failure is
`MISSING_LOCAL_HISTORY`; if the index still cannot be rebuilt locally it returns
`LocalNrdHistoryUnavailable` (never `InvalidBlock`). `ProcessBlockInternal`
catches the internal signal and returns `EBlockChainStatus::UNKNOWN_ERROR`
instead of throwing. The batched `Writer` destructor rolls back the uncommitted
batch (`ChainState::Rollback -> TxHashSet::Rollback` restores the NRD index
snapshot), so no partially applied state is published and the reorg path keeps
its rollback semantics.
Non-ban proof without editing P2P: `BlockPipe::ProcessNewBlock` bans only on
`EBlockChainStatus::INVALID` or an escaped exception; this path produces
`UNKNOWN_ERROR` and never lets its signal exception leave `ProcessBlockInternal`.
Regression tests: `tests/src/PMMR/Test_NrdFalseBan.cpp` (rebuild-once recovery;
unrebuildable local history ⇒ `LocalNrdHistoryUnavailable`, not `InvalidBlock`;
genuine duplicate with complete history ⇒ `InvalidBlock`; plain block applies
with incomplete index). **Executed PASS** on Tests SHA-256
`930148B6146A5E89B0B7FAC4A32B57212434B511E14F778A00AD6D6D24C1BB6E`
(line1 build-06, no line2 rebuild): `NrdFalseBan*` — 5 test cases / 17
assertions PASS; full `Nrd*` — 28 cases / 402 assertions PASS
(`falseban-930148B6-20261103-051527`, `nrd-930148B6-20261103-051449`).
Live end-to-end peer-ban reproduction remains **NOT VERIFIED** (the tests prove
the status/classification mapping; the static BlockPipe trace proves the ban
condition is not reachable on this path).

### 8.2 Paired Rust/C++ differential (EXECUTED PASS on 930148B6)

The historical C++ cases pass, but Rust's actual result for the same input was
not computed: `expected_rust_category` is metadata copied from the cited Rust
test sources. Therefore "14/14" is a C++-actual vs expected-metadata match, not
automation-verified parity (formal correction in section 2).

**Fix implemented in working tree (task-6):**
* `tools/audit/nrd/rust-oracle/` — standalone Rust crate (own `[workspace]`,
  read-only absolute path deps into `Temp\grin55-minpeers`; the reference tree is
  never edited) that reads the same `nrd_cases.json`, builds real blocks and
  transactions (including repeated NRD kernels with real signatures) and applies
  them through real `grin_chain` 5.5.2, emitting `status=computed` +
  `actual_rust_category` or an explicit `not_computed` reason.
* `tests/src/PMMR/Test_NrdDifferential.cpp` — when
  `GRINPP_NRD_RUST_ORACLE` points at the oracle output, the runner compares
  actual C++ vs actual Rust per case, records `rust_result_source =
  computed_by_rust_oracle`, requires zero mismatches, and fails if the oracle
  computed nothing or references unexecuted cases. Without the oracle it keeps
  the honest `expectation_metadata_not_computed` marker.
* Expected `not_computed` classes: `no_index` (Rust has no local-missing-index
  state) and `rebuild_complete=false` (invalid NRD history cannot be created by
  real chain apply; Rust `verify_kernel_pos_index` would need a crafted-MMR
  harness).
* **Paired N/N may only be claimed for the computed subset after an actual
  oracle run.**

**Oracle executed (build permitted by Lead, separate cargo target; no project
build):**
* binary `nrd-oracle.exe` SHA-256
  `615A3B63CECBDFF601C2858172882EC5C18DF9609C0641D7294F585F61512CC8`
  (2,024,960 B), preserved as `evidence/bin/nrd-oracle-615A3B63CECB.exe`; build
  log `evidence/rust-oracle-build.log`; `CARGO_TARGET_DIR=Temp\grin55-target`,
  reference tree read-only.
* output `evidence/rust-oracle-8F10BFD6.json` (SHA-256
  `3FCB01244CC2C14EC72FCED85581EC74318D2ED9C515EFE9B68DA82422B73DB0`):
  **11 computed / 3 not_computed**. Actual Rust categories:
  `accepted` — first-occurrence, exact-boundary, relative-height-1,
  three-occurrences, plain-excess-not-indexed, multi-kernel-mixed,
  incremental-exact-boundary;
  `accepted_non_nrd` — plain-repeat;
  `invalid_nrd_relative_height` — one-before-boundary and
  incremental-one-before-boundary (real `NRD Relative Height` from
  `chain.process_block`);
  `invalid_nrd_duplicate_in_block` — duplicate-nrd-in-one-block (real Rust
  rejection whose debug contains the inner `InvalidNRDRelativeHeight`; the
  duplicate sub-category is the input shape, the verdict is Rust's).
  `not_computed`: missing-local-history (Rust has no local-missing-index state),
  rebuild-rejects-historical-violation and
  rebuild-rejects-duplicate-history-block (invalid history cannot be constructed
  through real chain apply; crafted-MMR rebuild harness needed).
* Preliminary sanity (NOT paired): actual Rust vs the C++ *expected metadata* —
  **11/11 computed agree, 0 mismatches**, file
  `evidence/rust-oracle-vs-cpp-expectation-8F10BFD6.json`. This is not the
  paired C++-vs-Rust run and MUST NOT be reported as paired.
* Rebuild-mode valid-history fixtures are evaluated through the real consensus
  path (`chain.process_block` on the same real kernel history); the literal
  Rust `verify_kernel_pos_index` rebuild code path is not exercised for those —
  recorded per case in the oracle `detail`.
* Status: oracle PASS (11 computed, all real `grin_chain` 5.5.2); **paired run
  EXECUTED PASS** on Tests SHA-256
  `930148B6146A5E89B0B7FAC4A32B57212434B511E14F778A00AD6D6D24C1BB6E`
  (line1 build-06, no line2 rebuild):
  `nrd-paired-930148B6-20261103-051449` — **11 computed, 3 not_computed,
  11 paired cases, 0 paired mismatches**; report
  `evidence/paired-differential-report-nrd-paired-930148B6-20261103-051449.json`.
  Only the 11 computed cases are a paired claim; the 3 not_computed cases are
  listed with reasons and are **not** parity evidence. C++ actual categories
  matched actual Rust exactly for every paired case (`accepted` ×7,
  `accepted_non_nrd` ×1, `invalid_nrd_relative_height` ×2,
  `invalid_nrd_duplicate_in_block` ×1).
* **Paired-report pipeline:** `tools/audit/nrd/paired-differential.ps1` runs an
  existing Tests binary (never builds) with both `GRINPP_NRD_REPORT` and
  `GRINPP_NRD_RUST_ORACLE` and writes
  `evidence/paired-differential-report-<RunId>.json` with per-case
  `actual_cpp_category` / `actual_rust_category`, `paired`, `paired_match`,
  `not_computed` reasons and the exact Tests SHA-256. Used for the run above.

### 8.3 False PASS on missing fixture (FIXED; executed PASS)

`NrdDifferentialCasesMatchRustOracle` used to `WARN(...); return;` when the JSON
case file was absent. It now `REQUIRE`s the case-file path/parse and, when
`GRINPP_NRD_REPORT` is set, checks that the report was written, is non-empty,
parses back, has one entry per case and carries a valid `rust_result_source`.
**Executed PASS** on Tests SHA-256 `930148B6…BB6E` (case file present; paired
report written and read back).

### 8.4 Real PIBD state / rebuild interruption (OPEN)

Real PIBD-produced state → NRD → reorg → restart, and rebuild interrupted before
publication, remain **NOT VERIFIED**. Synthetic kernel-MMR + headers tests cover
the in-memory logic but not a real PIBD-produced state or crash/process-kill
injection.

### 8.5 Mainnet NRD gate — partial (ACCEPTED, not a mainnet authorization)

Only the block consensus path is wired and tested
(`BlockValidator::VerifyNrdNetworkGate`, mainnet rejects NRD). The txpool path
(`TransactionPoolImpl`) and the NRD-index policy early-return in `TxHashSetImpl`
still need Rust-semantics agreement before any wiring; until then mainnet
behaviour must not be claimed as Rust-parity and no mainnet network/data is used.
Status: **partial — block path PASS (testnet testing only), txpool/index policy
OPEN**.

## 9. Follow-up auditor findings (round 2)

### 9.1 NRD backup during a failed reorg (VERIFIED on C8C386BE)

Root cause found: `TxHashSet::RebuildRecentKernelIndex` always published its
result via `PublishRecentNrdIndexAsBackup()`. The task-6 rebuild-once path calls
it from `BlockProcessor::ApplyBlockNrdAware`, which can run inside an
uncommitted `ChainState` batch while processing an alternative reorg branch. If
that reorg then fails, `TxHashSet::Rollback` would restore the abandoned
branch's index instead of branch A's last-committed index.

Fix (working tree): the rebuild is split into
`RebuildRecentKernelIndex` (publishes the baseline, used outside batches) and
`RebuildRecentKernelIndexPreservingBaseline` (used by `ApplyBlockNrdAware`);
`TxHashSet::Commit` remains the only place that publishes the current index as
the rollback baseline. Regression test `Test_NrdBatchSafety.cpp`:
branch A commit → rewind → rebuild on branch B → simulated failed reorg
Rollback → compare committed tip header, kernel MMR, index entries and decisions
for the same excesses. **Executed PASS** on build-07 Tests SHA
`C8C386BE…B7FE` as part of `Nrd*` (30 cases / 428 assertions PASS); the test is
`NrdReorgBackup restores last committed index after failed alternative branch`.

### 9.2 Exception during rebuild must not ban (VERIFIED on C8C386BE)

Root cause found: a local header/kernel read exception thrown by
`RebuildRecentKernelIndex` could escape `ApplyBlockNrdAware` →
`ProcessBlockInternal` → `BlockChain::AddBlock` into `BlockPipe`, which bans on
escaped exceptions.

Fix (working tree): the rebuild contains `std::exception`/`...`, leaves the
index incomplete (fail-closed) and records `DidLastNrdRebuildThrow()`;
`ApplyBlockNrdAware` additionally catches defensively and returns
`LocalNrdHistoryUnavailable` (→ `UNKNOWN_ERROR`, no ban);
`TxHashSetProcessor::VerifyNrdIndexBeforeCommit` classifies a throw as
`LocalUnverifiable` before any `InvalidState` classification. Fault-injection
test in `Test_NrdBatchSafety.cpp` (header-read throw) asserts no exception
escapes, no partial apply, incomplete index and recovery; genuine invalid blocks
still classify `InvalidBlock` (`Test_NrdFalseBan.cpp`). **Executed PASS** on
build-07 Tests SHA `C8C386BE…B7FE` (`NrdRebuildException is local-unavailable and
never escapes`, included in the 30-case `Nrd*` PASS). Live end-to-end peer-ban
reproduction remains NOT VERIFIED.

### 9.3 Real Rust rebuild path in the differential (EXECUTED)

Oracle v3 (`evidence/rust-oracle-930148B6-v3.json`, sha
`6CF2582B…D195`; binary `evidence/bin/nrd-oracle-00D0AA518C36.exe`, sha
`00D0AA51…AD85`) now executes the real Rust
`TxHashSet::verify_kernel_pos_index` path (as used by `txhashset_write` /
desegmenter) for all **8 computed rebuild-mode cases**, then the target through
`process_block`. Structured fields `evaluation_path` / `rust_rebuild_executed`
record this per case:
- `rust_verify_kernel_pos_index_rebuild_then_chain_apply` (8): first-occurrence,
  exact-boundary, one-before-boundary, relative-height-1, three-occurrences,
  duplicate-in-one-block, plain-excess-not-indexed, multi-kernel-mixed;
- `chain_apply` only (3): plain-repeat, incremental-one-before-boundary,
  incremental-exact-boundary.
`not_computed` (explicit): missing-local-history; rebuild-rejects-historical-
violation; rebuild-rejects-duplicate-history-block (invalid history cannot be
built through real chain apply; crafted-MMR harness needed).
Paired actual-Rust vs actual-C++: on preserved Tests SHA `930148B6…BB6E`
**11 computed / 3 not_computed / 11 paired / 0 mismatches** (report
`evidence/paired-differential-report-nrd-paired-v3-930148B6-20261103-052546.json`,
sha `D1E13441…0E95`), and again on build-07 final candidate
`C8C386BE…B7FE` — **11 computed / 3 not_computed / 11 paired / 0 mismatches**
(report
`evidence/paired-differential-report-nrd-paired-c8c386-20261103-053139.json`,
sha `57978B2E…5AF6`; run `nrd-paired-c8c386-20261103-053139`).

### 9.4 Fixture negative/positive verification (EXECUTED on C8C386BE)

On build-07 Tests SHA `C8C386BE…B7FE` (direct isolated invocation; the runner
CWD would discover the repo fixture through the fallback walk, so fixture
controls are run with isolated HOME/TMP and explicit env):
- missing-fixture negative (`GRINPP_NRD_CASES` = nonexistent): **expected FAIL**
  at `Test_NrdDifferential.cpp:201 REQUIRE(fs::exists(casesPath))`, exit 1, no
  report — a missing required fixture can no longer become a silent PASS;
- positive with case file + report path: **PASS**, report written
  (276 assertions);
- report-write-error (`GRINPP_NRD_REPORT` into a nonexistent directory):
  **expected FAIL** at `Test_NrdDifferential.cpp:318 REQUIRE(out.good())`,
  exit 1, no report produced.
The same positive/report-write checks had already been executed on the preserved
`930148B6…BB6E` binary with identical outcomes.
