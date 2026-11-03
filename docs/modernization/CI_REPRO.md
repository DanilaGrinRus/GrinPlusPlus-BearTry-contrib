# CI_REPRO — изоляция тестов, ASan/fuzz smoke и воспроизводимость (линия 3)

Статус на 2026-11-03. Документ фиксирует минимальный воспроизводимый контур
проверок и границы доказательств. Build/unit/ASan/fuzz/live — **разные**
доказательства; они не объединяются в один безусловный PASS.

## 1. Изоляция тестового runtime (обязательное требование)

`tests/src/TestMain.cpp` в static-init выполняет:

```cpp
FileUtil::RemoveFile(Config::DefaultDataDir(Environment::AUTOMATED_TESTING));
```

- `FileUtil::RemoveFile` — это `std::filesystem::remove_all` (рекурсивное удаление),
  а не удаление одного файла;
- `Config::DefaultDataDir(env)` = `FileUtil::GetHomeDirectory()/.GrinPP/<ENV>`;
- `FileUtil::GetHomeDirectory()` на Windows собирается **только** из
  `HOMEDRIVE` + `HOMEPATH` (не из `USERPROFILE`).

Следствия:

1. Запуск `Tests.exe` без изоляции удаляет/создаёт `.GrinPP\AUTOMATED_TESTING` в
   видимом HOME. Наблюдался реальный каталог
   `C:\Users\nurli\.GrinPP\AUTOMATED_TESTING` (mtime 2026-11-02 21:43) — след
   прежнего запуска. Повторять это нельзя.
2. Если `HOMEDRIVE`/`HOMEPATH` пусты, путь становится **относительным**
   (`.GrinPP\AUTOMATED_TESTING` от cwd) — тоже недопустимо.
3. Тесты массово используют `fs::temp_directory_path()` с **фиксированными**
   именами (`pibd-sync-test`, `pibd-serving-test`, `leafset-roaring-units.bin` и
   т.д.), поэтому общий TMP между параллельными запусками — коллизия.

Скрипт `tools/build/run-tests-isolated.ps1` выполняет требования:

- RunRoot обязан лежать внутри `C:\DHW\.tmp` (одноразовый каталог), каталог
  прогона не должен существовать; проверяется отсутствие reparse points по всей
  цепочке пути;
- создаёт `home/`, `tmp/`, `cwd/`, копирует exe и проверяет
  SHA-256(source) == SHA-256(copy);
- выставляет `HOMEDRIVE`, `HOMEPATH`, `USERPROFILE`, `HOME`, `TMP`, `TEMP`,
  `TMPDIR` внутрь каталога прогона; cwd — одноразовый `cwd/`;
- снимает снимок `%USERPROFILE%\.GrinPP` до/после (recursive listing + mtime +
  SHA-256 листинга) и требует, чтобы он **не изменился**;
- фиксирует точный SHA/PE timestamp exe, env, cwd, command line, exit code
  (dec + hex), stdout/stderr (пути, размеры, SHA-256), Application Error events
  по уникальному пути копии и пишет `manifest.json` в
  `evidence/line3/<run-id>/`.

```powershell
pwsh tools/build/run-tests-isolated.ps1 -RunId <id> [-KeepBinary]
```

Признак PASS для тестового прогона: exit code 0, `user_profile_unchanged=true`,
`test_data_dir_created=true`, ожидаемая строка Catch2 «All tests passed» (или
«Passed all N test cases …» для compact reporter) в stdout. **Число assertions
не является стабильным идентификатором** (см. `EVIDENCE_LINE3.md`): для одного и
того же бинарника заявленные 205/3700 не воспроизвелись, воспроизводится
205/3358.

Если тест пытается очистить путь вне одноразового HOME — прогон остановить и
исправить изоляцию; пользовательские данные вручную не чистить.

## 2. Обычная сборка (не меняется)

Обычная сборка и `/W4 /WX` не изменялись:

