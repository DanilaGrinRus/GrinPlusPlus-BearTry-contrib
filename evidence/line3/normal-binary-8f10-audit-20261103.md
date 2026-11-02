# Normal-binary audit 8F10BFD6 (run-only integrated binding) — 2026-11-03

Lead-approved **run-only binding** to the new integrated SHA (no rebuild). This
audit re-hashes the preserved binary, verifies line2's run evidence, and runs an
independent isolated audit (including the full suite).

## 1. Binary identity (verified before AND after runs)

| Поле | Значение |
|---|---|
| SHA-256 | `8F10BFD67047DD25FAB3C201C29FA125096EF4DD7E8EFFFE33B61627CF1D73D6` |
| size | 9 172 480 bytes |
| PE ts | `0x6AE8FC0B` (2026-11-02T21:09:31Z) |
| mtime | 2026-11-03T04:09:31+07:00 |
| repo | `bin\Release\Release\Tests.exe` — SHA неизменен до и после моих прогонов |
| preserved (line2) | `tools/audit/nrd/evidence/bin/Tests-8F10BFD67047.exe` + `Tests-8F10BFD6.sha256.txt`; SHA копии = 8F10BFD6… |
| preserved (line1) | `C:\DHW\.tmp\line1-build-05\Tests-8F10BFD67047.exe` (same SHA); `build-console.log` BUILD OK; `GrinNode-5CA47DA95898.exe` рядом |
| previous | DDC21F45… и более ранние сохранены |

## 2. Line2 run-only binding — проверено

- `nrd-bind-8F10BFD6-20261103-041147`: PASS, exit 0, **23 cases / 382 assertions**;
  собственная копия `Tests.exe` в dir = 8F10BFD6…; профиль не изменён.
- `kernidx-bind-8F10BFD6-20261103-041147`: PASS, exit 0, **3 / 48**; копия = 8F10BFD6….
- `cpp-nrd-differential-report-8F10BFD6.json`: sha `EA44A390…` (14/14) — совпал.

## 3. Независимый аудит line3 — PASS

`run-tests-isolated.ps1` на preserved copy, `-KeepBinary` (kept `Tests.exe` в
каждом dir = 8F10BFD6…), isolated HOME/TMP, профиль не изменён, exit 0,
stderr пуст:

| run | filter | результат | manifest sha256 |
|---|---|---|---|
| `normal-8f10-nrd-01` | `Nrd*` | **23 cases / 382 assertions PASS** | `4D0860F0…` |
| `normal-8f10-kernidx-01` | `KernelIndex*` | **3 / 48 PASS** | `B5CECC9A…` |
| `normal-8f10-archive-01` | `PibdArchiveServer::*` | **8 / 187 PASS** | `7463675E…` |
| `normal-8f10-full-01` | full suite | **236 / 3952 PASS** | `06B30BE1…` |

Full-suite claim Lead/line1 (236/3952) **независимо подтверждён** line3.
Счётчики идентичны DDC21F45-раунду.

## 4. Provenance: 8F10 — source-identical rebuild of DDC21F45

Сканирование всех компилируемых корней (`CMakeLists.txt`, `cmake/`, `include/`,
`src/`, `tests/`, `deps/`) на файлы, изменённые между сборкой DDC
(2026-11-03T03:59:57) и сборкой 8F10 (04:09:31): **0 файлов**. За это окно
изменялись только `docs/modernization/{CI_REPRO,SANITIZERS}.md` и
`tools/fuzz/build_linked_targets.ps1` (не компилируются в Tests).
Следовательно 8F10 и DDC21F45 собраны из идентичного компилируемого
содержимого (объясняет одинаковый размер 9 172 480 B и идентичные счётчики);
SHA различаются из-за новой линковки/PE timestamp. DDC21F45 сохранён как
исторический, 8F10BFD6 — текущий интегрированный normal SHA.

Ключевые исходники текущего дерева (HEAD `e0a5d2d`, 37 dirty entries):
`src/P2P/PIBD/PibdArchiveServer.h 98522BCD1A5D5EAA…`,
`tests/src/PMMR/Test_PibdServing.cpp EDB1BA612B18B5DA…`,
`src/BlockChain/Validators/BlockValidator.cpp AAA96950A846EEC6…`,
`src/BlockChain/Processors/TxHashSetProcessor.cpp 0D10CEE54B201D24…`,
`include/Core/Config.h 12D9DED6FB065BE1…`, `src/Core/Config.cpp 5149F028558099C8…`,
`tests/src/TestMain.cpp 6BAC17446DE69651…`.

## 5. ASan binding

Мой ASan copied tree file-by-file совпадает с текущим repo по всем
компилируемым корням (`mismatch_count=0`). Значит ASan source manifest
`5A7696737C74B067D92973A7925CF6F23C88152B9C9F3841DDBAD5A1CA17EFD5` /
`TestsMini 4AB0CDDD…` построены из того же компилируемого исходного
содержимого, что и normal 8F10BFD6/DDC21F45. Но это разные бинарники и разные
доказательства:

- **Normal authoritative:** `8F10BFD6…` (full 236/3952, Nrd 23/382, KernelIndex
  3/48, archive 8/187).
- **ASan (source-bound):** Nrd* 21/362 (без `Test_NrdPreCommit`), archive 8/187,
  Segment 16/68, P2P 5/46, teardown PASS, 0 ASan reports. `Test_NrdPreCommit`
  (2/20) и `Test_KernelIndex` (3/48) ASan-**BLOCKED** линковкой
  BlockChain→Database→non-ASan rocksdb (`asan-d2-blocked-kernidx/linker_errors.txt`).

## 6. Ограничения

- Crafted-zip end-to-end `ProcessTxHashSet` — **NOT VERIFIED** (blocker у line2).
- D2 txpool gate и NRD-index policy early-return не подключены (line2 scope).
- Archive serving default-off; ordinary-binary live G-09 **NOT VERIFIED**.
- Database/ChainState/rocksdb под ASan не покрыты; TestsMini — подмножество;
  coverage не измерялась; full-suite PASS ≠ live/network compatibility.

## 7. Команды

```powershell
$exe='C:\DHW\Gplus\src\grinplusplus\tools\audit\nrd\evidence\bin\Tests-8F10BFD67047.exe'
pwsh tools/build/run-tests-isolated.ps1 -ExePath $exe -RunId normal-8f10-full-01 -KeepBinary
pwsh tools/build/run-tests-isolated.ps1 -ExePath $exe -RunId normal-8f10-nrd-01 -KeepBinary -ExeArgs 'Nrd*'
pwsh tools/build/run-tests-isolated.ps1 -ExePath $exe -RunId normal-8f10-kernidx-01 -KeepBinary -ExeArgs 'KernelIndex*'
pwsh tools/build/run-tests-isolated.ps1 -ExePath $exe -RunId normal-8f10-archive-01 -KeepBinary -ExeArgs 'PibdArchiveServer::*'
```
