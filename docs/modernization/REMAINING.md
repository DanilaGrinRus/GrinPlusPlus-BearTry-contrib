# REMAINING — что закрыто и что осталось

## Единый действующий план готовности — 03.11.2026, 13:52 (UTC+07)

**Назначение:** это единственный сводный список работ и критериев для решения
«готовы к тестнету / готовы к Mainnet». `THREE_LINE_STATUS.md` — оперативные
результаты линий, `GAP_MATRIX.md` — каталог исторических и технических находок,
`TEST_RESULTS.md` и `evidence/` — датированные доказательства. Подробные планы
PIHD, UI и тестов остаются в своих разделах, но не являются альтернативной
приёмочной сводкой. При расхождении статуса сверять более свежие первичные
свидетельства и обновлять этот список; не переписывать прошлые результаты.

**Срез / не переносить на другую сборку:** build-45 `Tests B0F4CE1E…`,
`GrinNode 4F9859C2…` — source-bound кандидат с исправлениями
`TransactionKernel::IsCoinbase`, chain-store writer, reorg-preflight и typed
`LOCAL_REWIND_DATA_MISSING`. Bodyless **fail-closed PASS**: bodyless-state с
отсутствующими телами отклоняет reorg до мутации, сохраняет tip/NRD-index и
переживает restart; full bodyless reorg **не поддерживается и не заявляется**.
Full suite **274/274, 4360 assertions** (bodyless-тест видим, не скрыт).
Layer A public-Floonet PASS — build-16 `GrinNode 7E3AA29E…`, `Tests 1592F3DF…`:
FULLY_SYNCED 3 321 243, restart catch-up 3 321 250, 0 state re-download/PIBD
requests, A3 PASS; public PIBD serving **NOT VERIFIED** (disabled). Public legacy
fallback **BLOCKED/PARTIAL**: pre-reboot public profile reached headers
3 321 636 and a 369 360 896 B temporary archive; post-reboot long run PID 3680
remains active at DOWNLOADING_TXHASHSET. No completed/activated legacy state yet.
Full local PIBD 7B — PASS на build-13. Recovery H3313440/H3315600 и staging-kill — PASS в пределах.
[Layer A](LAYER_A_FLOONET_2026-11-03.md),
[bodyless](../../evidence/line3/bodyless-nrd-20261103.md),
[legacy](../../evidence/line3/public-legacy-fallback-20261103.md),
[build-13 live](../../evidence/line3/build13-7b-full-audit-20261103.md).
[отчёт по архиву](ARCHIVE_RECOVERY_2026-11-03.md).

### Ворота A — готовность ограниченного тестнета / Floonet

**Статус: A1/A2/A3 — PASS на build-16; A4 — исключения зафиксированы.
Публичный PIBD-serving не проверялся и остаётся DISABLED.**
Это ограниченный public-Floonet клиентский сценарий, не полный G-09/Mainnet.
Экспериментальные функции выключены. ([Layer A](LAYER_A_FLOONET_2026-11-03.md))

- [x] **A1. Кандидат:** build-16 `GrinNode 7E3AA29E…`, `Tests 1592F3DF…`,
  `/W4 /WX`, source-bound build; known bodyless test-only exception; полный
  список ограничений в отчёте Layer A.
- [x] **A2. Публичный Floonet:** изолированный профиль, реальные Rust-пиры
  (caps 95/223, UA 5.4.0/5.5.1/5.5.2), headers from zero, PIBD client state sync,
  body sync, `FULLY_SYNCED` 3 321 243 / `17c9300b…b4eb5b`; restart resumed at
  3 321 243, catch-up to 3 321 250 / `1dc25a89…c906a41`, 0 state re-download,
  0 PIBD requests, 0 banned.
- [x] **A3. Безопасные границы:** REST только 127.0.0.1; owner reads PRIVATE,
  get_config PRIVATE, update_config ADMIN; foreign public; PIBD serving/PIHD OFF,
  PIBD/PIHD bits не рекламируются. Публичный PIBD serving — **NOT VERIFIED**.
- [x] **A4. Исключения:** независимая вторая public-нода PASS (6 real Rust peers,
  restart resume 367 616 → 529 408, API auth); ASan F13/archive PASS 0 reports
  (ASan-only extended link set). Public PIBD serving NOT VERIFIED, safely disabled.
  Bodyless **fail-closed PASS**: `TransactionKernel::IsCoinbase` fixed (NRD no
  longer counted as coinbase; test added), chain-store writer deadlock fixed, and
  reorg preflight now refuses before mutation if confirmed bodies are missing.
  Real TestServer/BlockDB/OpenExisting path is exercised: safe `UNKNOWN_ERROR`,
  unchanged tip/NRD-index, and a clean restart. Full bodyless **reorg is not
  supported/claimed**. Full suite **274/274, 4360 assertions**; test visible.
  Peak validation RSS ~9.7 GB, post-sync ~2.8 GB; practical ≥12 GB free RAM.
  Public legacy fallback **BLOCKED/PARTIAL**: pre-reboot run reached headers
  3 321 636 and a 369 MB temporary archive; resumed long run remains active.
  Remaining limitation is bodyless full-reorg support, not a timeout.

