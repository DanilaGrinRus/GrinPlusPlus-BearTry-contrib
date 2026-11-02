# Normal-binary audit FD7C0492 (line1 final integrated build) — 2026-11-03

Lead/line1 final normal Tests.exe after line1 lease-fixture fix + line2 task-5
sources. This document records the SHA-bound independent re-audit.

## 1. Binary identity (verified)

| Поле | Значение |
|---|---|
| SHA-256 | `FD7C04922A2E016723531EC389E21D3AC1E8C0F99134F003595DB18149AB16E5` |
| size | 9 154 048 bytes |
| PE ts | `0x6AE8F63F` (2026-11-02T20:44:47Z) |
| mtime | 2026-11-03T03:44:47+07:00 |
| repo path | `bin\Release\Release\Tests.exe` — **идентичен** сохранённой копии (SHA/size/mtime) |
| preserved copy | `C:\DHW\.tmp\line1-build-03\Tests-FD7C04922A2E.exe` — SHA совпал |
| build | `pwsh tools/build/build.ps1 -Target Tests` → BUILD OK (log `C:\DHW\.tmp\line1-build-03\build-console.log`) |

Line2 независимо сохранил ту же копию:
`tools/audit/nrd/evidence/bin/Tests-FD7C04922A2E.exe` (+ `.sha256.txt`), SHA
проверен — совпал.

## 2. Независимые изолированные прогоны line3 — PASS

`tools/build/run-tests-isolated.ps1`, exe = preserved FD7C copy, `-KeepBinary`
(в каждом evidence dir лежит `Tests.exe` с SHA FD7C0492…), isolated HOME/TMP,
`user_profile_unchanged=true`, exit 0, stderr пуст:

| run | filter | результат | manifest sha256 |
|---|---|---|---|
| `normal-fd7c-lease-01` | `PibdArchiveServer::LeaseOutlivesHorizonSwitch` | **1 case / 16 assertions PASS** | `9F665959…` |
| `normal-fd7c-archive-01` | `PibdArchiveServer::*` | **7 cases / 168 assertions PASS** | `AD146118…` |
| `normal-fd7c-nrd-01` | `Nrd*` | **20 / 375 PASS** (14/14 differential) | `AA121B36…` |
| `normal-fd7c-kernidx-01` | `KernelIndex*` | **3 / 48 PASS** | `13AEFAA1…` |
| `normal-fd7c-full-01` | full suite | **232 cases / 3926 assertions PASS** | `C4373967…` |

Это независимо подтверждает заявленные line1/Lead числа: full 232/3926,
archive 7/168, lease 1/16 — на точном SHA `FD7C0492…`.

Line2 binding на этот же SHA проверен: `nrd-bind-FD7C0492-20261103-034652`
(Nrd* 20/375 PASS) и `kernidx-bind-FD7C0492-20261103-034652` (3/48 PASS), в
каждом каталоге собственная копия `Tests.exe` с SHA FD7C0492…;
`cpp-nrd-differential-report-FD7C0492.json` sha `EA44A390…` (14/14).

## 3. Source/binary binding

Рабочее дерево (uncommitted) и copied ASan tree сверены файл-в-файл по
компилируемым корням `CMakeLists.txt, cmake/, include/, src/, tests/, deps/`:
**все файлы идентичны**, кроме:
- `tests/src/TestMain_legacy_static.cpp` — evidence-only negative control, **не
  компилируется** в TestsMini/нормальный билд;
- `docs/modernization/G09_ARCHIVE_SERVING.md` — документация, не компилируется.

Ключевые исходники: `src/P2P/PIBD/PibdArchiveServer.h` sha
`56EE9A8738D56576F74679AF35DA339329DF653F1BE5962DB510CBC2AE8490A0`,
`tests/src/PMMR/Test_PibdServing.cpp` sha
`4646F4DF256415DF561F66A6C16569AF068825D043F64B38D9A8CB604B3E9D52`,
`tests/src/TestMain.cpp` sha
`6BAC17446DE696518FD364220569185F43769B813F59A25372037FBC53D8527B`.

Следствие: normal `FD7C0492…` и ASan `TestsMini 2DA4727E…` собраны из
**идентичного компилируемого исходного содержимого**, но это разные бинарники:

- **Authoritative normal-binary evidence:** `FD7C0492…` (full 232/3926, archive
  7/168, lease 1/16, Nrd 20/375, KernelIndex 3/48).
- **ASan evidence remains bound to:** source manifest
  `857AA38E1E92D9C422AA02CF830A41B9AE3752000EC337255A2BEFCEA824ED9A` +
  `TestsMini.exe` `2DA4727EE676E6663B6EEBC00F5ECC73848EA808819436F4887891F8BE3166ED`,
  with zero ASan reports (archive 7/168, lease 1/16, Nrd 12/329, Segment 16/68,
  P2P 5/46; details `asan-line1-archive-subset-20261103.md`). Эти результаты не
  привязываются к normal SHA FD7C0492.

## 4. Классификация и ограничения

- PASS: normal-binary archive/lease/NRD/KernelIndex/full-suite на точном SHA
  FD7C0492; ASan subset (source-bound) без memory errors.
- Archive serving остаётся **default-off**; ordinary-binary **live G-09 —
  NOT VERIFIED** (нужен live chain/peer).
- `DeferredGarbageDrain` unit-tests проверяют executor (queueing само по себе
  ничего не удаляет; executor удаляет ровно поставленные dirs, идемпотентно);
  worker call-site — только code review, не unit-tested (нужен live chain).
- Database/ChainState/rocksdb под ASan не покрыты; TestsMini — подмножество;
  coverage не измерялась; full-suite PASS не доказывает network/live
  совместимость.

## 5. Команды

```powershell
$exe='C:\DHW\.tmp\line1-build-03\Tests-FD7C04922A2E.exe'
pwsh tools/build/run-tests-isolated.ps1 -ExePath $exe -RunId normal-fd7c-full-01 `
  -KeepBinary -ExeArgs ''
pwsh tools/build/run-tests-isolated.ps1 -ExePath $exe -RunId normal-fd7c-archive-01 `
  -KeepBinary -ExeArgs 'PibdArchiveServer::*'
# при необходимости: -ExeArgs 'Nrd*' / 'KernelIndex*' / 'PibdArchiveServer::LeaseOutlivesHorizonSwitch'
```
