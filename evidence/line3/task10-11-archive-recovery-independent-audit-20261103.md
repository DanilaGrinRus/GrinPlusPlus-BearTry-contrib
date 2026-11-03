# Task-10/11 archive recovery — independent read-only audit (2026-11-03)

Status: **FAIL confirmed for the observed recovery attempt**; automatic
next-horizon regeneration **NOT VERIFIED**; atomic publication **OPEN**.
Existing 7B PASS and H=3315600 archive serving remain valid. This audit is
evidence/hash + static review only; no build, no commit, and the original
`C:\DHW\.tmp\g09-live-02` fixture was not modified.

## 1. Scope and exact failure

Three independent C++ processes produced the same recovery failure after a
persisted-state restart / body-sync path:

```
TryRecoverArchive: Recovering the missing TxHashSet archive at height 3313440
TxHashSet snapshot: flushed height 3318655, requested height 3313440
Rewind: Spent output not found for commitment
Commitment{0989ac92c748aaf654f06fb9023746d3d34358b051dd2ac952bec39fecc6288967}
in block 3317930 (1 input(s), 0 spent position(s), target height 3313440)
ArchivePibdState: Failed to archive the activated TxHashSet: Rewind: Spent output not found...
```

- first occurrence: `cpp-7B-final-Node.log` 08:26:28.315+07
  (`77518AEA…`);
- second (build-09 persisted restart): `restart/7B-restart-cpp-new.log`
  08:45:10.317+07 (`0AC62411…`);
- third (build-10 persisted restart):
  `evidence/line3/task10-11-archive-recovery-20261103/build10-cpp-new.log`
  08:59:42.955+07 (`CFB19A60…`); restart `no_sync` at 3318655 was unaffected.
- all three report the same block 3317930, commitment
  `0989ac92…28967`, 1 input and 0 spent positions, target H=3313440.
- the failed recovery removed `tmp/Snapshots/TxHashSet.84d7ac676356.zip` and
  its staging dir; no H=3313440 final archive was published.
- Rust accepted block `30c524b8424f` at 3317930 (v3; in/out/kern 1/3/2) and
  C++ failed only when rewinding the copied state toward H=3313440.

The marker `pibd_state.complete` contains:

```
84d7ac6763566ac67eaa8393688320c79a644321124d7ba94a1300f56af9ccea
3313440
```

The only final archive in the original fixture is still
`TxHashSet.3315600.3615bb625917.zip` (SHA `13F9781E…`). Existing H=3315600
serving and the 7B/body-sync/no_sync PASS are therefore unaffected.

## 2. Independent code audit

### 2.1 Empty record, not a missing record

`src/Database/BlockDBImpl.cpp:288-300`:

- `GetSpentPositions(blockHash)` throws `DATABASE_EXCEPTION` when the
  `SPENT_OUTPUTS` record is **absent**;
- if the record exists, it returns `SpentOutputs::BuildMap()`.

The failure log says **0 spent position(s)**, so the `SPENT_OUTPUTS` record for
block `30c524b8424f` **exists but contains an empty vector**. This is the
task-11 invariant violation, not a missing-record condition.

`src/PMMR/TxHashSetImpl.cpp:665-717` obtains the map by the block being rewound,
iterates its inputs, and throws `Spent output not found` when an input commitment
is absent from the map. Thus the empty map makes every input lookup fail.

The normal write path is `src/PMMR/TxHashSetImpl.cpp:157-188` (ApplyBlock):
`AddSpentPositions` is called with one `SpentOutput` per input. A block with one
input should have a one-entry map. Why this particular PIBD/body-synced block
has an empty record is **not root-caused by this audit**; line2 task-11 owns
the record-level proof.

Source reviewed at HEAD `fa91e9219272aa8988c6ca27404554424a8341d9`:
`StateSyncer.cpp 31AF0EE8…`, `TxHashSetImpl.cpp 617AD908…`,
`TxHashSetManager.cpp F2C81C8A…`, `BlockDBImpl.cpp 8B4460B7…`.

### 2.2 Recovery and atomic publication

`src/P2P/Sync/StateSyncer.cpp:316-353` (`TryRecoverArchive`) reads the marker,
resolves the header, forms the final archive path, and returns if the final path
already exists. Otherwise it calls `ArchivePibdState`.

`src/P2P/Sync/StateSyncer.cpp:355-400` (`ArchivePibdState`):

1. `SnapshotTxHashSet(pHeader)` creates a zip in the temp Snapshot area
   (`TxHashSetManager.cpp:98-192`);
2. it then copies that zip to the final archive path with
   `fs::copy_file(zipPath, archivePath, overwrite_existing)` and removes the
   temp zip.

Findings:

- **No fsync/atomic rename**: `fs::copy_file` writes the final path directly;
  a crash or I/O error during the copy can leave a partial file at the final
  archive path.
