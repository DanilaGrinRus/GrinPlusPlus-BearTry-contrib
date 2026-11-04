# G-49 — isolated TxHashSet rangeproof validation: measurement + experimental parallel verification

Executor: g49-perf (task-18). Date: 2026-11-04. Work root: `C:\DHW\.tmp\g49-work`
(disposable run profiles); canonical evidence: `C:\DHW\Gplus\evidence\g49-20261104`.
Status: **experimental opt-in prototype accepted for further review; NOT merged into shared tree; live Floonet NOT VERIFIED.**

## 1. Scope / isolation

- Isolated git worktree `C:\DHW\.tmp\g49-work\grinplusplus`, detached HEAD `e8b47f4359eb8e6599f4b47499390bf7a63fbdb4`, plus Lead's dirty
  `KernelMMR.cpp` (SHA256 `7B1244358F13631B6748623457C0FDA0BC07F54CE0446E82D3B148E6B51E7C19`) and
  `KernelMMR.h` (`F93744E63EB3636FCF735413796662F85E750A6CEC8C9454AE0900BE68D65254`), copied read-only from shared.
- Shared tree never built or modified. No `GrinNode.exe` was started. Live processes were stopped by Lead; this work never touched
  `legacy-local-01`, ports 25831/25836/25837/25850 or the live profile.
- State fixture: copy of `C:\DHW\Gplus\work\runs\task2-legacy-public\cpp-profile` (not live). Archive:
  `fixtures\txhashset_3315600.zip`, 3,067,445,011 bytes, SHA256 `D73EBD1811A40AFC876D4AA5B006D2D2E11B1BE5EDD0E102E8DF7D7FB2036C6C`.
  Header `3615bb625917582f7d028fe4ff738d82cb939b71ac768e885519b8798544ec9f`, height 3315600,
  kernels=3,351,542, outputs=3,425,950, unspent rangeproofs=2,943,034 (last batch = 34 proofs).
- Benchmark executable `tools/g49bench` is an isolated harness (uses `Environment::AUTOMATED_TESTING` only to avoid spawning Tor;
  non-mainnet genesis/NRD rules are identical). It binds no ports; per-run config reserves 26931/26936/26937/26950 as unused values.
- Host: AMD Ryzen 7 7800X3D (8C/16T), 47.2 GB RAM, Windows 11 26200, MSVC 19.44, CMake VS2022, /W4 /WX /O2 /MT /std:c++17.

## 2. Baseline measurements (isolated where stated)

| measurement | source | result |
|---|---|---|
| extract archive | isolated, baseline binary | 6.41 s |
| load + leafset creation | isolated | 9.23 s |
| MMR hashes (3 MMRs) | isolated | 3.376 s |
| roots | isolated | 0.004 s |
| kernel history | isolated | 45.887 s (3,315,601 heights) |
| kernel sums | isolated | 118.694 s |
| range proofs, one core, 300k immutable slice | isolated | 119,580 ms, 2508.8 proofs/s, 300×1000 batches |
| per-batch one core | isolated slice | min 377, median 385, avg 397.6, max 476 ms |
| CPU/wall during slice run | isolated | 200.11 s / 195.84 s = 1.02 core |
| full validation + signatures + activation | live reference only (not isolated; killed 40-min run not repeated) | 28m41.5 s total; range 22m30 s / 2,943,034 proofs (~2180 proofs/s); signatures 169 s; activation ~64 s |

The 40-minute full sequential isolated run was intentionally not repeated (user/Lead decision). Isolated full-run phases above are from
a baseline run that was externally killed during the rangeproof phase; range-proof full timing is therefore a live reference, not isolated.
This is labelled **reference**, not proof of the isolated full baseline.

### Bottleneck

- `TxHashSetValidator::ValidateRangeProofs` is a single-threaded loop, 1000 proofs per `Crypto::VerifyRangeProofs` batch.
- The batch is CPU-bound in `secp256k1_bulletproof_rangeproof_verify_multi`; reading output/rangeproof PMMRs
  (`MappedFile::Read`, 2 reads/output, ~5.9M short reads) is secondary (seconds, not minutes).
