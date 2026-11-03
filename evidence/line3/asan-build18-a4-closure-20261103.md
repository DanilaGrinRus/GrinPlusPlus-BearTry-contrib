# A4 closure — ASan TestsMini F13/archive validation

Status: **PASS — F13/archive validation now runs under ASan; 0 ASan reports.**
ASan-only build-config change; normal `/W4 /WX` and normal CMake untouched.

## Source / build identity

- Git HEAD `83e7575392bc0ee726b67375236b3ed03ebd4dfd`.
- Full ASan run `full-asan-20261103-211031`:
  - source manifest SHA `C1C321E9E43D9886F1EE792425D103613DB225CFD90F64D496B721E64B727EF6`,
    1,299 files;
  - **BUILD_PASS**, 94.2 s; manifest SHA `88991795…`;
  - targets Common/Core/Crypto/PMMR/etc ASan libs.
- TestsMini `C608090B72596DB3DC4762FD8BCB2E02B26A21C129B185D4881A5438414E8C8D`;
  linked manifest `36B2A663…`.
- Runtime `clang_rt.asan_dynamic-x86_64.dll` copied; teardown manifest PASS.

## ASan-only link changes

`tools/fuzz/build_linked_targets.ps1` tests_mini source list (script SHA
`E0585CE2…`), adding:

- `src/P2P/Sync/StateSyncer.cpp`
- `src/P2P/ConnectionManager.cpp`, `Connection.cpp`
- `src/P2P/Sync/Syncer.cpp`, `BlockSyncer.cpp`, `HeaderSyncer.cpp`
- `src/P2P/Pipeline/BlockPipe.cpp`, `TransactionPipe.cpp`, `TxHashSetPipe.cpp`
- `src/P2P/Seed/Seeder.cpp`, `PeerManager.cpp`, `HandShake.cpp`, `DNSSeeder.cpp`
- `src/P2P/P2PServerImpl.cpp`, `BlockLocator.cpp`
- `src/P2P/MessageProcessor.cpp`, `Dandelion.cpp`

`Test_PibdServing.cpp` was already present. Normal production CMake and
`/W4 /WX` were not changed; only the ASan link harness script is modified.

## ASan runs

| Run | Result | Cases / assertions | ASan |
|---|---|---|---|
| `PibdArchiveServer::ExistingArchiveHeaderRootsValidated` (F13) | **PASS** | 1 / 38 | 0 |
| `PibdArchiveServer::*` (archive) | **PASS** | 9 / 225 | 0 |
| Teardown regression | **PASS** | zero tests, exit 0 | 0 |

No AddressSanitizer reports in stdout/stderr; teardown `no_asan_report=true`,
`process_exit_zero=true`, `runtime_dll_copied=true`.

## Boundary

- ASan source is current source tree at HEAD, not normal build artifacts;
  normal Tests/GrinNode remain build18 `2DACE82F…` / `43E0C3A7…`, untouched.
- F13/archive ASan coverage closes the previous link boundary; F11/D2 and other
  normal-only tests remain outside TestsMini unless separately added.
- No line1 profile/build touch; no normal build.

## Evidence

- `evidence/line3/asan-build18-a4-closure-20261103/`
  (`full-asan-build-manifest.json` `88991795…`,
  `linked-build-manifest.json` `36B2A663…`,
  `runs.log` `ECABFDA9…`, `full-asan.log` `9EC6AD5B…`)
- runs: `evidence/line3/b18-asan-f13-01/`, `b18-asan-archive-01/`,
  `b18-asan-teardown-01/`
- full ASan evidence: `evidence/line3/full-asan-20261103-211031/`,
  `linked-asan-build-20261103-211630/`
