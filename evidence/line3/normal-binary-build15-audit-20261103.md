# Normal-binary AUDIT build-15 (920F000F / A7D28BCC) — bodyless-only

Status: **FAIL overall, only known test-only bodyless**; zero production/
agreed-suite failures. Build-15 is a source-bound fail-closed/D2 candidate, not
a replacement live claim. Build-13 remains final live binding. No build/commit
by line3.

## Identity / source binding

| Artifact | SHA-256 | Size | PE timestamp |
|---|---|---|---|
| Tests.exe | `920F000F38958DA545ED3BC57DB796A0062CB5EF6E304E7DB354DAD065E6227D` | 9,495,552 B | `0x6AE9806A` |
| GrinNode.exe | `A7D28BCC2F4A1B8FC5D57083E0F5F43F75A81BDF1771C946BE041A4CADA7246C` | 7,850,496 B | `0x6AE956CB` |

- Tests built 13:34:19 after frozen `Test_PibdServing.cpp` 13:24:59
  (`351083D5…`); object 13:33:18. Source binding is valid.
- Build log `build-console-f13fix-20261103-132502.log`
  `5C70A1FB…`, BUILD OK.
- Source freeze `build-15-freeze-snapshot-final.json` (1005 entries,
  aggregate `D6A66439…`).

## Normal audit

Serialized isolated runner, quoted filters, `-KeepBinary`, `-FailOnNoTests`.

- Full: **260 cases / 259 passed / 1 failed**; **4267 assertions / 4266
  passed / 1 failed**. Sole failure known test-only
  `Test_NrdBodylessRestart.cpp(103)`; zero `mg_start`.
- F11 `Rewind fails closed when the previous header is missing`: **PASS** 1/2.
- F13 `PibdArchiveServer::ExistingArchiveHeaderRootsValidated`: **PASS** 1/38.
- D2 `TxPool NRD policy matches Rust verify_kernel_variants`: **PASS** 1/8.
- `PibdArchiveServer::*`: **PASS** 9/225.
- PASS groups: SpentOutputs 1/6, exact preflight 1/4, boundary 1/8,
  ArchivePublication 1/83, GetKernel 2/49, schema 2/26, duplicate 1/10,
  Nrd 30/428, KernelIndex 5/74, advert 2/15.

Build-14 invalid/stale artifact stays historical; it is not counted.

## ASan

- Full ASan `full-asan-20261103-133739` **BUILD_PASS** 102.7 s; source manifest
  `BBEFC6FC…` (1295 files); copied tree matches build-15 freeze 1005/1005.
- TestsMini `9D9AA171…` built from a temp harness with Test_PibdServing omitted:
  the normal source-copy F13 test references
  `StateSyncer::ValidateExistingArchiveForHeader`, which is not in the
  TestsMini link set (LNK2019). This is a harness/link boundary, not a product
  failure.
- ASan PASS: Nrd 21/365, Segment 16/68, P2P+advert 7/61; teardown PASS;
  **0 ASan reports**. Archive/F13 ASan **NOT COVERED**; F13/archive passed in
  normal Release instead.
- Optional `tests_mini_prefix` not built (missing legacy source).

## Classification

- Normal: **FAIL bodyless-only**; production/agreed-suite 0.
- Build-15 source-bound, but fail-closed/D2 hardening only; no live replacement.
- Build-13 full 7B live binding remains final.
- Crash/publication/resource matrix remains prepared, NOT RUN.
- No full G-09/Mainnet claim. Original `g09-live-02` untouched.

## Evidence

- `evidence/line3/b15-full-01/` (260/259/1)
- `b15-rewindf11-01/`, `b15-f13-01/`, `b15-d2-01/`,
  `b15-pibdarchive-01/`, `b15-archivepub-01/`, `b15-spent-01/`,
  `b15-preflight-01/`, `b15-boundary-01/`, `b15-getkernel-01/`,
  `b15-schema-01/`, `b15-dup-01/`, `b15-nrd-01/`, `b15-kernidx-01/`,
  `b15-advert-01/`
- `evidence/line3/build-15-freeze-snapshot-final.json`
- `build15-asan-refresh-20261103.md` / `.json`