```powershell
pwsh tools/build/build.ps1 -Target GrinNode
pwsh tools/build/build.ps1 -Target Tests
```

Общий build directory `C:\DHW\Gplus\.toolchain\grinpp-build` — только одна линия
за раз; в этой работе он не использовался. SANITIZER-профиль собирается в
`C:\DHW\.tmp\asan-line3\fuzz` (см. `SANITIZERS.md`).

## 3. Минимальный CI / repro smoke

### Job A — обычная сборка + unit

1. `git -C <repo> rev-parse HEAD` и `git status --porcelain` → в evidence.
2. `pwsh tools/build/build.ps1 -Target GrinNode` и `-Target Tests` (слот
   эксклюзивный, без параллельных сборок в том же дереве).
3. `pwsh tools/build/run-tests-isolated.ps1 -RunId ci-$rev` → exit 0.
4. Артефакты: `manifest.json` прогона, stdout/stderr, SHA Tests.exe/GrinNode.exe.

### Job B — ASan/fuzz smoke (отдельный каталог, без shared build dir)

1. `pwsh tools/fuzz/build_asan.ps1` → 4 exe + `build_manifest.json`.
2. `fuzz_bytebuffer.exe --asan-selftest` → ожидается ASan report (stderr
   содержит `AddressSanitizer`) и exit 1; без отчёта — FAIL инструментации.
3. `pwsh tools/fuzz/run_fuzz_smoke.ps1 -Iterations 200 -MaxSeconds 30 -RunId ci-fuzz-$rev`
   → `result=PASS`; условия FAIL: любой exit≠0, timeout, `AddressSanitizer` в
   stderr/stdout, либо отсутствие ASan DLL.
4. `pwsh tools/fuzz/run_asan_teardown_regression.ps1 -ExePath <ASan test exe> -RunId ci-teardown-$rev`
   → PASS только при exit 0, `No tests ran`, отсутствии `AddressSanitizer`,
   изолированном HOME и неизменном профиле пользователя (7 проверок в
   `teardown_regression.json`). Pre-fix negative control должен FAIL.
5. Артефакты: `evidence/line3/<run-id>/manifest.json`, логи кампаний и
   one-shot, SHA exe и corpus.

Ориентировочное время: Job B 3–5 мин на 4 targets по 200 итераций.

### Что каждый шаг доказывает

| Шаг | PASS означает | PASS НЕ означает |
|---|---|---|
| обычная сборка | код компилируется с `/W4 /WX` на зафиксированном toolchain | корректность сети/протокола |
| изолированный unit | тесты прошли в одноразовом HOME, пользовательский профиль не тронут | live-совместимость, отсутствие утечек |
| ASan selftest | ASan реально инструментирует harness | что в коде нет OOB |
| fuzz smoke | за бюджет/seed не найдено crash/timeout на объявленных targets | безопасность компонента; coverage не измерялась |

## 4. Воспроизводимость и evidence

- Каждый результат привязан к `git rev-parse HEAD` + `git status --porcelain`
  (включая чужие незакоммиченные изменения!), SHA-256 реально запущенных
  бинарников (`Tests.exe`, fuzz exe), seed/лимитам и версии toolchain.
- Если SHA бинарника изменился (параллельная сборка), старые логи к новому
  бинарнику не привязывать: повторить прогон.
- Временные каталоги прогонов не являются вечным архивом: итоговые manifest и
  raw-логи копируются в `evidence/line3/<run-id>/`; exe-хэш фиксируется в
  манифесте. Для key-run допускается `-KeepBinary` (копия Tests.exe в evidence).
- Секреты/seed/приватные ключи/mainnet-каталоги в CI не используются; все
  сетевые проверки — только testnet/Floonet с отдельными профилями.
- Windows Application Error events и WER Report.wer при crash копировать в
  evidence сразу: бинарник может быть перезаписан следующей сборкой.

## 5. Текущий статус линии 3

