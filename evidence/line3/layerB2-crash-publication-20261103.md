# L-B2 crash-publication evidence audit

Status: **PARTIAL EVIDENCE / PUBLICATION-CRASH ATOMICITY NOT VERIFIED.**
Read-only audit of copied evidence; no live profile/ports/source/build access.

## 1. Evidence

| File | SHA-256 / size |
|---|---|
| `evidence/line3/layerB2-crash-publication-20261103/crash-cpp-window.log` | `DD8B8CC7…` |
| `retry-cpp-window.log` | `2A816DAA…` |
| `retry-node.log` | `8774780B…` (4,194,675 B) |
| `retry-post-status.json` | `3B1BF0EE…` |
| `retry-post-roots.json` | present |
| `old-H3315600.zip` (source `.tmp` copy) | `DA23A1C7…` (3,118,756,420 B) |

Crash log tail: 13:05:05 recovery begins; 13:05:07 `SaveSnapshot` flushed
3318655 → requested 3315600; last line 13:05:15 Ping, then process dies. No
explicit crash-point marker/signal/exit code.

Retry log: 13:06:54 start; 13:07:30 recovery; 13:07:33 SaveSnapshot; 13:07:51
extract; 13:07:59 `Archived TxHashSet at height 3315600 for serving`; old
`old-H3315600.zip` preserved in evidence. Post status: `FULLY_SYNCED`,
tip 3318655/`2945348e…`; roots kernel `17d1cc11…`, output `8a9be5f8…`,
rangeproof `971c4ebd…`.

## 2. What this proves

- A kill during archive preparation (inside/around `SaveSnapshot`, before
  publication log) followed by restart/recovery can converge to a validated
  H=3315600 archive without losing the pre-existing archive copy.
- Retry publication succeeded with normal logs and final tip/roots unchanged.
- The copied old archive hash matches the known H=3315600 archive.

## 3. What it does NOT prove

- No controlled crash at the exact staged-copy / validation / rename / fsync
  boundary; crash point inferred only from last log line.
- No process signal/exit code or crash-injection marker.
- No snapshot of the final archive/staging directory at crash time; cannot prove
  no partial final was present immediately after kill.
- No concurrent publisher/readers; no ENOSPC/ACL fault; no repeated-crash loop.
- Therefore PUB-01..PUB-10 remain **NOT VERIFIED**; this is a successful
  prepare-crash/retry observation, not atomic-publication closure.

## 4. Next crash tests (copies only)

Repeat with explicit phase marker + process kill at: after staged copy, during
Extract/LoadAndValidate, after validation before PublishStaged, during
rename/fsync. Capture pre/post archive directory hashes, staging/partial
presence, marker, exit signal, restart result. Test invalid/truncated final and
ENOSPC/read-only archive dir.
