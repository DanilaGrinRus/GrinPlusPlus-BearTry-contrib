# Build-12 ASan refresh — PASS (subset), copied tree

Status: **PASS for the ASan subset only**; no full Tests.exe ASan claim; the
Release-mode SpentOutputs regression is covered by the normal Release binding,
not by TestsMini (source list excludes Test_SpentOutputs.cpp).

## Binding

- Source tree `C:\DHW\.tmp\asan-line3\gpp-tree`, build-12 frozen sources.
- Source manifest SHA
  `3DB12364A6F88401CFAF9148CD9495CFF5FFCAEDDF6A7778E50C1821D643C9BA`
  (1293 files).
- Full ASan `full-asan-20261103-101529`, **BUILD_PASS**, 96.4 s; git HEAD
  `6dbf8a7…`; build manifest
  `evidence/line3/full-asan-20261103-101529/build_manifest.json` SHA
  `08D8A055D73628D335D7158FD7C7E9ADF788D7B8056B807BA4870FF1B8BD145C`.
- Build-12 freeze `build-12-freeze-snapshot-final.json`; copied tree matches
  **1003/1003 entries, 0 mismatch**.
- TestsMini `CB09CEE89699DF0B35ABB3FD81C25470953A4C1B885C20DABC0AD930A92E7EBD`
  (PE `0x6AE95265`); linked manifest
  `evidence/line3/build12-asan-refresh-20261103/linked-build-manifest.json`
  SHA `BACA310B00F8F8516890029C1A941CAA4794435E103D936615276A68ECB3E1CA`.
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

`Test_SpentOutputs.cpp` is not in the TestsMini source list, so the Release
deserialize regression was **not** rerun under ASan. It PASSed in the normal
Release binary (`SpentOutputs*` 1/6). No ASan claim is made for that test.
ASan-BLOCKED normal-only remain PreCommit/FalseBan/BatchSafety/KernelIndex/
BlockPipe/bodyless/kill; rocksdb/sysroot uninstrumented.

## Evidence

- `evidence/line3/full-asan-20261103-101529/`
- `evidence/line3/build12-asan-refresh-20261103/`
- `evidence/line3/b12-asan-nrd-01/`, `b12-asan-archive-01/`,
  `b12-asan-segment-01/`, `b12-asan-p2p-01/`,
  `b12-asan-p2p-archive-01/`, `b12-asan-teardown-01/`
- [`normal-binary-build12-audit-20261103.md`](<normal-binary-build12-audit-20261103.md>)
