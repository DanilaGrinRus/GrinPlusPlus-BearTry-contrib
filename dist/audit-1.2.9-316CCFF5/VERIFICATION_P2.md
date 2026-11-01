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
- Full suite: **187 cases / 2072 assertions PASS**.

## Artifacts

- `GrinNode 316CCFF5B5954474A21C35041F985D1570C9828016CC097C6155717E604A3B09`
- `Tests 70405047C79FB301DDB2879E544B367EC49063CBB8DCC37C01393FFD2B5C953F`

## Remaining P2

- chain-bound `KernelIndex` (`get_kernel` fast/verified lookup);
- pagination/position limits for `get_blocks` and `get_outputs`;
- corresponding tests from the API kit.
