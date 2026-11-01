# PMMR audit — C++ vs Rust grin 5.5.2

## Scope

- `src/PMMR/Common/MMRUtil.*`, `Index.*`, `MMRHashUtil.*`;
- `src/PMMR/Common/PruneList.*`, `PruneableMMR.h`, `LeafSet.h`;
- `src/PMMR/Models/Segment*.h`;
- `src/P2P/PIBD/PibdStateWriter.h` (state production).

Reference: `reference/grin-5.5.2/core/src/core/pmmr/pmmr.rs`,
`store/src/prune_list.rs`, `store/src/pmmr.rs`,
`core/src/core/pmmr/segment.rs`, `chain/src/txhashset/desegmenter.rs`.

## Formula and layout parity

| Function | Rust | C++ | Result |
|---|---|---|---|
| `insertion_to_pmmr_index` | `2*n - n.count_ones()` | `MMRUtil::InsertionToPmmrIndex` | PASS |
| `bintree_postorder_height` | `peak_map_height(pos).1` | `MMRUtil::BintreePostorderHeight` | PASS |
| `family` / parent-sibling | branch by peak bit | `Index::GetParent/GetSibling` | PASS |
| `family_branch` | parent+sibling up to `size` | `MMRUtil::FamilyBranch` | PASS |
| `peaks` | scan peak sizes | `MMRUtil::GetPeakIndices` | PASS |
| `bintree_leftmost/rightmost` | `pos+2-2^(h+1)` / `pos-h` | same | PASS |
| `pmmr_leaf_to_insertion_index` | height 0 only | same | PASS |
| Prune shift | rank + shift cache | `PruneList::GetShift` | PASS |
| Shift cache | `2*((1<<h)-1)` for pruned roots | same | PASS |
| Leaf shift | `1<<h` for pruned roots | same | PASS |
| Leaf-required rule | own/sibling bit or last leaf | `IsSegmentLeafRequired` | PASS |
| Segment proof | family branch + bagged/left peaks | `SegmentGenerator`/`SegmentProof` | PASS (wire/live tests) |

## Findings

### F1 (superseded): prune-ignoring hash reads were wrong for serving

An earlier attempt added `GetHashAtIgnoringPrune` and used it as a PIBD-serving
fallback. The independent audit showed that Rust `get_from_file` returns `None`
for a compacted internal position: it does not recover the node through a
shifted index. The fallback could therefore return a different node and produce
a valid-looking but wrong segment root.

The fallback was removed. `PibdSegmentService::LookupHash` now uses only the
prune-aware `GetHashAt`; a missing/compacted hash returns `nullopt`, and read
errors are caught. Pruned-root hashes remain readable through the normal path.

### F2 (superseded): shifted reads and bounds

The first fallback also computed `fileIndex = pos - shift` without bounds
checks. `AppendOnlyFile::Read` and `MMRHashUtil::GetHashAt/GetShiftedIndex` now
reject underflow, out-of-range and boundary-spanning reads; `DataFile::GetDataAt`
throws. A regression test covers OOB reads.

- `mmrIndex < GetSize()`;
- `shift <= pos`;
- `fileIndex < hashFile->GetSize()`.

`PibdSegmentService::*`: 31 assertions / 3 cases PASS after the guard.

### F3 (known blocker): PibdStateWriter retains only pruned roots and data nodes

`PibdStateWriter::WriteTree` writes hashes for nodes with data and for maximal
pruned subtree roots. Compacted descendants are not stored. A service proof
that asks for one of those exact positions therefore still fails
(`Source is missing the sibling hash`). The sidecar experiment confirmed the
hashes were not present in the peer segments either. This is a state-retention
issue, not an MMR formula bug; F1 only makes serving able to use a hash that is
actually present (e.g. a full archive).

### F4 (low risk): root skips zero peak hashes

`MMRHashUtil::Root` ignores a peak whose hash reads as `ZERO_HASH`, while Rust
`peaks()` uses `Option<Hash>` and includes a genuine zero hash. A zero Blake2b
peak is cryptographically negligible and existing root tests pass; documented
rather than changed.

## Verdict

- PMMR position/prune/segment formulas: **PASS** against Rust.
- Two real defects found and fixed: compacted-hash visibility and unsafe
  shifted-index access.
- Live PIBD serving remains **NOT VERIFIED / BLOCKED** on the current compacted
  state; the fallback is ready for a full horizon snapshot.

## Evidence

- `Test_PibdServing` wire/live tests; `Test_MmrStructure`;
- full suite 191 cases / 2200 assertions PASS;
- artifacts `GrinNode F197C7D2…`, `Tests ED64D384…`.
