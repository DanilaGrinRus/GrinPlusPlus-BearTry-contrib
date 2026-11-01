# Independent PMMR review — assessment

Source: user-supplied independent analysis of the C++ PMMR vs Rust PIBD serving.

## Verdict

The analysis is technically strong and matches the evidence from E56–E61. It adds
one concrete bug that our own audits had not isolated:

## 1. Confirmed: segment generator omitted missing-leaf hashes

Rust `Segment::from_pmmr` adds a leaf's hash to the segment when the leaf data is
absent (a spent/pruned leaf). `SegmentGenerator::GenerateFromSource` only added
hashes for internal nodes; for a leaf with no data it did nothing.

This is a real generator bug and can break proofs independently of the state
retention problem. A first patch was attempted:

- leaf data is used when present and required;
- otherwise `hashAt(pos)` is added for a prunable segment.

The patch caused a SIGSEGV in the synthetic `RangeProofSegmentWireRoundTrip`
fixture, so it was reverted rather than left half-tested. A proper port needs:

- a dedicated unit test with a prunable segment whose leaf data is absent;
- validation of the reconstructed segment root, not only serialize/deserialize;
- a safe leaf-hash source when the file-backed PMMR is compacted.

## 2. Confirmed: `IsCompacted` missed the leaf-set exception

Rust `backend::is_compacted()` first checks `leaf_set.includes(pos)`: a leaf that
is still in the leaf set is never compacted. C++ delegated only to the prune list.
**Fixed:** `PruneableMMR::IsCompacted` now returns false for an unspent leaf in
the leaf set before consulting the prune list. Full suite remains green.

## 3. State-model diagnosis: confirmed

`PibdStateWriter` rebuilds a compact hash file containing only data nodes and
pruned roots; Rust PIBD serving operates on the append-only full hash history.
This is the core reason a PIBD-activated C++ node cannot prove some segments.
The report's distinction is correct.

## 4. Sidecar claim is outdated

The report notes a sidecar read without a writer. That was true during an earlier
experiment; the sidecar was reverted. Current serving uses
`GetHashAtIgnoringPrune` only as an additional lookup, with bounds checks. The
underlying state-retention point remains valid.

## 5. Test-coverage criticism: valid

`Test_PibdServing` mostly checks segment completeness/round-trip and, in one case,
a small validated state. It does not sweep all segments and validate the
reconstructed MMR root against the horizon for a realistic state. That is exactly
the test needed before claiming PIBD serving.

## Practical conclusion

G-09 needs both:

1. the generator leaf-hash fix, with a validating test;
2. a state where the required hashes exist (full snapshot or equivalent), or a
   serving model that can read the full append-only hash history.

The full-state archive node is still in `body_sync current=0`, so it is not a
usable source yet.

## Artifacts after the IsCompacted fix

- `GrinNode 728A1E5E…`
- `Tests 0AE6BD43…`
- full suite **187 / 2078 PASS**
