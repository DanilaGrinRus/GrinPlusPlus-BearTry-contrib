# Normal-binary audit 930148B6 + conflicting full-suite classification (2026-11-03)

Line1 P0 advertised-tip build; line2 paired/false-ban evidence; Lead requested an
independent full-suite rerun because line1 reported 242/3980 PASS while line2's
isolated harness reported 14 wallet/Tor failures.

## 1. Binary identity (verified)

| Поле | Значение |
|---|---|
| Tests SHA-256 | `930148B6146A5E89B0B7FAC4A32B57212434B511E14F778A00AD6D6D24C1BB6E` |
| size | 9 214 976 bytes |
| PE ts | `0x6AE90AE3` |
| repo binary | identical to `C:\DHW\.tmp\line1-build-06\Tests-930148B6146A.exe` |
| line2 preserved | `tools/audit/nrd/evidence/bin/Tests-930148B6146A.exe` + `.sha256.txt` — SHA совпал |
| GrinNode | `9548B83F2741EE5911B6A72BBADDE59EFD32A3AF784A8280DFAC904E6D342B79` (repo == preserved `GrinNode-9548B83F2741.exe`) |
| build | line1-build-06 `build-console.log` → BUILD OK |

## 2. Мои изолированные прогоны — PASS для согласованных групп

`run-tests-isolated.ps1` на preserved copy, `-KeepBinary`, isolated HOME/TMP,
профиль не изменён, exit 0:

| filter | результат |
|---|---|
| `Nrd*` | **28 cases / 402 assertions PASS** |
| `KernelIndex*` | **3 / 48 PASS** |
| `PibdArchiveServer::*` | **8 / 187 PASS** |
| `P2P::AdvertisedTipUsesHeaderHead` | **1 / 8 PASS** |
| `Slatepack*` (8 повторов) | **8/8 PASS**, по 3 cases / 39 assertions |

Line2 evidence проверен: `nrd-930148B6-…` 28/402 PASS, `falseban-…` 5/17 PASS,
`kernidx-…` 3/48 PASS; per-dir `Tests.exe` = 930148B6…; paired report — 11
computed / 3 not_computed / 11 paired / 0 mismatches.

## 3. Full-suite: FLAKY environment-dependent — NOT a deterministic PASS

Мои 5 полных прогонов на exact SHA (fresh isolated HOME/TMP каждый):

| # | результат | провал |
|---|---|---|
| 1 | FAIL 241/242, 3979 assertions | `Test_Slatepack.cpp` «Slatepack - E2E» — unexpected exception «Unknown exception» |
| 2 | PASS 242/3980 | — |
| 3 | PASS 242/3980 | — |
| 4 (repro) | FAIL 241/242, 3970 assertions | `tests/src/API/Wallet/Owner/Test_Receive.cpp(12)` — «DecryptSlatepack: Failed to decrypt Slatepack» |
| 5 (repro) | PASS 242/3980 | — |
| 6 (CI pipeline `ci-930-02`) | PASS 242/3980 | — |

Итог: **4 PASS / 2 FAIL**; `Slatepack*`-only фильтр 8/8 PASS. Провалы — только
wallet/slatepack-decrypt пути, не NRD/archive/advert. То есть заявленный
line1 PASS 242/3980 **возможен, но не детерминирован** в моём окружении; нельзя
объявлять его воспроизводимым PASS.

> **Статус SHA:** по решению Lead `930148B6…` **не финальный**; принят новый
> atomic advertised-tip P0 и запрошен build line1-build-07. Этот документ —
> historical/partial evidence; после прихода нового SHA выполняются повторные
> прогоны и обновление normal/ASan binding.

Wallet/Tor доступность:
- Tor binary присутствует: `dist\win\tor\tor.exe`; но `Environment::AUTOMATED_TESTING`
  пропускает инициализацию TorProcess, Tor-зависимые тесты остаются env-sensitive.
- Wallet manager использует **фиксированный** `127.0.0.1:3421`. У line2 14
  провалов — ровно `Create - mg_start failed 127.0.0.1:3421`; в моей среде на
  3421 наблюдались TIME_WAIT сокеты. Параллельные/перекрывающиеся полные прогоны
  (line1 + line2 + line3) конкурируют за этот порт → среда, а не NRD-регрессия.

**Вывод для CI:** раздельных HOME/TMP **недостаточно**; wallet/full-suite нельзя
запускать параллельно с другими инстансами и нужно либо сериализовать, либо
выделять уникальные порты. Slatepack-decrypt редкий full-suite-only флейк —
требует отдельного детерминированного репро и, вероятно, изоляции глобального
crypto/wallet state; до этого full-suite — `FAIL_ENV_DEPENDENT`, не PASS.

## 4. ASan / fuzz binding

После P0 изменений исходники изменились (`include/P2P/AdvertisedTip.h` и др.);
ASan source manifest `5A769673…` (TestsMini `4AB0CDDD…`) больше не соответствует
текущим compiled sources → ASan subset помечен **STALE / NOT RUN**, пока не будет
разрешена пересборка ASan в copied tree. Никаких ASan-claims к `930148B6…` нет.
Fuzz-бинарники также source-stale; smoke помечается advisory.

## 5. Ограничения

- Full-suite PASS не достигнут детерминированно; 0xC0000409 root cause остаётся
  NOT VERIFIED; live G-09 NOT VERIFIED; crafted-zip NOT VERIFIED.
- RocksDB/Database/ChainState под ASan не покрыты; coverage-guided fuzzing нет;
  snapshot/NRD fault-injection — только unit-симуляции (`FAULT_INJECTION.md`).
