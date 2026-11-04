# Build-12 pre-audit — Release SpentOutputs fix + recovery

Status: **PRE-AUDIT / FIX PRESENT / NOT BUILT / AWAITING BUILD-12 SHA**.
No build or commit. Build-11 remains bound only to its own snapshot and has the
confirmed production Release bug; do not reuse its artifacts to validate the fix.

## 1. Current source (build-12 candidate)

- HEAD `6dbf8a7da067f35ba585371d82123ad21edf3a9a`.
- Fix: `include/Core/Models/SpentOutput.h` SHA
  `D27A607D32F6CA587EF66D2086C5F6A10E6B8469A5EF3EAEAB803DEA5269E211`
  (3341 B); explicit version `ReadU8`, reject non-zero, then `ReadU16`.
- Regression: `tests/src/Database/Test_SpentOutputs.cpp` SHA
  `6157AC56683BB9C908E0EAC5F75FCB7CFE84171F539208CB172CC77E9EEF34A1`; two
  tests (Release deserialize + preflight).
- Build-11 old source: `SpentOutput.h` `F3F38B44…` (assert version);
  build-11 normal/ASan does not cover the bug.
- Current modified source/test set (11 files): BlockChain.h/Impl.*,
  ArchivePublication.h, StateSyncer.*, BlockDBImpl.cpp, RocksDB.h,
  Test_BlockPipeBan.cpp, SpentOutput.h, Test_SpentOutputs.cpp.
- Build-12 binary SHA/source freeze: **not available yet**.

## 2. Required build-12 normal audit

Serialized isolated runner, quoted filters, `-KeepBinary`, `-FailOnNoTests`,
exclusive port 3421, per-run HOME/TMP; record every source SHA.

- Full suite; expected +2 targeted cases (do not preclaim exact count), only
  known bodyless test exception may fail.
- Mandatory filters/groups:
  - `SpentOutputs*` — Release-mode regression must PASS;
  - `BlockChain*preflight*` — mapped/missing input coverage;
  - `PibdArchiveServer::*` and `P2P::ArchivePublication*`;
  - `Nrd*`, `KernelIndex*`, `GetKernelHandler*`, schema-v2,
    duplicate-range, `P2P::AdvertisedTip*`.
- Release regression proof: run normal Release binary, not ASan-only. Record
  `SpentOutputs Release-mode deserialize consumes version byte` PASS.
- Fail-before-fix proof: old `F3F38B44…` data path yields 0 entries; fixed
  yields 1. This is a focused source/unit proof; build-11 artifact must not be
  relabeled PASS.

## 3. Recovery acceptance (copy only; original untouched)

Use a fresh copy of `C:\DHW\Gplus\work\runs\g09-live-02`; never modify the original.

### H=3313440 (missing archive, flushed 3318655)

- With `GRINPP_SPENT_DIAG=1`, canonical key `30c524b8…bfcb` raw 52 B must
  decode to `mapEntries=1`; no `archive preflight ... 0 mapped spent
  position(s)` for block 3317930.
- Preflight `(target, flushed]` passes; recovery produces a **validated**
  H=3313440 archive (zip entries + roots/LoadAndValidate), atomically published
  (staged -> validate -> fsync -> rename); no partial final, no stale staging
  advertised.
- Tip/root unchanged on failure paths; restart does not loop or publish stale
  H; same-target failure marker behavior checked.
- Evidence: log lines, archive path/SHA/size, roots, staging/partial checks,
  marker, DB CURRENT/MANIFEST.

### H=3315600 (existing archive and boundary)

- Existing H=3315600 archive still validates/serves; no regression from
  H=3313440 recovery.
- In a copy, exercise H=3315600 regeneration (remove only the copy's archive or
  use a controlled target) across 3317930; expect the same decode/preflight
  success and exact-height archive validation.
- Verify H=3315600 pre-body-sync archive is not treated as a counterexample;
  archive roots/hash and height must match the target.
- No `fs::exists`-only readiness; invalid/partial archive rejected.

## 4. ASan / evidence

- One ASan refresh on build-12 frozen source; SpentOutputs regression is not
  rocksdb-blocked and must run under ASan; Nrd/archive/Segment/P2P+advert,
  teardown, 0 reports.
- Capture build-12 binary SHA/bytes/PE, source freeze aggregate, build logs,
  normal/ASan manifests, recovery logs/archive hashes.
- Update `EVIDENCE_LINE3`, `CI_REPRO`, `SANITIZERS`; no commit.

## 5. Blockers

- Build-12 binary/source freeze absent.
- Build-11 binaries contain old assert code and cannot validate the fix.
- Live H=3313440/H=3315600 evidence not yet produced.
- Atomic-publication live crash/interruption still not exercised.

## 6. Pointers

- `task10-11-spent-deserialize-rootcause-20261103/README.md`
- `task10-11-spent-deserialize-rootcause-20261103/rootcause-20261103.json`
- `task10-11-archive-recovery-independent-audit-20261103.md`
- `build-11-freeze-snapshot-final.json`
- `normal-binary-build11-audit-20261103.md`