- The pinned Windows secp256k1-zkp is portable C: `USE_ASM_X86_64` undef, `USE_FIELD_10X26` (not 5x52), `USE_SCALAR_8X32`
  (not 4x64), `USE_ENDOMORPHISM` undef, CMake compiles only `src/secp256k1.c`. No asm, no AVX path. This is a separate
  build/toolchain decision (clang-cl/gcc or 5x52), outside G-49 scope; expected 1.2–1.5× per PERF_RANGEPROOF but **not measured here**.
- secp thread-safety checked against pinned source commit `5b39fbf7f1e16e126404791923511723c4c0876d`:
  `secp256k1.h` explicitly allows concurrent read-only use of a constructed context; scratch space is *not* shareable;
  generators are read-only. Prototype uses one independent `Bulletproofs` instance per worker (own context, generators,
  per-call scratch, own cache). No `context_randomize` can race because generation is not used here.

## 3. Implemented experiment (isolated copy only)

New:
- `src/PMMR/RangeProofBatchVerifier.{h,cpp}` — one sequential reader (immutable staging PMMRs), bounded queue
  (capacity = 2×workers), 2/4 worker threads, one independent `Bulletproofs` per worker, progress only from successfully
  verified proofs (conservative), exact accounting (submitted vs verified batches/proofs), fail-closed on any failed batch,
  worker exception, reader exception, cancellation or empty batch. Thread-start failure falls back to a full sequential re-read.
- `tools/g49bench` — isolated benchmark harness (process / validate / rangeproofs / crypto-neg / self-test modes,
  resource monitor, per-phase G49T/G49R logs, JSON evidence).
Modified:
- `TxHashSetValidator::ValidateRangeProofs` dispatches to the batch verifier. Default remains sequential workers=1;
  opt-in only through `GRINPP_RANGEPROOF_WORKERS` (clamped to 4). `GRINPP_RANGEPROOF_LIMIT` is benchmark-only.
- Instrumentation logs phase times/roots/sums; public `ValidateRangeProofsForBenchmark` wrapper for the harness.
- Production activation code is untouched: `ProcessTxHashSet` activates only after `ValidateTxHashSet` returns non-null;
  any failed rangeproof batch returns false, so no partial activation path was added.

Diff (worktree, no shared changes): new verifier + harness; `TxHashSetValidator.cpp/.h`; `src/PMMR/CMakeLists.txt`; tools CMake;
plus Lead's copied `KernelMMR.cpp/.h`. `git diff --stat`: 6 files changed, 497 insertions(+), 331 deletions(-) + new verifier/harness files.

## 4. Results (same immutable state; ranges are unspent proofs from leaf 0)

### 300,000-proof slice (immutable, 300 × 1000 batches)

| mode | range proofs | proofs/s | speedup | CPU-s / wall | peak RSS | private | notes |
|---|---|---|---|---|---|---|---|
| baseline 1w (inline loop) | 119,580 ms | 2508.8 | 1.00× | 200.11 / 195.84 (1.02 core) | 2819.5 MB | 2339.2 MB | batch median 385 ms |
| exp 1w (refactored sequential) | 127,334 ms | 2356.0 | 0.94× | 204.50 / 199.21 | 2663.2 MB | 2337.1 MB | within run-to-run variance (see 100k repeats) |
| exp 2w | 59,499 ms | 5042.1 | 2.01× | 191.11 / 126.26 | 2674.0 MB | 2340.9 MB | queue cap 4, max depth 4 |
| exp 4w | 32,759 ms | 9157.8 | 3.65× | 201.64 / 99.77 | 2725.9 MB | 2395.9 MB | queue cap 8, max 8; live delta 4.07 cores |

100k repeat pairs (variance check, same slice prefix):

| run | range proofs | proofs/s | batch median |
|---|---|---|---|
| baseline 1w A | 40,443 ms | 2472.6 | 391 ms |
| exp 1w A | 45,240 ms | 2210.4 | 443 ms |
| baseline 1w B | 43,826 ms | 2281.8 | 435 ms |
| exp 1w B | 43,289 ms | 2310.1 | 430 ms |

Baseline spread 8.0%, exp spread 4.4%; paired differences −11.9% and +1.2% → no reproducible sequential regression, but exact
parity is **NOT VERIFIED** (n=2, noisy). Multiple runs are noticeably affected by time/thermal order; single fresh runs are not a
basis for a multi-percent claim.