| Пункт | Статус | Комментарий |
|---|---|---|
| Кодовая проверка изоляции (`RemoveFile`=recursive, HOME из HOMEDRIVE/HOMEPATH) | PASS | см. §1, доказано чтением кода |
| Изолированный прогон `Tests.exe` (SHA 4F75DCD4…), профиль не тронут | PASS | 4 прогона, exit 0; 205 cases / 3358 assertions |
| Заявленные 205/3700 для точного SHA | NOT VERIFIED | не воспроизводится; сырой формат подтверждён, привязки к SHA нет |
| `tests-proof-boundaries.log` как доказательство 205/3700 | FAIL (stale) | лог содержит 203/3676 |
| `Tests.exe` 0xC0000409 | классифицировано: abort/FAST_FAIL (exception data 7); root cause NOT VERIFIED | событие/ WER есть, бинарник не сохранён |
| Отдельный MSVC ASan build + доказанная инструментация | PASS | `/W4 /WX /Zi /MT /fsanitize=address`, selftest с отчётом |
| Fuzz targets + corpus + bounded runs | PASS (smoke) | 5 targets, 1300 мутаций + 69 one-shot, 0 ASan/timeout; coverage не измерялась |
| NRD production rules под ASan (`nrd_rules`) | PASS (smoke) | 500 итераций + 26 seeds из `nrd_cases.json`; exe `4D7275E7…` |
| Полная ASan-сборка проекта (copied tree, libs Common/Core/Crypto/PMMR) | PASS | `BUILD_PASS` 75.6 s; LNK2038 решён; manifest `210FBAB7…` |
| Production-тесты под ASan (TestsMini: NRD diff+rebuild, Segment, P2P compat) | PASS | 12/329, 16/68, 5/46; exit 0, ASan-report 0 |
| Archive-server тесты линии 1 под ASan (TestsMini + Test_PibdServing) | PASS | fixed fixture: lease 1/16, `PibdArchiveServer::*` 7/168; pre-fix fixture bug воспроизведён на normal SHA `2032536D…` |
| ASan refresh build-07 sources (`57590D1E…`, TestsMini `19FE6005…`) | PASS (build-07-only) | Nrd* 21/365, archive 8/187, Segment 16/68, P2P 7/61 (incl. advert), teardown PASS, 0 ASan. Blocked: NrdPreCommit/NrdFalseBan/NrdBatchSafety/KernelIndex (BlockChain→Database→rocksdb). Post-05:50 WIP → STALE (mismatch=3); refresh приостановлен до freeze/build-08 |
| **Build-08 historical** 7994F191… (Pong fix + task-8 tests) | **FAIL (historical)** | full **249 cases / 237 passed / 12 failed** (2 прогона): 11× wallet/Tor `mg_start 127.0.0.1:3421` + 1× test-only `Test_NrdBodylessRestart` header-version defect. PASS: Nrd 30/428, KernelIndex 3/48, archive 8/187, advert 2/15, BlockPipe 3/16. ASan refresh cancelled per Lead (stale after fixes). Evidence: `normal-binary-build08-historical-audit.md` |
| **Authoritative normal binary** build-09 0A3B7D66… + GrinNode F90EEF38… | **PASS WITH EXPLICIT EXCEPTION** (не fully green) | full 249 cases / **248 passed / 1 failed** (4033/4032) ×2; единственный провал — test-only `Test_NrdBodylessRestart` (short-chain header version); bodyless-NRD + kill-recovery NOT VERIFIED; production/agreed suites 0 fails. PASS: Nrd 30/428, KernelIndex 3/48, archive 8/187, advert 2/15, BlockPipe 3/16. Evidence: `normal-binary-0a3-audit-20261103.md` |
| **FINAL CANDIDATE build-10** Tests C17C3BC0… + GrinNode 60D7F7A6… | **FAIL overall — bodyless-only; bound with explicit exception** | clean serialized full **254 cases / 253 passed / 1 failed** (4118/4117); zero `mg_start`; only test-only `Test_NrdBodylessRestart.cpp(103)`. All filters PASS: archive 8/187, Nrd 30/428, KernelIndex 5/74, GetKernelHandler 2/49, schema-v2 2/26, duplicate-range 1/10, advert 2/15. `b10-full-01` INVALID (15× port-3421 collision) not counted; clean `b10-full-02` used. Source freeze `F1E47615…` / snapshot `0E51D632…`. Evidence: `normal-binary-build10-audit-20261103.md`, `evidence/line3/b10-*-02/` |
| **FINAL CANDIDATE build-11** Tests 02DC3D9C… + GrinNode 565BC2DB… | **FAIL overall — bodyless-only; bound with explicit exception** | build-11 retry, 9-file freeze; full **255 cases / 254 passed / 1 failed** (4201/4200); zero `mg_start`; only test-only `Test_NrdBodylessRestart.cpp(103)`. All groups PASS: archive 8/187, Nrd 30/428, KernelIndex 5/74, GetKernelHandler 2/49, schema-v2 2/26, duplicate-range 1/10, `P2P::ArchivePublication*` 1/83, advert 2/15. Freeze `E5F7519C…`; failed first attempt `ACED40F9…` excluded. Evidence: `normal-binary-build11-audit-20261103.md`, `evidence/line3/b11-*-01/` |
| ASan refresh build-09 frozen sources (`CF78C188…`, TestsMini `12C3D16C…`) | **PASS** | Nrd* 21/365, archive 8/187, Segment 16/68, P2P 7/61 (incl. advert), teardown PASS, 0 ASan; Blocked normal-only: PreCommit/FalseBan/BatchSafety/KernelIndex/BlockPipe/bodyless/kill (BlockChain→Database→rocksdb) |
| ASan refresh for build-10 sources | **PASS (subset)** | source manifest `776B0DB3…` (1292 files), full ASan BUILD_PASS 98.6 s; TestsMini `497353EC…`; Nrd* 21/365, archive 8/187, Segment 16/68, P2P+advert 7/61, teardown PASS, **0 ASan reports**. ASan-BLOCKED normal-only: PreCommit/FalseBan/BatchSafety/KernelIndex/BlockPipe/bodyless/kill; rocksdb and sysroot C libs uninstrumented. Optional `tests_mini_prefix` not built (missing legacy source). Evidence: `asan-build10-refresh-20261103.md`, `build10-asan-refresh-20261103.json` |
| ASan refresh for build-11 sources | **PASS (subset)** | source manifest `69B01EE6…` (1293 files), full ASan BUILD_PASS 101.1 s; TestsMini `C76CCCF2…`; tree matches build-11 freeze 1003/1003; Nrd* 21/365, archive 8/187, Segment 16/68, P2P+advert 7/61, `P2P::ArchivePublication*` 1/83, teardown PASS, **0 ASan reports**. ASan-BLOCKED normal-only: PreCommit/FalseBan/BatchSafety/KernelIndex/BlockPipe/bodyless/kill; rocksdb/sysroot uninstrumented. Optional `tests_mini_prefix` not built. Evidence: `asan-build11-refresh-20261103.md`, `build11-asan-refresh-20261103.json` |
| Pipeline `ci-0a3-01` (build-09, ASan current) | **FAIL (honest)** | provenance PASS, regressions PASS, full-suite FAIL 249/248/1 (test-only bodyless), asan-subset PASS (4/4 sha_match), fuzz advisory PASS; overall FAIL because full suite not green — bound only with explicit exception |
| **Build-08 historical** 7994F191… | **FAIL (historical)** | full 249/237/12: 11× wallet/Tor 3421 env + 1× test-only bodyless; ASan refresh cancelled. Не привязан |
| **Historical normal binaries** C8C386BE…, 930148B6…, 8F10BFD6…, DDC21F45…, FD7C0492… | PASS (historical, не привязаны к final) | C8C 8/8 full 245/4013; 930 4/6 (env flake); 8F10/DDC 236/3952; FD7C 232/3926 |
| `Test_NrdPreCommit` + `Test_NrdFalseBan` + `Test_NrdBatchSafety` + `Test_KernelIndex` + BlockPipe/bodyless/kill под ASan | BLOCKED (link boundary) | BlockProcessor/BlockChain→Database→non-ASan rocksdb; покрыты normal-binary suite build-09 |
| **FINAL CANDIDATE build-12** Tests D1103F34… + GrinNode 833CCBFD… | **FAIL overall — bodyless-only; bound with explicit exception** | root-cause fix included; full **256 cases / 255 passed / 1 failed** (4211/4210); only test-only `Test_NrdBodylessRestart.cpp(103)`. `SpentOutputs*` 1/6 PASS, exact preflight PASS, ArchivePublication 1/83, archive 8/187, GetKernel 2/49, schema 2/26, duplicate 1/10, Nrd 30/428, KernelIndex 5/74, advert 2/15. Freeze `64BE17E5…`; build log `76BC0E49…`. Evidence: `normal-binary-build12-audit-20261103.md`, `evidence/line3/b12-*-01/` |
| ASan refresh for build-12 sources | **PASS (subset)** | source manifest `3DB12364…` (1293 files), full ASan BUILD_PASS 96.4 s; TestsMini `CB09CEE8…`; freeze match 1003/1003; Nrd 21/365, archive 8/187, Segment 16/68, P2P+advert 7/61, ArchivePublication 1/83, teardown PASS, **0 ASan reports**. SpentOutputs test not in TestsMini; Release regression covered normal only. `tests_mini_prefix` not built. Evidence: `asan-build12-refresh-20261103.md`, `build12-asan-refresh-20261103.json` |
| **FINAL CANDIDATE build-13** Tests 001DF4BC… + GrinNode F39BDFE5… | **FAIL overall — bodyless-only; bound with explicit exception** | target-boundary fix included; full **257 cases / 256 passed / 1 failed** (4219/4218); only test-only `Test_NrdBodylessRestart.cpp(103)`. `SpentOutputs*` 1/6, exact preflight 1/4, boundary regression 1/8, ArchivePublication 1/83, archive 8/187, GetKernel 2/49, schema 2/26, duplicate 1/10, Nrd 30/428, KernelIndex 5/74, advert 2/15. Freeze `4FC8DCE1…`; build log `8C54DD17…`. Evidence: `normal-binary-build13-audit-20261103.md`, `evidence/line3/b13-*-01/` |
| ASan refresh for build-13 sources | **PASS (subset)** | source manifest `BFBF2186…` (1293 files), full ASan BUILD_PASS 87.5 s; TestsMini `ECDD6D5E…`; freeze match 1003/1003; Nrd 21/365, archive 8/187, Segment 16/68, P2P+advert 7/61, ArchivePublication 1/83, teardown PASS, **0 ASan reports**. SpentOutputs/boundary tests not in TestsMini; normal Release PASS. `tests_mini_prefix` not built. Evidence: `asan-build13-refresh-20261103.md`, `build13-asan-refresh-20261103.json` |
| Build-13 recovery (copy) | **H3313440 observed PASS; H3315600 snapshot/serving observed** | H3313440 archive `TxHashSet.3313440.84d7ac676356.zip` 3,116,449,317 B SHA `7FB23058…`; 17 entries/required/leaf; LoadAndValidate before atomic publish; no failure signals; tip/roots unchanged; no partial. H3315600 new snapshot 3,118,756,420 B SHA `DA23A1C7…`, old `13F9781E…` preserved, serving ready across 3317930. Evidence: `task10-11-build13-recovery-audit-20261103.md` |
| **BUILD-13 FULL 7B LIVE** Tests 001DF4BC… + GrinNode F39BDFE5… | **PASS (test/floonet; final live binding)** | tip **3318655 / 2945348e…**, C++ FULLY_SYNCED, Rust no_sync, roots pre==post; PIBD leaves **10,203,442** @H3315600, range **2,943,034**, kernels **3,351,542**; duration ~1h58m59s; H3315600 archive 3,118,756,420 B SHA `DA23A1C7…`. Non-blocking env: admin DACL refusal + initial loopback connect failure; stale C++ waiting_reason; Rust 50,056 WARN / 0 ERROR/panic. Evidence: `build13-7b-full-audit-20261103.md`, `build13-7b-full-20261103/`, hash manifest `6CAE425C…`. Crash/publication/resource matrix prepared, NOT RUN; not Mainnet/full G-09 |
| ASan ↔ normal binding | разграничено | normal authoritative = **build-13** Tests `001DF4BC…` + GrinNode `F39BDFE5…` **with exception**; ASan = build-13 refresh `BFBF2186…`/TestsMini `ECDD6D5E…` (subset PASS). Build-12/11 historical; build-10 final only for its snapshot; H3313440/H3315600 observed recovery evidence recorded, line1 formal pending |
| Hosted CI | NOT VERIFIED | `.github/workflows/line3-ci.yml` self-hosted Windows template; runner не подключён; `ci-0a3-01` локальный, overall FAIL (full-suite test-only) |
| Hosted CI | NOT VERIFIED | `.github/workflows/line3-ci.yml` — self-hosted Windows template; runner не подключён, repo содержит uncommitted cross-line изменения; `ci-c8c-02` валиден только для C8C |
| Ordinary-binary live G-09 (archive serving default-off) | NOT VERIFIED | нужен live chain/peer; full-suite PASS не заменяет live |
| p2p-ответы 24/26/28 и proof-валидация под ASan | PASS (smoke) | linked targets `p2p_responses`/`segment_validate`; 400 мутаций + 120 one-shot, 0 ASan |
| Exit-time UAF в `TestMain`/`Global` (static deinit order) | FIXED в тестовом runtime (PASS regression); production hazard NOT VERIFIED | `TestMain.cpp` → `CATCH_CONFIG_RUNNER` + локальный runtime; Global.cpp не менялся; teardown regression PASS, pre-fix negative control FAIL |
| Zero-test ASan teardown regression | PASS | `tools/fuzz/run_asan_teardown_regression.ps1`; exe sha `50076B06…`; post/pre manifests `FC7C5BCE…`/`5C7AEEC1…` |
| Normal-profile impact of TestMain fix | PASS (compile-only) / full normal build NOT VERIFIED | `/W4 /WX /O2 /MT` compile exit 0; полный normal Tests build не запускался (shared slot) |
| Database/ChainState/rocksdb под ASan | NOT COVERED | rocksdb.lib без ASan; полный Tests.exe потребует ASan-пересборки sysroot |
| Snapshot activation / NRD transitions под ASan + fault-injection | NOT COVERED | нет контролируемых crash/rename/commit/disk-full сценариев и NRD apply/rewind/reorg/restart последовательностей под ASan; нужны harness'ы линий 1/2 |
| Настоящий CI-пайплайн | RUNNABLE (local PASS); hosted NOT DEPLOYED | `tools/build/ci-pipeline.ps1` выполнен на historical 930148B6: overall `PASS_WITH_UNCOVERED`; hosted runner/PR-интеграция не настраивались |
| Coverage-guided fuzzing | NOT AVAILABLE | нет clang-cl/libFuzzer в окружении; текущий драйвер — детерминированные bounded-мутации, coverage не измеряется |
| `Tests.exe` 0xC0000409 (исторический) | root cause NOT VERIFIED | событие/WER сохранены; исправленный teardown-UAF — отдельная находка на другом бинарнике, причинная связь не утверждается |
| Common CI harness: cross-line регрессии | PASS | `tools/build/ci-regressions.ps1` на 930148B6 (SHA-bound, parallel HOME/TMP): advertised-tip, false-ban, nrd-rollback-reorg, rebuild-interruption — все PASS |
| Full-suite determinism (wallet/Tor env) | FAIL_ENV_DEPENDENT | line3: 4 PASS/6 прогонов 242/3980, 2 FAIL только в wallet/slatepack decrypt; line2: 14× `mg_start failed 127.0.0.1:3421`; Slatepack*-only 8/8 PASS. Порт 3421 фиксирован → wallet/full suite нельзя параллелить |
| ASan binding после P0 atomic-tip sources | STALE / NOT RUN | текущий source manifest `5A769673…`/TestsMini `4AB0CDDD…` не соответствует новым source (mismatch=19); refresh требует разрешённой ASan-пересборки copied tree |