### Ворота B — готовность к Mainnet

**Статус: PARTIAL/NOT VERIFIED.** B1/B2 подтверждены unit/build-18 и
ограниченным crash-publication; B3/B4/B5 остаются открытыми. Абсолютная
безошибочность не заявляется; ниже — условия для оценки режима релиза.

- [ ] **B1. G-09 / межсетевой PIBD:** negative wire/resource unit PASS build-18;
  public PIBD serving NOT VERIFIED (disabled); controlled reorg/horizon BLOCKED;
  public PIBD client/restart PASS A2 build-16; **public legacy fallback
  BLOCKED/PARTIAL** (long run active; pre-reboot 369 MB temporary archive, no
  activation/restart) — [legacy evidence](../../evidence/line3/public-legacy-fallback-20261103.md).
- [x] **B2. Fail-closed и публикация:** R1/R2/R3/R4/D2 PASS build-18;
  staging-kill PASS build-13; полный PUB/RES crash matrix NOT VERIFIED; no partial
  final archive observed.
- [ ] **B3. NRD / криптография:** D2 txpool unit PASS; **IsCoinbase exact-match bug
  fixed** (NRD was treated as coinbase; build-30 regression PASS); chain-store
  writer deadlock fixed; bodyless **safe-refusal variant PASS** on build-45
  (real TestServer/BlockDB/OpenExisting, typed `LOCAL_REWIND_DATA_MISSING`, no
  state mutation, restart consistent; test visible, full 274/274). Full bodyless
  **reorg remains unsupported/not claimed** —
  [bodyless evidence](../../evidence/line3/bodyless-nrd-20261103.md).
  Real bodyless PIBD state and kill/recovery remain NOT VERIFIED.
- [ ] **B4. БД и устойчивость:** G-23 corrupted HeaderMMR/partial PMMR fault
  injection NOT VERIFIED; disk-full/power-loss NOT VERIFIED; Output Commitment
  not reproduced; historical 0xC0000409 NOT VERIFIED.
- [ ] **B5. Релизная воспроизводимость и сеть:** clean checkout/toolchain pins,
  hosted CI/эквивалент и Mainnet dry-run NOT VERIFIED; public Floonet A2 PASS.
- [ ] **B6. Кошелёк/UI:** отдельная приёмка; не входит в текущий A/B результат.

### Полнота пользовательского продукта — после ворот A и B по указанию владельца

- [ ] **P1. UI + слой Flint:** до подключения UI дождаться результатов отдельного
  исследования C++ wallet backend и Rust grin-wallet, оценить выявленные риски,
  совместимость и готовность wallet API и учесть их в интеграционном плане.
  Материалы: `C:/DHW/Gplus/wallet-backend-audit/README.md`, `VERIFICATION_2026-11-03.md`,
  `PLAN.md` и `START_READINESS.md`. Сам по себе результат аудита не означает, что
  исправления или приёмка кошелька завершены. Подготовка к старту (DoR, backlog, оценки,
  UI-контракт) — `C:/DHW/Gplus/wallet-backend-audit/START_READINESS.md`; реализация не
  начата, старт заблокирован воротами A/B и отсутствием указания владельца.
  Реализация слоя C — после ворот A и B и отдельного указания владельца.
  Отдельный проект собственного открытого протокола Flint Messenger и адаптеров
  Rust/C++: `C:/DHW/Gplus/flint-messenger-project/PLAN.md`. Выбраны direct Tor,
  отдельная chat identity с проверяемой привязкой к кошельку, новый протокол без
  миграции. Дополнительно запланировано исследование собственной P2P-сети с
  добровольным транзитом и защитой метаданных: `C:/DHW/Gplus/flint-messenger-project/OWN_NETWORK_RESEARCH.md`.
  I2P/Lokinet/Nostr — возможные будущие адаптеры; централизованный relay и
  гарантированная offline-доставка не входят. Планирование не снимает запрет
  на реализацию слоя C до ворот A/B и отдельного указания владельца.
  Затем адаптировать Electron/React UI к C++ wallet API;
  отдельный Tor/messenger слой с контактами, устойчивой inbox/outbox и opt-in
  приёмом Slatepack. Сначала выбрать безопасную интеграцию, проверить лицензии,
  границы ключей/IPC и сохранение профилей; исходящая отправка требует явного
  подтверждения. Реальный двухпрофильный testnet цикл до chain confirmation и UI
  сценарии — отдельно; история Flint/mock не заменяет приёмку Grin++.
- [ ] **P2. PIHD G-27/G-44:** по желанию владельца реализовать/проверить клиент,
  сервер header-сегментов и честную рекламу capability; по умолчанию off. Локальный
  защищённый owner API enable/disable во время работы, readiness, lifecycle уже
  установленных соединений, fallback, reorg, пустой ответ и Rust staging live в
  обоих направлениях. Это желаемая часть полного продукта, но выключенный PIHD
  не блокирует ограниченный тест ноды или совместимость с Rust 5.5.2.
