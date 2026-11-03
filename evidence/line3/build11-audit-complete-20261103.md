# Build-11 audit completed — normal bodyless-only + ASan subset PASS

Status: **FAIL overall normal (known test-only bodyless); ASan subset PASS.**
Not a full G-09/Mainnet PASS; task-10/11 live recovery remains open.

**Post-audit root cause (2026-11-03):** build-11 contains the production
Release `SpentOutputs::Deserialize` NDEBUG assert-side-effect bug (old
`SpentOutput.h F3F38B44…`). This build's normal/ASan PASS does **not** validate
the fix. Build-12 must audit `D27A607D…` + regression `6157AC56…` and live
H=3313440/H=3315600 recovery. See
`task10-11-spent-deserialize-rootcause-20261103/README.md` and
`build12-preaudit-20261103.md`.

## Identity

- Tests `02DC3D9C00BB1E1B7256771A158F04F1DB3B739839A7AFF2391AEAA5DAAFC121`
  (9,438,720 B, PE `0x6AE94C3F`)
- GrinNode `565BC2DBF30C638F84B458012F1F07D091B8EF6DB9C933E821A58EF3CF6021A4`
  (7,843,328 B, PE `0x6AE94B43`)
- Build log retry `B274B34A…` ends BUILD OK; failed first attempt
  `ACED40F9…` (C1083) excluded.
- Source freeze `build-11-freeze-snapshot-final.json` `E5F7519C…`; 9-file
  uncommitted set; HEAD `6dbf8a7`; no source changes during successful retry.

## Normal

- Full **255 cases / 254 passed / 1 failed**; **4201 assertions / 4200 passed**;
  sole failure known test-only `Test_NrdBodylessRestart.cpp(103)`.
- All groups PASS: archive 8/187, Nrd 30/428, KernelIndex 5/74,
  GetKernelHandler 2/49, schema-v2 2/26, duplicate-range 1/10,
  `P2P::ArchivePublication*` 1/83, advert 2/15.
- Zero `mg_start`; user profile unchanged; manifests `evidence/line3/b11-*-01/`.
- Classification: **FAIL overall, explicit bodyless exception**, production/
  agreed-suite failures 0.

## ASan

- Source manifest `69B01EE6…` (1293 files); full ASan
  `full-asan-20261103-095356` BUILD_PASS 101.1 s; TestsMini `C76CCCF2…`;
  copied tree matches build-11 freeze **1003/1003, 0 mismatch**.
- PASS: Nrd* 21/365, archive 8/187, Segment 16/68, P2P+advert 7/61,
  ArchivePublication 1/83; teardown PASS; **0 ASan reports**.
- ASan-BLOCKED normal-only: PreCommit/FalseBan/BatchSafety/KernelIndex/
  BlockPipe/bodyless/kill; rocksdb/sysroot uninstrumented. Optional
  `tests_mini_prefix` not built (missing legacy source); teardown uses TestsMini.

## Still open — do not claim full G-09/Mainnet

- Task-10: live automatic horizon regeneration / crash-interrupted publication
  not demonstrated; ArchivePublication unit test passes, but no live recovery
  PASS.
- Task-11: actual Rewind key/DB view causing logged `0` still unknown; canonical
  preserved DB has count=1.
- Build-11 is only a normal/ASan build audit with bodyless exception. Existing
  7B PASS remains valid only for the fixed-H scenario.

## Evidence

- `normal-binary-build11-audit-20261103.md`
- `asan-build11-refresh-20261103.md`, `build11-asan-refresh-20261103.json`
- `build-11-freeze-snapshot-final.json`
- `evidence/line3/b11-*-01/`, `b11-asan-*-01/`,
  `full-asan-20261103-095356/`
