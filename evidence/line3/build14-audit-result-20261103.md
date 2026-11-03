# Build-14 audit result — FAIL / INVALID SOURCE BINDING

Status: **FAIL / BUILD14_ARTIFACT_STALE_RELATIVE_TO_SOURCE**. Build-13 remains
the final live binding. No build/commit/source edit by line3. ASan refresh was
**not started** because the normal build is not source-bound.

## Artifact identity (as supplied)

- Tests `50359F14F82958BF5891D57CF67A132F137F9A2C64189286648B708D7F667ADC`
  (9,494,528 B)
- GrinNode `A7D28BCC2F4A1B8FC5D57083E0F5F43F75A81BDF1771C946BE041A4CADA7246C`
  (7,850,496 B)
- Build log `build-console-build14-fix-20261103-131251.log`
  SHA `40A45B4A…`, `build OK`.

## Normal audit

- Full: **260 cases / 258 passed / 2 failed**; **4264 assertions / 4262
  passed / 2 failed**.
  - known test-only bodyless `Test_NrdBodylessRestart.cpp(103)`;
  - **new F13 failure** `Test_PibdServing.cpp` good-header
    `ValidateExistingArchiveForHeader` false.
- F11 `Rewind fails closed when the previous header is missing`: **PASS**.
- D2 `TxPool NRD policy matches Rust verify_kernel_variants`: **PASS**.
- F13 exact: **FAIL/1** (35 assertions, 34 passed).
- `PibdArchiveServer::*`: 9 cases / 8 passed / 1 failed (222/221).
- PASS groups: `SpentOutputs*` 1/6, exact preflight 1/4, boundary 1/8,
  ArchivePublication 1/83, GetKernel 2/49, schema 2/26, duplicate 1/10,
  Nrd 30/428, KernelIndex 5/74, advert 2/15.

## Root cause of F13 failure: stale test artifact

- `tests/src/PMMR/Test_PibdServing.cpp` mtime **13:24:59**, current hash
  `351083D5DADCF757FC389CEC881310EDED2618C627E24F139A6D72D9ECFE88F5` (computed at audit), while `Tests.exe` mtime **13:22:33** and the
  compiled `Test_PibdServing.obj` mtime **13:21:38**.
- Build log compiled `Test_PibdServing.cpp` before the current source edit.
- Failure log shows zip leaf `output/pmmr_leaf.bin.edc758c1370d` while
  `TxHashSetZip::Extract` searched for
  `output/pmmr_leaf.bin.000000000000`: the binary contains the pre-fix test path
  (no `headerShortHash` argument / default genesis short hash), not current F13
  source.
- Therefore F13 failure is **not valid evidence against the current fix**, but
  the build14 artifact is also **not auditable** as a source-bound build.

## Required action

1. Do not promote build14; do not run ASan/full live on this stale artifact.
2. Line1/Lead must rebuild after source freeze; capture new Tests/GrinNode SHA,
   build log, source freeze.
3. Re-run full/F11/F13/D2 and ASan on the new artifact; then copy-only
   H3313440/H3315600 regression.
4. Build13 final live binding remains unchanged throughout.

## Evidence

- `evidence/line3/b14-full-01/` — 260/258/2
- `evidence/line3/b14-f13-01/` — F13 failure, log expected `.000000000000`
- `evidence/line3/b14-rewindf11-01/`, `b14-d2-01/` — PASS
- `evidence/line3/b14-pibdarchive-01/` — 8/9
- `evidence/line3/b14-*-01/` other PASS runs
- `build14-failclosed-preauth-20261103.*`, `build14-independent-audit-plan-20261103.*`
- `build-14-freeze-snapshot-final.json` (contains current source snapshot, but
  binaries are from before the source edit; **not a valid binding**)
