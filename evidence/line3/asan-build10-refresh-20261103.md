# Build-10 ASan refresh — PASS (subset), copied tree

Status: **PASS for the ASan subset only**. This is not a full Tests.exe ASan
claim. No commit was made and no shared build directory was used.

## Source and build binding

- Source tree: `C:\DHW\.tmp\asan-line3\gpp-tree` (copied build-10 sources).
- ASan source manifest SHA-256:
  `776B0DB3356FE953930E91B464F9BD6C5C8D8B30CACA9D53EED22AB785A37E7F`
  (1292 files, `source_tree_manifest.txt`).
- Build manifest: `evidence/line3/full-asan-20261103-090921/build_manifest.json`,
  SHA `88ADE17A7FBAA2542E9BB93A72B18C14E3F5D0CCE0D9045EA1A28694590BEA24`.
- Full ASan build: `BUILD_PASS`, targets
  Common/Core/Crypto/PMMR, wall 98.649 s, build manifest
  `full-asan-20261103-090921`.
- Build git HEAD: `77b397032bb48317c6b944582ae9722dec5e85da` (build-10
  API/index commit).
- Build-10 source freeze:
  `evidence/line3/build-10-freeze-snapshot-final.json`, SHA
  `0E51D63287AF8F96B3094A9F24B120C2F663736CAA1E9B4DF5E8C2B6EB6CCA5E`,
  1002 entries, aggregate
  `F1E47615D024E566CFF6EB2E908326364529813CF85111F7280B41EC6ADD869C`.
- ASan copied tree matches all 1002 build-10 freeze entries: **0 mismatch,
  0 missing**. HEAD advanced from `ee843f05` to `77b3970`; compiled-root
  contents are identical.
- ASan runtime: MSVC 14.44 dynamic ASan DLL SHA
  `3AA742521E8CBD23252F0545AA51D5A15624A3E4430FBE2F81567AFAEB3FC59B`.

## TestsMini / linked targets

- TestsMini SHA:
  `497353ECC81E3F88E414773F29FE0618C59707DB45FD9643418A89CA4758007B`,
  8,343,552 B, PE `0x6AE942E7`.
- Linked manifest:
  `C:\DHW\.tmp\asan-line3\fuzz-linked\build_manifest.json`, SHA
  `96353019A62E2158E558CDE189A635DF70162DE726615D65A130EEE47BB38382`.
- Built: `p2p_responses` `898C56CC…`, `segment_validate` `F8D880B7…`,
  `diag_lifetime` `72B96DA2…`, TestsMini above.
- `tests_mini_prefix` was not built: the script references
  `tests/src/TestMain_legacy_static.cpp`, which is absent in the build-10
  source tree. This optional prefix target is **NOT BUILT / not used**; the
  teardown regression uses TestsMini and PASSes. This is a harness-script
  mismatch, not a source/product failure.

## ASan subset results

| Filter / check | Result | Assertions / cases |
|---|---|---|
| `Nrd*` | **PASS** | 365 assertions / 21 cases |
| `PibdArchiveServer::*` | **PASS** | 187 / 8 |
| `Segment*` | **PASS** | 68 / 16 |
| `Capabilities::*,MessageTypes::*,GrinVersion::*,P2P::AdvertisedTip*` | **PASS** | 61 / 7 |
| Teardown regression (zero-test filter) | **PASS** | exit 0; no ASan; runtime DLL copied |

- ASan report occurrences across all subset stdout/stderr: **0**.
- Teardown checks all true: runner exit 0, manifest PASS, process exit 0,
  `no_asan_report`, `zero_tests_ran`, profile unchanged, isolated data dir,
  runtime DLL copied. Evidence:
  `evidence/line3/b10-asan-teardown-01/teardown_regression.json`.
- Run manifests: `evidence/line3/b10-asan-nrd-01`,
  `b10-asan-archive-01`, `b10-asan-segment-01`, `b10-asan-p2p-01`.

## Boundaries — not proven by this ASan run

- ASan-BLOCKED normal-only tests (link boundary via
  BlockChain→Database→non-ASan rocksdb): `Test_NrdPreCommit`,
  `Test_NrdFalseBan`, `Test_NrdBatchSafety`, `Test_KernelIndex`,
  BlockPipe, bodyless-restart, kill-recovery. These remain covered by the
  normal suite only.
- Database/ChainState/rocksdb are not ASan-instrumented.
- Sysroot C libraries (zlib, minizip, libsodium, secp256k1-zkp, roaring) are
  uninstrumented.
- No claim for full Tests.exe under ASan, no fuzz/coverage claim, no hosted CI
  claim. This ASan refresh does not alter the build-10 normal bodyless
  exception.

## Reproduction

```powershell
pwsh tools/fuzz/build_full_asan.ps1 -FreshTree
pwsh tools/fuzz/build_linked_targets.ps1 -Clean
$env:ASAN_OPTIONS='detect_leaks=0'
pwsh tools/fuzz/run_asan_teardown_regression.ps1 -ExePath .tmp\asan-line3\fuzz-linked\TestsMini.exe -KeepBinary
# Nrd*, PibdArchiveServer::*, Segment*, Capabilities::*,MessageTypes::*,GrinVersion::*,P2P::AdvertisedTip*
# via tools/build/run-tests-isolated.ps1 -ExePath TestsMini -TestArgs @(...) -FailOnNoTests -KeepBinary
```
