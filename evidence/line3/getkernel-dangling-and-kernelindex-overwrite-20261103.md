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

Therefore the **287 shortfall is exactly the duplicate-excess effect** on this
fixture. The node's refusal to publish and fallback to direct scan is correct
defensive behavior — but it routes `get_kernel` into finding A.

### PROVEN by line2 + independent line3 recount (2026-11-03)

Line2 evidence:
`tools/audit/nrd/evidence/kernelindex-duplicate-excess-g09-20261103.json`
(sha256 `CDEC6A9EEDCF4E2429DF49B263CA12FE435DC32B4B294AF721FDF59EA83F652A`).
Copied kernel PMMR: source == copy sha256
`D4D73A0DB6BCA8CD6E801C8C82BD1F8B3F7379DD43363D7A9B3D3D90E31E9108`,
382,425,882 B, original hash unchanged.

Line3 independent recount (114-byte V1 entries, excess offset 17..49, 128-bit
fingerprint + exact multiplicity; evidence
`evidence/line3/kernelindex-duplicate-excess-independent-20261103.json`):

```
total=3354613 unique=3354326 extras=287 duplicate_excesses=284
multiplicity 2x=281, 3x=3
```

Exactly matches line2 and the g09 Node.log count `3354326/3354613`. Consequences
confirmed:

- rebuild verification (count==leaves, checksum over every leaf vs unique-key
  read-back) can **never** publish on this chain — correct fail-closed;
- runtime falls back to direct scan; `get_kernel` numeric results via fallback
  match Rust latest-in-range semantics, but every call hits the dangling-pointer
  path from finding A;
- Rust `find_kernel` scans backward and returns the **latest** matching excess
  within `[min_index, max_index]`; there is no persistent excess→position index
  in Rust. Any C++ index must reproduce latest-in-range, not one-position-per-excess.
- No consensus/G-09 claim; this is an index-availability/performance and
  API-correctness issue.

### Fix design (deferred until first full G09 live + Lead build-10 approval)

1. **A (recommended):** primary index keyed by unique kernel position
   (MMR pos/leaf) storing excess+height, plus secondary excess→positions so
   `get_kernel` selects the **latest** position within `[min,max]`; count/checksum
   over every leaf; schema/version bump + generation rebuild/migration.
2. **B (smaller):** canonical latest-position per excess with duplicate-aware
   state validation (total leaves vs unique entries stored separately) and
   handler fallback to direct scan when the range/canonical position is
   ambiguous; preserves Rust semantics but does not index earlier positions.
3. Do **not** merely relax verification while keeping count==kernelCount —
   range behavior with duplicates becomes ambiguous.

Tests to bind after fix: duplicate-excess fixture/synthetic chain (281×2 + 3×3),
total-leaves vs unique-keys, latest-in-range `get_kernel` with min/max, reorg /
restore / restart, migration/version bump; handler hoist regression via task-9
draft. Real-DB paths are normal-only (BlockChain→Database→rocksdb ASan boundary);
ASan only where linkable.

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

## E. Task-9 draft regression harness (pattern-level failing-before)

Lead approved a draft harness outside the compiled tree:
`C:\DHW\.tmp\task9-api-harness` (README, `Test_GetKernelHandlerLifetime.cpp`
skeleton for the real handler, `repro_lifetime_pattern.cpp`,
`build_pattern_repro.ps1`). No CMake/default-suite changes; tree untouched.

Pattern-level ASan build (MSVC 14.44, `/Od /Ob0 /fsanitize=address`) result:

```
buggy=16045690984833335030 fixed=12
```

- the buggy shape (block-local optional → pointer → scope exit → dereference)
  returns the deterministic stack-poison value, demonstrating the UB;
- the hoisted-optional fixed shape returns 12;
- MSVC ASan did not emit `stack-use-after-scope` for this pattern here, so the
  real-handler regression must assert correct JSON/behavior (and use ASan only
  if the toolchain actually reports the class);
- this is **pattern-level**, not a build-09 API reproduction. Real-handler run
  remains **planned** until the fake-based test is wired with Lead approval and
  line2's fix.

## Evidence / constraints

- Log slice: `evidence/line3/g09-kernel-index-log-20261103.txt`
- Fixture: `C:\DHW\.tmp\g09-live-01\cpp-profile` (NODE/CHAIN, TXHASHSET)
- No files in fixture modified; no build/commit; no real-handler ASan run yet.
