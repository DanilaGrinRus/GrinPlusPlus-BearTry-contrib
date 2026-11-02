# Verification P2 — API state snapshot and unspent pagination

## Implemented

- `IBlockChain::ApiStateSnapshot` with a monotonic mutation sequence.
- `StateWriteGuard` serializes chain mutations and flips the sequence
  odd/even around `AddBlock`, `ResyncChain`, `ProcessTransactionHashSet` and
  `ProcessExistingTxHashSet`.
- `GetApiStateSnapshot()` re-checks the sequence after reading tip + TxHashSet
  and retries if a mutation started/completed.
- `IsApiStateCurrent(sequence)` exposed for handlers.
- `get_unspent_outputs` now uses the snapshot and the kit's `PlanScan`:
  - 1-based start translated to 0-based PMMR position;
  - exclusive end bound;
  - `max` output cap (10,000);
  - 8,192 scanned-position cap;
  - correct `last_retrieved_index` / `highest_index`;
  - no BlockDB reader held across the PMMR scan.

## Live check

Started the test node and called:

```json
{"jsonrpc":"2.0","method":"get_unspent_outputs",
 "params":[1,null,10,false],"id":1}
```

It returned `Ok` with `highest_index: 1`, `last_retrieved_index: 1` and one
genesis coinbase output. Node stopped afterwards.

## Tests

- New `Test_UnspentPagination`: 5 cases / 34 assertions PASS (bounded range,
  end beyond MMR, empty/out-of-range, scan cap, extreme indices).
- Full suite: **194 cases / 2224 assertions PASS**.

## Artifacts

- `GrinNode D15174D98E75D706C781DAD760C1410AEC069784A30B1F4FB1D1270259779B74`
- `Tests 1D38C73B96563E0DC00613FA023A1E937817200B91445C741776E0B0CB776E9B`

## Remaining P2

- chain-bound `KernelIndex` (`get_kernel` fast/verified lookup);
- pagination/position limits for `get_blocks` and `get_outputs`;
- corresponding tests from the API kit.
