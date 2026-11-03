# L-B1 resource/concurrency matrix — PibdArchiveServer / publication / TxHashSet

Status: **STATIC CODE AUDIT (read-only; no source/build/live/profile access).**
HEAD `921bcfd…`; evidence only.

## 1. Limits → locks → tests → gaps

| Area | Limit / state | Lock / atomic | Existing test | Gap |
|---|---|---|---|---|
| PibdArchiveServer | zip **8 GiB**, extracted **16 GiB**, free-after **4 GiB** (`PibdArchiveServer.h:86-88`) | worker mutex/cv; one preparation; registry state | `HorizonSelectionAndSpaceBoundary`, `ArchiveZipExtractsAndValidates` | no ENOSPC/partial write injection; placeholder limits not calibrated |
| PibdArchiveServer | **2000 requests/min** global | `m_requestTimes` under mutex; excess dropped | none explicit for saturation | no rate-limit/concurrent-peer test |
| PibdArchiveServer | one ready snapshot + leases; retired staging until last lease | registry mutex + shared_ptr | `LeaseOutlivesHorizonSwitch`, `StagingCleanup`, `DeferredGarbageDrain` | no many-reader/lease-exhaustion or lease-vs-prepare stress |
| PibdArchiveServer | exact-hash acquire; refusing mismatch | registry `Acquire` | `RegistryGatingAndExactHash` | concurrent acquire while horizon switch not stress-tested |
| ArchivePublication | staged `.staging` → validate → fsync/rename | no cross-process publication lock; fixed staging path | `ArchivePublicationValidatedAtomicPublish`, F13 | no concurrent publisher test; crash at rename/fsync matrix not run |
| ArchivePublication | target roots validated before publish | `LoadAndValidate` before `PublishStaged` | F13 build15 PASS | no wrong-height/oversized archive integration test |
| TxHashSetManager | snapshot copies live TxHashSet dir; Rewind to target | no explicit snapshot/write mutex in `SaveSnapshot` | `Pibd*`, copy-only recovery evidence | no concurrent snapshot vs live write test; copy-dir disk/time unbounded |
| TxHashSet | Rewind missing previous header | **build15 null-check + typed throw** | F11 `Test_RewindFailClosed` PASS | no direct reorg-path stress |
| TxHashSet | archive rewind range | preflight bodies/spent above target; target body not required | build13 boundary test/F13 | R2 flushed-vs-confirmed invariant still unchecked (build15 scope did not fix) |
| TxHashSet/DB | SPENT/undo availability | per-record query; fail-closed | F13/preflight, recovery evidence | no DB-fault matrix; R2/R4 open |

## 2. Concurrency hazards

- Two processes/workers can stage/publish the same archive path; staging name is
  fixed (`<final>.staging`). No lock/lease across processes is shown.
- Recovery worker and serving worker both touch the archive directory; leases
  protect loaded snapshots, not on-disk final/staging publication.
- `SaveSnapshot` copies the live TxHashSet directory without a visible
  snapshot/write lock; concurrent commit/rewind could produce inconsistent
  copied bytes.
- PibdArchiveServer global request budget is minute-window based; no byte/lease
  cap on serving load beyond one snapshot.
- PIBD wire segment counts/sizes rely on downstream validation; no explicit
  wire-count cap in the inventory.

## 3. Required tests (minimal)

1. ENOSPC during staged copy/extract and fsync/rename; final remains old/absent.
2. Two concurrent recovery/publish attempts at same target; one wins, no torn
   final.
3. Many concurrent `Acquire` leases + horizon switch; old snapshot lives until
   last lease, staging not deleted early.
4. `SaveSnapshot` during live commit/rewind; either consistent snapshot or
   explicit refusal.
5. Request-rate saturation and dropped-request behavior; no ban/leak.
6. Process kill at staged/validate/rename; PUB-01..10 matrix.
7. DB fault matrix (L-B4) for missing/empty/corrupt spent/current/manifest.

All are **NOT VERIFIED** until run on copies; no source edits now.
