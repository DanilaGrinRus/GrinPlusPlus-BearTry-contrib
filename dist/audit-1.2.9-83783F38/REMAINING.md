# REMAINING — что закрыто и что осталось

Обновлено: 01.11.2026, после E54. Источник истины —
`TEST_RESULTS.md` (E1–E54), `GAP_MATRIX.md`, `ACCEPTANCE_REPORT.md`.
Текущий срез: **176 кейсов / 1907 проверок PASS**,
`GrinNode 83783F38…`, `Tests 38B9AF99…`.

## 1. Закрыто (PASS, с доказательствами)

| Пункт | Чем подтверждено |
|---|---|
| Baseline-аудит и сборка `/W4 /WX` | `BASELINE_AUDIT.md`, `BUILD.md` |
| GAP_MATRIX с фактическими расхождениями | G-01…G-52 |
| P2P capabilities/handshake, PIHD не рекламируется | E30/E33, unit |
| PIBD wire: горизонт 720, `BitmapBlock` 64 чанка, MSB-first, корни | оракул `grin_chain 5.5.2`, E31/E32/E35 |
| PIBD download → staging → активация → валидация | E32/E34/E42 |
| Полный цикл с горизонта → `FULLY_SYNCED` (testnet) | E42, E53 |
| Resume журнала (рестарт + ротация пира) | E41/E42 |
| **G-52: сохранение и живая выдача архива на горизонте** | E54 (3,1 ГБ, extraction + validation) |
| **G-41: живой legacy `txhashset.zip` fallback** | E54 (`FULLY_SYNCED` 3 317 664) |
| G-43: ротация по нулевому прогрессу/скорости, 3 круга с backoff | unit, E40–E42 |
| G-38: reassignment вместо бана; FraudHeight только по TD | E37/E42/E48 |
| G-42: legacy-пир ротируется, не банится | E41/E42 |
| G-46/G-47: починка хранилищ, NULL-guard Ping | E39 |
| G-48: atomic `chain.checkpoint`, no-loss repair | E43/E44/E48 |
| G-23 (структурная): `2L−popcount(L)`, rebuild MMR из block DB | E44/E48 |
| G-16: лимит `txhashset.zip` 8 GiB | E44, unit |
| G-17: rate limiting 500/мин (существующее) | обзор кода |
| G-21: staging-распаковка legacy zip, atomic swap, marker recovery | E49, `Test_TxHashSetStaging` |
| G-22: бэкап БД перед миграцией | E47, `Test_DbBackup` |
| G-50: DNS-discovery + цель 8 outbound вместо MIN_PEERS | E52, `Test_SeederPolicy` |
| G-04: допустимые PoW edge-bits (29, 31–63) | E50, `Test_PoWEdgeBits` |
| G-20: лимиты PIBD/segment-парсеров | E50, `Test_Segment` |
| G-25: версия 1.2.9 | unit + `/v1/status` |
| G-53: 13 методов `/v2/foreign` + 3 `/v2/owner`, каталог `/v1/` | E53 (unit + build; живые вызовы ещё нет) |
| Recovery-копии + `db_repaired`/`db_last_repair` | E44, unit |
| fsync-порядок MMR/chain/checkpoint | E51 |
| Аудит-дельта EN/DE/RU | 29 карточек, 8 полей, 18 diff-блоков, `PARITY OK` |

## 2. В работе / частично

| Статус | Пункт | Что осталось |
|---|---|---|
| **DONE** | **G-51** аудит-пакет | закрыто приёмкой владельца 01.11.2026; внешний независимый ревьюер не назначен; пакет `dist/audit-1.2.9-83783F38` + SHA256SUMS |
| **PARTIAL (FAIL live)** | **G-09** | живая выдача PIBD: `RangeProofSegment` не десериализуется (`Proof of size 20998`, E55); Rust/иной клиент не подтверждён |
| **PARTIAL** | **G-23** hash-level | побайтовая/корневая ревалидация MMR (`--validate-on-load`) |
| **PARTIAL** | **G-24** | CI-проводка + пин vcpkg (offline build/`/W4 /WX` готовы) |

## 3. Запланировано / отложено / N/A

| Статус | Пункт |
|---|---|
| PLANNED | **G-44** спящий каркас PIHD (2–3 дня, без сети) |
| PLANNED | **G-45** mainnet dry-run (финальный гейт) |
| PLANNED | **G-49** ускорение валидации rangeproofs (31 мин → ~6–7) |
| DEFERRED | **G-07** NRD (осознанно отложен) |
| N/A | **G-27** PIHD в v5.5.2 (сообщений нет; бит 0x80 только детектируется) |

## 4. Отдельный остаток после E54

Сервер `pp-archsrv` после активации архива сам остался в `SYNCING_BLOCKS`
на горизонте 3 314 160; legacy-клиент догнался через внешних пиров. Нужен
отдельный разбор block sync после активации состояния (G-52/G-41 это уже
не блокирует).

## 5. Вне матрицы (фазы 5–7)

* fuzz + ASan/UBSan/TSan;
* differential-fixtures C++ ↔ Rust;
* reorg-сценарии;
* регресс кошелька/API/GUI;
* CI-проводка + пин vcpkg (G-24 частично).

## 6. Итог по матрице

**45 закрыто + 3 частично (G-09, G-23, G-24) + 0 в работе + 0 открыто
+ 3 запланировано (G-44, G-45, G-49) + 1 отложено (G-07) + 1 N/A (G-27) = 53.**
Критический путь **G-52 → G-41 закрыт**.
