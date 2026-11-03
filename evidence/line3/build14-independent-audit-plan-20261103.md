# Build-14 independent audit plan — R1 / D2 / R3 / F13

Status: **BUILD/TEST IN PROGRESS (Lead) / PRE-AUDIT / NO SOURCE EDITS OR BUILD
BY LINE3.** Build-13 remains the final live binding. Build-14 is fail-closed
hardening + D2, **not** a replacement live claim.

## 0. Binding context

- Build-13 final live: Tests `001DF4BC…`, GrinNode `F39BDFE5…`, full 7B PASS
  (tip 3318655/`2945348e…`, PIBD 10,203,442 / range 2,943,034 / kernels
  3,351,542). This binding is unchanged by build-14.
- Build-14 scope: R1 Rewind null previous-header fail-closed; R3/F13 exact
  archive-header readiness (wrong roots must not suppress regeneration); D2
  txpool NRD policy parity.
- Build-14 source is uncommitted worktree `HEAD 75a09d63…`; build process started
  13:08:30. No line3 source edits/build/tests.

## 1. Current patch hashes (read-only capture)

| File | SHA-256 |
|---|---|
| `src/PMMR/TxHashSetImpl.cpp` | `84FA42B1C1FF6536C9E366F4861F46FAD436C2338DC31C26029E0B2E2D63DBC1` |
| `src/P2P/Sync/StateSyncer.cpp` | `9C57E30F40ABA9EEE23D95863291E0729DBB35418BA1FF940B35E51FEBB03908` |
| `src/P2P/Sync/StateSyncer.h` | `F921D4FF71067FE13A4736AECB31939E63292D8C2D81C6CEB6691721F578D8B6` |
| `src/TxPool/TransactionPoolImpl.cpp` | `E9EBBB9AFE499050839B2152617644421B172CFC21C88B8B11B38FF377A8642B` |
| `src/TxPool/TransactionPoolImpl.h` | `DB1671E81B1681167F3329B24D0CDDE56584A207739A1B2DECB62E91001E7BDB` |
| `tests/src/PMMR/Test_RewindFailClosed.cpp` | `4842D0931FE2BFAE7AA42724385BCE20EB3E34EF8B0B28E12EB3F5EFA829F56D` |
| `tests/src/PMMR/Test_TxPoolNrdPolicy.cpp` | `5760E9F10E0AD6F76181F0C995497D6E46020FAB984E9F7F4C1CEE7D85180628` |
| `tests/src/PMMR/Test_PibdServing.cpp` | `D471EC3AAECD1FBE293F37B1563EEEF2EFAB63E748C422E04D65F562AFE1AA5D` |
| `tests/src/PMMR/CMakeLists.txt` | `EFBD1E637020FA17BE0E062F3E6224AD810CDA51471502D9291089642B11916F` |

Build-14 binary SHA/bytes/PE/source freeze not yet available; do not audit until
Lead reports BUILD OK.

## 2. Code-level acceptance

### R1 — Rewind missing previous header

Patch at `TxHashSetImpl.cpp:719-729`: reads previous hash, obtains previous
header, checks null, logs, throws `TXHASHSET_EXCEPTION_F`; only then advances
`m_pBlockHeader`. Direct caller `BlockProcessor::HandleReorg:253` stays inside
the uncommitted batch.

Audit gates:
- F11 `Rewind fails closed when the previous header is missing` PASS; exception
  is `TxHashSetException`, **no null deref/crash**.
- Verify batch rollback/tip/root unchanged and no partial state.
- Check no other `GetBlockHeader` deref/advance was weakened.

### R3 / F13 — exact-header readiness

Patch adds `StateSyncer::ValidateExistingArchiveForHeader` /
`IsReadyArchiveForHeader`: calls container check, extracts to private temp,
calls `PibdArchiveServer::LoadAndValidate(validationRoot, pHeader, ...)`, and
only then treats an existing final archive as ready. Both
`TryRecoverArchive` and `ArchivePibdState` call it before clearing the marker;
failure logs and regenerates/fails closed.

