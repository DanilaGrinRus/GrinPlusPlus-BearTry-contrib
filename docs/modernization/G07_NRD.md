# G-07: NRD (NoRecentDuplicate) kernels

## Implemented

- `TxHashSet` keeps a recent NRD kernel index:
  `excess -> [(kernel PMMR position, block height)]`.
- On `ApplyBlock`, every NRD kernel is checked against the previous occurrence:
  - duplicate NRD kernels in the same block are rejected;
  - `current_height - previous_height < relative_height` is rejected;
  - entries older than `2 * WEEK_HEIGHT` are ignored/pruned.
- On `Rewind`, NRD entries introduced by the rewound block are removed using
  the previous header's kernel MMR size.
- Startup and PIBD activation rebuild the recent index from block history
  (last two weeks).
- `BlockValidator` rejects NRD kernels before header version 3.

The pure relative-height/duplicate rules are in `include/PMMR/NrdRules.h` and
covered by unit tests for `relative_height` 1/2 and the recent cutoff.

## Status

**PARTIAL.** The enforcement path is implemented and the suite passes, but:

- there is no separate NRD config/activation flag; enforcement is tied to the
  presence of the kernel feature and header version;
- the index is in-memory and rebuilt on startup, not persisted incrementally in
  RocksDB like Rust's `MultiIndex`;
- no full block-processing/reorg NRD test and no live NRD peer test were run.

## Artifacts

`GrinNode D15174D9…`, `Tests 1D38C73B…`; full suite 194 cases / 2224 assertions.
