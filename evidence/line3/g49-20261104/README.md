# G-49 evidence — parallel TxHashSet rangeproof verification (experimental)

Date: 2026-11-04. Executor: g49-perf (task-18). Status: **experimental opt-in,
NOT merged into shared implementation; live Floonet NOT VERIFIED.**

Full narrative: [G49_RANGEPROOF_PARALLEL_2026-11-04.md](../../../docs/modernization/G49_RANGEPROOF_PARALLEL_2026-11-04.md),
SHA-256 `422429463E6CA7FD19D9EE90B30670DF984FAC40BD6272784E678DE132DC62C0`.

## Measurement (isolated, same immutable state H=3315600)

| mode | 300k range proofs | proofs/s | speedup | approx cores | peak RSS |
|---|---:|---:|---:|---:|---:|
| baseline 1 worker | 119.58 s | 2508.8 | 1.00× | 1.0 | 2819 MB |
| exp 1 worker | 127.33 s | 2356.0 | 0.94× (variance) | 1.0 | 2663 MB |
| exp 2 workers | 59.50 s | 5042.1 | 2.01× | ~2 | 2674 MB |
| exp 4 workers | 32.76 s | 9157.8 | 3.65× | 4.07 | 2726 MB |

Full state, 4 workers: 2944/2944 batches, 2,943,034/2,943,034 proofs,
last batch 34, queue 8/8, range proofs 389.29 s (7560 proofs/s),
validate total 713.09 s, activation 55.96 s; final confirmed
`3315600 / 3615bb625917…`, roots/sums matched. Peak RSS 5298.7 MB,
private 3944.6 MB.

Negative: tampered proof with same commitment rejected in 4w and 1w
(verified=0, first batch FAIL); crypto same-commitment/swap rejected;
self-test 10/10 (partial last batch, gaps/dups, batch failure, worker
exception, cancellation, empty batch, bounded queue, workers&lt;2, stress).

## Binaries / sources

- baseline binary SHA-256
  `4939C9C2B7E828B51ABAD0F6DD966D7A7287E7BD059C398062C38C968B14BCC0`
- full-state 4w exp binary SHA-256
  `26721EAF4926D10D521E2F97B239F9B7AEE3F4F609BD5E40F8C1CA6BF8EB7595`
- latest exp (corrupt-prefix harness only) SHA-256
  `0CCB0ED0690E09458576D4056C27042CC6B59D9136B2229D8BDD7C0D8F241BC8`
- `g49-code.diff` — worktree diff; `RangeProofBatchVerifier.{h,cpp}` copies.
- Raw `json/`, `logs/`, `bin/` from executor worktree.

## Decision / limits

Accept only as **experimental opt-in** (`GRINPP_RANGEPROOF_WORKERS=2/4`,
default sequential). Do not treat as release/default. Pending: independent
review, TSan/ASan (Windows unavailable → NOT VERIFIED), full-state negative
activation test, repeat full runs, live Floonet slot. No consensus/format
change; activation only after every batch succeeds.
