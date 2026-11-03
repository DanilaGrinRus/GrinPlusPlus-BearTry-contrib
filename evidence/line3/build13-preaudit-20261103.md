# Build-13 pre-audit — H=3313440 target-boundary fix + recovery

Status: **PRE-AUDIT / BUILD-12 ROOT-CAUSE FIX VERIFIED / BOUNDARY FIX PENDING /
AWAITING BUILD-13 SHA**. No build/commit.

## 1. Build-12 verified state

- Root-cause fix verified live: build-12 decoded canonical raw 52 B/count1 as
  `mapEntries=1` in `SPENT_DIAG` (build-12 evidence `h3313440-node1.log`).
- Build-12 normal 256/255/1 bodyless-only; Release `SpentOutputs*` 1/6 PASS;
  exact preflight test PASS; ASan subset PASS, 0 reports.
- Build-12 **recovery still FAILS** at H=3313440 on a separate boundary bug:
  line 269 of `h3313440-node.log`:

```text
archive preflight: missing block body at height 3313440
hash 84d7ac6763566ac67eaa8393688320c79a644321124d7ba94a1300f56af9ccea
(target 3313440)
```

## 2. Exact boundary defect

`src/P2P/Sync/StateSyncer.cpp` `PreflightArchiveRewind` (current hash
`CF273B95…`, lines 454-490):

```cpp
454 for (uint64_t height = flushedHeight; ; --height) {
455     pHeader = GetBlockHeaderByHeight(height, CONFIRMED);
...
464     pBlock = GetBlockByHeight(height);   // body required
...
475     AreSpentPositionsComplete(pHeader->GetHash(), inputs, mappedCount);
...
487     if (height == targetHeight) break;
490 }
```

The loop includes `targetHeight` and requires a full body + spent map for the
target block itself. `TxHashSet::Rewind`
(`src/PMMR/TxHashSetImpl.cpp:665-720`) only processes headers while
`*m_pBlockHeader != header`: it fetches the body/spent map for blocks **above**
the target, then exits when the header equals the target. The target body is not
needed; its header/roots are.

The preflight boundary must:

- still require a valid confirmed target header and target roots;
- require bodies + complete spent maps for `flushedHeight..targetHeight+1`
  (all blocks actually rewound);
- **not** require a target block body or target spent record;
- fail if any body/spent record above target is missing;
- fail if target > flushed or target header missing;
- retain exact error context.

## 3. Build-13 recovery acceptance

Fresh copy of the g09 fixture only; original untouched.

### H=3313440

- Canonical target hash `84d7ac67…9ccea`; flushed 3318655.
- `SPENT_DIAG` for block 3317930: `rawBytes=52`, `mapEntries=1` (and no
  `0 mapped` above target).
- Preflight must pass without target body at 3313440.
- Rewind from 3318655 to 3313440 succeeds; snapshot zip is validated
  (entries + `LoadAndValidate` roots) before atomic publish.
- Verify produced H=3313440 archive: path, size/SHA, target hash, roots:
  output root `49fe85ec…`, range-proof root `38a596b7…`, kernel root
  `0cba0e2a…`; no partial final/staging advertised; tip/root unchanged on
  failure; no retry loop/stale target marker.

### H=3315600

- Use line1's new H=3315600 snapshot (identity/hash/size captured when
  available; do not reuse the old pre-body-sync `13F9781E…` as the counterexample).
- Existing H=3315600 archive validates/serves; copied regeneration/removal path
  passes through the same target-boundary preflight without requiring a body at
  3315600; exact height/roots/zip validation; no `fs::exists`-only readiness.
- Verify both H=3313440 and H=3315600 in the same build/source freeze.

## 4. Build-13 normal/ASan

- Build-13 binary SHA/bytes/PE + source freeze before any PASS.
- Normal full/filters: SpentOutputs*, exact preflight, ArchivePublication*,
  PibdArchiveServer*, GetKernel/schema/duplicate, Nrd*, KernelIndex*, advert;
  bodyless exception only; zero mg_start.
- Regression for boundary: target body absent + blocks above complete => PASS;
  missing body above target => FAIL; target header missing => FAIL; incomplete
  spent map above target => FAIL.
- ASan refresh on build-13 source; subset + teardown; 0 reports.

## 5. Current blockers

- Build-13 not built/no SHA.
- Boundary fix not present in current `StateSyncer.cpp` (`CF273B95…`, inclusive
  target loop); line1/Lead to apply/freeze.
- H=3315600 snapshot from line1 not yet captured/identified.
- Live H=3313440/H=3315600 evidence absent; build-12 normal/ASan does not
  close recovery.

## 6. Evidence

- `build12-h3313440-node.log` SHA `270D4265…` (line 269 boundary failure)
- `task10-11-spent-deserialize-rootcause-20261103/` (root-cause fix)
- `normal-binary-build12-audit-20261103.md`
- `asan-build12-refresh-20261103.md`
- `build-12-freeze-snapshot-final.json`
