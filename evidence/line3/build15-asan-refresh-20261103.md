# Build-15 ASan refresh — PARTIAL PASS (link boundary)

Status: **ASan subset PASS; F13/archive ASan NOT COVERED due TestsMini link
dependency.** Normal Release F13/archive PASS. Build-15 is fail-closed/D2
hardening, not a replacement live claim; build-13 remains final live binding.

## Binding

- Source manifest `BBEFC6FC8130D9A272628A9DF96D4E8B26C2A41FC3498682BB919DA8EF3C071A`
  (1295 files); full ASan `full-asan-20261103-133739` **BUILD_PASS** 102.7 s.
- Build-15 freeze `build-15-freeze-snapshot-final.json`; copied tree matches
  **1005/1005**, 0 mismatch.
- TestsMini `9D9AA171475CC6B5C713C80F4F4A839931A2039DB54E4857329C65B0B6C81418`
  (PE `0x6AE9821C`), temp harness with `Test_PibdServing.cpp` omitted.
- Linked manifest `B21DBB30…`; build log `5C70A1FB…`.

## Link boundary

Normal TestsMini includes `Test_PibdServing.cpp`; the new F13 test calls
`StateSyncer::ValidateExistingArchiveForHeader`. `StateSyncer.cpp` is not in
the TestsMini source/link set, so the attempted TestsMini link failed LNK2019.
A temp copy of the harness (not repo source) removed `Test_PibdServing.cpp` to
produce the subset binary. Therefore F13/archive ASan is **NOT RUN**, not PASS;
F13/archive passed under normal Release.

## Results

| Filter | Result | Cases / assertions |
|---|---|---|
| `Nrd*` | **PASS** | 21 / 365 |
| `Segment*` | **PASS** | 16 / 68 |
| P2P capabilities/messages/version/advert | **PASS** | 7 / 61 |
| Teardown | **PASS** | zero tests, exit 0, DLL copied |
| F13/archive | **NOT COVERED** | link boundary |
| F11/D2 | **NOT COVERED** | tests not in TestsMini |

ASan reports: **0** in executed subset.

## Boundaries

- ASan-BLOCKED normal-only: PreCommit/FalseBan/BatchSafety/KernelIndex/
  BlockPipe/bodyless/kill; plus F13/archive/F11/D2 for TestsMini link/source
  set.
- rocksdb/sysroot uninstrumented; `tests_mini_prefix` not built.
- No live promotion; build-13 full 7B remains final live binding.

## Evidence

- `evidence/line3/full-asan-20261103-133739/`
- `evidence/line3/build15-asan-refresh-20261103/`
- `evidence/line3/b15-asan-nrd-01/`, `b15-asan-segment-01/`,
  `b15-asan-p2p-01/`, `b15-asan-teardown-01/`
- `normal-binary-build15-audit-20261103.md`