## 6. Остающиеся работы (не входят в текущий PASS)

Эти пункты **не закрыты** и должны оставаться видимыми в приёмке:

1. **Развернуть настоящий CI.** Подключить Job A (normal build + isolated
   tests) и Job B (ASan build/selftest/fuzz-smoke + teardown regression) к
   hosted runner, зафиксировать toolchain image/VC-версию, артефакты и
   обязательность на PR. Сейчас есть только воспроизводимые скрипты и
   локальные прогоны. **Обновление:** runnable pipeline уже есть
   (`tools/build/ci-pipeline.ps1`, run `ci-930-02` → `PASS_WITH_UNCOVERED`);
   hosted runner/PR-обязательность остаются неразвёрнутыми.
2. **Закрыть ASan-пробел RocksDB/Database/ChainState.** ASan-пересборка
   rocksdb (и остальных C++ sysroot-библиотек) либо отдельный ASan-профиль
   этих компонентов; после этого связать полный `Tests.exe` под ASan. Текущие
   `Test_NrdPreCommit`/`Test_KernelIndex` остаются ASan-BLOCKED по LNK2019
   (`evidence/line3/asan-d2-blocked-kernidx/linker_errors.txt`).
3. **Coverage-guided fuzzing.** Получить clang-cl/libFuzzer (или внешний
   runner с clang) и заменить/дополнить детерминированный мутационный драйвер;
   публиковать coverage и минимальные входы. Сейчас smoke-прогоны — не
   доказательство безопасности.