Audit gates:
- F13 `PibdArchiveServer::ExistingArchiveHeaderRootsValidated` PASS.
- Wrong-root/container-valid archive is **not** ready; regeneration path runs;
  no early return that suppresses recovery.
- Valid archive still idempotent; temp validation roots removed; no partial
  final; original copy untouched.
- Verify both call sites (`TryRecoverArchive`, `ArchivePibdState`) and no
  remaining `IsReadyArchive`-only success path in recovery.
- This is derived from build-14 source; audit only after build SHA/freeze.

### D2 — txpool NRD parity

Patch adds `TransactionPool::CheckNrdPolicy(kernels, headerVersion, environment)`
and wires `AddTransaction` to `Global::GetEnv()` and
`lastConfirmedBlock.GetVersion()`:
- any NRD + feature disabled -> `NRD_NOT_ENABLED`;
- any NRD + header version <4 -> `NRD_PRE_HF3`;
- else OK; plain/empty kernel sets OK.

Audit gates:
- F-test `TxPool NRD policy matches Rust verify_kernel_variants` PASS with exact
  matrix, including order: disabled wins over pre-HF3; mixed NRD rejected.
- `AddTransaction` integration rejects before duplicate/insert, returns
  invalid status, no pool mutation; confirm error path/logging.
- Cross-check Rust `verify_kernel_variants` order and that header version used is
  the intended head/confirmed version (not a different tip); flag if mismatch.
- D2 is txpool policy only; do not claim consensus/bodyless parity.

## 3. Build/test audit after Lead reports BUILD OK

1. Capture Tests/GrinNode SHA-256, bytes, PE timestamp, build log hash, source
   freeze (HEAD/status/aggregate). Build-14 binaries must be source-bound; no
   source edits during build.
2. Normal Release, isolated runner, quoted filters, `-KeepBinary`,
   `-FailOnNoTests`, exclusive port 3421:
   - full suite; expected bodyless-only exception unless fixed; record counts.
   - `Test_RewindFailClosed*` / exact F11 name;
   - `PibdArchiveServer::*` / exact F13 name;
   - `TxPool NRD policy*` / D2 exact;
   - existing `SpentOutputs*`, exact preflight, boundary, ArchivePublication,
     Nrd, KernelIndex, handler/schema/duplicate, advert.
3. ASan refresh on build-14 source; test seam/F13/D2 where linkable; archive/
   Nrd/Segment/P2P+advert/teardown; 0 reports; state ASan boundaries.
4. Copy-only A/B:
   - missing previous header via F11 fake DB;
   - wrong-root container-valid archive at final path (F13) must regenerate/
     fail closed, not suppress;
   - valid H3313440/H3315600 recovery regression on a fresh copy; no partial
     publish, tip/roots unchanged on failure.
5. Crash/publication/resource checklist remains separate (PUB-01..10,
   RES-01..08); not run until scoped.
6. Classify PASS/FAIL/NOT VERIFIED; build-13 live binding stays final; do not
   promote build-14 to live binding without a separately ordered live run.

## 4. Discrepancy watch

- Build-14 full suite should not regress build-13 257/256/1 bodyless-only baseline.
- F11 must be a real missing-header scenario (not a no-op), with null-check
  coverage.
- F13 must cover both readiness and the two recovery call sites, not just a
  direct helper.
- D2 test seam must match `AddTransaction` wiring; test-only seam PASS alone is
  insufficient.
- No source/build edits by line3; original `g09-live-02` untouched.

## 5. Evidence

- Line2 fail-closed audit: `tools/audit/nrd/evidence/rewind-preflight-fail-closed-audit-20261103.md`
  SHA `EB4DB197…`.
- Build-14 pre-audit JSON:
  `build14-failclosed-preauth-20261103.json`.
- Build-13 7B binding: `build13-7b-full-audit-20261103.md` / `.json`.
- This plan: `build14-independent-audit-plan-20261103.md`.
