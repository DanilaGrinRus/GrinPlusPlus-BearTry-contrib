# Archive recovery and spent-position failure (2026-11-03)

Status: **FAIL for the observed old-archive recovery attempt**; root cause
**NOT VERIFIED**. Existing archive serving at H=3315600 and the 7B live PASS remain
valid for their scenario. This is a blocker for automatic horizon regeneration
and Mainnet acceptance, not for the already verified testnet serving path.

## Observed failure

- `pibd_state.complete` marker: hash `84d7ac67…`, height **3313440**.
- Existing archive: H=**3315600**, SHA-256 `13F9781E…C530`.
- `TryRecoverArchive()` copies the confirmed state at H=3318655 and rewinds the
  copy toward H=3313440.
- Rewind fails at block **3317930**, commitment
  `0989ac92c748aaf654f06fb9023746d3d34358b051dd2ac952bec39fecc6288967`:
  **1 input, 0 spent positions**, exception `Spent output not found`.
- Reproduced in three independent C++ processes (build-09 restart at 08:45:10,
  build-10 restart at 08:59:42, and during the original live run).
- Temporary snapshot files removed; warning only; live node continued and
  restart reached the same `no_sync` tip 3318655 / `2945348e…`.

## Confirmed code facts

- `BlockDBImpl::GetSpentPositions` throws when the `SPENT_OUTPUTS` record is absent.
  The failing log reports a **0-entry map**, but a direct read-only audit of the
  preserved canonical DB (copy) finds for block 3317930 hash
  `30c524b8424f91e1b2b154112f19d77d4c0db6887d6a1cdc96ef2055e022bfcb`:
  - BLOCK 2852 B, 1 input, commitment `0989ac92…8967`;
  - SPENT_OUTPUTS record **exists, 52 B**, count **1**, location leaf 6856589 /
    output height 3317928.
  - Adjacent blocks 3317929/3317931 are `000000` (count 0) and have 0 inputs.
  - All 5215 retained BLOCK records 3313441–3318655 have
    `spentCount == inputCount`, 0 mismatches/missing.
- Therefore **"record exists with an empty vector" is NOT proven for the
  canonical fixture**. The logged 0-map must come from a different hash/view,
  different DB state, or a runtime lookup discrepancy. Root cause is
  **NOT VERIFIED**.
- Descending from tip 3318655, 0/725 blocks above 3317930 have inputs;
  3317930 is the first input-bearing block. Both H=3315600 and H=3313440 cross it.
  The existing H=3315600 archive was created at 06:43 from a pre-body-sync state,
  not by a fresh rewind from the final tip, so it does not prove that boundary safe.
- Existing H=3315600 serving has passed size/root/MMR and Rust consumption;
  this does not make old-archive regeneration valid.

## Atomic publication risk (static, confirmed)

- `StateSyncer::ArchivePibdState` copies directly to the final archive path with
  `fs::copy_file`; no fsync/close barrier, no atomic rename, and the error path
  does not remove a partial final file.
- Readiness uses `fs::exists` without validating zip/roots.
- A crash/interruption can leave a visible partial file; a retry may be skipped
  or legacy `TxHashSetPipe` may select it.
- Required: staged copy → complete write/fsync → validate zip/roots/size →
  atomic rename/publish; reject existing partial; readers only see validated
  published archives; error removes temp and preserves the previous archive.

## Required acceptance

1. Reproduce on an isolated copy with exact-hash/DB-view diagnostics; explain
   why the runtime log reports 0 while the canonical copy has a 1-entry record,
   or prove a concrete DB/view invariant violation.
2. Failed recovery never changes confirmed tip/root/live state, and never
   publishes a temporary/partial archive.
3. Restart does not loop forever on an unusable completion marker; the target is
   bounded/fail-closed, and an unavailable H is not advertised/served.
4. Snapshot creation at current tip, above block 3317930, and below it (including
   H=3315600) is characterized; a usable archive is validated by size/roots and
   consumed by Rust.
5. Interrupted publication and legacy reader are tested on a copied profile.
6. Spent-position root cause is fixed or the recovery path fails closed with no
   silent state mutation; tests cover duplicate/empty undo data and reorg.

## Evidence

- `evidence/line3/task10-11-archive-recovery-independent-audit-20261103.md`
- `evidence/line3/task10-11-archive-recovery-manifest-20261103.json`
- `evidence/line3/task10-11-archive-recovery-20261103/` (build-10 logs/status)
- Original fixture: archive `13F9781E…`, marker `B671C82A…` = `84d7ac67…` /
  3313440, configs `355B15F7…` / `AEE5E753…`; original `g09-live-02` untouched.
- Source audited at HEAD `fa91e92…`; `StateSyncer.cpp` `31AF0EE8…`,
  `TxHashSetImpl.cpp` `617AD908…`, `BlockDBImpl.cpp` `8B4460B7…`.