- [ ] **P3. G-49 / G-49-1 — подготовка стенда и уведомление:** G-49-1
  подготавливает методику, критерии и инвентаризацию для сравнения rangeproof
  validation. До завершения A и B и отдельного указания владельца — только
  подготовительная работа, без реализации оптимизации, изменений кода или запуска
  экспериментальных испытаний. После выполнения применимых предварительных PASS
  и готовности стенда **уведомить владельца, что G-49 готова к тестированию**,
  с указанием артефактов и ограничений; запуск сравнения и тестового билда —
  только по отдельному указанию. Тестирование и его итоговый PASS не являются
  предварительным условием уведомления. Если оптимизированного билда ещё нет,
  указать это и не представлять его как готовый. [G-49-1 и текст группе](PERF_RANGEPROOF.md#g-49-1--подготовка-стенда-и-уведомление-владельца).
  Параллелизм, персистентный resume и asm необязательны.
- [ ] **P4. x13 — последний этап API:** скрытая read-only пасхалка с обычной
  авторизацией, зелёным фоном, чёрной M/жёлтой W, именами и надписями по плану,
  реальными peers и прогрессом четырёх стадий (нет данных → N/A). Встроенная
  метка отсутствует/не проверена → красная `(`; это не проверка SHA/подлинности.
  Исходники и тесты остаются, публичный каталог не перечисляет метод. Не влияет
  на кошелёк, синхронизацию или консенсус. Это желаемая часть полного продукта,
  но она не блокирует отдельный тестовый релиз ноды.

**Общее правило остановки:** не искать бесконечно маловероятные ошибки. После
достаточных для конкретного режима проверок зафиксировать PASS/FAIL/NOT VERIFIED,
остаточный риск и кандидата. Не смешивать кошельковые данные, порты, live-копии
и общие сборочные директории при параллельной работе. Хеши или mock не заменяют
живую проверку; никакие секреты/seed в отчёты и логи не публиковать.

## Исторический список от 01.11.2026 — только для истории

> **Исторический план.** Текущий интеграционный статус: [THREE_LINE_STATUS.md](THREE_LINE_STATUS.md); риски: [GAP_MATRIX.md](GAP_MATRIX.md). Числа тестов и ссылки на «текущий срез» ниже относятся к дате документа.

> **Снимок устарел.** Этот файл остановлен на 01.11.2026 (после E54) и не
> учитывает E60–E68, поздние изменения HEAD `82beccf` и незакоммиченные файлы.
> Комментарии и порядок работ того периода — `REMAINING_TASKS_COMMENTS.md`;
> прежние статусы сохранены в `GAP_MATRIX.md` и `TEST_RESULTS.md`.

Обновлено: 01.11.2026, после E54. Источники того среза —
`TEST_RESULTS.md` (E1–E59), `GAP_MATRIX.md`, `ACCEPTANCE_REPORT.md`.
Текущий срез: **194 кейсов / 2227 проверок PASS**,
`GrinNode E16A014E…`, `Tests CC6472B1…`.

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

| Recovery-копии + `db_repaired`/`db_last_repair` | E44, unit |
| fsync-порядок MMR/chain/checkpoint | E51 |
| Аудит-дельта EN/DE/RU | 29 карточек, 8 полей, 18 diff-блоков, `PARITY OK` |

## 2. В работе / частично

| Статус | Пункт | Что осталось |
|---|---|---|
| **DONE** | **G-51** аудит-пакет | закрыто приёмкой владельца 01.11.2026; внешний независимый ревьюер не назначен; пакет `dist/audit-1.2.9-83783F38` + SHA256SUMS |
| **PARTIAL (offline PASS)** | **G-09** | anchor-search + leaf-hash фикс слиты; offline 4993/4993 сегментов PASS; live Rust-клиент/смена H/полный state — NOT VERIFIED (E77/E80) |
| **PARTIAL** | **G-23** hash-level | hash/root ревалидация заголовков и TxHashSet реализована; fault-injection/полный PMMR sweep — NOT VERIFIED (E82) |
| **PARTIAL** | **G-24** | CI-проводка + пин vcpkg (offline build/`/W4 /WX` готовы) |
| **PARTIAL** | **G-53** API | KernelIndex/get_kernel и позиции PMMR реализованы; live `/v2`/reorg/pagination — NOT VERIFIED (E78/E81) |

## 3. Запланировано / отложено / N/A

| Статус | Пункт |
|---|---|
| PLANNED | **G-44** спящий каркас PIHD (2–3 дня, без сети) |
| PLANNED | **G-45** mainnet dry-run (финальный гейт) |
| PLANNED | **G-49** ускорение валидации rangeproofs (31 мин → ~6–7) |
| OPEN | **G-07** NRD (recent kernel index; E64/E65) |
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

**44 закрыто + 4 частично (G-09, G-23, G-24, G-53) + 0 в работе + 1 открыто (G-07)
+ 3 запланировано (G-44, G-45, G-49) + 0 отложено + 1 N/A (G-27) = 53.**
Критический путь **G-52 → G-41 закрыт**.