- On a copy error the code logs a warning but **does not remove the partial
  final archive**; the next `TryRecoverArchive`/`ArchivePibdState` treats
  `fs::exists(archivePath)` as ready.
- There is no archive validation (zip open/hash/roots) before treating the
  final path as ready.
- `SaveSnapshot` itself is safer (temp zip; catch removes temp zip), but the
  final publication in `ArchivePibdState` is not atomic.
- Task-10 requirement “staged copy -> fsync/complete -> atomic rename/publish,
  reject existing partial file” is therefore **OPEN/NOT MET** in the audited
  source.

The `.staging-3615bb625917` directory still present under the original
`NODE\TXHASHSET\archive` was observed read-only and was **not touched**; it is
not used as a final archive by `TxHashSetArchive::Path`.

### 2.3 Existing archive serving

The H=3315600 archive is present and was served by the 7B PIBD path; restart
and body sync reached the same tip/hash without PIBD re-download. The recovery
failure at H=3313440 is therefore a **next-horizon/automatic-regeneration**
problem, not a regression of the fixed-H H=3315600 PASS.

## 3. Classification

| Item | Status | Basis |
|---|---|---|
| Observed recovery H=3313440 | **FAIL** | two C++ logs, same block/commitment |
| Spent positions for block 3317930 | **EMPTY record confirmed** (not missing record) | log `0` + `GetSpentPositions` code |
| Root cause of empty record | **NOT PROVEN here**; task-11 line2 | no DB record-level repro in this audit |
| Existing H=3315600 serving / 7B PASS | **UNAFFECTED / PASS stays valid** | archive present; PIBD/body/no_sync evidence |
| Automatic next-horizon regeneration | **NOT VERIFIED / FAIL for observed attempt** | recovery failure |
| Atomic archive publication | **OPEN / static gap** | direct `fs::copy_file`, no fsync/rename/validation |
| Mainnet readiness | **NOT READY while this finding is open** | automatic horizon regeneration required |
| Original fixture modified | **NO** | read-only hashes; no writes |

## 4. Boundaries / non-claims

- This audit did not re-run recovery on a copy; line1 task-10 owns the
  controlled copy reproduction.
- It did not inspect the RocksDB `SPENT_OUTPUTS` record bytes directly; line2
  task-11 owns the block/record proof.
- It did not build or run any new binary; no source changes or commit.
- No next-horizon success, reorg/horizon-switch, negative/resource, or
  Mainnet-readiness PASS is claimed.
- Existing 7B PASS remains valid for the fixed-H scenario with its original
  caveats (fixture C++ data, loopback allow-list, no public peers).

## 5. Evidence hashes

See `task10-11-archive-recovery-manifest-20261103.json` for the full manifest.
The build-10 copies are in `task10-11-archive-recovery-20261103/`; the original
fixture was read-only and its archive/marker hashes are captured in the
manifest. Key values:

- failure logs (repo copies): `cpp-7B-final-Node.log`
  `77518AEA4AB6CC50AE91F73FAFF4A5D6D2AB7486C70C87BD4D0110CFA208C99D`;
  `restart/7B-restart-cpp-new.log`
  `0AC6241121FF35BECD29632540B326B7467CAD720E6B5476BF46E23AFBA11086`;
  Rust final `B904A7A73AAB162964951C0C497C363E76130CA5B315EEFC272D099EF2FEB518`.
- original archive `13F9781E207F787F5F58367F9C98947D4A645D660E1CA23186AD55D9D064C530`;
- original `pibd_state.complete`
  `B671C82AFD2ACAE07738344141ECB8A7AC84235C25387E1A0AAAEC237B90D40B`;
- original configs `355B15F7…` / `AEE5E753…`;
- source files:
  `StateSyncer.cpp 31AF0EE8…`,
  `TxHashSetImpl.cpp 617AD908…`,
  `TxHashSetManager.cpp F2C81C8A…`,
  `BlockDBImpl.cpp 8B4460B7…`.

## 6. Reproduction / inspection commands (read-only)

```powershell
Select-String -Path .\evidence\line3\g09-live-02-7b-success-20261103\cpp-7B-final-Node.log `
  -Pattern 'TryRecoverArchive|SaveSnapshot|Spent output not found|ArchivePibdState'
Select-String -Path .\evidence\line3\g09-live-02-7b-success-20261103\restart\7B-restart-cpp-new.log `
  -Pattern 'TryRecoverArchive|SaveSnapshot|Spent output not found|ArchivePibdState'
Get-FileHash C:\DHW\.tmp\g09-live-02\cpp-profile\NODE\TXHASHSET\archive\TxHashSet.3315600.3615bb625917.zip
```

Do not modify `g09-live-02`; line1/line2 use separate copies for repro.
