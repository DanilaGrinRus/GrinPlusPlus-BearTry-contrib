# REMAINING — что закрыто и что осталось

Обновлено: 01.11.2026, после E49. Источник истины по доказательствам —
`TEST_RESULTS.md` (E1–E49), `GAP_MATRIX.md`, `ACCEPTANCE_REPORT.md`.
Текущий тестовый срез: **170 кейсов / 1822 проверки PASS**,
`GrinNode D2BBF889…`, `Tests 0B80AD8D…`.

## 1. Закрыто (PASS, с доказательствами)

| Пункт | Чем подтверждено |
|---|---|
| Baseline-аудит и сборка `/W4 /WX` | `BASELINE_AUDIT.md`, `BUILD.md` |
| GAP_MATRIX с фактическими расхождениями | G-01…G-52 |
| P2P capabilities/handshake, PIHD не рекламируется | E30/E33, unit |
| PIBD wire: горизонт 720, `BitmapBlock` 64 чанка, MSB-first, корни | оракул `grin_chain 5.5.2`, E31/E32/E35 |
| PIBD download → staging → активация → валидация | E32/E34/E42 |
| Полный цикл с горизонта → `FULLY_SYNCED` (testnet) | E42, `evidence/g38_full_horizon_fully_synced.json` |
| Resume журнала (рестарт + ротация пира) | E41/E42 |
| G-43: ротация по нулевому прогрессу/скорости, 3 круга с backoff | unit `SlowPeerRateFails`, E40–E42 |
| G-38: reassignment вместо бана; FraudHeight только по TD | E37/E42/E48 |
| G-42: legacy-пир ротируется, не банится | E41/E42 |
| G-46/G-47: починка хранилищ, NULL-guard Ping | E39 |
| G-48: atomic `chain.checkpoint`, no-loss repair | E43/E44/E48 |
| G-23 (структурная): `2L−popcount(L)`, rebuild MMR из block DB | E44/E48 |
| G-16: лимит `txhashset.zip` 8 GiB | E44, unit |
| G-17: rate limiting 500/мин (существующее) | обзор кода |
| G-22: бэкап БД перед миграцией | E47, `Test_DbBackup` |
| G-21: staging-распаковка legacy zip, atomic swap, marker recovery | E49, `Test_TxHashSetStaging` |
| Recovery-копии + `db_repaired`/`db_last_repair` | E44, unit |
| G-50: повторный DNS-discovery | живой прогон |
| G-25: версия 1.2.9 | unit + `/v1/status` |
| Аудит-дельта EN/DE/RU | 27 карточек, 8 полей, 16 diff-блоков, `PARITY OK` |

## 2. Открыто / IN PROGRESS (полный список)

| # | Статус | Что осталось | Критерий закрытия |
|---|---|---|---|
| 1 | **IN PROGRESS** | **Замер полного синка с нуля** на `pp-archsrv` (тот же прогон используется для G-52/G-41): старт 31.10 23:55:27, идёт фаза заголовков; далее PIBD → валидация → архив → догон | `FULLY_SYNCED` и записанное суммарное время в `sync_timing.txt` / `TEST_RESULTS` |
| 2 | **OPEN (частично)** | **G-52**: исторический snapshot/serving. Код сохранения архива на горизонте и выдачи по hash добавлен; **живая передача архива не проверена**; разрыв spent-позиций остаётся для произвольных исторических snapshot (block 3 316 349) | loopback/Rust legacy-клиент получает архив, валидирует, доходит до `FULLY_SYNCED` |
| 3 | **OPEN** | **G-41**: живой legacy `txhashset.zip` fallback | `FULLY_SYNCED` по legacy-пути с реальным пиром |
| 4 | **OPEN** | **Живой PIBD serving** Rust-клиенту (unit `Test_PibdServing` есть) | Rust-клиент запрашивает сегменты и собирает состояние |
| 5 | **PARTIAL** | **G-23 hash-level**: побайтовая/корневая ревалидация MMR | опция `--validate-on-load` + тест |
| 6 | **OPEN** | **fsync-порядок** между `header_mmr.bin`/chain/checkpoint | документированный порядок + crash-тест |
| 7 | **OPEN** | **G-20** оставшиеся лимиты парсеров (varint/vector) | unit/fuzz |
| 8 | **OPEN** | **G-04** PoW edge-bits | дифференциальный тест против Rust |
| 9 | **OPEN** | **G-07** NRD | реализация либо осознанный отказ + тест |
| 10 | **OPEN** | **G-49** ускорение валидации rangeproofs | замеры до/после (`PERF_RANGEPROOF.md`) |
| 11 | **OPEN** | **Дифференциальные/fuzz-тесты** C++ vs Rust сверх живого interop | набор + отчёт |
| 12 | **OPEN** | **G-51** аудит-пакет | ревизия комментариев (`AUDIT_COMMENTS.md`), скан секретов/дампов, лицензии, финальные hashes/provenance, чек-лист, `dist/audit-*` |
| 13 | **PLANNED** | **G-44** спящий каркас PIHD | вне текущего объёма; PIHD для v5.5.2 — N/A |
| 14 | **PLANNED** | **G-45** mainnet dry-run | вне объёма (DEV/testnet) |

## 3. Текущий живой прогон

* `pp-archsrv`, старт **31.10.2026 23:55:27**, HOME
  `C:\DHW\.tmp\interop\pp-archsrv`, REST `13424`.
* Цель прогона: полный синк с нуля + PIBD на горизонте 3 314 160 →
  сохранение архива → legacy-клиент `pp-resume` на том же горизонте.
* После полного синка снять итоговое время и обновить E50/`ACCEPTANCE_REPORT`.

## 4. Что НЕ является остатком

* **PIHD** — N/A для grin v5.5.2 (сообщений PIHD нет; бит 0x80 только
  детектируется). Зафиксировано в E33/G-27.
* **Mainnet** — вне объёма задачи (testnet/DEV).
