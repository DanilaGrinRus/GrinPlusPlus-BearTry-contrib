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
| **Authoritative normal binary** C8C386BE… (build-07 final integration candidate) | PASS | line3 independent: full **3/3 PASS 245/4013**, archive 8/187, Nrd 30/428, KernelIndex 3/48, advert 2/15, Slatepack* 5/5; repo==preserved (Tests+GrinNode F9EF06B4…); pipeline `ci-c8c-01` PASS_WITH_UNCOVERED. Flake resolved: serialized 4/4 full PASS; wallet port 3421 must be serialized |
| **Historical normal binaries** 930148B6… (non-final), 8F10BFD6…, DDC21F45…, FD7C0492… | PASS (historical, не привязаны к final) | 930: 4/6 full PASS (env flake wallet port); 8F10/DDC 236/3952; FD7C 232/3926; preserved |
| ASan D2 subset (`NrdPolicy`+`NrdRules`, archive, Segment, P2P) | PASS (source-bound; stale for C8C) | source manifest `5A769673…`, TestsMini `4AB0CDDD…`: Nrd* 21/362, archive 8/187, Segment 16/68, P2P 5/46, teardown PASS, 0 ASan; для C8C source set STALE (mismatch=19) → NOT RUN, без PASS-claims |
| `Test_NrdPreCommit` + `Test_KernelIndex` под ASan | BLOCKED (link boundary) | BlockChain→Database→non-ASan rocksdb; LNK2019 evidence `asan-d2-blocked-kernidx/linker_errors.txt`; покрыты normal-binary suite C8C |
| ASan ↔ normal binding | разграничено | ASan = source manifest `5A769673…`/TestsMini `4AB0CDDD…` (stale для C8C); normal authoritative = `C8C386BE…`; prior 930/8F10/DDC/FD7C — historical |
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
