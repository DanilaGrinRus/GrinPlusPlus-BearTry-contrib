# CI pipeline run ci-c8c-02

Overall: **PASS_WITH_UNCOVERED**

| stage | status | note |
|---|---|---|
| provenance | PASS |  |
| normal-build | NOT RUN | provided binary; build not performed in this invocation |
| unit-regressions | PASS |  |
| full-suite | PASS | Full suite exclusive (lock + port 3421 preflight). Only pure wallet-port/Tor failures are ENV_DEPENDENT; rare Slatepack-decrypt failures are NOT blanket-attributed to environment and remain FAIL/open flake. |
| asan-subset | NOT RUN (STALE) | ASan source binding does not match current compiled sources (mismatch=3); rebuild requires explicit build authorization |
| fuzz-smoke | PASS (ADVISORY) | Bounded mutation smoke; coverage not measured; source binding may be stale (see asan-subset stage). Not a safety proof. |
| untested-zones | REPORTED |  |

Evidence: `C:\DHW\Gplus\src\grinplusplus\evidence\ci\ci-c8c-02\summary.json`
