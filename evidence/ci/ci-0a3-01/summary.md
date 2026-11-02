# CI pipeline run ci-0a3-01

Overall: **FAIL**

| stage | status | note |
|---|---|---|
| provenance | PASS |  |
| normal-build | NOT RUN | provided binary; build not performed in this invocation |
| unit-regressions | PASS |  |
| full-suite | FAIL | Full suite exclusive (lock + port 3421 preflight). Only pure wallet-port/Tor failures are ENV_DEPENDENT; rare Slatepack-decrypt failures are NOT blanket-attributed to environment and remain FAIL/open flake. |
| asan-subset | PASS | ASan rebuilt from current compiled sources; subset + zero-test teardown only. Blocked tests stay covered by the normal-binary suite. |
| fuzz-smoke | PASS (ADVISORY) | Bounded mutation smoke; coverage not measured; source binding may be stale (see asan-subset stage). Not a safety proof. |
| untested-zones | REPORTED |  |

Evidence: `C:\DHW\Gplus\src\grinplusplus\evidence\ci\ci-0a3-01\summary.json`
