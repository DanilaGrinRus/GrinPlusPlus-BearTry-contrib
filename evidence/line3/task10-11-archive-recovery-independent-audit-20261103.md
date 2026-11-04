# Task-10/11 archive recovery — independent read-only audit (2026-11-03)

Status: **FAIL confirmed for the observed recovery attempt**; automatic
next-horizon regeneration **NOT VERIFIED**; atomic publication **OPEN**.
Existing 7B PASS and H=3315600 archive serving remain valid. This audit is
evidence/hash + static review only; no build, no commit, and the original
`C:\DHW\Gplus\work\runs\g09-live-02` fixture was not modified.

## 0. Post-audit root cause (build-11 diagnostics)

The earlier “actual Rewind key unknown / unresolved contradiction” is
**superseded** by the build-11 `GRINPP_SPENT_DIAG` proof:

- canonical key `30c524b8424f91e1b2b154112f19d77d4c0db6887d6a1cdc96ef2055e022bfcb`;
- raw SPENT_OUTPUTS value **52 B**, `rawHex=0000010989ac92…32a0a8`;
- log line `SPENT_DIAG ... rawBytes=52 ... mapEntries=0` at Node.log:23840;
- preflight failure line 23841: 1 input, 0 mapped spent positions, target
  3313440/flushed 3318655.

Root cause: `include/Core/Models/SpentOutput.h` (build-11 hash
`F3F38B44…`) line 95 `assert(byteBuffer.ReadU8() == 0);`. Release targets
compile with `NDEBUG` (`GrinNode.vcxproj:156`, `Tests.vcxproj:168`), so the
assert expression is not evaluated and the version byte is not consumed.
`ReadU16()` then reads bytes `00 00` -> count=0 -> empty map. The DB record is
not empty; the deserializer misreads it.

Current worktree fix (unbuilt) `SpentOutput.h D27A607D…` reads the version
explicitly and rejects non-zero versions; regression tests are in
`tests/src/Database/Test_SpentOutputs.cpp` `6157AC56…`. Build-11 normal/ASan
PASS did not cover this nonzero-SpentOutputs Release path. Build-12 must
re-audit. Evidence:
`task10-11-spent-deserialize-rootcause-20261103/`.

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

The failure log says **0 spent position(s)**. A naive reading would mean the
`SPENT_OUTPUTS` record for block `30c524b8424f` **exists but contains an empty
vector**, which is the task-11 invariant violation. **This inference was
retracted after independent DB audit.**

Line2's preserved-DB audit and line3's fresh copy of
`C:\DHW\Gplus\work\runs\line2-nrd-spent-audit\copy\NODE` both show:

- canonical chain hash at H=3317930:
  `30c524b8424f91e1b2b154112f19d77d4c0db6887d6a1cdc96ef2055e022bfcb`;
- SPENT_OUTPUTS value for that key: 52 B, version 0, **count 1**, exact logged
  commitment, leaf 6,856,589 / output height 3,317,928;
- only one BLOCK/HEADER record at H=3317930; 5,215 retained BLOCK records have
  `spentCount == inputCount` (0 mismatches); 68 input-bearing blocks;
- 5,147 empty SPENT records, all for 0-input blocks; **0 empty records for any
  input-bearing block**, 0 orphan empty keys;
- original and copy CURRENT/MANIFEST hashes are identical
  (`69705DE0…` / `191FF9A4…`).

Therefore the logged `0` cannot come from the canonical hash against this
preserved DB. Either the failing runtime queried a different hash/DB view, or
the DB was different at failure time. **The actual Rewind key/view was not
instrumented and remains unknown.** Our prior "empty record confirmed" claim is
retracted; this remains an unresolved contradiction, not proof of an empty
canonical record. `GetSpentPositions` still throws on absent records, so a
runtime empty map implies a record existed in whatever view was queried.

`src/PMMR/TxHashSetImpl.cpp:665-717` obtains the map by the block being rewound,
iterates its inputs, and throws `Spent output not found` when an input commitment
is absent from the map. The normal write path is
`src/PMMR/TxHashSetImpl.cpp:157-188` (ApplyBlock): `AddSpentPositions` gets one
`SpentOutput` per input. Why the failing runtime saw zero is **not root-caused
by this audit**; line2 task-11 owns the record/runtime proof.

Source reviewed at HEAD `fa91e9219272aa8988c6ca27404554424a8341d9`:
`StateSyncer.cpp 31AF0EE8…`, `TxHashSetImpl.cpp 617AD908…`,
`TxHashSetManager.cpp F2C81C8A…`, `BlockDBImpl.cpp 8B4460B7…`.