4. **Snapshot activation и NRD transitions:** fault-injection harness
   (crash/failure до и после staging, rename/commit, disk-full, restart) и
   ASan-последовательности NRD apply/rewind/reorg — совместно с линиями 1/2.
   Матрица F1–F4 и точки передачи: `docs/modernization/FAULT_INJECTION.md`.
5. **0xC0000409:** root cause остаётся NOT VERIFIED; исторический бинарник не
   сохранён. Исправленный teardown-UAF (TestMain/Global) — отдельная находка;
   не использовать её как объяснение исторического crash. Для повторного
   расследования нужен сохранённый бинарник/дамп и тот же сценарий запуска.

## 7. Common CI harness и выполняемый pipeline

### 7.1 Cross-line регрессии (общий harness)

```powershell
pwsh tools/build/ci-regressions.ps1 -TestsExe <preserved Tests.exe> `
  -ExpectedSha <sha256> -RunId ci-reg-<id> [-Parallel]
```

Группы: `advertised-tip` (`P2P::AdvertisedTipUsesHeaderHead`,
`...ReadsOneSnapshot`), `false-ban` (`NrdFalseBan*`), `nrd-rollback-reorg`
(`NrdRollback*`, `NrdReorgBackup*`), `rebuild-interruption` (`KernelIndex*`,
`NrdRebuild*`). Каждая группа — отдельный one-shot HOME/TMP, в JSON
фиксируются SHA бинарника, exit code, summary и SHA manifest. `-Parallel`
разрешён только для этих групп: они не используют фиксированный wallet-порт.

### 7.2 Выполняемый pipeline

```powershell
pwsh tools/build/ci-pipeline.ps1 -TestsExe <preserved Tests.exe> `
  -ExpectedTestsSha <sha256> -GrinNodeExe <...> -ExpectedGrinNodeSha <sha256> `
  -SkipBuild -RunFullSuite -ParallelRegressions `
  -FuzzBuildDir <fuzz dir> -FuzzIterations 100 -RunId ci-<id>
```