### Full state, 4 workers, production `process` path (includes activation)

| phase | time |
|---|---|
| process_begin → validate_begin (extract+load) | 12.8 s |
| sizes / MMR hashes / roots | 0 / 2.557 s / 0.004 s |
| kernel history | 49.496 s |
| kernel sums | 112.018 s |
| range proofs | 389,286 ms; 2944/2944 batches; 2,943,034/2,943,034 proofs; last_batch=34; 7560.1 proofs/s; max_queue 8/8; failed=no |
| kernel signatures | 159.724 s |
| validate total | 713,089 ms |
| activation (validated→process_end) | 55.96 s (NRD + SaveOutputPositions 52.3 s + confirmed chain 5.0 s) |
| final confirmed | height 3315600, hash 3615bb… (requested header) |
| roots | kernel `60ab2c88…a29a5`, output `fc207197…1dcbc`, rangeproof `6bdb5037…c03c0` |
| sums | output `084c2787…47e5b6`, kernel `08e807fc…d86b59` |
| resources | wall 841.05 s, CPU 2059.52 s (~2.45 cores avg; 4 during phase), peak RSS 5298.7 MB, private 3944.6 MB, peak threads 48, I/O 13.33/5.96 GB |

Range phase speedup vs live one-core reference: 1349.9 s → 389.3 s = **3.47×**; validation total vs live reference: 1721.5 s → 713.1 s
= **2.41×**. Conditions differ (live node had peer/background load, isolated run did not), so these are not a controlled A/B.
Linear scaling is **not claimed** (2.01× at 2w, 3.65× at 4w on the slice).

## 5. Correctness / negative tests

- **PASS** self-test batch runner 10/10: success with last partial batch (2 proofs), no gaps/duplicates, exact counts,
  stress 25 runs × 4 workers (1600 proofs, queue bound respected), batch failure → overall FAIL, worker exception → FAIL,
  cancellation → FAIL, empty batch → FAIL, bounded queue max_depth=2, workers<2 rejected.
- **PASS** crypto negative (real proofs, 4.1 MB/200-proof batch): valid batch accepted; cache-hit accepted; tampered proof for
  the **same commitment** rejected (accepted=false); proof swapped to another commitment rejected; valid after negatives accepted.
- **PASS** slice reject, 4 workers: 1 bit flipped in every one of the first 1,200,000 extracted rangeproof records
  (commitments unchanged). First batch failed; submitted_batches=12, verified_batches=0, verified_proofs=0,
  failed=yes reason=batch verification failed; overall validation FAIL (scenario PASS, exit 0 expected).
- **PASS** slice reject, 1 worker: first batch failed in 391 ms, verified 0, overall FAIL.
  Note: an earlier single-record test at data record 0 was not rejected because that record belongs to a spent/retained leaf
  not enumerated by the validator; that was test-design, not a validator bypass. The prefix corruption covers all shifted
  indices reachable by the 300k slice.
- **PASS** full-state 4w accept: exact proof accounting, last partial batch 34, roots and sums as above, final confirmed tip
  equals requested header, TXHASHSET activated (`txhashset_changed=true`).
- Fail-closed activation: structurally there is one activation site reached only after `ValidateTxHashSet` success; slice rejects
  and self-test cover fail propagation. A full-state tampered-proof **process** rejection (to observe unchanged live files)
  was **NOT VERIFIED** (not run; would need ~4–5 min) — do not report it as PASS.
- **NOT VERIFIED**: ThreadSanitizer/ASan on Windows (not available in this setup); no race detector evidence. No UAF/race was
  observed in the full 4w run + 25×4w stress, but this is not a proof. Live Floonet activation and restart are **NOT VERIFIED**
  (requires a separate approved live slot; benchmark does not substitute for live).
- Build: exp and baseline targets both exit 0 with `/W4 /WX`; no shared-tree build.

## 6. SHAs / evidence

Binaries:
- baseline `g49bench_baseline.exe` SHA256 `4939C9C2B7E828B51ABAD0F6DD966D7A7287E7BD059C398062C38C968B14BCC0`
- exp binary used for full 4w process `26721EAF4926D10D521E2F97B239F9B7AEE3F4F609BD5E40F8C1CA6BF8EB7595`
  (recorded in `evidence/logs/exp-process-w4-full.pre.json`)