Evidence for the correction:
`C:\DHW\Gplus\src\grinplusplus\tools\audit\nrd\evidence\spent-positions-3317930-audit-20261103.json`
SHA `7F636BBEFFBBF2D458E52F7903E278E7F7E708F95A198D252CA696D955551856`;
fresh copy audit
`evidence/line3/task11-spent-positions-fresh-copy-audit-20261103.json`.

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
| Observed recovery H=3313440 | **FAIL** | three C++ logs, same block/commitment |
| Spent positions for block 3317930 | **canonical record 52 B/count=1; deserializer returned 0** | build-11 SPENT_DIAG line 23840 + raw hex |
| Actual Rewind key/view | **RESOLVED: canonical 30c524…, MANIFEST-000451, SPENT_OUTPUTS** | build-11 diagnostic |
| Root cause of log `0` | **CONFIRMED: NDEBUG removed assert(ReadU8) side effect; count misread 0** | old SpentOutput.h:95 + Release NDEBUG |
| Build-12 fix/regression | **FIX PRESENT BUT UNBUILT / PENDING AUDIT** | current D27A607D… + test 6157AC56… |
| Existing H=3315600 serving / 7B PASS | **UNAFFECTED / PASS stays valid** | archive present; PIBD/body/no_sync evidence |
| Automatic next-horizon regeneration | **NOT VERIFIED / FAIL for observed attempt** | recovery failure |
| Atomic archive publication | **OPEN in build-11 source; build-12 fix unbuilt** | direct `fs::copy_file` in build-10; ArchivePublication unit only |
| Mainnet readiness | **NOT READY while H=3313440 recovery is unproven** | build-12 live recovery required |
| Original fixture modified | **NO** | read-only hashes; no writes |

## 4. Boundaries / non-claims

- This audit did not execute an instrumented runtime recovery; line1 task-10
  owns the controlled runtime reproduction. A line1 `task10-copy` run at
  09:24-09:27 did not reach `TryRecoverArchive` (no peer/body-sync trigger), so
  it neither reproduced nor exonerated the runtime.
- It did inspect the preserved DB via line2's read-only scripts and a fresh
  copy; canonical hash record is count1, no empty input-bearing record.
- It did not obtain the actual runtime Rewind hash/view: no instrumentation,
  no PDB/debugger symbols, no source patch/build. This is the required next
  diagnostic for build-11.
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
Get-FileHash C:\DHW\Gplus\work\runs\g09-live-02\cpp-profile\NODE\TXHASHSET\archive\TxHashSet.3315600.3615bb625917.zip
```

## 7. Build-11 update (2026-11-03)

Build-11 retry (`Tests 02DC3D9C…`, `GrinNode 565BC2DB…`) contains line1's
atomic-publication/preflight code and line2's `GRINPP_SPENT_DIAG` diagnostics.
Normal and ASan (`P2P::ArchivePublication*` 1/83, 0 reports) pass. This is
**unit-level** evidence only:

- no live H=3313440 automatic regeneration run has passed;
- no crash-interrupted publication/fail-closed preflight exercise exists;
- build-11 itself has the confirmed Release `SpentOutputs` bug; its diagnostic
  logged canonical raw 52 B/count1 as `mapEntries=0`.

Root cause is now resolved (section 0): NDEBUG removed the assert side effect
(`ReadU8`) and count was misread as 0. Fix `D27A607D…` and regression
`6157AC56…` are built in build-12 (`Tests D1103F34…`, `GrinNode 833CCBFD…`):
normal **256/255/1** bodyless-only, `SpentOutputs*` 1/6 and exact preflight PASS;
ASan subset PASS, 0 reports. Live recovery **H=3313440/H=3315600 is still
pending**, so task-10/11 remain **OPEN** until that evidence exists.


## 8. Build-12 recovery boundary (2026-11-03)

Build-12's SpentOutputs fix is verified: canonical build-12 `SPENT_DIAG` for
`30c524b8…bfcb` now reads the same raw 52 B as **`mapEntries=1`**
(`h3313440-node1.log:24645`; build-11 logged `mapEntries=0` at line 23840).
Build-12 normal **256/255/1** bodyless-only and ASan subset PASS.

Recovery H=3313440 still fails on a **separate preflight boundary defect**:

```text
TryRecoverArchive(421): TxHashSet archive recovery failed (bounded; no retry
for this target): archive preflight: missing block body at height 3313440
hash 84d7ac6763566ac67eaa8393688320c79a644321124d7ba94a1300f56af9ccea
(target 3313440)
```

`StateSyncer::PreflightArchiveRewind` (current `CF273B95…`, lines 454-490) loops
through `targetHeight` inclusive and demands the target body and target spent
record. `TxHashSet::Rewind` (`TxHashSetImpl.cpp:665-720`) stops when the header
equals the target; it only needs bodies/spent maps for blocks **above** the
target. Fix approved: validate target header/roots, preflight
`flushedHeight..targetHeight+1`, do not require target body. Build-13 must
re-audit recovery H=3313440 and H=3315600; line1's H=3315600 snapshot is
pending. Build-12 normal/ASan does **not** close task-10/11. See
`build13-preaudit-20261103.md` and
`evidence/line3/build13-preauth-20261103/build12-h3313440-node.log`.


## 9. Build-13 recovery result (2026-11-03)

Build-13 (Tests `001DF4BC…`, GrinNode `F39BDFE5…`) passed normal
257/256/1 bodyless-only + ASan subset, and independently observed:

- **H=3313440:** archived `TxHashSet.3313440.84d7ac676356.zip`,
  3,116,449,317 B, SHA `7FB23058…`, 17 entries/required/leaf; Extract +
  `LoadAndValidate` before atomic publish; no missing-body/preflight/spent
  failure; tip/roots unchanged; no partial H3313440 final.
- **H=3315600:** new snapshot 3,118,756,420 B SHA `DA23A1C7…` (old
  `13F9781E…` preserved); archived and PIBD serving ready across 3317930.

Line1 formal report pending. This closes the observed H=3313440/H=3315600
recovery evidence for build-13 but is not a crash-interruption or full
G-09/Mainnet PASS. Build-12 historical; build-13 next final candidate.


Do not modify `g09-live-02`; line1/line2 use separate copies for repro.