Стадии отчёта раздельные: `provenance`, `normal-build` (NOT RUN при
`-SkipBuild`), `unit-regressions`, `full-suite`, `asan-subset`, `fuzz-smoke`,
`untested-zones`. ASan/fuzz **не** превращаются в PASS при stale source
binding: ASan получает `NOT RUN (STALE, mismatch=N)`, fuzz — `PASS
(ADVISORY)`. Overall: `PASS`/`PASS_WITH_UNCOVERED`/`FAIL`; exit 1 при любом
FAIL/FAIL_ENV_DEPENDENT.

Прогон `ci-930-02` (historical 930148B6, до финального build-07):
`PASS_WITH_UNCOVERED`; unit-regressions PASS; full-suite PASS в этом прогоне;
ASan `NOT RUN (STALE, mismatch=19)`; fuzz advisory PASS (bytebuffer 21 ok/79
reject, zip 9 ok/91 FileException, nrd_rules 100 ok; 0 ASan); untested-zones
REPORTED. Evidence: `evidence/ci/ci-930-02/summary.json`.

**Правило портов:** wallet/full suite использует фиксированный
`127.0.0.1:3421`. Раздельных HOME/TMP **недостаточно**; такие прогоны нельзя
параллелить (line2 и line3 независимо наблюдали `mg_start failed`/wallet-
decrypt флейки). Полный suite — только последовательно; в CI закрепить
сериализацию или выделение уникальных портов.

