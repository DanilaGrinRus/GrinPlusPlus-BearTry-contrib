# Build-13 ASan refresh — PASS (subset), copied tree

Status: **PASS for the ASan subset only**; no full Tests.exe ASan claim.
`Test_SpentOutputs.cpp` and the exact boundary test are not in TestsMini, so
those Release-mode regressions are covered by the normal build, not this ASan
run.

## Binding

- Source tree `C:\DHW\.tmp\asan-line3\gpp-tree`, build-13 frozen sources.
- Source manifest SHA
  `BFBF218697466C50A0422F36835E00545C0F316836F9C000491BBD92551D94C3`
  (1293 files).
- Full ASan `full-asan-20261103-104255`, **BUILD_PASS**, 87.5 s; git HEAD
  `6dbf8a7…`; build manifest
  `evidence/line3/full-asan-20261103-104255/build_manifest.json` SHA
  `1938D573297672641129B9D339C0142A50D7A999AE2D6D82E3A20E4493425891`.
- Build-13 freeze `build-13-freeze-snapshot-final.json`; copied tree matches
  **1003/1003 entries, 0 mismatch**.
- TestsMini `ECDD6D5E70C65317D3A0A30F5BC35D4E86227A44776504FCEA10966CD6CEDDC4`
  (PE `0x6AE958CA`); linked manifest
  `evidence/line3/build13-asan-refresh-20261103/linked-build-manifest.json`
  SHA `2B4A53DC6A5D18046C705D485D6BC997F32BA0C27E08F075007739E02AA8D8AD`.
- Optional `tests_mini_prefix` NOT BUILT (`TestMain_legacy_static.cpp`
  missing); teardown uses TestsMini.

## Results

| Filter / check | Result | Cases / assertions |
|---|---|---|
| `Nrd*` | **PASS** | 21 / 365 |
| `PibdArchiveServer::*` | **PASS** | 8 / 187 |
| `Segment*` | **PASS** | 16 / 68 |
| `Capabilities::*,MessageTypes::*,GrinVersion::*,P2P::AdvertisedTip*` | **PASS** | 7 / 61 |
| `P2P::ArchivePublication*` | **PASS** | 1 / 83 |
| Teardown (zero-test) | **PASS** | exit 0, no ASan, DLL copied |

ASan report occurrences: **0**.

## Coverage note

`Test_SpentOutputs.cpp` (Release deserialize/preflight/boundary) is not in the
TestsMini source list; normal Release PASS: `SpentOutputs*` 1/6, preflight 1/4,
boundary 1/8. ASan-BLOCKED normal-only remain PreCommit/FalseBan/BatchSafety/
KernelIndex/BlockPipe/bodyless/kill; rocksdb/sysroot uninstrumented.

## Evidence

- `evidence/line3/full-asan-20261103-104255/`
- `evidence/line3/build13-asan-refresh-20261103/`
- `evidence/line3/b13-asan-*-01/`
- [`normal-binary-build13-audit-20261103.md`](<normal-binary-build13-audit-20261103.md>)
