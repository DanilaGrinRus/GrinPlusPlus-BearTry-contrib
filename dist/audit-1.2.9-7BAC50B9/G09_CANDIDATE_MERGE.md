# G-09 candidate merge and remaining-tasks commentary

## What was merged

The external candidate patch was merged manually onto the current tree (not
`git apply`):

1. `PruneableMMR::GetRawAt` rejects a compacted/out-of-range leaf before doing
   the shifted data read.
2. `SegmentGenerator` leaf policy:
   - required leaf with data -> leaf bytes;
   - required leaf without data -> `MissingLeaf`;
   - optional leaf without data -> prune-aware hash if available;
   - optional leaf with data is not sent (Grin++ representation choice).
3. A fully-pruned segment walks the **entire** `FamilyBranch` to the first
   stored ancestor instead of only `branch[0]`; no ancestor is an error.
4. `PibdStateWriter` and `PibdTreeRebuilder` persist/use hash-only boundary
   leaves (spent sibling of an unspent leaf), but never leaf hashes inside a
   fully pruned subtree.

## Offline replay

The external fixture is at
`C:\Users\nurli\Downloads\Grin-G09-audit-2026-11-02\G-09-floonet-replay.zip`
(not added to the repository). Using the compact `pp-ckpt` state and protocol
body serialization:

| body | result |
|---|---|
| output (11,1100) | **byte-for-byte identical** to the Rust-validated fixture |
| rangeproof (11,1100) | **byte-for-byte identical** |
| kernel (11,1637) | **MISMATCH**: ours 49,545 bytes, fixture 43,081 bytes |

The output/rangeproof match validates the ancestor-search fix for the reported
E56 segment. The kernel mismatch is **open** and is not claimed as PASS. The
fixture's kernel body has 404 leaf positions but is smaller than 404 serialized
kernels plus positions, so the fixture/state relationship needs a separate
investigation before it can be acceptance evidence.

## Tests

- Full suite: **187 cases / 2109 assertions PASS** before restoring the temporary
  fixture test includes; final rebuild follows.
- New/retained tests: anchor-search/required-leaf coverage through existing
  PIBD serving tests, `LeafSet` position conversion, state writer root checks.

## Remaining-tasks commentary (Codex audit)

`docs/modernization/REMAINING_TASKS_COMMENTS.md` is a plan, not independently
verified evidence. Confirmed reading points:

- **N-01 (P0)**: `TxHashSet::Commit` runs three commits in threads; an exception
  in a thread can terminate the process and leave trees inconsistent.
- **N-02 (P0)**: ignored `FileSync::Flush` results and Windows checkpoint
  delete-before-rename; checkpoint publication needs a strict transaction.
- **N-06 (P0)**: `GetKernelHandler` mixes PMMR positions with 1-based leaf
  indices; `get_kernel` can return the wrong/missing kernel. The current
  `get_kernel` is still the direct-scan version.
- **N-03/N-04/N-05/N-07 (P1)**: recovery-contract dead code, async rate-limit
  accounting, unbounded write queue, and swallowed `get_outputs` read errors.

## Status

**G-09 live: NOT VERIFIED.** No full serving sweep, no Rust client PIBD
completion, and no live test after this merge. The archive node remains in
`body_sync current=0`, so no full-state source is available.
