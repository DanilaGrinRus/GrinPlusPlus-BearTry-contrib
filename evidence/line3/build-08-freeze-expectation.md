# Build-08 freeze expectation (line3 pre-registration) — 2026-11-03

Lead plan: build-08 = frozen integration of line1 Pong advertisement fix + line2
task-8 tests. ASan refresh held until build-08 SHA; C8C386BE/F9EF06B4 historical.

## Frozen production delta (verified 2026-11-03T06:0x)

Exactly one production file:

| Файл | SHA-256 | Size | mtime | git diff |
|---|---|---|---|---|
| `src/P2P/MessageProcessor.cpp` | `964FE6D745C5EF4987FB699E3856BDE7C40277885CD338A7D17F1504C812830B` | 21 794 B | 05:52:02 | 12+/5− |

`git status --porcelain -- src include` → только `M src/P2P/MessageProcessor.cpp`
(иных production-изменений line1 на момент проверки нет).

Семантика `case Ping` (по diff): Pong использует один снимок
`GetAdvertisedTip()` (header head); при `nullopt` — Pong **не отправляется**;
fallback на CONFIRMED/смешивание значений отсутствует. Root-cause evidence:
`C:\DHW\.tmp\task7-A\A-root-cause.txt` (978 B), `restart-trace.log` (1 681 093 B).

## Test-only delta (line2 task-8/8b, не production)

- `tests/src/P2P/Test_BlockPipeBan.cpp` (+ `tests/src/P2P/CMakeLists.txt`),
  `tests/CMakeLists.txt` — line2 task-8a;
- ожидаемые task-8b: `tests/framework/TestServer` (OpenExisting/preserve-data),
  `tests/framework/TxBuilder` (optional NRD kernel criteria/lock_height),
  real-DB bodyless→NRD→reorg→restart test под `tests/src/BlockChain` + CMake.
- В C8C386BE этих файлов нет; к C8C не привязываются. ASan-линковка части этих
  тестов ожидаемо упирается в BlockChain→Database→rocksdb (normal-only).

## Обязательный чек-лист при приходе build-08

1. Репо `Tests.exe` == preserved copy == line1-build-08 copy; SHA + size + PE ts
   + build log BUILD OK; `GrinNode.exe` SHA/PE/size и preserved copy.
2. `src/P2P/MessageProcessor.cpp` SHA == `964FE6D7…` (frozen production delta);
   дерево после сборки не меняется (mtime/scan компилируемых корней).
3. Line2 task-8/8b files присутствуют в собранном дереве и отражены в
   `Tests`; test-only изменения не меняют production-поведение.
4. Normal re-audit с `-KeepBinary`: full (serialized, exclusive port-3421 lock +
   preflight), archive, Nrd*, KernelIndex*, `P2P::AdvertisedTip*` (+ новые P2P
   тесты line1/line2 в normal-binary).
5. **Один** ASan refresh на frozen build-08 source в copied tree; subset
   Nrd*/archive/Segment/P2P+advert/teardown; ASan-BLOCKED тесты (PreCommit,
   FalseBan, BatchSafety, KernelIndex, BlockPipeBan, real-DB) — normal-only.
6. Pipeline re-run на build-08; обновить binding (C8C → historical; build-08 →
   normal authoritative; ASan → source-bound к frozen manifest). Hosted CI
   остаётся NOT VERIFIED.
