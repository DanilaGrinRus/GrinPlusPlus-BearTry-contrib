# Fail-closed P1 audit — build-14 pre-audit (read-only)

Status: **READ-ONLY / P1 CONFIRMED / BUILD-14 PATCH AUDIT PENDING AFTER
BUILD-13 LIVE**. No source edits, build, tests, or commit now. Build-13 full 7B
live is in progress; original `g09-live-02` untouched.

## 0. Scope

Independent verification of line2 read-only fail-closed audit
(`tools/audit/nrd/evidence/rewind-preflight-fail-closed-audit-20261103.md`,
SHA `EB4DB197…`). Source binding:
`TxHashSetImpl.cpp 617AD908…`, `StateSyncer.cpp FDC5B4BB…`,
`ArchivePublication.h F1E43737…`, `TxHashSetManager.cpp F2C81C8A…`.

## 1. P1 findings

### R1 — Rewind null previous-header dereference

`src/PMMR/TxHashSetImpl.cpp:719`:

```cpp
m_pBlockHeader = pBlockDB->GetBlockHeader(m_pBlockHeader->GetPreviousHash());
```

No null check; the loop condition at line 668 dereferences
`*m_pBlockHeader` on the next iteration. Missing/corrupt previous header
becomes a null dereference instead of a typed fail-closed exception.
Direct caller: `BlockProcessor::HandleReorg` line 253
(`pTxHashSet->Rewind(pBlockDB, *pCommonHeader)`), inside a batch whose rollback
must handle the failure. Preflight covers archive recovery only, not all callers.

**Build-14 acceptance:** F11 fake DB missing previous header must throw
`TXHASHSET_EXCEPTION`, no crash/null deref; batch rollback leaves tip/root
unchanged.

### R2 — Preflight confirmed tip vs TxHashSet flushed header

`StateSyncer::PreflightArchiveRewind` uses
`GetTipBlockHeader(EChainType::CONFIRMED)` (`StateSyncer.cpp:439-447`).
`TxHashSetManager::SaveSnapshot` uses
`m_pTxHashSet->GetFlushedBlockHeader()` as the actual rewind start
(`TxHashSetManager.cpp:113-135`). There is no checked equality invariant.
If flushed header lags confirmed tip, preflight checks a different range than
Rewind traverses; target above actual flushed can cause backward walk/missing
header.

**Build-14 acceptance:** F12 actual flushed header < confirmed tip must be
rejected before snapshot/copy, or preflight must use the actual flushed header
and prove the range. No archive publish on mismatch.

### R3 — Container-only readiness suppresses regeneration

`ArchivePublication::IsReadyArchive` validates file regularity, size, zip
open, required entries, and leaf prefixes, but **not** target roots/MMR sizes
(`ArchivePublication.h:68-113`). Both `TryRecoverArchive` (`StateSyncer.cpp:403-405`)
and `ArchivePibdState` (`StateSyncer.cpp:508-510`) return success/clear marker
when this container check passes. A complete but wrong-root archive at the final
path can therefore suppress automatic regeneration. `LoadAndValidate` catches
it at serving time, but recovery is not driven again.

**Build-14 acceptance:** F13 container-valid/wrong-root archive must not be
treated as ready; validate target roots/MMR sizes before early return, or
explicitly regenerate/fail closed. Never advertise invalid archive.

### R4 — Marker write failure enables restart retries

`RecordRecoveryFailure` (`StateSyncer.cpp:55-66`) calls
`FileUtil::WriteTextToFile` + `FileSync::Flush`, catches all exceptions, and
does not check durability/visibility. On disk-full/ACL failure the
`pibd_state.recovery_failed` marker is absent, so every restart re-attempts the
same unbounded recovery once per process. `TryRecoverArchive` is bounded only
by `m_archiveRecoveryChecked` within a process.

**Build-14 acceptance:** F14 unwritable marker path/disk full must be
bounded/logged or surfaced; no unlimited restart attempts; original failure not
masked. Prefer atomic marker write + explicit write-failure signal/attempt cap.

### R5/R6 (P2/P3) — include in same matrix if patch touches

- Missing `SPENT_OUTPUTS` for zero-input block → fail closed with actionable
  error, no publish/loop (F15).
- Missing matrix gaps: target==flushed-1, target>flushed, missing target/flushed/
  intermediate header, target=0, missing previous header, actual flushed !=
  confirmed tip, malformed/partial archive, marker-write failure, empty-input
  absent record.

## 2. Build-14 patch-audit plan

After build-13 live ends and Lead supplies build-14 SHA/source freeze:

1. Verify exact patch hashes; no unrelated source drift.
2. R1/R2/R3/R4 code review against above acceptance; no weakening of
   fail-closed or target-boundary behavior.
3. Fail-before/pass-after tests:
   - F11 missing previous header → typed exception, no null deref, rollback.
   - F12 flushed/confirmed mismatch → reject before snapshot/publish.
   - F13 wrong-root/container-valid archive → not ready; regenerate or fail
     closed; no suppression.
   - F14 unwritable/disk-full marker → bounded/logged, no restart loop.
   - F16 same-target marker restart skip; different target one fresh try.
   - F17 publish/rename/fsync failure → old-valid/absent final, staging removed.
4. Normal Release full/filters + boundary/SpentOutputs/ArchivePublication;
   ASan subset (new tests if link deps permit); 0 reports.
5. Copy-only recovery H=3313440/H=3315600, target roots/archive hashes, no
   partial publish, tip/roots unchanged on failures.
6. Crash/publication/resource matrix from
   `build13-7b-live-audit-plan-20261103.md` (PUB/RES) on build-14.
7. Update EVIDENCE/CI/SANITIZERS; no commit.

## 3. Scheduling / non-interference

- Build-13 full 7B live is running; do NOT build, run Tests/port 3421, edit
  source/test/config, or touch the live copy until it ends.
- Build-13 is final candidate; build-09/build-12 historical; build-10 final only
  for its snapshot.
- Build-14 fixes/tests scheduled after live + Lead order.
- No full G-09/Mainnet claim from this read-only audit.

## 4. Evidence

- Line2 audit: `tools/audit/nrd/evidence/rewind-preflight-fail-closed-audit-20261103.md`
  SHA `EB4DB197A7F96E63E88C7E4B1102D7F156636D850671A42CC4B26927F62B13FE`
- Build-13 live audit plan: `build13-7b-live-audit-plan-20261103.md`
- Build-14 pre-audit JSON: `build14-failclosed-preauth-20261103.json`