- latest exp binary (adds only harness `--corrupt-prefix`; verifier/validator identical) `0CCB0ED0690E09458576D4056C27042CC6B59D9136B2229D8BDD7C0D8F241BC8`

Sources (latest, `evidence/bin/exp/sources-final.sha256`):
- `RangeProofBatchVerifier.h` `2F7600427A593D966F46A676877CA4D83CFD49ADAB692BD5C6490B2BB4FDA8F6`
- `RangeProofBatchVerifier.cpp` `1E128732E20C25EDCF9B6B7733C894B975384ED985D07CA40BD9A63B85440751`
- `TxHashSetValidator.cpp` `D7DEFD29702692DD044773FCA7E350AD2E2108FECEBF9D5B1AD0A0CDD185E748`
- `TxHashSetValidator.h` `32C521862B5C72CBEF47A9B3DD04AE9E9F2D0BC68C50549DF1BF31F22DE03F6A`
- `KernelMMR.cpp` `7B1244358F13631B6748623457C0FDA0BC07F54CE0446E82D3B148E6B51E7C19`; `.h` `F93744E63EB3636FCF735413796662F85E750A6CEC8C9454AE0900BE68D65254`
- `src/PMMR/CMakeLists.txt` `BAF9D615DA1EC037A10E5E68307531FFD64B547516C9603513582F156C65DFE7`
- `tools/g49bench/g49bench.cpp` current `7C2C06FA2FB20CF111F613F602F9AF51F25FB954DF55D6F17503DB25D29A63DE`
  (full-run binary used `103F72D377B38E8A81B4BEE5507A419D498C58872E6EA44A35742A7461D61354`, before the corruption-option edit)
- `tools/g49bench/CMakeLists.txt` `5D4F144E9A16E0587AC6291678719E042B6D2DEB12EAEDE5D6BBCC365853B07B`; `tools/CMakeLists.txt` `01850CB22F106EA840D739B89CEBB09DC94D4543F258A81367FE7C04AF6E5B6E`
- baseline build instrumentation sources: `Evidence/bin/baseline/sources.sha256` (validator `038025CE…`).

Evidence paths (canonical copy under `C:\DHW\Gplus\evidence\g49-20261104`; the
disposable `C:\DHW\.tmp\g49-work` run profiles are not documentation):
- JSON per run: `json\*.json` (baseline slice, exp 1w/2w/4w, full 4w process, self-test, rejects, crypto-neg)
- raw logs: `logs\*.console.txt`, `logs\*.pre.json`, `logs\*.post.json`; full Node.log remains in each `profiles\run-*\LOGS\Node.log`
- binaries + SHAs: `bin\baseline\`, `bin\exp\`
- build logs: `build-baseline.log`, `build-exp.log`; `system-info.json`; `setup-copy.log`
- this report: `C:\DHW\Gplus\src\grinplusplus\docs\modernization\G49_RANGEPROOF_PARALLEL_2026-11-04.md`
  (byte-identical copy: `C:\DHW\Gplus\evidence\g49-20261104\G49_REPORT.md`)
- worktree/diff: `C:\DHW\.tmp\g49-work\grinplusplus` (`git status`, `git diff`); shared tree untouched.

## 7. Decision / recommendation

**ACCEPT as an experimental opt-in only; do not switch the default and do not call it a release optimization yet.**
- Real measured benefit: 3.65× rangeproof phase on a 300k slice, 3.47× range phase and 2.41× validation total vs live reference
  on the full state, with bounded memory (peak RSS ≈ baseline, 2.7–2.9 GB; full run 5.3 GB) and exact accounting.
- Remaining gates: independent review; TSan/ASan-equivalent run on Linux/CI; a repeated full-state run; full-state negative
  activation test; live Floonet with separate approval; decide whether 2/4 workers become a config option.
- Separate, not part of G-49: secp asm/5x52 build (different toolchain), which may add 1.2–1.5× but is unmeasured here.
- Sequential default stays unchanged; refactored sequential showed variance-level differences only (n=2) — not enough for a
  parity PASS claim.
