# ASan archive-server subset (line 1) + post-task-5 refresher — 2026-11-03

Lead: после появления archive-server тестов линии 1 включить их в полный
ASan-subset, если link deps позволяют. Выполнено в copied tree; normal build не
запускался.

## Источники, к которым привязан результат

- `tests/src/PMMR/Test_PibdServing.cpp` (fixed lease fixture line1):
  sha256 `4646F4DF256415DF561F66A6C16569AF068825D043F64B38D9A8CB604B3E9D52`
  (repo и copied tree совпадают; `pFirst.reset()`, `pSecond.reset()`,
  `REQUIRE(lease.use_count() == 1)` присутствуют).
- Полная ASan-сборка (copied tree, после line1/line2 обновлений):
  source manifest sha256
  `857AA38E1E92D9C422AA02CF830A41B9AE3752000EC337255A2BEFCEA824ED9A`
  (1283 файла), build manifest sha256
  `27380EBC170DD1E5027EF25DE76509C673D74374113C40D85EEF3DB3C9902A37`,
  evidence `full-asan-20261103-033906/` (BUILD PASS, 87.5 s).
- `TestsMini.exe` sha256
  `2DA4727EE676E6663B6EEBC00F5ECC73848EA808819436F4887891F8BE3166ED`
  (включает Test_PibdServing.cpp + прежние NRD/Segment/P2P тесты; линковка с
  полными ASan-библиотеками прошла без Database/rocksdb).

**Важно:** это source-based ASan-сборка из copied tree с незакоммиченным фиксом
line1 и изменениями task-5 line2. Она **не** привязана к нормальному бинарнику
`2032536D…`; нормальный `Tests.exe` в repo остаётся старым (full suite с ним
красный до пересборки).

## Прогоны под ASan — PASS

Все: isolated HOME/TMP, `GRINPP_NRD_CASES` → repo cases json, профиль не
изменён, exit 0, stderr пуст, ASan-отчётов нет; `Tests.exe` сохранён в каждом
evidence dir (`-KeepBinary`, SHA = TestsMini SHA).

| run | filter | результат | manifest sha256 |
|---|---|---|---|
| `asan-line1-lease-01` | `PibdArchiveServer::LeaseOutlivesHorizonSwitch` | **1 case / 16 assertions PASS** | `33D38525…` |
| `asan-line1-archive-01` | `PibdArchiveServer::*` | **7 cases / 168 assertions PASS** | `92363128…` |
| `asan-posttask5-nrd-01` | `Nrd*` | 12 / 329 PASS | `53618F6E…` |
| `asan-posttask5-segment-01` | `Segment*` | 16 / 68 PASS | `6B3304B5…` |
| `asan-posttask5-p2p-01` | `Capabilities::*,MessageTypes::*,GrinVersion::*` | 5 / 46 PASS | `FA9F6F74…` |
| `asan-teardown-postline1-01` | zero-test teardown regression | **PASS** (7/7 checks) | `7D9DDF8A…` |

Pre-fix контекст: на нормальном бинарнике `2032536D…` тот же lease-тест
детерминированно падал (`REQUIRE(destroyed == 1)` → `0 == 1`), независимо
воспроизведено line3 в `evidence/line3/line3-audit-pibd-lease-01/`
(и line2: `full-20261103-033401`, `lease-recheck-20261103-033526`). Line1
классифицировал это как **test fixture bug** (лишний сильный owner `pFirst`),
production lifetime корректна; фикс не ослабляет assertion. Под ASan
исправленный fixture проходит, memory-errors не найдено — это подтверждает
классификацию line1 в рамках ASan-профиля.

## Ограничения

- Database/ChainState/rocksdb не инструментированы; archive-тесты использовали
  mmap/minizip — minizip uninstrumented (ASan проверяет вызывающий код).
- TestsMini — подмножество (41 тест: 7+12+16+5+…), не полный набор 231 case.
- Coverage не измерялась; ASAN_OPTIONS=detect_leaks=0.
- ASan-результат не заменяет normal-build PASS: после пересборки нормального
  Tests.exe нужен новый SHA и новый изолированный прогон (full suite должен
  стать зелёным).

## Команды

```powershell
pwsh tools/fuzz/build_full_asan.ps1          # copied tree, ASan libs
pwsh tools/fuzz/build_linked_targets.ps1 -Only tests_mini
pwsh tools/build/run-tests-isolated.ps1 -ExePath <...>\TestsMini.exe `
  -ExeArgs 'PibdArchiveServer::*' -KeepBinary
pwsh tools/fuzz/run_asan_teardown_regression.ps1 -ExePath <...>\TestsMini.exe
```
