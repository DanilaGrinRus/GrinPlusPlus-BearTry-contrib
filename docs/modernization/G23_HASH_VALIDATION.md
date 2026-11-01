# G-23: startup hash revalidation

## Implemented

- `IHeaderMMR::GetHashAt(Index)` exposes the exact stored MMR leaf hash.
- When a durable `chain.checkpoint` exists, `BlockChain::Create` now verifies
  the complete candidate chain against the header MMR before opening the chain
  state:
  - every candidate index has a block header with the same hash and height;
  - each header's `previous` hash links to the previous candidate hash;
  - each header MMR leaf equals
    `HashLeafWithIndex(PoW serialization, LeafIndex::At(height).GetPosition())`.
- A mismatch throws and aborts startup. Nothing is deleted, truncated or
  rewritten by the hash-validation path.
- If a TxHashSet is open, its output/rangeproof/kernel roots are checked against
  its flushed header with `ValidateRoots`; mismatch is fatal.

## Why checkpoint-gated

Fresh databases and test fixtures legitimately have no durable checkpoint and
may not populate the header MMR until `ChainState::Create` adds genesis. The
hash pass runs only for a state that claims durability through
`chain.checkpoint`; size/structure repair remains the earlier startup path.

## Evidence

- Full suite: **192 cases / 2207 assertions PASS**.
- Live `pp-archsrv` startup with a 3.3M-header chain: API became ready in
  **75.4 s**, no hash mismatch; status `SYNCING_HEADERS`, header 3 318 961.
- `GrinNode F197C7D2…`, `Tests F25A02A0…`.

## Still open

- Fault-injection test that flips a byte in `header_mmr.bin` and verifies a
  precise abort (manual/live, not in the default suite).
- Full node-by-node validation of output/rangeproof PMMR contents; the current
  TxHashSet check validates the three roots, not every stored node.
- G-23 remains **PARTIAL** until corruption injection and a full state sweep are
  recorded.
