# EVIDENCE_LINE3 — точный SHA и доказательства (линия 3)

Создано: 2026-11-03 (Asia/Bangkok). Все пути — в `evidence/line3/` этого
репозитория, если не указано иное. Формат: PASS / FAIL / NOT VERIFIED / BLOCKED.

## 0. Исходное состояние дерева

```
repo:  C:\DHW\Gplus\src\grinplusplus
HEAD:  e0a5d2d68b990234e7bf1fb1b651f868dc30e9b6 (master)
```

`git status --porcelain` на момент съёмки доказательств содержал чужие
незакоммиченные изменения (линии 1/2 и аудитор): `include/P2P/Capabilities.h`,
`src/BlockChain/Validators/BlockValidator.cpp`, `src/P2P/MessageProcessor.cpp`,
`src/P2P/PIBD/PibdParams.h`, `src/P2P/Seed/HandShake.cpp`, `src/P2P/Seed/Seeder.cpp`,
`src/PMMR/TxHashSetImpl.cpp/.h`, `src/PMMR/Zip/ZipFile.cpp`,
`tests/src/Net/Test_P2PCompat.cpp`, `tests/src/PMMR/CMakeLists.txt`,
`Test_NrdRules.cpp`, `Test_PibdServing.cpp`, `Test_ZipFile.cpp` и новые файлы
линий 1/2. Ничего из этого линия 3 не редактировала и не коммитила.

Файлы, добавленные линией 3 (незакоммиченные): `tests/fuzz/**`,
`tools/fuzz/**`, `tools/build/run-tests-isolated.ps1`,
`docs/modernization/SANITIZERS.md`, `docs/modernization/CI_REPRO.md`,
`evidence/line3/**`.

## 1. Тестовый runtime: проверенный бинарник и прогоны

```
Tests.exe : C:\DHW\Gplus\src\grinplusplus\bin\Release\Release\Tests.exe
sha256    : 4F75DCD431DF4A26BB34B4548F078338D9E3FB48F1043E531B3A498CCA636C7B
size      : 8 876 032 bytes
PE ts     : 0x6AE8E02E  (2026-11-02T19:10:38Z = 2026-11-03 02:10:38 +07)
file mtime: 2026-11-03T02:10:39+07
provenance: бинарник содержит строку "ZipFile rejects a CRC-corrupted snapshot
            entry" => собран из дерева с незакоммиченными изменениями аудитора
            (Test_ZipFile.cpp / tests/src/PMMR/CMakeLists.txt)
```

Изолированные прогоны (HOMEDRIVE/HOMEPATH/TMP/TEMP в одноразовом каталоге,
cwd — одноразовый, реальный `%USERPROFILE%\.GrinPP` снят до/после):

| run | evidence dir | exit | wall | результат | профиль |
|---|---|---|---|---|---|
| 001 | `baseline-e0a5d2d-001` | 0 | 58.974 s | 205 cases / 3358 assertions (console) | не изменён |
| 002 | `baseline-e0a5d2d-002` | 0 | 54.871 s | 205 / 3358 | не изменён |
| 004 | `baseline-e0a5d2d-004` | 0 | 54.575 s | `-r compact`: `Passed all 205 test cases with 3358 assertions.` | не изменён |
| 005 | `baseline-e0a5d2d-005` | 0 | ~54 s | то же, summary зафиксирован в manifest | не изменён |

Прогон 003 — невалидный attempt (параметр `-TestArgs '-r','compact'` ушёл в
фильтр: `No tests ran`, exit 0 за 0.18 s); в доказательства PASS не входит.

Каждый manifest содержит SHA exe, PE ts, env (HOMEDRIVE=HOMEPATH=…),
`expected_test_data_dir`, `test_data_dir_created=true`,
`user_profile_unchanged=true`, stdout/stderr SHA и exit code. Ключевые SHA-256
manifest: 001 `655156A3…`, 002 `F1821FFF…`, 004 `60D62D4B…`.

Изоляция подтверждена кодом: `TestMain.cpp` → `FileUtil::RemoveFile` =
`fs::remove_all` от `DefaultDataDir(AUTOMATED_TESTING)` =
`GetHomeDirectory()/.GrinPP/<ENV>`, где `GetHomeDirectory()` берётся только из
`HOMEDRIVE`/`HOMEPATH`. Отдельный скрипт: `tools/build/run-tests-isolated.ps1`.

## 2. Заявленные 205/3700 — NOT VERIFIED

Найден «правильный» лог: `C:\DHW\.tmp\tests-capabilities.log`
(sha256 `6A54E158DEBD460DEABA25A8FFC28307F939E6A47C1897152A15A32596B9EF14`,
14 807 B, mtime 2026-11-03 02:11:48), заканчивается
`Passed all 205 test cases with 3700 assertions.` Формат — **нативный compact
reporter** Catch2 (проверено: тот же бинарник с `-r compact` печатает
`Passed all 205 test cases with 3358 assertions.`).

Но:

1. К логу нет manifest/команды/cwd/env/SHA бинарника. Связь с SHA
   `4F75DCD4…` только временная (лог записан через 69 с после сборки).
2. Три независимых прогона **того же самого** бинарника (SHA и файл не менялись
   с 02:10:39) дают 205/3358, а не 3700; разница 342 assertions.
3. `tests-proof-boundaries.log`
   (`A91278A20FC8EE6E2DBC66561E042CD67444E0B092DD50442E1A27250BFD45C6`,
   mtime 2026-11-02 21:49) содержит **203/3676** — как доказательство 205/3700
   он stale/непригоден.
4. `tests-g09-pass.log` (205/3697, sha256 `21982DFE…`, mtime 01:43:57) также без
   привязки к SHA; из него следует только, что бинарник до пересборки в 02:10
   давал 3697.

Вывод: **заявленные 205/3700 для SHA `4F75DCD4…` не воспроизведены. Доказанное
значение — 205 cases / 3358 assertions.** Число assertions нельзя использовать
как стабильный идентификатор без manifest; 205/3700 в отчётах не цитировать как
PASS. Более старые числа (203/3676, 205/3694, 205/3697 и т.д.) — исторические
артефакты без SHA-привязки.

## 3. `Tests.exe` 0xC0000409 — событие подтверждено, root cause NOT VERIFIED

Windows Application Log, Event ID 1000 (Application Error), три события:

| время (+07) | PID | exception | fault offset | PE timestamp сбойного exe |
|---|---|---|---|---|
| 2026-11-03 01:51:08.529 | 14884 | 0xC0000409 | 0x646B29 | 0x6AE8D9B3 |
| 2026-11-03 01:51:31.236 | 22300 | 0xC0000409 | 0x646B29 | 0x6AE8D9B3 |
| 2026-11-03 01:52:16.024 | 31768 | 0xC0000409 | 0x646B29 | 0x6AE8D9B3 |

Сбойное приложение = `C:\DHW\Gplus\src\grinplusplus\bin\Release\Release\Tests.exe`,
сбойный модуль = тот же файл. WER `Report.wer`:
`TargetAppVer=2026//11//02:18:42:59` → сбойный бинарник собран
**2026-11-03 01:42:59 +07**, PE ts `0x6AE8D9B3`; `EventType=BEX64`;
`Sig[8]` exception data = `0000000000000007` = `FAST_FAIL_FATAL_APP_EXIT` →
`abort()`/`std::terminate`, а не классическое stack-cookie OOB (было бы
`FAST_FAIL_STACK_COOKIE_CHECK_FAILURE` = 2).

Копии для сохранности (ProgramData/WER может чиститься):

- `evidence/line3/crash-0xc0000409/Report_1.wer` (`24608BBB…`)
- `Report_2.wer` (`023AB40C…`), `Report_3.wer` (`C2A04376…`)
- `application_error_events.txt` (`E6BB9092…`) — экспорт событий 1000.

Точный SHA сбойного бинарника **не сохранён**: файл был перезаписан сборкой в
02:10:39; поиск `Tests*.exe` по `C:\DHW` не нашёл копии с PE ts `0x6AE8D9B3`.
Сохранённого stdout/stderr/cwd этих трёх запусков нет (в `.tmp` в окне
01:48–01:55 файлов нет). Поэтому:

- PASS: событие/код/смещение/сборка сбойного бинарника и класс abort доказаны
  независимо (Event Log + WER);
- NOT VERIFIED: root cause, точный вход/путь, полный SHA, stdout; «падение до
  вывода тестов» не доказано (при перенаправлении в файл stdout
  block-buffered — пустой лог не доказывает смерть до первого вывода);
- контекст: тот же PE-бинарник по wrapper-логу `tests-g09-pass.log` в 01:43:57
  проходил набор (205/3697), т.е. падение не детерминировано. Гипотезы (не
  доказаны): исключение в static-init (создание 12-поточного
  `Bosma::Scheduler(12)` в `Context::Create`, инициализация логгера) с
  `std::terminate`; конкуренция запусков за общий HOME/TMP. Повторять только с
  сохранением SHA/PE-ts/WER/raw stdout и не перезаписывая бинарник.