## 8. Filter quoting и guard «No tests ran»

**Исправлено в `tools/build/run-tests-isolated.ps1`:**
`-TestArgs` теперь квотирует каждый элемент для Windows command line, поэтому
фильтры с пробелами доходят до Catch2 одним аргументом. Старый space-joined
путь молча превращал `KernelIndex schema v2*` в `KernelIndexschemav2*`
(«No tests ran», exit 0 → **ложный PASS**). Для обратной совместимости
`-ExeArgs` остаётся raw-фрагментом (caller отвечает за кавычки).

Пример:

```powershell
pwsh tools/build/run-tests-isolated.ps1 -ExePath <Tests.exe> `
  -RunId ci-schema -KeepBinary -TestArgs @('KernelIndex schema v2*') -FailOnNoTests
```

**Guard:** `-FailOnNoTests` переводит «No tests ran / No test cases matched» в
`status=FAIL_NO_TESTS`, exit 1, `no_tests_ran=true` в манифесте. Zero-test
teardown-regression не использует этот флаг (там пустой фильтр — намеренный
PASS). `ci-regressions.ps1` передаёт `-TestArgs` + `-FailOnNoTests` и считает
группу PASS только при PASS-манифесте; группы `get-kernel-handler`,
`kernel-index-schema`, `kernel-duplicate-range` включаются через
`-IncludeTask9` (или явный `-Only`). `ci-pipeline.ps1` получил
`-IncludeTask9` и тот же guard для ASan-фильтров.

Проверено (evidence `evidence/line3/runner-quoting-guard-20261103.json`,
sha `32AE57F9…3E48`; runner `2A842CE1…5BA9`, ci-regressions `AA378EDC…1853`):
quoted `KernelIndex schema v2*` → PASS 2 cases/26 assertions с корректным
`args`; bogus-фильтр с guard → FAIL_NO_TESTS/exit 1; zero-test без guard → PASS;
CI-группы: handler PASS, schema PASS, duplicate-range FAIL (известный test-only
дефект до финального build-10).
