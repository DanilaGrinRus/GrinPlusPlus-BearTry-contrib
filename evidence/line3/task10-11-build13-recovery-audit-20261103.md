# Build-13 recovery audit — H=3313440 + H=3315600

Status: **H=3313440 observed PASS (archive validated/published); H=3315600
snapshot/serving observed; line1 formal report pending.** No build/commit;
original `g09-live-02` untouched.

## Identity

- Build-13 Tests `001DF4BC31F194DCDDE83FE736B06EB6FD942666F2BEFF62FF1FB5F1BB8DB3A8`;
  GrinNode `F39BDFE5A8DAB22BE969406874EDCEFD023770BE1A1BA6330D7506B2C2571201`.
- Freeze `build-13-freeze-snapshot-final.json` SHA `4FC8DCE1…` after audit
  results; source boundary fix `FDC5B4BB…` / ArchivePublication `F1E43737…`.
- Copy profile: `C:\DHW\Gplus\work\runs\task10-build11-live` (original untouched).

## H=3313440

- Target `84d7ac6763566ac67eaa8393688320c79a644321124d7ba94a1300f56af9ccea`;
  flushed 3318655 (`2945348e…`).
- `ArchivePibdState(564) — Archived TxHashSet at height 3313440 for serving`
  at `10:43:40.660` (Node.log:2985).
- Archive: `TxHashSet.3313440.84d7ac676356.zip`,
  **3,116,449,317 B**, SHA
  `7FB230583BD697EA3FF3B58A073462144750E3D07DC02820FBE99649E39AB837`.
- Zip: 17 entries; all required kernel/output/rangeproof `pmmr_*` entries
  present; `output/pmmr_leaf.bin.84d7ac676356` and
  `rangeproof/pmmr_leaf.bin.84d7ac676356` present.
- Validation path: `ArchivePibdState` extracts staging to private temp and calls
  `PibdArchiveServer::LoadAndValidate(validationRoot, pHeader, ...)` before
  `PublishStaged` (StateSyncer.cpp:532-558); log shows Extract/Load/validation
  temp removal then publish. No `missing block body`/preflight failure; no
  `spent output not found`.
- Temporary validation/snapshot dirs removed; no partial final H3313440;
  `.staging-3615bb625917` is the H3315600 serving staging, not a partial
  H3313440 archive.
- `SPENT_DIAG` mapEntries=1 count in this run: 53; build-12 canonical line for
  `30c524b8…bfcb` raw52/map1 remains `h3315600-window-final.log:744` and
  `h3313440-node1.log:24645`. Line1 formal report pending for full build-13
  canonical window.

## H=3315600

- New snapshot: `TxHashSet.3315600.3615bb625917.zip`,
  **3,118,756,420 B**, SHA
  `DA23A1C721BE96BA215A70022B3C26DC262E3122E5BE41CDBDA560631F0E78F5`;
  old pre-body-sync archive preserved as
  `old-TxHashSet.3315600.3615bb625917.zip` SHA `13F9781E…`.
- Zip: 17 entries; required entries and H3315600 leaf files present.
- `Archived TxHashSet at height 3315600 for serving` at `10:29:42.333`;
  PIBD archive serving `ready at height 3315600` at `10:30:01.860`.
- Existing H3315600 archive serves; regeneration crossed 3317930 successfully.

## Classification / caveats

- H=3313440: **observed PASS** for archive creation, target-root validation
  path, atomic publish, no partial evidence in the copy.
- H=3315600: **snapshot/serving observed PASS**.
- Line1 formal report/evidence copy still pending; no full G-09/Mainnet claim,
  no crash-interruption fault injection on this run.
- Cleanup/restart after failure and hosted CI remain unproven for this scenario.
- Original fixture untouched; all tests on copy.

## Evidence

- `build13-h3313440-node.log` SHA `D099D379…` (Node.log:2985 archive success)
- `h3315600-snapshot-serving-context.txt`
- `post-status.json`, `post-roots.json` (API snapshot while live)
- `build-13-freeze-snapshot-final.json` `4FC8DCE1…`
- H3313440 archive path/hash above (live copy)
- H3315600 archive path/hash above (live copy)