- **Явно о причинности:** отдельно найденный и исправленный exit-time
  heap-use-after-free тестового runtime (`TestMain`/`Global`, ASan-находка
  2026-11-03 на **другом** бинарнике) — это самостоятельный дефект порядка
  статической деинициализации. Он **не является доказанной причиной**
  исторического 0xC0000409 (PE `0x6AE8D9B3`, 01:51–01:52; бинарник не сохранён).
  Причинная связь не утверждается; root cause 0xC0000409 остаётся
  **NOT VERIFIED**, даже при том что teardown-дефект исправлен и покрыт
  регрессией.

## 4. MSVC AddressSanitizer: сборка и инструментация — PASS

Toolchain: MSVC `14.44.35207`, VS 2022 BuildTools 17.14, Windows SDK
`10.0.26100`; ASan runtime
`clang_rt.asan_dynamic-x86_64.dll` sha256
`3AA742521E8CBD23252F0545AA51D5A15624A3E4430FBE2F81567AFAEB3FC59B`.

Профиль: `/W4 /WX /O1 /Zi /MT /EHsc /std:c++17 /fsanitize=address`,
линковка `/DEBUG /INCREMENTAL:NO`. `/Zi` обязателен (иначе `C5072` + `/WX`).
Обычная сборка и её флаги не менялись; общий build directory не использовался.

Сборка: `pwsh tools/fuzz/build_asan.ps1` → `C:\DHW\.tmp\asan-line3\fuzz\`.
Манифест сборки скопирован: `evidence/line3/asan-build-manifest.json`
(sha256 исходного `B10E62A7…`).

Итоговые exe (ASan):

| target | sha256 |
|---|---|
| bytebuffer | `2312BE962C06D8FB51B100E2BCC589318CCD8EA955A234B85DE661D359C49691` |
| p2p_body | `7363C8E5C9C4684FBECA79F985EED983297F2F9A4A9BAE5D8CB8BD85870241C2` |
| segment | `C7AF094E085EAAA641E6C085923EF5BC0B863784822ABE595B8420586C12A0A6` |
| zip | `1619BFBC781A8A5FEADF123F52427E6B3869C96B98792CD7747671C88DA78921` |

Доказательство инструментации (one-shot, намеренное OOB в instrumentation
self-test, не в production-коде):

```
fuzz_bytebuffer.exe --asan-selftest
→ ERROR: AddressSanitizer: heap-buffer-overflow ...
  #0 ... in main .../tests/fuzz/fuzz_driver.cpp:222
exit code 1
```

Логи: `asan_instrumentation_selftest.stdout.log`,
`asan_instrumentation_selftest.stderr.log`
(sha256 `DE0357CF9B2C0C48F0D0BC33B889A0E2C58D1E37B71F2FF33278272D14DE8090`,
3 001 B), `asan_instrumentation_selftest.exit.txt`.

## 5. Fuzz smoke — PASS (в рамках бюджета)

`pwsh tools/fuzz/run_fuzz_smoke.ps1 -Iterations 200 -MaxSeconds 30 -RunId fuzz-smoke-20261103-01`
(seed `20261103`; каждый из 43 corpus-файлов дополнительно прогнан через
`--one-shot` в отдельном процессе с timeout 30 s).

| target | executed | ok | controlled rejection | ASan | timeout | slowest |
|---|---|---|---|---|---|---|
| bytebuffer | 200 | 42 | 158 `DeserializationException` | 0 | 0 | 0.137 ms |
| p2p_body | 200 | 34 | 166 `DeserializationException` | 0 | 0 | 0.156 ms |
| segment | 200 | 45 | 155 `DeserializationException` | 0 | 0 | 0.165 ms |
| zip | 200 | 25 | 175 `FileException` | 0 | 0 | 197.763 ms |

Итог: `result=PASS`, 43/43 one-shot без crash/timeout/ASan.
Manifest: `fuzz-smoke-20261103-01/manifest.json` (sha256 `AEB5F452…`).
Corpus: `tests/fuzz/corpus/` (43 файла, синтетика; генерируется
`tools/fuzz/gen_corpus.ps1`).

Ограничение: 200 итераций/30 с на target — smoke, не исчерпывающий fuzz;
coverage не измерялась; отсутствие crash не доказывает безопасность.

## 6. Что НЕ покрыто / ограничения

- p2p-сообщения 24/26/28 (`Segment<OutputIdentifier/RangeProof/TransactionKernel>`)
  и `Segment::Root`/`SegmentProof::ReconstructRoot`: **BLOCKED** для ASan-harness.
  Причина: обычные `Core.lib`/`Crypto.lib` собраны без ASan и несут MSVC STL
  `detect_mismatch` (`LNK2038: annotate_string/annotate_vector mismatch`), а
  `#undef` через `/U` эту проверку не снимает. Нужна полная ASan-сборка проекта
  в изолированном дереве (не выполнялась).
- libFuzzer/coverage-guided fuzzing недоступны (нет clang-cl).
- minizip/zlib и прочие sysroot-библиотеки не инструментированы.
- UBSan/MSan/TSan, многопоточность и live-сеть не проверялись.
- Snapshot activation (staging/rename/commit/disk-full) и NRD transitions не
  покрыты этим harness: нужны контролируемые барьеры и последовательности
  состояний (зона линий 1/2). ZIP-target проверяет только парсинг архива и
  извлечение в изолированный staging. Чистые NRD-правила (`NrdRules.h`) покрыты
  target `nrd_rules` (см. §8); построение KernelMMR/исторического индекса под
  ASan не покрыто (нужна полная ASan-сборка проекта).
- Автоматической минимизации crash-входа нет; сохраняется `current_case.bin`,
  минимизация ручная.
- Число assertions нестабильно/непривязано; единственный PASS по тестам —
  exit 0 + «tests passed» на конкретном SHA с изолированным HOME и неизменным
  пользовательским профилем.

## 7. Точные команды воспроизведения

```powershell
# состояние
git -C C:\DHW\Gplus\src\grinplusplus rev-parse HEAD
git -C C:\DHW\Gplus\src\grinplusplus status --porcelain
Get-FileHash C:\DHW\Gplus\src\grinplusplus\bin\Release\Release\Tests.exe -Algorithm SHA256

# изолированный unit-прогон
pwsh C:\DHW\Gplus\src\grinplusplus\tools\build\run-tests-isolated.ps1 -RunId repro-01 -ExeArgs '-r compact'

# ASan build + доказательство инструментации + smoke
pwsh C:\DHW\Gplus\src\grinplusplus\tools\fuzz\build_asan.ps1
C:\DHW\.tmp\asan-line3\fuzz\fuzz_bytebuffer.exe --asan-selftest
pwsh C:\DHW\Gplus\src\grinplusplus\tools\fuzz\run_fuzz_smoke.ps1 -Iterations 200 -MaxSeconds 30 -RunId repro-fuzz

# полная ASan-сборка проекта (copied tree) + linked targets
pwsh C:\DHW\Gplus\src\grinplusplus\tools\fuzz\build_full_asan.ps1
pwsh C:\DHW\Gplus\src\grinplusplus\tools\fuzz\build_linked_targets.ps1
# TestsMini под ASan (Nrd* / Segment* / Capabilities::*,MessageTypes::*,GrinVersion::*)
pwsh C:\DHW\Gplus\src\grinplusplus\tools\build\run-tests-isolated.ps1 `
  -ExePath C:\DHW\.tmp\asan-line3\fuzz-linked\TestsMini.exe -ExeArgs 'Nrd*' -KeepBinary

# история crash-событий (не перезаписывая текущий Tests.exe)
Get-WinEvent -FilterHashtable @{LogName='Application'; Id=1000; StartTime='2026-11-03 01:50'; EndTime='2026-11-03 01:55'} |
  Where-Object Message -match 'Tests\.exe'
