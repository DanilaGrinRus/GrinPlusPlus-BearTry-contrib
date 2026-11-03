# Build-11 readiness — audit completed (normal bodyless-only + ASan subset PASS)

Status: **BUILD-11 RETRY AUDITED: normal FAIL bodyless-only; ASan subset PASS.**
Build-10 remains final only for its own snapshot. Build-11 is **not** a full
G-09/Mainnet PASS; live task-10/11 recovery evidence remains open.

Build-11 identity: Tests
`02DC3D9C00BB1E1B7256771A158F04F1DB3B739839A7AFF2391AEAA5DAAFC121`
(9,438,720 B, PE `0x6AE94C3F`), GrinNode
`565BC2DBF30C638F84B458012F1F07D091B8EF6DB9C933E821A58EF3CF6021A4`
(7,843,328 B, PE `0x6AE94B43`); freeze snapshot
`build-11-freeze-snapshot-final.json` SHA `E5F7519C…`; retry build log
`B274B34A…`; failed first attempt `ACED40F9…` excluded. Full results:
`build11-audit-complete-20261103.md`.

## 1. Build-10 scope (frozen; do not extend to HEAD)

- Tests `C17C3BC04BE04C501DB425E3617A71E61985E9257EF41D879E130E28F8B621A4`
  (9,410,560 B, PE `0x6AE93F14`)
- GrinNode `60D7F7A6B3FFCC702B260E80036B7143A06DB9B036AD3DF3754B3C7B9438CB23`
  (7,809,024 B, PE `0x6AE93E33`)
- Source freeze `build-10-freeze-snapshot-final.json`, SHA
  `0E51D63287AF8F96B3094A9F24B120C2F663736CAA1E9B4DF5E8C2B6EB6CCA5E`
  (1002 entries, aggregate `F1E47615…`)
- Build-10 normal: clean full 254/253/1 bodyless-only; all pre-registered
  filters PASS; ASan refresh subset PASS (0 reports). **Scope ends at this
  snapshot.**

## 2. Current source drift (unbuilt)

- Current HEAD: `6dbf8a7da067f35ba585371d82123ad21edf3a9a`
  (`evidence: archive recovery discrepancy and atomic-publication audit`).
- Current working tree still has archive/recovery and task-11 diagnostic edits.
  The active uncommitted source set is **8 files**:
  - `include/BlockChain/BlockChain.h` `DE7231DF…`
  - `src/BlockChain/BlockChainImpl.h` `C5667C94…`
  - `src/BlockChain/BlockChainImpl.cpp` `F0E51263…`
  - `src/P2P/Sync/StateSyncer.h` `134CD9D6…`
  - `src/P2P/Sync/StateSyncer.cpp` `B1CBE207…`
  - `src/Database/BlockDBImpl.cpp` `F88A107D…`
  - `src/Database/RocksDB/RocksDB.h` `42B5BF3C…`
  - `tests/src/P2P/Test_BlockPipeBan.cpp` `D11872BF…`
- These are **not built**; no build-11 binaries or source freeze exist yet.
  No final claim may cite HEAD `6dbf8a7` or current working tree.

### 2.1 Line1 frozen hashes — verified

- `src/P2P/ArchivePublication.h` `D0D1E611…` (6683 B)
- `src/P2P/PIBD/PibdArchiveServer.h` `7F6308F9…` (27622 B)
- `src/P2P/Pipeline/TxHashSetPipe.cpp` `C508EA48…` (8902 B)
- `src/P2P/Sync/StateSyncer.cpp` `B1CBE207…` (20573 B; FullBlock.h include
  present at line 18)
- `tests/src/PMMR/Test_PibdServing.cpp` `E3A5543D…` (34658 B)
- `include/BlockChain/BlockChain.h` `DE7231DF…` (6838 B)
- `src/BlockChain/BlockChainImpl.cpp` `F0E51263…` (31326 B)

### 2.2 Freeze reconciliation / blockers before build

- Line1 and line2 now agree on the **8-file source set**:
  line1's 5 (`BlockChain.h`, `BlockChainImpl.h`, `BlockChainImpl.cpp`,
  `StateSyncer.h`, `StateSyncer.cpp`) plus line2's 3
  (`BlockDBImpl.cpp`, `RocksDB.h`, `Test_BlockPipeBan.cpp`).
- `Test_BlockPipeBan.cpp` is now `FFBF3DF2…`, mtime 09:36:24; it contains
  exactly **one** inert `AreSpentPositionsComplete` override
  (`mappedCount=0; return false;`). Line1's `7DA77CDB…` and the earlier
  `D11872BF…` are stale pre-resolution values; compile blocker is
  **statically resolved** in this worktree.
- `GRINPP_SPENT_DIAG` exists only in `BlockDBImpl.cpp F88A107D…` and
  `RocksDB.h 42B5BF3C…`; `StateSyncer.cpp B1CBE207…` has no SPENT_DIAG.
  FullBlock.h include is present. No static compile blocker is known from the
  reconciled set (no build was run by line3).
