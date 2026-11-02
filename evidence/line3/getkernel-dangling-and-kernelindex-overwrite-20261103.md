# Confirmed code-level findings: get_kernel dangling pointer + kernel-index overwrite (2026-11-03)

Independent review by line3 (code + log). No production/test edits, no builds, no
process changes. Both findings are **not** PIBD-serving/consensus claims.

## A. get_kernel uses an object after its lifetime (CONFIRMED BY CODE)

File: `src/API/Node/Handlers/GetKernelHandler.h`

- line 139: `if (pKernel == nullptr) {`
- line 140: `const auto kernelWithIndex = pTxHashSet->FindKernelByExcess(excess, minIndex, maxIndex);`
- line 145: `pKernel = &kernelWithIndex->first;`
- line 146–150: mmrIndex/height set; height failure returns early;
- line 151: **block ends — `kernelWithIndex` is destroyed**;
- line 153: `const TransactionKernel& kernel = *pKernel;` → **dangling pointer
  dereference (use-after-scope, UB)**.

Earlier indexed path uses an `indexedKernel` optional declared outside the
per-leaf block (line ~131 `indexedKernel.emplace(...)`, `pKernel = &indexedKernel.value()`),
so it is safe. Only the **direct-scan fallback** (used when the kernel index is
unavailable/stale) is defective. Impact: wrong API JSON, crash/UB; exact crash
not reproduced by line3.

### Fix direction (line2 / API G-53)

Hoist the resolved object to function scope, e.g.
`std::optional<TransactionKernel> resolvedKernel;` before the `if`, then
`resolvedKernel.emplace(std::move(kernelWithIndex->first)); pKernel = &resolvedKernel.value();`
(or store the pair). Ensure lifetime until serialization (line 169).

### Required regression (line3 after fix)

- handler-level call with kernel index **disabled/unavailable** (direct-scan path),
  existing excess (result fields correct), missing excess (NOT_FOUND);
- `min_height`/`max_height` bounded range behavior (+ outside range), and
  index-enabled path unchanged;
- verify the referenced TransactionKernel lives through JSON serialization;
- normal harness mandatory; ASan if the API/handler test links without
  BlockChain→Database→rocksdb (currently likely normal-only; classify honestly).

## B. Kernel index rebuild mismatch on g09-live-01 fixture (CONFIRMED LOG; MECHANISM CONFIRMED BY CODE)

Log: `C:\DHW\.tmp\g09-live-01\cpp-profile\LOGS\Node.log`
(full sha256 `29184C03BE47CAE03962F5509FE2EF3890E9A1EC649C675AC652EA5C52AF213B`;
slice preserved at `evidence/line3/g09-kernel-index-log-20261103.txt`).

```
[04:15:54.636] Ensure(121) - Kernel index rebuild verification failed:
  count 3354326/3354613, checksum 4a153183...60fd vs 435c0876...bfb992.
[04:15:54.661] Create(438) - Kernel index is unavailable; get_kernel will use the direct scan.
```
Repeated deterministically at 05:17:14, 05:33:38, 05:45:10 with same counts and
checksums; state was missing and rebuilt at 04:14:18 (generation 1).
**Expected 3 354 613, stored 3 354 326 → shortfall 287.**

### Mechanism (code-confirmed)

- `src/PMMR/KernelIndex.cpp:93-95`: for **every kernel leaf**
  `AddKernelPosition(generation, pKernel->GetExcessCommitment(), location)` and
  checksum XOR over every leaf.
- `src/Database/BlockDBImpl.cpp:346-350`: key is
  `KERNELPOS/<generation>/<excess>` → RocksDB `Put` → **duplicate excess
  overwrites the earlier position** (one key per excess).
- `GetKernelPositionCount` (415-430) counts **unique keys**; `GetKernelPositionChecksum`
  (432-455) iterates **unique stored keys** and recomputes XOR.
  → duplicate excess leaves produce exactly a count shortfall and a checksum
  mismatch, because the verification checksum was computed over duplicates but
  the read-back checksum is over unique keys.

Therefore the **287 shortfall is consistent with 287 duplicate excess occurrences
beyond the first** on this fixture (if no other DB/state corruption). The node's
refusal to publish and fallback to direct scan is correct defensive behavior —
but it routes `get_kernel` into finding A.

### Not yet proven / next steps (owner: line2 / kernel index; line3 assists)

1. Count duplicates on a copy of the fixture: open
   `C:\DHW\.tmp\g09-live-01\cpp-profile\NODE` (CHAIN/TXHASHSET) and enumerate
   kernel leaves (`GetKernelByLeafIndex`) — count total vs unique excess,
   duplicates should be **287**; identical-excess positions and heights recorded.
   (No RocksDB dump tool was available to line3; no DB copy was modified.)
2. Compare Rust semantics for duplicate excess: what position does Rust
   `get_kernel` return, does it reject duplicate excess, and how do
   `min_height`/`max_height` behave.
3. Decide fix: key by leaf index (or generation+leaf) and add a secondary
   excess→position mapping; or store all positions and define deterministic
   choice; version/migrate the table; keep fail-closed verification.
4. Tests: duplicate-excess fixture, reorg/restore, restart, get_kernel range;
   normal harness + ASan where linkable (real-DB path is likely normal-only).
5. **No consensus/G-09 claim** from this finding; it is an index integrity and
   API-correctness issue.

## C. TransactionInput V3 deserialize (code observation, NOT reproduced)

`src/Core/Models/TransactionInput.cpp:26-37`: for protocol ≥ V3,
`Deserialize` calls `Global::GetCoinView()->GetOutputType(commitment)` immediately,
so output type depends on current coin-view state at deserialization time. A
dependent block arriving before its output is applied can fail or resolve
differently. This matches the reported "Output Commitment not found" symptom
mechanism, but **no exact live reproduction/linkage yet**. Handoff: line1 to
capture the live event (block hashes, order), line2/model owner to fix if
confirmed. Do not rewrite sync before reproduction.

## D. ACL note (not a defect)

`Refusing PRIVATE/ADMIN API access: secret file owner/DACL is not owner-only` is
defensive: the isolated profile's `.api_secret` must be owner-only. Correct
action is fixing the isolated profile ACL (without publishing secrets or
weakening the check globally), not code change.

## Evidence / constraints

- Log slice: `evidence/line3/g09-kernel-index-log-20261103.txt`
- Fixture: `C:\DHW\.tmp\g09-live-01\cpp-profile` (NODE/CHAIN, TXHASHSET)
- No files in fixture modified; no build/commit; no ASan run for these findings.