```

## 9. Полная ASan-сборка проекта и найденный UAF (2026-11-03)

Подробно: `evidence/line3/full-asan-audit-20261103.md`.

- **PASS:** полная ASan-сборка в copied tree
  `C:\DHW\.tmp\asan-line3\gpp-tree` (libs Common/Core/Crypto/PMMR + deps;
  build manifest sha256 `210FBAB7…`, source manifest `E73103AB…`, 75.6 s).
  LNK2038 решён: все линкуемые project C++ TU собраны с ASan; Database/rocksdb
  не линкуются.
- **PASS:** TestsMini под ASan (`TestsMini.exe` sha256 `A34F068A…`, сохранён в
  `asan-testsmini-*/Tests.exe`): `Nrd*` 12/329 (differential + KernelIndex
  rebuild), `Segment*` 16/68, `Capabilities/MessageTypes/GrinVersion` 5/46;
  exit 0, ASan-отчётов нет, изолированный HOME, профиль не изменён.
  Manifests: `7EAB64BE…`, `05CF5FEA…`, `BEB62E36…`.
- **PASS (smoke):** linked targets `p2p_responses` (22/24/26/28 real
  deserialize + SerializePayload + bounded Root/Validate) и `segment_validate`
  (Root/Validate/ReconstructRoot, Hasher инструментирован): 200+200 итераций,
  120/120 one-shot, 0 ASan/timeout (manifest `E440A0CF…`).
- **FAIL (finding, не исправлено):** исходный `TestMain.cpp` со `static
  TestRuntime` даёт exit-time heap-use-after-free (`Global::Shutdown`/
  `Global::GetContext` + статические держатели `Core/Global.cpp`). Воспроизводится
  без тестов (`asan-testsmini-empty-01`), refcount-диагностика
  `diag_runtime_lifetime.*`. Класс — static deinit order; impact на production
  NOT VERIFIED; workaround только в harness (`tests_mini_main.cpp`), обычный
  Tests.exe не изменён. Нужно решение Lead по исправлению Global/TestMain.
- Ограничения: Database/ChainState/rocksdb не инструментированы; TestsMini —
  подмножество (33 теста), не полный Tests.exe; coverage не измерялась.

## 8. NRD handoff line2 — аудит evidence и ASan reuse (2026-11-03)

Подробности: `evidence/line3/nrd-handoff-audit-20261103.md`.

- Заявленный SHA `0EDC0F810E7D9BB864F27266EEB0872DC12288B6F08A9E79293B44D912235FC7`
  подтверждён: текущий `bin\Release\Release\Tests.exe` (сборка line2, mtime
  2026-11-03 02:56:09, PE ts `0x6AE8EAD8`) имеет этот SHA. Note: этот бинарник
  новее и заменяет `4F75DCD4…` из §1; копия `4F75DCD4…` сохранена в
  `evidence/line3/baseline-e0a5d2d-001/Tests.exe`.
- Line2 `nrd-20261103-025613`: 17 cases / 350 assertions PASS; stdout SHA
  совпадает с manifest, изоляция и `user_profile_unchanged=true` подтверждены.
  Однако копия бинарника в evidence line2 **не сохранена** — привязка держалась
  на изменяемом `bin\...\Tests.exe`. **Gap закрыт line2 2026-11-03**: immutable
  copy `tools/audit/nrd/evidence/bin/Tests-0EDC0F810E7D.exe`
  (SHA `0EDC0F81…`, 9 012 736 B, PE ts `0x6AE8EAD8`) + `Tests-0EDC0F81.sha256.txt`,
  привязывающий `nrd-025613`, `kernidx-025622` и line3 reruns; проверено line3
  независимым хешированием, repo binary по-прежнему тот же SHA.
- Независимое воспроизведение line3 (с сохранением бинарника):
  `nrd-audit-20261103-01` (`KernelIndex*`) = 3 cases / 48 assertions PASS,
  `nrd-audit-20261103-02` (`Nrd*`) = 17 cases / 350 assertions PASS; SHA тот же,
  HOME изолирован, профиль не изменён, `Tests.exe` сохранён в обоих каталогах.
- `cpp-nrd-differential-report.json`: 14/14 `actual == expected`
  (sha256 `EA44A390…`); `nrd_cases.json` — 14 chain cases (sha256 `AD2B77AC…`).
  Rust oracle commit остаётся `NOT VERIFIED` по собственной пометке line2.
- ASan reuse: target `nrd_rules` (production header-only `include/PMMR/NrdRules.h`)
  + 26 seeds (12 граничных + 14 из `nrd_cases.json`), build sha256 `4D7275E7…`,
  smoke: 500 итераций + 26 one-shot, `ok=500`, ASan=0, timeout=0, slowest 0.062 ms
  (manifest `fuzz-nrd-smoke-20261103-01/manifest.json`, sha256 `E4789A72…`).
  `missing_local_history` не используется как доказательство невалидности peer.

## 10. TestMain UAF fix + teardown regression (Lead option b, 2026-11-03)

Подробно: `evidence/line3/testmain-uaf-fix-20261103.md`.

- **Fix (approved):** `tests/src/TestMain.cpp` → `CATCH_CONFIG_RUNNER` +
  `TestRuntime` как локальный объект в `main()` (sha256
  `6BAC17446DE696518FD364220569185F43769B813F59A25372037FBC53D8527B`,
  +18/−2). `src/Core/Global.cpp` **не менялся**; production static-lifetime
  hazard остаётся отдельным NOT VERIFIED finding.
- **Полная ASan-сборка обновлена:** copied tree синхронизирован, source manifest
  `A827E5008922762F36338606230C21BFDF97E3787585E5AC894526B7F529B982`,
  build manifest sha `8CA486B5…`, evidence `full-asan-20261103-032246/` (PASS).
- **Post-fix ASan runs** (`TestsMini.exe` sha
  `50076B06C766E4EE07C1D91C60853E32B6E8CEEA2BA4A3B9D076FFB55F2E7283`,
  сохранён в `asan-testsmini-postfix-*/Tests.exe` и
  `full-asan-linked-artifacts/`): `Nrd*` 12/329 PASS (manifest `64AE37CD…`),
  `Segment*` 16/68 PASS (`D803DAA0…`), P2P compat 5/46 PASS (`5C798783…`);
  exit 0, stderr пуст, профиль не изменён.
- **Zero-test teardown regression:** `tools/fuzz/run_asan_teardown_regression.ps1`
  → PASS на fixed exe (`asan-teardown-postfix-01`, manifest `FC7C5BCE…`,
  `teardown_regression.json` `C71CD1BA…`, stderr = empty);
  negative control на pre-fix main (`C8C10963…`, `TestMain_legacy_static.cpp`) →
  FAIL с `AddressSanitizer: heap-use-after-free` (`asan-teardown-prefix-01`,
  manifest `5C7AEEC1…`). Это подтверждает, что regression ловит исходный дефект.
- **Pre-fix evidence сохранён:** `asan-testsmini-nrd-01`, `asan-testsmini-empty-01`,
  `diag_runtime_lifetime.*` — не изменялись.
- **Normal build impact:** compile-only `/W4 /WX /O2 /MT` для нового
  `TestMain.cpp` — PASS (log sha `B79C7B3A…`); полный normal `Tests.exe` build
  **NOT VERIFIED** (shared slot не использовался). Следующий normal build должен
  получить новый Tests.exe SHA; старые ASan-результаты к нему не привязывать.
- **Остаток:** production `Global` static-init/deinit hazard — NOT VERIFIED;
  Database/rocksdb под ASan не покрыты; TestsMini — подмножество.
- План: когда появятся archive-server тесты линии 1 — добавить их в ASan-subset
  (если link deps allow), с новым source manifest и отдельными манифестами.

## 11. Line1 archive-server subset под ASan + line2 task-5 round (2026-11-03)

Подробно: `evidence/line3/asan-line1-archive-subset-20261103.md` и
`evidence/line3/nrd-handoff-audit-20261103.md` §5.

- **Line2 task-5 новый нормальный бинарник** `2032536D810F9DD78E57658FDEC39DCA06B71368551B2B1C6CAB08B79592140F`
  (9 141 248 B, PE ts `0x6AE8F3B3`) проверен: preserved copy + `.sha256.txt`
  совпадают; `nrd-precommit` 20/375 PASS, `kernidx` 3/48 PASS; финальные
  keepbin-прогоны содержат собственные копии Tests.exe с тем же SHA
  (`nrd-final-keepbin-…`, `kernidx-final-keepbin-…`); differential report
  `EA44A390…` (14/14) не изменился. **NRD scope: PASS.**
- **Full-suite FAIL на этом SHA** (231/3917, 230 pass): `PibdArchiveServer::LeaseOutlivesHorizonSwitch`
  `REQUIRE(destroyed == 1)` → 0 == 1, детерминированно. Независимо воспроизведено
  line3 на preserved binary (`line3-audit-pibd-lease-01`, exit 1, тот же
  REQUIRE, изолированный HOME, профиль не изменён). Line1 классифицировал как
  **test fixture bug** (лишний сильный owner `pFirst`; production lifetime
  корректна) и исправил fixture без ослабления assertion
  (`tests/src/PMMR/Test_PibdServing.cpp` sha
  `4646F4DF256415DF561F66A6C16569AF068825D043F64B38D9A8CB604B3E9D52`).
  Нормальный бинарник с фиксом ещё не собран (build slot pending).
- **ASan subset расширен на archive-тесты:** copied tree пересинхронизирован,
  source manifest `857AA38E1E92D9C422AA02CF830A41B9AE3752000EC337255A2BEFCEA824ED9A`,
  TestsMini sha `2DA4727E…`. Результаты (exit 0, ASan-report 0, профиль не
  изменён): lease 1/16 PASS, `PibdArchiveServer::*` 7/168 PASS, `Nrd*` 12/329,
  `Segment*` 16/68, P2P 5/46; zero-test teardown regression PASS
  (`asan-teardown-postline1-01`). Манифесты: `33D38525…`, `92363128…`,
  `53618F6E…`, `6B3304B5…`, `FA9F6F74…`, `7D9DDF8A…`.
- **Важно:** ASan-результаты привязаны к source manifest/`TestsMini.exe`, а не к
  нормальному SHA `2032536D…`; после появления нового normal Tests.exe нужен
  новый изолированный прогон и новый SHA-манифест.

## 12. Authoritative normal binary FD7C0492 — независимый re-audit (2026-11-03)

Подробно: `evidence/line3/normal-binary-fd7c-audit-20261103.md`.

- Binary: `FD7C04922A2E016723531EC389E21D3AC1E8C0F99134F003595DB18149AB16E5`,
  9 154 048 B, PE ts `0x6AE8F63F`, mtime 03:44:47. Repo
  `bin\Release\Release\Tests.exe` **идентичен** preserved copy
  `C:\DHW\.tmp\line1-build-03\Tests-FD7C04922A2E.exe`; line2's preserved copy в
  `tools/audit/nrd/evidence/bin/Tests-FD7C04922A2E.exe` тоже совпала по SHA.
- Line3 independent isolated runs (`-KeepBinary`, профиль не изменён, exit 0,
  stderr пуст): lease **1/16 PASS** (manifest `9F665959…`), archive
  **7/168 PASS** (`AD146118…`), `Nrd*` **20/375 PASS** (`AA121B36…`),
  `KernelIndex*` **3/48 PASS** (`13AEFAA1…`), full suite **232/3926 PASS**
  (`C4373967…`). Все копии `Tests.exe` в evidence dirs = FD7C0492….
- Line2 binding проверен: `nrd-bind-FD7C0492-…`/`kernidx-bind-FD7C0492-…`
  с собственными копиями FD7C0492…; differential report `EA44A390…` (14/14).
- Source/binary binding: компилируемые корни (`CMakeLists/cmake/include/src/
  tests/deps`) в repo и ASan copied tree **идентичны**; единственные отличия —
  evidence-only `TestMain_legacy_static.cpp` (не компилируется в TestsMini) и
  документация. Ключевые SHA: `PibdArchiveServer.h 56EE9A87…`,
  `Test_PibdServing.cpp 4646F4DF…`, `TestMain.cpp 6BAC1744…`.
- Разграничение доказательств сохранено: ASan = source manifest
  `857AA38E…` + TestsMini `2DA4727E…` (zero reports); normal authoritative =
  `FD7C0492…`. Архивный serving по-прежнему default-off, ordinary-binary live
  G-09 **NOT VERIFIED**; DrainGarbage worker call-site только code-reviewed.

## 13. D2 mainnet NRD gate build DDC21F45 — re-audit (2026-11-03)

Подробно: `evidence/line3/normal-binary-ddc-audit-20261103.md`.

- Binary: `DDC21F45EBFE6FA4846FFFB0FA5E376297EED5BFFEFAA5583E3A6B04B0BCF714`,
  9 172 480 B, PE ts `0x6AE8F9CD`; repo binary == preserved
  `tools/audit/nrd/evidence/bin/Tests-DDC21F45EBFE.exe`; `.sha256.txt` +
  build log BUILD OK; FD7C0492/2032536D/0EDC0F81/06309AF0 сохранены.
- Line2 evidence проверен: `nrd-d2-…` (Nrd* 23/382 PASS), `kernidx-d2-…` (3/48
  PASS), `full-d2-…` (236/3952 PASS) — в каждом `Tests.exe` копия с SHA
  DDC21F45…; differential `EA44A390…` (14/14).
- Line3 independent isolated reruns (SHA-bound, `-KeepBinary`, профиль не
  изменён, exit 0): `Nrd*` **23/382 PASS**, `KernelIndex*` **3/48 PASS**,
  archive **8/187 PASS**, full suite **236/3952 PASS**; manifests
  `DB5856FF…`, `859029CE…`, `D47CF517…`, `C92E2CA3…`.
- **Provenance-нюанс:** archive subset 7/168→8/187 и full 232→236 объясняются
  +3 `Test_NrdPolicy` cases (D2) **и** правкой line1
  `tests/src/PMMR/Test_PibdServing.cpp` в 03:54:55 (sha `EDB1BA61…`, +1 case,
  +19 assertions). Line2 «no line1 archive files touched» верно для line2, но
  DDC build включает line1-обновление.
- **ASan D2 subset (source-based):** source manifest
  `5A7696737C74B067D92973A7925CF6F23C88152B9C9F3841DDBAD5A1CA17EFD5`,
  TestsMini `4AB0CDDD…`: `Nrd*` **21/362** (NrdRules+NrdPolicy+Differential+
  IndexRebuild), archive **8/187**, Segment 16/68, P2P 5/46, teardown PASS,
  0 ASan reports; manifests `45DF041E…`, `5D0BA2B7…`, `7C885312…`,
  `78042830…`, `94F84F1A…`.
- **BLOCKED под ASan:** `Test_NrdPreCommit` (2/20) и `Test_KernelIndex` (3/48) —
  линковка требует BlockChain→Database→неинструментированный rocksdb
  (`LNK2019`; evidence `asan-d2-blocked-kernidx/linker_errors.txt`). Покрыты
  normal-binary suite на DDC21F45….
- ASan-результаты привязаны к source manifest/TestsMini SHA, не к DDC21F45…;
  normal authoritative evidence = DDC21F45….
- **Атрибуция прироста подтверждена взаимно (line2 + line3, арифметика
  проверена):** Nrd* 20/375→23/382 = +3/+7 (только `Test_NrdPolicy` 1→4);
  full 232/3926→236/3952 = +3/+7 D2 **и** +1/+19 line1 archive; archive
  7/168→8/187 = целиком line1 update (`Test_PibdServing.cpp`, sha `EDB1BA61…`,
  modified-uncommitted, последний коммит `b82b8d5`, line2 файл не редактировал).
  Записано в DDC audit doc; line2 — в `binaries-sha256.txt`/`NRD_DIFFERENTIAL.md`
  §7.2. Crafted-zip end-to-end остаётся NOT VERIFIED.
- **Repo binary drift после аудита:** появился новый build
  `C:\DHW\.tmp\line1-build-05\Tests-8F10BFD67047.exe` = SHA `8F10BFD6…`
  (9 172 480 B, PE ts `0x6AE8FC0B`, build-console.log BUILD OK; рядом
  GrinNode-5CA47DA95898); текущий repo `Tests.exe` = `8F10BFD6…`. Handoff от
  line1 по этому SHA ещё не получен — DDC-результаты к нему **не привязываются**;
  нужен отдельный re-hash/re-audit.

## 14. Run-only binding 8F10BFD6 (Lead-approved) — PASS (2026-11-03)

Подробно: `evidence/line3/normal-binary-8f10-audit-20261103.md`.

- Identity: `8F10BFD67047DD25FAB3C201C29FA125096EF4DD7E8EFFFE33B61627CF1D73D6`,
  9 172 480 B, PE ts `0x6AE8FC0B`; repo binary = line2 preserved
  `Tests-8F10BFD67047.exe` (+`.sha256.txt`) = line1-build-05 copy; SHA
  **стабилен до и после** моих прогонов; build-console.log BUILD OK.
- Line2 run-only evidence проверен: `nrd-bind-8F10BFD6-…` 23/382 PASS,
  `kernidx-bind-8F10BFD6-…` 3/48 PASS, per-dir `Tests.exe` = 8F10BFD6…;
  differential report `EA44A390…` (14/14).
- Line3 independent isolated runs (`-KeepBinary`, профиль не изменён, exit 0,
  stderr пуст): `Nrd*` **23/382 PASS** (`4D0860F0…`), `KernelIndex*` **3/48**
  (`B5CECC9A…`), archive **8/187** (`7463675E…`), full suite **236/3952**
  (`06B30BE1…`); kept `Tests.exe` везде = 8F10BFD6….
- **Provenance:** скан всех компилируемых корней показал **0 файлов**,
  изменённых между сборками DDC21F45 (03:59:57) и 8F10BFD6 (04:09:31) — это
  source-identical rebuild; поэтому счётчики совпадают с DDC. DDC21F45 сохранён
  исторически, 8F10BFD6 — текущий normal authoritative SHA.
- **ASan binding:** copied tree файл-в-файл == текущие компилируемые источники
  (mismatch=0), т.е. ASan source manifest `5A769673…`/TestsMini `4AB0CDDD…`
  построены из того же исходного содержимого; но ASan-результаты не
  привязываются к normal SHA. `Test_NrdPreCommit` (2/20) и `Test_KernelIndex`
  (3/48) остаются ASan-BLOCKED (rocksdb link boundary), покрыты normal-suite.
- Ограничения: crafted-zip NOT VERIFIED, txpool gate unwired, archive
  default-off, live G-09 NOT VERIFIED, DB/rocksdb не ASan, coverage нет.

## 15. Common CI harness, fault-injection gap matrix, pipeline (2026-11-03)

- **Cross-line CI регрессии:** `tools/build/ci-regressions.ps1` — группы
  `advertised-tip` (`P2P::AdvertisedTipUsesHeaderHead`, `...ReadsOneSnapshot`),
  `false-ban` (`NrdFalseBan*`), `nrd-rollback-reorg` (`NrdRollback*`,
  `NrdReorgBackup*`), `rebuild-interruption` (`KernelIndex*`, `NrdRebuild*`).
  Прогон `ci-reg-930-01` на preserved 930148B6: все 4 PASS, SHA-bound
  (`evidence/ci/ci-reg-930-01/regressions.json`); параллельный режим с
  раздельными HOME/TMP; wallet/full suite в parallel не входят (фиксированный
  порт 3421).
- **Runnable CI pipeline:** `tools/build/ci-pipeline.ps1`, run `ci-930-02` на
  historical 930148B6 (до финального build-07): overall **PASS_WITH_UNCOVERED**;
  provenance PASS (Tests `930148B6…`, GrinNode `9548B83F…`), normal-build
  NOT RUN (provided binary, no build), unit-regressions PASS, full-suite PASS в
  этом прогоне (242/3980), ASan-subset **NOT RUN (STALE, mismatch=19)**,
  fuzz-smoke PASS (advisory, 100 итераций: bytebuffer 21 ok/79 reject, zip 9
  ok/91 FileException, nrd_rules 100 ok; 0 ASan), untested-zones REPORTED.
  Summary: `evidence/ci/ci-930-02/summary.json`.
- **Flake classification (930, historical):** 6 полных прогонов: 4 PASS
  242/3980, 2 FAIL только в wallet/slatepack decrypt (`Slatepack - E2E`
  unknown exception; `Test_Receive` DecryptSlatepack), при `Slatepack*`-only
  8/8 PASS. Line2: 14 провалов = `Create - mg_start failed 127.0.0.1:3421` +
  Tor env. Вывод: full-suite PASS **не детерминирован** в параллельной среде;
  причина — фиксированный wallet-порт 3421/глобальное wallet-состояние, не
  NRD/archive. CI обязан сериализовать wallet/full или выделять порты.
- **Fault-injection gaps:** `docs/modernization/FAULT_INJECTION.md` — матрица
  F1–F4 (read error при NRD rebuild; прерывание до commit; snapshot activation
  errors; rollback после reorg), текущее покрытие — unit-симуляции, и что
  должны передать линии 1/2 (точки инъекции/контракты). ASan-часть F1–F3
  заблокирована DB/rocksdb и требует разрешённой пересборки.
- **Статус 930:** по решению Lead **не финальный**; ожидается line1-build-07
  (atomic advertised-tip P0). 930 остаётся historical evidence; ASan binding
  после новых source changes STALE (mismatch=19) и не обновлялся без build.

## 16. FINAL build-07 C8C386BE — rebind/audit (2026-11-03)

Подробно: `evidence/line3/normal-binary-c8c-audit-20261103.md`.

- **Identity (verified):** Tests
  `C8C386BE576A62604404E0FA02E56EE9E8F06B50EA0B15AD109F36F5AFF5B7FE`
  (9 241 600 B, PE ts `0x6AE90EE9`); repo == line1-build-07 == line2 preserved
  copy; GrinNode
  `F9EF06B4CB0962B50E6DDBD186E1C61F5872FAFF9E41A779EE7C317A2ED25BDA`
  (7 805 440 B, PE ts `0x6AE90E4D`); build-console.log BUILD OK.
- **Line3 independent isolated (`-KeepBinary`, profile unchanged, exit 0):**
  full **3/3 PASS — 245 cases / 4013 assertions** (manifests `D2F48904…`;
  02/03 NoEvidenceCopy), `Nrd*` **30/428** (`EFF8DE30…`), `KernelIndex*`
  **3/48** (`BDC1AC83…`), archive **8/187** (`052F6FBE…`), advert
  **2/15** (`01D14772…`), `Slatepack*` **5/5 PASS**.
- **Line2 evidence на C8C проверено:** preserved copy SHA = C8C…;
  Nrd* 30/428, NrdFalseBan* 5/17, KernelIndex* 3/48 — kept copies C8C…;
  paired oracle v3: 11 computed / 3 not_computed / 11 paired / 0 mismatches,
  report `57978B2E…`; oracle `00D0AA51…`; fixture checks без false PASS.
  Плюс line2 full `full-c8c386-20261103-053707` — **PASS 245/4013**, kept
  Tests.exe C8C…, isolated, профиль не изменён, stderr 0, manifest
  `A3B9A607…`; итого на C8C **5/5 полных прогонов PASS** (3 line3 + pipeline +
  line2), wallet/Tor флейка нет.
- **CI pipeline `ci-c8c-01`:** PASS_WITH_UNCOVERED; provenance PASS,
  normal-build NOT RUN (no build), unit-regressions PASS, full-suite PASS
  245/4013, ASan **NOT RUN (STALE, mismatch=19)**, fuzz-smoke PASS (advisory,
  0 ASan), untested-zones REPORTED. Evidence: `evidence/ci/ci-c8c-01/`.
- **Flake resolved:** на 930 2/6 full FAIL из-за фиксированного wallet-порта
  3421/параллельных прогонов; на C8C при сериализации — 4/4 full PASS, 5/5
  Slatepack PASS, line2 full не дублировал. Вывод: environment, не NRD; в CI
  wallet/full — только последовательно или с выделенными портами.
- **Binding:** normal authoritative = Tests `C8C386BE…` + GrinNode
  `F9EF06B4…`; 930/8F10/DDC/FD7C — historical, не привязаны. ASan = source-bound
  `5A769673…`/TestsMini `4AB0CDDD…`; для C8C source set **STALE**, refresh
  требует разрешённой ASan-пересборки. Никаких ASan-claims к C8C386BE.
- Ограничения: rocksdb/DB ASan не покрыты; Test_NrdPreCommit/KernelIndex
  ASan-BLOCKED; coverage-guided fuzzing нет; snapshot/NRD fault-injection —
  unit-симуляции; live G-09 NOT VERIFIED; crafted-zip NOT VERIFIED;
  0xC0000409 root cause NOT VERIFIED.

## 17. ASan refresh build-07, CI port guard, source-drift freeze gate (2026-11-03)

- **ASan refresh (05:42, build-07 source snapshot):** source manifest
  `57590D1ED05CDC59E68D38C66F34F40D0CC24556D58BB2F1D76691055B3D8000`,
  TestsMini `19FE60057312CDC6D28E430F1247E5069E7DE136D1388482FCC3668936244ACF`.
  Subset PASS: `Nrd*` 21/365, archive 8/187, Segment 16/68, P2P 7/61 (оба
  advert-теста), zero-test teardown PASS, 0 ASan reports. ASan-BLOCKED:
  `Test_NrdPreCommit`, `Test_NrdFalseBan`, `Test_NrdBatchSafety`,
  `Test_KernelIndex` (BlockProcessor/BlockChain→Database→non-ASan rocksdb).
- **Stress/flake:** C8C — **8/8 full PASS** (3 line3 + pipeline + line2 + 3
  serialized stress); Slatepack* 5/5. Редкий Slatepack-decrypt FAIL механически
  не доказан: тайминговая корреляция есть (мой FAIL-прогон 930 стартовал через
  3 с после full line2), но причинность не утверждается; классификация
  сохранена как открытый риск, не blanket-env.
- **CI pipeline:** `tools/build/ci-pipeline.ps1` получил exclusive lock +
  preflight `127.0.0.1:3421`; `ci-c8c-02` = PASS_WITH_UNCOVERED (full-suite
  PASS 245/4013, regressions PASS, fuzz advisory PASS, ASan STALE из-за
  post-build drift). Hosted template добавлен:
  `.github/workflows/line3-ci.yml` (self-hosted Windows; **NOT VERIFIED**).
- **Source drift / freeze gate:** после build-07 (05:30) в 05:50 добавлены
  `tests/src/P2P/Test_BlockPipeBan.cpp`, `tests/src/P2P/CMakeLists.txt`,
  изменён `tests/CMakeLists.txt` — атрибуция line2 task-8 (test-only;
  P2P/BlockPipe production не тронуты). В C8C их нет; к C8C не привязываются.
  ASan 57590D1E — **build-07-only**, против dirty tree STALE; refresh
  приостановлен. Lead: build-08 = frozen integration (line1 Ping/Pong fix +
  line2 task-8), затем один ASan refresh и normal re-audit. C8C остаётся
  historical; `ci-c8c-02` валиден только для C8C; никаких claims на WIP.
- **Build-08 freeze pre-registered (verified):** production delta = ровно
  `src/P2P/MessageProcessor.cpp`, SHA
  `964FE6D745C5EF4987FB699E3856BDE7C40277885CD338A7D17F1504C812830B`
  (21 794 B, 12+/5−; `case Ping` → один `GetAdvertisedTip()` snapshot, nullopt
  ⇒ Pong withheld, без CONFIRMED fallback); единственный production-файл в
  `git status -- src include`. Test-only delta — line2 task-8/8b
  (`Test_BlockPipeBan`, CMake, TestServer/TxBuilder helpers, real-DB test).
  Чек-лист прихода build-08: `evidence/line3/build-08-freeze-expectation.md`.
  ASan paused; сборок нет.

## 18. Build-08 historical FAIL → build-09 re-freeze (2026-11-03)

Подробно: `evidence/line3/normal-binary-build08-historical-audit.md`.

- **Build-08 identity (verified):** Tests
  `7994F1915F3FC770A5658034174808C1AC4CC8E1CB1410B540259D762ECFE45F`
  (9 316 864 B, PE `0x6AE9185D`) + GrinNode
  `33258AE746DAA733CCF12529DF591D5C2D340B68A694A90B41C00A2CDC9BE974`
  (7 805 440 B, PE `0x6AE917B2`); repo == line1-build-08 retry copies ==
  line2 preserved; build-log BUILD OK; freeze aggregate
  `EAF90A6F…` (1289 files, 0 source changes after build start).
- **Line3 normal audit:** full suite **FAIL — 249 cases, 237 passed / 12
  failed** (2 прогона, assertions 4001→3989 / 4017→4005); archive **8/187
  PASS**, Nrd* **30/428 PASS**, KernelIndex* **3/48 PASS**, advert **2/15
  PASS**, BlockPipe* **3/16 PASS**.
- **Провалы:** 11× environment `Create - mg_start failed 127.0.0.1:3421`
  (8 API wallet + 2 Wallet init + TorConnection) + **1× test-only task-8
  дефект** `Test_NrdBodylessRestart.cpp(103)` — TestMiner header version 1 vs
  NRD требует 4 (`VerifyKernelFeatures`), production не затронут. Kill-harness
  (`[.]` hidden, отдельный run) — test-only `fs::copy` restore error.
- Line2 build-08 evidence проверено: preserved SHA 7994F191…; BlockPipe 3/16,
  Nrd 30/428, kernidx 3/48 PASS; bodyless FAIL (тот же test-only дефект).
- **Build-08 — historical, не финальный.** Lead: test-only fixes approved,
  build-09 follows. **ASan refresh build-08 отменён** по Lead (стал бы сразу
  stale).
- **Build-09 re-freeze verified:** TestMiner.h
  `712A8B03048AC2864E01C3EB4F257794C166F96A94328F37BD1D5988FAB42CD5`,
  Test_NrdBodylessRestart.cpp
  `63BE69E07860042F5AAD585C94D3E20D8B49480219D8F5C3DAD43FBB333AD29A`,
  Test_NrdKillHarness.cpp
  `EA289E8685F3D57494029D95460507056464A26630AE77D061A8EC3F2D405A6C`;
  все три в `tests/CMakeLists.txt` (kill harness теперь компилируется);
  production `MessageProcessor.cpp` остаётся `964FE6D7…`; repo Tests —
  historical 7994F191…; после build-08 старта изменены ровно эти 3 test-файла.
- **Ждём build-09 SHA:** затем один ASan refresh на frozen build-09 source +
  normal re-audit full (serialized, port lock)/archive/Nrd/KernelIndex/advert;
  ASan-BLOCKED normal-only: BlockPipe/bodyless/kill (BlockChain→Database→rocksdb).
  C8C/F9EF06B4 и build-08 — historical.

## 19. FINAL build-09 0A3B7D66 — bound with explicit exception (2026-11-03)

Подробно: `evidence/line3/normal-binary-0a3-audit-20261103.md`.

- **Identity verified:** Tests
  `0A3B7D66E78F5E70A6D95616F0138D994AEB820796C0C77E98AEC7EAC4F2735B`
  (9 335 808 B, PE `0x6AE91AB1`) + GrinNode
  `F90EEF389D1B35B7A17BD8E3C35F06AECFD9C39410C4F755713E1CD1F448A2F8`
  (7 805 440 B, PE `0x6AE919F2`); repo == `C:\DHW\.tmp\build-09` copies;
  BUILD OK; freeze aggregate `832C378C…` (1289 files, 0 source changes after
  build start); production `MessageProcessor.cpp` `964FE6D7…`.
- **Normal audit:** full ×2 **FAIL — 249 cases / 248 passed / 1 failed;
  assertions 4033 / 4032 passed** (identical both runs). PASS: `Nrd*` 30/428,
  `KernelIndex*` 3/48, archive 8/187, advert 2/15, BlockPipe* 3/16.
  Единственный провал — **test-only** `Test_NrdBodylessRestart.cpp(103)`
  (short-chain header-version contradiction); bodyless-NRD + kill-recovery
  **NOT VERIFIED**; production/agreed-suite failures = 0.
- **Lead decision (b):** build-09 = normal authoritative **WITH EXPLICIT
  EXCEPTION**, не fully green; build-10 — backlog, только по отдельному approval.
- **Line2 build-09 evidence verified:** preserved `Tests-0A3B7D66E78F.exe`
  SHA 0A3B7D66…; blockpipe 3/16, Nrd 30/428, kernidx 3/48 PASS; bodyless FAIL
  (тот же test-only); kill NOT VERIFIED.
- **ASan refresh (ONE):** source manifest
  `CF78C188A3F69F242F326B5DEFDA3DFD1D4A1E3A11591DAB8A9CDBF98DB68D85`,
  TestsMini `12C3D16C21E80F86BC3C736BE5046872454D1A6D7FA26B018D782E89D153778B`;
  subset PASS: Nrd* 21/365, archive 8/187, Segment 16/68, P2P 7/61 (incl.
  advert), teardown PASS, **0 ASan reports**; ASan-BLOCKED normal-only:
  PreCommit/FalseBan/BatchSafety/KernelIndex/BlockPipe/bodyless/kill.
- **Pipeline `ci-0a3-01`:** overall **FAIL** (честно, из-за 1 test-only
  full-suite провала); provenance PASS, unit-regressions PASS, full-suite FAIL
  (249/248/1), **asan-subset PASS**, fuzz advisory PASS, untested-zones
  REPORTED. Evidence: `evidence/ci/ci-0a3-01/`.
- **Binding:** normal authoritative = Tests `0A3B7D66…` + GrinNode `F90EEF38…`
  **with exception** (не fully green); ASan = source-bound
  `CF78C188…`/TestsMini `12C3D16C…`; C8C/build-08/930/8F10/DDC/FD7C —
  historical. Hosted CI NOT VERIFIED.

## 20. G-09 7B live PASS — build-09, scenario-scoped (2026-11-03)

Lead independently observed the final success; line1 final evidence captured
2026-11-03T01:35:38Z (08:35:38+07:00). Rust `no_sync` tip 3318655 hash
`2945348ef765160a160008cfd7c2b41aa44ed3cea77d6bcfe2083acf0fc4bbf1`, conn=1;
C++ `FULLY_SYNCED`, same tip/hash/TD, conn=1, in/out=1/0. This is the first
ordinary repo-binary full PIBD → validation → activation → body sync → no_sync
on the fixture.

- **Build binding:** GrinNode `F90EEF38…` (7 805 440 B) + build-09 freeze
  `cb510302f5bebc0d64c5d6a1c0571046a63593ef`; 1289 files aggregate
  `832C378C…`; `MessageProcessor.cpp` `964FE6D7…`; freeze snapshot
  `8DADD770…`. Tests `0A3B7D66…` was not used in the live 7B run. Current HEAD
  `ee843f05…` is post-build evidence/docs/task-9 and is **not** the build-09
  source freeze.
- **Rust reference:** `G09RustLive.exe` `E5219C91…`, user agent
  `MW/Grin 5.5.2.dbaff34`.
- **Restart PASS:** persisted profile restarted (C++ first, then Rust); final
  Rust `no_sync` tip 3318655 same hash, C++ `FULLY_SYNCED` inbound=1; **0** new
  C++ PIBD segment requests and **0** Rust PIBD markers. Evidence
  `7B-restart-final.txt` `A6105921…`, status `A14B96DE…`, cpp-new
  `0AC62411…`, rust-new `545119FF…`.
- **Scope:** initial correct-H fixture only; C++ fixture data; loopback
  allow-list; no public peers; initial run no restart. Reorg/horizon are
  BLOCKED; negative/resource and Output Commitment are NOT VERIFIED. Build-10
  final normal/ASan remains pending; no ASan claim.
- **Live findings (no patch):** 60s C++ receive-idle drop during rangeproof
  validation self-healed; C++ `ArchivePibdState` logged once
  `Rewind: Spent output not found for Commitment{0989ac92…}` after activation,
  archive-regeneration issue separate from restart `no_sync`, needs
  line2/G-09 archive review.
- **7D Output Commitment:** 0 live occurrences in 7B/restart logs; static
  `TransactionInput.cpp:27-31` / `CoinView.cpp:5-17` mechanism code-confirmed;
  controlled A→B repro NOT EXECUTED/BLOCKED under no-source; evidence
  `7D-output-commitment.txt` `BF4DF41D…`. Not reproduced, no consensus/PIBD
  impact shown.
- **Evidence:** `evidence/line3/g09-live-02-7b-success-20261103.md` +
  `.../MANIFEST.json`; final line1 copies verified byte-for-byte (7B-pass
  `A18EA2C2…`, cpp log `77518AEA…`, rust log `B904A7A7…`, post-validation
  `C0054CAB…`). Nodes were stopped after evidence capture; line3 did not restart.

## 21. FINAL build-10 normal audit — bodyless-only exception; ASan refresh PASS (2026-11-03)

Final candidate identity: Tests
`C17C3BC04BE04C501DB425E3617A71E61985E9257EF41D879E130E28F8B621A4`
(9,410,560 B, PE `0x6AE93F14`) + GrinNode
`60D7F7A6B3FFCC702B260E80036B7143A06DB9B036AD3DF3754B3C7B9438CB23`
(7,809,024 B, PE `0x6AE93E33`); BUILD OK; preserved `C:\DHW\.tmp\build-10`
and `line1-build-10`.

- **Clean full:** serialized `b10-full-02` **254 cases / 253 passed / 1 failed;
  4118 assertions / 4117 passed**; zero `mg_start`; sole failure is known
  test-only `Test_NrdBodylessRestart.cpp(103)`.
- **Invalid run:** `b10-full-01` (254/238/16; 15× wallet/Tor `mg_start`
  port-3421 collision) caused by a parallel line2 Tests process; preserved but
  **not counted**.
- **Filters all PASS:** archive 8/187, `Nrd*` 30/428, `KernelIndex*` 5/74,
  `GetKernelHandler*` 2/49, `KernelIndex schema v2*` 2/26,
  `TxHashSet duplicate-excess*` 1/10, `P2P::AdvertisedTip*` 2/15. Quoted
  `-TestArgs`, `-FailOnNoTests`, `-KeepBinary`; source SHA checked in each
  manifest; real user profile unchanged.
- **Result:** normal **FAIL overall, bodyless-only**; production/agreed-suite
  failures = 0; bound as final normal candidate **with explicit exception**,
  not fully green. Bodyless-NRD/kill recovery remain NOT VERIFIED.
- **Source freeze:** `evidence/line3/build-10-freeze-snapshot-final.json`
  (`0E51D632…`, 1002 files, aggregate `F1E47615…`, HEAD `77b3970…`); only docs
  changed after build; ASan copied tree matches all 1002 entries (0 mismatch).
- **ASan refresh PASS:** source manifest `776B0DB3…` (1292 files), build
  `full-asan-20261103-090921` BUILD_PASS 98.6 s; TestsMini `497353EC…`.
  Nrd* 21/365, archive 8/187, Segment 16/68, P2P+advert 7/61, teardown PASS
  (zero tests), **0 ASan reports**. ASan-BLOCKED normal-only:
  PreCommit/FalseBan/BatchSafety/KernelIndex/BlockPipe/bodyless/kill; rocksdb/
  Database/ChainState and sysroot C libs uninstrumented. Optional
  `tests_mini_prefix` NOT BUILT (missing `TestMain_legacy_static.cpp`); teardown
  uses TestsMini and PASSes.
- **Evidence:** `normal-binary-build10-audit-20261103.md`,
  `asan-build10-refresh-20261103.md`, `build10-asan-refresh-20261103.json`,
  `evidence/line3/b10-*-02/`, `evidence/line3/b10-asan-*`.

## 22. Task-10/11 archive recovery — independent audit (2026-11-03)

Read-only audit; original `g09-live-02` not modified; no build/commit. Existing
7B PASS remains valid.

- **Recovery FAIL confirmed (three processes):** target H=3313440, snapshot
  flushed 3318655; block 3317930; commitment
  `0989ac92c748aaf654f06fb9023746d3d34358b051dd2ac952bec39fecc6288967`;
  1 input / **0 spent positions**. Occurrences: 08:26:28, 08:45:10 (build-09
  restart), 08:59:42 (build-10 restart). Existing H=3315600 archive serving and
  7B body/no_sync are unaffected.
- **DB audit contradiction (line2 + line3 fresh copy):** canonical H=3317930
  hash `30c524b8…bfcb` has SPENT_OUTPUTS count=1 (52 B, exact commitment,
  leaf 6856589/output H=3317928); all 68 input-bearing blocks match spent/input
  counts; 5,147 empty records are all 0-input blocks; 0 empty input-bearing
  records. The logged 0 cannot come from the canonical key against the preserved
  DB. Prior “empty record confirmed” inference is **RETRACTED**; actual Rewind
  key/DB view is unknown and requires instrumentation. Line2 evidence
  `7F636BBE…1856`; fresh-copy audit
  `task11-spent-positions-fresh-copy-audit-20261103.json`.
- **Atomic publication OPEN in build-10 source:** direct `fs::copy_file` final
  publish, no fsync/atomic rename, no validation — line1's frozen build-11
  `ArchivePublication.h` is intended to fix this but is unbuilt/unaudited here.
- **Original hashes:** archive H=3315600 `13F9781E…`; marker
  `B671C82A…` = hash `84d7ac67…` / height `3313440`; configs `355B15F7…` /
  `AEE5E753…`; archive dir also contains untouched `.staging-3615bb625917`.
- **Evidence:** `task10-11-archive-recovery-independent-audit-20261103.md`,
  `task10-11-archive-recovery-manifest-20261103.json`,
  `evidence/line3/task10-11-archive-recovery-20261103/`; failure logs
  `77518AEA…` (final), `0AC62411…` (build-09 restart), `CFB19A60…`
  (build-10 restart).

## 23. Build-11 audit — normal bodyless-only + ASan subset PASS (2026-11-03)

Build-11 retry is audited; build-10 remains final only for its own snapshot.

- Tests `02DC3D9C00BB1E1B7256771A158F04F1DB3B739839A7AFF2391AEAA5DAAFC121`
  (9,438,720 B, PE `0x6AE94C3F`); GrinNode
  `565BC2DBF30C638F84B458012F1F07D091B8EF6DB9C933E821A58EF3CF6021A4`
  (7,843,328 B, PE `0x6AE94B43`).
- Build log `B274B34A…` BUILD OK; failed first attempt `ACED40F9…` (C1083)
  excluded. Source freeze `build-11-freeze-snapshot-final.json` `E5F7519C…`;
  HEAD `6dbf8a7`; 9-file set; include fixes ArchivePublication.h
  `D6F3A1C8…`, StateSyncer.cpp `CF273B95…`; line2 files unchanged
  (`BlockDBImpl.cpp F88A107D…`, `RocksDB.h 42B5BF3C…`, Test_BlockPipeBan.cpp
  `FFBF3DF2…`, one inert AreSpentPositionsComplete stub).
- Normal full: **255 cases / 254 passed / 1 failed; 4201 assertions / 4200
  passed**; sole failure known test-only `Test_NrdBodylessRestart.cpp(103)`;
  zero `mg_start`.
- All groups PASS: archive 8/187, Nrd 30/428, KernelIndex 5/74,
  GetKernelHandler 2/49, schema-v2 2/26, duplicate-range 1/10,
  `P2P::ArchivePublication*` 1/83, advert 2/15.
- ASan subset PASS: source manifest `69B01EE6…` (1293 files), full ASan
  `full-asan-20261103-095356` BUILD_PASS 101.1 s, TestsMini `C76CCCF2…`;
  copied tree matches freeze 1003/1003; Nrd 21/365, archive 8/187, Segment
  16/68, P2P+advert 7/61, ArchivePublication 1/83, teardown PASS, **0 reports**.
- Classification: normal **FAIL overall, known test-only bodyless**, production/
  agreed-suite failures 0; ASan subset PASS. This is **not** full G-09/Mainnet
  readiness.
- Still open: build-12 **Release SpentOutputs fix + live H=3313440/H=3315600
  recovery**. Build-11 has the confirmed NDEBUG assert-side-effect bug; its
  normal/ASan PASS does not validate the fix. Existing 7B PASS remains valid
  only for its fixed-H scenario.
- Evidence: `normal-binary-build11-audit-20261103.md`,
  `asan-build11-refresh-20261103.md`, `build11-asan-refresh-20261103.json`,
  `build11-audit-complete-20261103.md`, `evidence/line3/b11-*-01/`,
  `b11-asan-*-01/`, `full-asan-20261103-095356/`.

## 24. Task-10/11 root cause — Release NDEBUG SpentOutputs (2026-11-03)

**CONFIRMED production Release bug.** No build/commit by line3.

- build-11 `SpentOutput.h` `F3F38B44…` line 95:
  `assert(byteBuffer.ReadU8() == 0);`.
- Release targets compile with NDEBUG (`GrinNode.vcxproj:156`,
  `Tests.vcxproj:168`), so assert is a no-op and does not consume the version
  byte. `ReadU16()` then reads `00 00` => count=0; every non-empty record
  returns an empty map.
- Live build-11 proof, Node.log:23840: canonical key
  `30c524b8424f91e1b2b154112f19d77d4c0db6887d6a1cdc96ef2055e022bfcb`,
  `rawBytes=52`, `rawHex=000001…32a0a8`, `mapEntries=0`; line 23841 preflight
  fails “1 input(s) but 0 mapped spent position(s)” (target 3313440, flushed
  3318655). Raw value is correct: version 00, count 0001.
- DB record is **not** empty; earlier “unknown key / unresolved contradiction”
  is resolved. Actual key is canonical, MANIFEST-000451.
- Fix present but UNBUILT: `SpentOutput.h D27A607D…` explicit version read +
  reject non-zero; regression `Test_SpentOutputs.cpp 6157AC56…` (Release
  deserialize + preflight). Build-11 binaries contain old code and cannot
  validate the fix.
- Build-12 status: normal Release full **256/255/1** (4211/4210; only bodyless),
  `SpentOutputs*` 1/6 PASS, exact preflight PASS, ArchivePublication 1/83, all
  other groups PASS; ASan subset PASS (0 reports). Live `H=3313440` and
  `H=3315600` recovery evidence is still **PENDING** (section 25).
- Evidence: `task10-11-spent-deserialize-rootcause-20261103/README.md`,
  `rootcause-20261103.json`, `Node.build11-live.log` (`6552D5E3…`),
  `spent-diag-context.txt` (`22FDD497…`).

## 25. Build-12 audit — normal bodyless-only + ASan subset PASS (2026-11-03)

Build-12 retry is audited for normal/ASan; recovery remains open.

- Tests `D1103F3470C1E7E2507093418392CC69CF24E52137AEB5FC81CB7CE8A06A38CD`
  (9,449,472 B, PE `0x6AE95151`); GrinNode
  `833CCBFD8C344D79AAC748107F60492B055D995BE4A2C70D6F19115C8A01751C`
  (7,843,328 B, PE `0x6AE95056`); build log `76BC0E49…` BUILD OK.
- Source freeze `build-12-freeze-snapshot-final.json` `64BE17E5…`; 11-file set;
  `SpentOutput.h D27A607D…` explicit version fix; regression
  `Test_SpentOutputs.cpp 6157AC56…`.
- Normal full **256 cases / 255 passed / 1 failed; 4211 assertions / 4210
  passed**; sole test-only bodyless `Test_NrdBodylessRestart.cpp(103)`.
- PASS groups: `SpentOutputs*` 1/6, exact preflight PASS, ArchivePublication
  1/83, archive 8/187, GetKernel 2/49, schema 2/26, duplicate-range 1/10,
  Nrd 30/428, KernelIndex 5/74, advert 2/15.
- ASan subset PASS: source manifest `3DB12364…` (1293 files), full ASan
  `full-asan-20261103-101529` BUILD_PASS 96.4 s, TestsMini `CB09CEE8…`;
  freeze match 1003/1003; Nrd 21/365, archive 8/187, Segment 16/68,
  P2P+advert 7/61, ArchivePublication 1/83, teardown PASS, **0 reports**.
  Test_SpentOutputs not in TestsMini; Release regression covered normal only.
- Still pending: live copied-fixture recovery **H=3313440** (raw52 -> map1,
  preflight pass, validated atomic archive) and **H=3315600** boundary;
  crash-interrupted publication; full G-09/Mainnet. Task-10/11 remain open
  until that evidence exists.
- Evidence: `normal-binary-build12-audit-20261103.md` (`025B1F92…`),
  `asan-build12-refresh-20261103.md` (`3AB72CD5…`),
  `build12-asan-refresh-20261103.json` (`2645717B…`),
  `build-12-freeze-snapshot-final.json` (`64BE17E5…`),
  `evidence/line3/b12-*-01/`, `b12-asan-*-01/`,
  `full-asan-20261103-101529/`.

## 26. Build-12 recovery boundary — build-13 pending (2026-11-03)

Build-12's SpentOutputs root-cause fix is **verified**: canonical raw 52 B
`30c524b8…bfcb` now logs **`mapEntries=1`** (`h3313440-node1.log:24645`;
build-11 was `mapEntries=0`). Normal 256/255/1 bodyless-only and ASan subset
PASS.

Recovery H=3313440 still fails on a **separate preflight boundary bug**:
`StateSyncer::PreflightArchiveRewind` (`CF273B95…`, lines 454-490) includes
`targetHeight` and requires the target block body/spent record. Build-12 log
line 269:

```text
archive preflight: missing block body at height 3313440
hash 84d7ac67…9ccea (target 3313440)
```

`TxHashSet::Rewind` (`TxHashSetImpl.cpp:665-720`) only processes blocks above
the target; target body is not needed. Fix approved: validate target
header/roots and preflight `flushedHeight..targetHeight+1`, no target body.
Build-13 must audit H=3313440 and H=3315600 (line1 snapshot pending) on copies,
with normal/ASan, atomic archive validation, no partial publish, and original
untouched. Build-12 normal/ASan does not close task-10/11.

- Evidence: `build13-preaudit-20261103.md`,
  `build13-preaudit-20261103.json`,
  `evidence/line3/build13-preauth-20261103/build12-h3313440-node.log`
  (`270D4265…`).

## 27. Build-13 audit — normal/ASan + H=3313440/H=3315600 recovery (2026-11-03)

Build-13 is the next final candidate; build-12 historical.

- Tests `001DF4BC31F194DCDDE83FE736B06EB6FD942666F2BEFF62FF1FB5F1BB8DB3A8`
  (9,465,856 B, PE `0x6AE957C5`); GrinNode
  `F39BDFE5A8DAB22BE969406874EDCEFD023770BE1A1BA6330D7506B2C2571201`
  (7,846,912 B, PE `0x6AE956CB`); build log `8C54DD17…` BUILD OK.
- Source freeze `build-13-freeze-snapshot-final.json` `4FC8DCE1…`; boundary
  fix `StateSyncer.cpp FDC5B4BB…`, `ArchivePublication.h F1E43737…`, regression
  `Test_SpentOutputs.cpp A80D1D97…`.
- Normal full **257 cases / 256 passed / 1 failed; 4219 assertions / 4218
  passed**; sole test-only bodyless. PASS: SpentOutputs 1/6, exact preflight
  1/4, boundary 1/8, ArchivePublication 1/83, archive 8/187, GetKernel 2/49,
  schema 2/26, duplicate 1/10, Nrd 30/428, KernelIndex 5/74, advert 2/15.
- ASan subset PASS: source manifest `BFBF2186…` (1293 files), full ASan
  `full-asan-20261103-104255` BUILD_PASS 87.5 s, TestsMini `ECDD6D5E…`;
  freeze match 1003/1003; Nrd 21/365, archive 8/187, Segment 16/68,
  P2P+advert 7/61, ArchivePublication 1/83, teardown PASS, **0 reports**.
- **H=3313440 observed PASS:** archived `TxHashSet.3313440.84d7ac676356.zip`
  3,116,449,317 B SHA `7FB23058…`; 17 entries/required/leaf; Extract +
  `LoadAndValidate` before atomic publish; no failure signals; post tip/roots
  unchanged; no partial H3313440 final.
- **H=3315600 observed PASS:** new snapshot 3,118,756,420 B SHA `DA23A1C7…`;
  old `13F9781E…` preserved; archived + PIBD serving ready, crosses 3317930.
- **Line1 formal report pending.** No crash-interruption/full G-09/Mainnet
  claim. Canonical build-12 map1 evidence remains `30c524b8…` raw52/map1;
  build-13 archive success independently observed.
- Evidence: `normal-binary-build13-audit-20261103.md`,
  `asan-build13-refresh-20261103.md`, `build13-asan-refresh-20261103.json`,
  `task10-11-build13-recovery-audit-20261103.md`,
  `task10-11-build13-recovery-20261103.json`,
  `build13-h3313440-node.log` (`D099D379…`), `b13-*-01/`, `b13-asan-*-01/`.
