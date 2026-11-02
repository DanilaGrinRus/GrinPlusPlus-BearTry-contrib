# Normal-binary audit DDC21F45 (D2 mainnet NRD gate build) — 2026-11-03

Line2 Lead-approved D2 wiring: `BlockValidator::VerifyNrdNetworkGate` on the
block consensus path via `Config::IsNrdKernelEnabled(Global::GetEnv())`.
Txpool gate and NRD-index policy early-return remain unwired (line2 scope note).

## 1. Binary identity (verified)

| Поле | Значение |
|---|---|
| SHA-256 | `DDC21F45EBFE6FA4846FFFB0FA5E376297EED5BFFEFAA5583E3A6B04B0BCF714` |
| size | 9 172 480 bytes |
| PE ts | `0x6AE8F9CD` (2026-11-02T20:59:57Z) |
| mtime | 2026-11-03T03:59:57+07:00 |
| repo binary | `bin\Release\Release\Tests.exe` — **идентичен** сохранённой копии |
| preserved copy | `tools/audit/nrd/evidence/bin/Tests-DDC21F45EBFE.exe`, `.sha256.txt` присутствует |
| build | `pwsh tools/build/build.ps1 -Target Tests` → BUILD OK (log `evidence/cpp-build-20261103-035328.log`) |
| previous | FD7C0492…, 2032536D…, 0EDC0F81…, 06309AF0… сохранены |

## 2. Независимые изолированные прогоны line3 — PASS

`run-tests-isolated.ps1` на preserved copy, `-KeepBinary` (в каждом evidence dir
`Tests.exe` с SHA DDC21F45…), isolated HOME/TMP, профиль не изменён, exit 0,
stderr пуст:

| run | filter | результат | manifest sha256 |
|---|---|---|---|
| `normal-ddc-nrd-01` | `Nrd*` | **23 cases / 382 assertions PASS** | `DB5856FF…` |
| `normal-ddc-kernidx-01` | `KernelIndex*` | **3 / 48 PASS** | `859029CE…` |
| `normal-ddc-archive-01` | `PibdArchiveServer::*` | **8 / 187 PASS** | `D47CF517…` |
| `normal-ddc-full-01` | full suite | **236 / 3952 PASS** | `C92E2CA3…` |

Совпадает с line2: Nrd* 23/382, KernelIndex* 3/48, full 236/3952.
Line2 binding проверен: `nrd-d2-…`/`kernidx-d2-…`/`full-d2-…` (keepbin copies
SHA DDC21F45…), differential report `EA44A390…` (14/14).

### Атрибуция прироста (взаимно подтверждено line2, проверено арифметически)

| Переход | Cases | Assertions | Атрибуция |
|---|---|---|---|
| `Nrd*` 20/375 → 23/382 | +3 | +7 | только line2 `Test_NrdPolicy` 1→4 |
| full 232/3926 → 236/3952 | +4 | +26 | +3/+7 line2 D2 **и** +1/+19 line1 archive |
| archive 7/168 → 8/187 | +1 | +19 | целиком line1 update; D2-тестов в archive subset нет |

Git-атрибуция: `Test_PibdServing.cpp` — modified-uncommitted (line1); последний
коммит, трогавший файл: `b82b8d5`; line2 файл не редактировал. Line2 записал
атрибуцию в `binaries-sha256.txt` (DDC provenance) и `NRD_DIFFERENTIAL.md` §7.2.

### Ограничение из line2 acceptance

Crafted-zip end-to-end `ProcessTxHashSet` harness остаётся **NOT VERIFIED**
(blocker задокументирован line2); D2 txpool gate и NRD-index policy
early-return не подключены.

### Важно: repo binary сменился после аудита

На момент этого аудита repo/bin был = DDC21F45…, preserved copy = DDC21F45….
После аудита появился новый build `C:\DHW\.tmp\line1-build-05\Tests-8F10BFD67047.exe`
(SHA `8F10BFD6…`, 9 172 480 B, PE ts `0x6AE8FC0B`, build-console.log BUILD OK,
GrinNode-5CA47DA95898 рядом) — текущий repo `bin\Release\Release\Tests.exe` =
`8F10BFD6…`. Handoff/claims от line1 по этому SHA ещё нет; результаты этого
документа относятся **только** к DDC21F45… и не переносятся на 8F10BFD6….
Новый SHA требует отдельного re-hash/re-audit.

## 3. Изменение archive-теста между FD7C и DDC (provenance)

`tests/src/PMMR/Test_PibdServing.cpp` изменён **2026-11-03 03:54:55**, sha256
`EDB1BA612B18B5DA167F00EC6A7A7F6EDBCCC2DAE024C055E28FA1572FD799A3`
(в FD7C-раунде был `4646F4DF…`, 03:38). Поэтому archive subset вырос с 7/168 до
8/187 (+1 case: `PibdArchiveServer::ServingOptInParsing`, +19 assertions), и
full suite вырос с 232/3926 до 236/3952 (+4 cases = +3 `Test_NrdPolicy`
cases от D2 + +1 archive case). Это изменение **line1-файла**, не D2 NRD gate;
line2 заявил «no line1 archive files touched» — это верно для line2-изменений,
но DDC build включает line1-обновление от 03:54:55.

## 4. Ограничения

- Archive serving default-off; ordinary-binary live G-09 **NOT VERIFIED**.
- D2 gate покрыт unit/consensus-тестами (`Test_NrdPolicy` 4 cases), но
  txpool gate / NRD-index policy early-return не подключены (line2 scope).
- Database/ChainState/rocksdb не ASan-инструментированы; full-suite PASS ≠
  live/network compatibility; coverage не измерялась.
- ASan binding обновляется отдельно (source-based; см. запись об обновлении
  ASan subset для D2/line1-теста).
