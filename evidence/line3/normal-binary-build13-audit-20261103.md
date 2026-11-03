# Normal-binary AUDIT build-13 (001DF4BC / F39BDFE5) — bound with explicit exception

Status: **FAIL overall, only known test-only bodyless**; zero
production/agreed-suite failures. Build-13 includes the SpentOutputs fix and
target-boundary preflight fix. No build/commit by line3.

## 1. Identity

| Artifact | SHA-256 | Size | PE timestamp |
|---|---|---|---|
| Tests.exe | `001DF4BC31F194DCDDE83FE736B06EB6FD942666F2BEFF62FF1FB5F1BB8DB3A8` | 9,465,856 B | `0x6AE957C5` |
| GrinNode.exe | `F39BDFE5A8DAB22BE969406874EDCEFD023770BE1A1BA6330D7506B2C2571201` | 7,846,912 B | `0x6AE956CB` |

Preserved: `C:\DHW\.tmp\build-13\Tests.exe` / `GrinNode.exe`.
Build log `8C54DD1759AE3D4E22B7EC29B6AF1A1F5FB81125727613B6739D1A32B70F2FF7`,
ends `build OK`.

Source freeze: `evidence/line3/build-13-freeze-snapshot-final.json`, HEAD
`6dbf8a7…`, 11-file set. Key fixes:
`SpentOutput.h D27A607D…`; `StateSyncer.cpp FDC5B4BB…`;
`ArchivePublication.h F1E43737…`; `Test_SpentOutputs.cpp A80D1D97…`.
No source changes during the build.

## 2. Normal audit

Serialized isolated runner, per-run HOME/TMP, `-KeepBinary`, quoted filters;
port 3421 free at start.

### Full

- **257 cases — 256 passed / 1 failed**
- **4219 assertions — 4218 passed / 1 failed**
- sole failure: `Test_NrdBodylessRestart.cpp(103)` (`Invalid version for
  header`) — known test-only bodyless exception.
- zero `mg_start`; user profile unchanged.

### Targeted / pre-registered groups — PASS

| Filter | Cases / assertions | Run |
|---|---|---|
| `SpentOutputs*` | 1 / 6 | `b13-spent-01` |
| exact preflight | 1 / 4 | `b13-preflight-01` |
| exact new boundary test | 1 / 8 | `b13-boundary-01` |
| `P2P::ArchivePublication*` | 1 / 83 | `b13-rewindpreflight-01` |
| `PibdArchiveServer::*` | 8 / 187 | `b13-archive-01` |
| `GetKernelHandler*` | 2 / 49 | `b13-getkernel-01` |
| `KernelIndex schema v2*` | 2 / 26 | `b13-schema-01` |
| `TxHashSet duplicate-excess*` | 1 / 10 | `b13-dup-01` |
| `Nrd*` | 30 / 428 | `b13-nrd-01` |
| `KernelIndex*` | 5 / 74 | `b13-kernidx-01` |
| `P2P::AdvertisedTip*` | 2 / 15 | `b13-advert-01` |

Exact boundary test:
`ArchivePublication preflight skips target body but fails rewound block gaps`
PASS (1/8). This directly covers the H=3313440 target-body boundary fix.

## 3. Classification / caveats

- Normal overall: **FAIL (bodyless-only)**; production/agreed-suite failures 0.
- Binding: Tests `001DF4BC…` + GrinNode `F39BDFE5…`, **with explicit exception**.
- Build-13 is the next final candidate; build-12 historical.
- Live recovery H=3313440/H=3315600 is separate acceptance; see recovery
  evidence. No ASan claim in this normal binding.
- Full G-09/Mainnet remains NOT VERIFIED.

## 4. Evidence

- `evidence/line3/b13-full-01/` — 257/256/1
- `b13-spent-01/`, `b13-preflight-01/`, `b13-boundary-01/`,
  `b13-rewindpreflight-01/`, `b13-archive-01/`, `b13-getkernel-01/`,
  `b13-schema-01/`, `b13-dup-01/`, `b13-nrd-01/`, `b13-kernidx-01/`,
  `b13-advert-01/`
- `evidence/line3/build-13-freeze-snapshot-final.json`
- this document
