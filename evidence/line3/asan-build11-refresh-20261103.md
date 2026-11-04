# Build-11 ASan refresh — PASS (subset), copied tree

Status: **PASS for the ASan subset only**; no full Tests.exe ASan claim.
No commit; copied tree only; normal build-11 remains bodyless-only exception.

## Binding

- Source tree: `C:\DHW\.tmp\asan-line3\gpp-tree` (build-11 frozen sources).
- ASan source manifest SHA-256:
  `69B01EE653BBC2CFE815FEF26530BC40E6C4A3C9F775967197600EABB5479008`
  (1293 files).
- Full ASan build: `full-asan-20261103-095356`, **BUILD_PASS**, targets
  Common/Core/Crypto/PMMR, wall 101.1 s; git HEAD
  `6dbf8a7da067f35ba585371d82123ad21edf3a9a`.
- Build manifest:
  `evidence/line3/full-asan-20261103-095356/build_manifest.json`,
  SHA `E5137B7DCB4BFDA5FB55048C7B27B83697F24ED0ABDBC2D97B552790744255B3`.
- Build-11 freeze:
  `evidence/line3/build-11-freeze-snapshot-final.json`.
- ASan tree matches **all 1003 build-11 freeze entries**: 0 mismatch, 0 missing.
- TestsMini SHA:
  `C76CCCF2F8FB5F2B3154ABC5676AE0D877B20CFA3C3845E167CABC0C7D82C344`
  (8,343,552 B, PE `0x6AE94D5C`).
- Linked manifest SHA:
  `BB350D440BC16255CD447B77D5DF68E449306F2D69FA69D8CCF999BCDDDE3071`.
- ASan runtime: `3AA74252…`.

`tests_mini_prefix` optional target remains NOT BUILT (source
`TestMain_legacy_static.cpp` absent); teardown uses TestsMini and PASSes.

## Results

| Filter / check | Result | Cases / assertions |
|---|---|---|
| `Nrd*` | **PASS** | 21 / 365 |
| `PibdArchiveServer::*` | **PASS** | 8 / 187 |
| `Segment*` | **PASS** | 16 / 68 |
| `Capabilities::*,MessageTypes::*,GrinVersion::*,P2P::AdvertisedTip*` | **PASS** | 7 / 61 |
| `P2P::ArchivePublication*` | **PASS** | 1 / 83 |
| Teardown regression (zero-test) | **PASS** | exit 0, no ASan, runtime DLL copied |

ASan report occurrences across stdout/stderr: **0**.

## Boundaries

- ASan-BLOCKED normal-only: `Test_NrdPreCommit`, `Test_NrdFalseBan`,
  `Test_NrdBatchSafety`, `Test_KernelIndex`, BlockPipe, bodyless, kill.
- Database/ChainState/rocksdb and sysroot C libs uninstrumented.
- ArchivePublication tests are unit-level; they do not prove live recovery,
  crash-interrupted publication, or Mainnet readiness.
- No full Tests.exe ASan, fuzz/coverage, hosted-CI, or full G-09 PASS claim.
- Existing 7B PASS remains valid only for its fixed-H scenario.

## Evidence

- `evidence/line3/full-asan-20261103-095356/`
- `evidence/line3/build11-asan-refresh-20261103/` (linked manifest, runs log)
- `evidence/line3/b11-asan-nrd-01/`, `b11-asan-archive-01/`,
  `b11-asan-segment-01/`, `b11-asan-p2p-01/`,
  `b11-asan-p2p-archive-01/`, `b11-asan-teardown-01/`
- [`normal-binary-build11-audit-20261103.md`](<normal-binary-build11-audit-20261103.md>)
