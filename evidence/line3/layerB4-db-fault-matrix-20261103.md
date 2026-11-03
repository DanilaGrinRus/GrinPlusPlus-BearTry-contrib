# L-B4 DB fault matrix — archive recovery / spent / RocksDB

Status: **STATIC MATRIX (read-only; no DB/profile/source/build access).**
HEAD `921bcfd…`; evidence only.

| Fault | Expected | Existing coverage | Gap |
|---|---|---|---|
| Missing `SPENT_OUTPUTS` record | typed fail-closed, no publish | `BlockDBImpl::GetSpentPositions` throws; preflight catches | no direct DB-injection test; R5/F15 not implemented |
| Empty spent record for input block | fail-closed (0 mapped) | old bug; build12/13 fix, F13/preflight evidence | no portable fixture for empty record on current build |
| Corrupt SpentOutputs version/count | typed reject | `Test_SpentOutputs` Release regression 1/6 | only synthetic serializer bytes; no DB corruption |
| Wrong key/length/leaf/height | reject preflight | partial F13/root checks | no wrong-leaf/height DB test |
| DB `CURRENT`/`MANIFEST` missing/corrupt | clear failure, no silent recovery | RocksDB open failure only | **GAP** |
| Read-only DB/archive dir | fail closed, no partial | **no test** | **GAP** |
| Disk full/WAL/fsync during DB write | no torn publish; recoverable | **no fault injection** | **GAP** |
| Snapshot/DB view mismatch (flushed vs confirmed) | checked invariant | R2 found; no fix | **GAP** |
| Concurrent DB reader/writer/snapshot | consistent view or refusal | **no stress test** | **GAP** |
| Archive final wrong roots while DB valid | regenerate/fail closed | F13 build15 normal PASS; ASan link-blocked | no DB+archive combined fault |
| Recovery marker path unwritable/disk full | bounded/logged | R4 found; no fix | **GAP** |

## Evidence anchors

- `BlockDBImpl::GetSpentPositions` absent-vs-empty semantics; `SpentOutput.h`
  Release deserialize fix; build15 normal F13/SpentOutputs PASS.
- `StateSyncer::ValidateExistingArchiveForHeader` exact-root check (build15).
- `TxHashSetImpl::Rewind` build15 null-check; R2 flushed/confirmed invariant
  remains open, R4 marker write remains open.
- No DB fault injection, no live profile access; all rows NOT VERIFIED where
  marked.

## Minimal fixture plan

1. Copy DB/CHAIN; inject missing/empty/wrong SPENT record for canonical hash;
   run copy-only recovery/preflight; expect no publish/tip change.
2. Corrupt `CURRENT`/delete MANIFEST; expect open failure, no recovery marker
   loop.
3. Read-only DB/archive dir; expect fail-closed with actionable error.
4. ENOSPC/WAL fault during write; verify no partial archive/final and no tip
   mutation.
5. Flushed-header vs confirmed-tip mismatch; verify preflight rejection.
6. Concurrent reader/snapshot while write; verify consistent view or refusal.

No source edits/build; evidence-only.