- Build-10 remains final only for its snapshot; no audited build-11 candidate
  exists.

### 2.3 Build-11 process observed — not auditable yet

At 2026-11-03T09:36:34+07:00 a build-11 started without a Lead order:

- `C:\DHW\.tmp\build-11\build-console.log`; pwsh PID 22136
  (`tools/build/build.ps1 -Target 'GrinNode;Tests'`), MSBuild PID 6372;
  cl PID 31648 was observed compiling earlier but is no longer present.
- The console stopped updating at 09:37:52; only the log exists (no binaries)
  and the build appears stalled. Line3 did not start, stop, or kill it.
- `Test_BlockPipeBan.cpp` FFBF3DF2 has mtime **09:36:24**, i.e. before the
  build start; it may be included, but no build manifest/source freeze was
  produced. Because no Lead order and no completed artifact, any binary from
  this run is **not auditable/bindable**. Wait for Lead. 

## 3. Required order before audit

1. Wait for Lead order/build-11 candidate.
2. Verify build-11 binary identity: SHA-256, bytes, PE timestamp, preserved
   path, BUILD OK; capture build-11 source freeze with HEAD/status/hashes
   including archive/recovery and task-11 diagnostic changes.
3. Confirm the frozen source contains the atomic-publication changes and the
   spent-position diagnostic path (exact Rewind hash/DB view logging or
   equivalent evidence hook); otherwise build-11 ASan/final audit is blocked.
4. No shared `tools/build/build.ps1` concurrent use; no commit/reset/clean.

## 4. Build-11 normal audit pre-registration

- Full suite (serialized exclusive 3421 lock, quoted filters, `-KeepBinary`);
  bodyless-only remains the known test-only exception until fixed.
- Filters: `PibdArchiveServer::*`, `Nrd*`, `KernelIndex*`,
  `GetKernelHandler*`, `KernelIndex schema v2*`,
  `TxHashSet duplicate-excess*`, `P2P::AdvertisedTip*`.
- Record exact cases/assertions, manifests, source SHA in every run; invalid
  port-collision/partial runs must be excluded, not averaged.

## 5. Task-10/11 acceptance checks

- **Task-10 atomic publication:** staged copy → fsync/complete → atomic
  rename/publish; reject existing partial file; readiness must validate zip,
  roots/hash and current horizon, not only `fs::exists`; crash/interrupted
  publication leaves no servable partial; failed recovery does not publish,
  alter tip/root, or loop forever.
- **Task-11 spent-position:** fresh copy of `g09-live-02` only; original never
  modified. Capture the exact Rewind key/hash, DB path + CURRENT/MANIFEST, and
  SPENT_OUTPUTS key/value/map size at the failure point. Existing contradiction:
  preserved DB canonical key `30c524b8…bfcb` has count=1; all input-bearing
  blocks match spent counts; logged 0 must be explained by either a different
  hash/view or a failure-time DB view.
- Fail-closed preflight: every block in `(target_height, flushed_tip]` and every
  input commitment must have undo data before snapshot/publish; absence is a
  state-availability error, not a silent rewind.
- Validate H=3313440 recovery from a copied consistent DB; boundaries above/
  below 3317930; H=3315600 pre-body-sync archive is not a counterexample.
- Existing 7B PASS remains valid; do not relabel or modify the original
  `g09-live-02` fixture.

## 6. ASan/CI after normal audit

- ONE ASan refresh on frozen build-11 sources in a copied tree; source
  manifest + TestsMini; Nrd*/archive/Segment/P2P+advert/teardown; mark
  normal-only tests and rocksdb/sysroot boundaries; no fake PASS if stale.
- Update `EVIDENCE_LINE3`, `CI_REPRO`, `SANITIZERS`; no commit until Lead says.

## 7. Current blockers

- Build-11 binaries not provided/built.
- Archive source unbuilt; exact failing Rewind key/view not instrumented yet.
- Task-10 atomic publication is an open static gap in audited build-10 source.
- Task-11 runtime contradiction unresolved; line1/line2 copy repro still needed.

## 8. Evidence pointers

- [`normal-binary-build10-audit-20261103.md`](<evidence/line3/normal-binary-build10-audit-20261103.md>)
- [`task10-11-archive-recovery-independent-audit-20261103.md`](<evidence/line3/task10-11-archive-recovery-independent-audit-20261103.md>)
- [`task10-11-archive-recovery-manifest-20261103.json`](<evidence/line3/task10-11-archive-recovery-manifest-20261103.json>)
- [`task11-spent-positions-fresh-copy-audit-20261103.json`](<evidence/line3/task11-spent-positions-fresh-copy-audit-20261103.json>)
- [`build-10-freeze-snapshot-final.json`](<evidence/line3/build-10-freeze-snapshot-final.json>)
