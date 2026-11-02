# TestMain UAF fix + ASan teardown regression (Line 3, 2026-11-03)

Lead decision: option (b) — move the test runtime from a static object to a
local object inside `main()`; do NOT change `src/Core/Global.cpp` production
lifetime. The production static-init/deinit hazard remains a separate
NOT VERIFIED finding.

## 1. Change

`tests/src/TestMain.cpp`:
- before: `#define CATCH_CONFIG_MAIN` + `static TestRuntime RUNTIME;` (implicit
  static destructor called `Global::Shutdown()` after Global.cpp's statics were
  already destroyed → ASan heap-use-after-free on every exit).
- after: `#define CATCH_CONFIG_RUNNER`; `TestRuntime runtime;` is a **local**
  object in `main()`, destroyed before static destruction;
  `return Catch::Session().run(argc, argv);`.
- repo file sha256 `6BAC17446DE696518FD364220569185F43769B813F59A25372037FBC53D8527B`;
  diff: 18 insertions, 2 deletions. `src/Core/Global.cpp` untouched.
- copied ASan tree synced to the same file; full-ASan source manifest updated to
  `A827E5008922762F36338606230C21BFDF97E3787585E5AC894526B7F529B982`
  (build manifest sha `8CA486B53A8E367E3228252BDE8620BE30B9BE78CB31AD433F6F6DF993223F03`,
  evidence `full-asan-20261103-032246/`, BUILD PASS 84.4 s).

## 2. After fix — PASS

`TestsMini.exe` sha256 `50076B06C766E4EE07C1D91C60853E32B6E8CEEA2BA4A3B9D076FFB55F2E7283`
(7 140 352 B; fixed `TestMain.cpp` + selected production tests, linked against
the full ASan libs). Isolated runs, profile unchanged, stderr empty, exit 0:

| run id | filter | result | manifest sha256 |
|---|---|---|---|
| `asan-testsmini-postfix-nrd-01` | `Nrd*` | 12 cases / 329 assertions PASS | `64AE37CD…` |
| `asan-testsmini-postfix-segment-01` | `Segment*` | 16 / 68 PASS | `D803DAA0…` |
| `asan-testsmini-postfix-p2p-01` | `Capabilities::*,MessageTypes::*,GrinVersion::*` | 5 / 46 PASS | `5C798783…` |

The exe is preserved next to each manifest (`-KeepBinary`) and in
`evidence/line3/full-asan-linked-artifacts/TestsMini.exe`.

## 3. Zero-test ASan teardown regression — PASS

`tools/fuzz/run_asan_teardown_regression.ps1` runs an ASan test exe with a filter
that matches nothing and requires: exit 0, `No tests ran`, no `AddressSanitizer`
in stdout/stderr, isolated HOME, user profile unchanged, ASan runtime DLL copied.

- AFTER (fixed exe `50076B06…`), run `asan-teardown-postfix-01`:
  **PASS**, manifest sha `FC7C5BCE7BA6C62CBC9EAFF2FE4166B12A8149C30F75A19A50328B4A2408185C`,
  `teardown_regression.json` sha `C71CD1BA…`, stderr sha256 = SHA-256(empty).
- BEFORE / negative control (pre-fix `TestMain` kept in the copied tree as
  `TestMain_legacy_static.cpp`, exe `C8C1096311D7F49B5C71F52CB3BAF7F60A471891D279C30BB6B990761B1C29F8`),
  run `asan-teardown-prefix-01`: **FAIL as expected** — `AddressSanitizer:
  heap-use-after-free`, `WRITE` in `shared_ptr<Context>::{dtor}` at
  `TestMain_legacy_static.cpp:31`, `freed by Global::Shutdown()`.
  This proves the regression detects the original bug; the run is preserved as
  negative-control evidence (manifest sha `5C7AEEC1…`).

Pre-fix failing evidence is preserved unchanged:
`evidence/line3/asan-testsmini-nrd-01/` (12/329 tests passed, exit 1 due to UAF),
`evidence/line3/asan-testsmini-empty-01/` (zero tests, exit 1),
`evidence/line3/diag_runtime_lifetime.*` (refcount diagnostics).

## 4. Normal build impact — classified separately

- Compile-only check of the fixed `tests/src/TestMain.cpp` with the normal
  profile (`/W4 /WX /O2 /Ob2 /MT /GS /EHsc /std:c++17`, no ASan): **exit 0**
  (`C:\DHW\.tmp\asan-line3\normal-profile\`, log sha `B79C7B3A…`).
- Full normal `Tests.exe` build with `tools/build/build.ps1`: **NOT VERIFIED** in
  this pass (shared build slot not used; line2/line1 own it). The next normal
  build will include this source change; its Tests.exe SHA and output must be
  recorded fresh — do not attach these ASan/normal-TU results to a new normal
  binary SHA.
- Behavioural intent: same TestRuntime setup and same Catch2 session; only the
  lifetime/entry-point construction changed.

## 5. Residual finding (production) — NOT VERIFIED

`src/Core/Global.cpp` keeps process-wide static `SHARED_CONTEXT` (strong) and
`GLOBAL_CONTEXT` (weak). Using `Global::GetConfig/GetContext/Shutdown` after
static deinitialization (or from another static destructor) can touch a released
control block. Not observed in production runs; not fixed in this pass. Needs an
owner/Lead decision and a dedicated production-lifetime change.

## 6. CI addition

Add to the ASan job after build:

```
pwsh tools/fuzz/run_asan_teardown_regression.ps1 -ExePath <ASan test exe> -RunId ci-teardown-$rev
```

PASS conditions are the seven checks in `teardown_regression.json`; any ASan
report, non-zero exit or matched test is FAIL.
