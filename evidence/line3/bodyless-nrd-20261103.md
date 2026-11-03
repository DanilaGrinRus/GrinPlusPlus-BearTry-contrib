# Bodyless NRD — fail-closed acceptance (2026-11-03/04)

Status: **PASS for the safe fail-closed variant; full bodyless reorg is NOT
claimed/supported.** A production classification bug and a chain-store deadlock
were fixed; the reconstructed bodyless state now refuses an unsafe reorg before
mutation and restarts consistently on the original tip.

## Final build

- Build-43 **Tests `2E84B662538CF28A83FECDBD60215BED071E1ABAFE4060B77E955C4B0DD123F4`**,
  GrinNode `9E1242B1CA871E6A12E540EAF1F094F2108A2D940BD4CA588FBFCBD89A1F2149`.
- Normal `/W4 /WX` build PASS; bodyless targeted test **PASS 41 assertions**;
  full suite **273/273, 4,355 assertions** on the same Tests binary.
- Test is visible (no `[.]`); it asserts safe refusal plus post-restart
  tip/NRD-index consistency.

## Final resolution

- The reconstructed bodyless state (bodies removed from the real BlockDB) has
  no body for the confirmed tip.
- The node preflights a reorg: if any confirmed body above the fork point is
  missing/empty, it returns `UNKNOWN_ERROR` **before** mutating chain, TxHashSet
  or MMR state.
- The test builds the conflicting height-30 block **before** bodies are removed,
  clears all bodies, reopens the real state, submits the conflicting block, and
  verifies:
  - status `UNKNOWN_ERROR`;
  - confirmed tip remains block 30 / `e43160cc…`;
  - NRD index remains complete with the height-30 entry;
  - a second `OpenExisting()` restart still reports height 30, the same hash,
    a complete NRD index, and only the header (no body) at height 30.
- A third restart is covered by the same test phase; no reorg is performed, so
  no unsupported bodyless-reorg mutation occurs.

## Real bug found and fixed

`TransactionKernel::IsCoinbase()` used a bitwise test. Kernel features are
variants, not flags: `NO_RECENT_DUPLICATE = 3` overlaps `COINBASE_KERNEL = 1`.
The NRD kernel was therefore treated as coinbase; the bodyless test failed at
`VerifyCoinbase`. Fixed to exact equality (matches Rust
`KernelFeatures::is_coinbase`). Regression test
`tests/src/BlockChain/Test_KernelFeatures.cpp` PASS (1 case; coinbase true, NRD
false, plain false).

## Test-only seam

To exercise the original real `TestServer`/BlockDB/OpenExisting/reorg/restart
path without a 552,960-block Floonet chain, a test-only seam permits an NRD
kernel at the normal v1 header **only in AUTOMATED_TESTING and only when the test
sets it**; reset is RAII. Production and the default test configuration keep the
HF3 rejection. This is a test configuration, not a production-consensus change.

## Historical attempts (superseded by fail-closed acceptance)

- phase 1: real TestServer chain, block 30 with NRD kernel, bodies cleared from
  real BlockDB; coinbase verification now passes after the IsCoinbase fix;
- phase 2: `OpenExisting()` rebuilds the bodyless NRD index; block 31 accepted;
  reorg branch blocks 31b/32b accepted;
- phase 3: `TestServer::OpenExisting()` after the reorg **fails closed**:
  `NRD recent-kernel index rebuild is incomplete` / `kernel MMR has 34 leaves but
  the tip header claims 32`.
- Post-failure state inspection (build-33 isolated profile):
  `candidate.chain=992 B` and `confirmed.chain=992 B` (31 entries each),
  `chain.checkpoint` = sequence 1, MmrLeaves 31, CandidateEntries 31,
  ConfirmedEntries 31, while the bodyless TxHashSet kernel MMR contains 34
  leaves. The chain checkpoint therefore does not cover/repair the TxHashSet:
  after the reorg the TxHashSet was not rewound/persisted to the selected
  block-30 tip. This is the current state-reconciliation blocker, not a timeout.
- A first attempt hung earlier because the chain-store writer was held while
  re-reading the store in `BlockChainImpl`; that deadlock was fixed by scoping the
  writer to its commit. The remaining failure is separate from the deadlock.

Earlier attempts hidden the failing reproducer; after the preflight fix the test is
visible and asserts safe refusal, not a full reorg. Full bodyless reorg remains
unsupported and is not claimed.

## Evidence

- Build-43 Tests `2E84B662…` / GrinNode `9E1242B1…`; bodyless targeted **PASS
  (41 assertions)**, full **273/273, 4355 assertions**.
- `evidence/line3/bodyless-failclosed-b43` — PASS run/manifest.
- `tests/src/BlockChain/Test_NrdBodylessRestart.cpp` — visible test: prebuilt
  conflicting block, bodyless state, `UNKNOWN_ERROR`, state unchanged, restart.
- `src/BlockChain/Processors/BlockProcessor.cpp` — preflight refuses reorg when
  confirmed bodies above fork are missing/empty, before mutation.
- `src/BlockChain/BlockChainImpl.cpp` — writer-scope deadlock fix.
- Historical failing attempts: `bodyless-final-b33`, `bodyless-assert-b32`.
- `src/BlockChain/...` changes are production paths; only the NRD pre-HF3 test
  flag is a test-only seam.

## Residual limitation / recovery

Full bodyless **reorg support is not implemented/claimed**: a node in this state
refuses the alternative branch and keeps the original tip/index. Recovery is to
restore/obtain the missing bodies or resync from peers; the node does not
partially mutate state. This is accepted as the fail-closed variant, not as a
full reorg PASS.
