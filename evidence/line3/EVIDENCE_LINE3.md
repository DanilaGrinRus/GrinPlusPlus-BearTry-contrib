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
