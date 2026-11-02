# CI pipeline run ci-c8c-01

Overall: **PASS_WITH_UNCOVERED**

| stage | status | note |
|---|---|---|
| provenance | PASS |  |
| normal-build | NOT RUN | provided binary; build not performed in this invocation |
| unit-regressions | PASS |  |
| full-suite | PASS | Full suite serialized. Wallet manager binds fixed 127.0.0.1:3421; concurrent runs/TIME_WAIT cause mg_start failures. Slatepack decrypt paths showed rare full-suite-only failures (line3 stress: 3/5 full PASS, 2/5 FAIL; 8/8 Slatepack-only PASS). |
| asan-subset | NOT RUN (STALE) | ASan source binding does not match current compiled sources (mismatch=19); rebuild requires explicit build authorization |
| fuzz-smoke | PASS (ADVISORY) | Bounded mutation smoke; coverage not measured; source binding may be stale (see asan-subset stage). Not a safety proof. |
| untested-zones | REPORTED |  |

Evidence: `C:\DHW\Gplus\src\grinplusplus\evidence\ci\ci-c8c-01\summary.json`
