# IMPLEMENTATION_PLAN — Grin++ → совместимость с Grin v5.5.2

Основание: `BASELINE_AUDIT.md`, `GAP_MATRIX.md`, `TEST_RESULTS.md`.
Принципы: маленькие атомарные изменения; каждая позиция закрывается либо тестом,
либо differential-проверкой; незавершённое не выдаётся за готовое; `/WX` не отключается,
предупреждения исправляются.

---

## Фаза 0 — baseline и аудит (выполнено)

* Снапшот 1.2.9 зафиксирован git-коммитом, сверен с тегом `v1.2.9`.
* Созданы `BASELINE_AUDIT.md`, `GAP_MATRIX.md`, этот план и `TEST_RESULTS.md`.
* Закрыты и проверены: G-08 (ID 21-28), G-10 (capabilities), G-11 (неизвестные
  сообщения), G-12 (genesis handshake), G-14 (последний чанк TxHashSet),
  G-15 (непрошеный TxHashSet), G-19 (`ByteBuffer` overflow).
* Regression-тесты: `tests/src/Core/Serialization/Test_ByteBuffer.cpp`,
  `tests/src/Net/Test_P2PCompat.cpp` (изолированно: 7 кейсов / 35 проверок PASS).

**Критерий выхода не выполнен в части сборки:** `GrinNode.exe`/`Tests.exe` не собраны.

---

## Фаза 1 — воспроизводимая сборка (выполнено)

Цель: `GrinNode.exe` и `Tests.exe` собираются с `/W4 /WX`, тесты проходят,
сборка повторяется на чистой машине.

Сделано (вариант B, без расширения прав): `tools/deps/build_sysroot.py` собирает
11 зависимостей зафиксированных версий в `.toolchain/sysroot` (SHA-256 исходников,
статический CRT), `tools/build/build.ps1` конфигурирует проект и собирает цели.

Результат фазы 1 (на тот момент): `GrinNode.exe` (7 225 344 B) и `Tests.exe`
(7 747 584 B); полный набор тестов — **80 кейсов / 558 проверок PASS**; smoke-запуск
ноды на floonet — PASS. Актуальный прогон — в последней записи `TEST_RESULTS.md`
(E35: 140 кейсов / 1603 проверки PASS).
Подробности и команды: `BUILD.md`, `TEST_RESULTS.md` (E6-E9).

Остаток фазы: перенести пин зависимостей в репозиторий (`vcpkg.json` с
`builtin-baseline` либо offline-манифест), обновить `GrinVersion.h` (G-25),
добавить этот путь сборки в CI.

---

## Фаза 2 — PIBD: клиент + desegmenter (P0)

**Прогресс (раунды 2–5):** проводной слой, верификация (включая pruned-сегменты)
и генерация готовы, 28 unit-кейсов (`src/PMMR/Models/*`, `src/P2P/Messages/Segment*Message.h`):
MMR-позиции, `SegmentIdentifier`, `SegmentProof`, `Segment<T>` с
`Root`/`Validate`/`RootWithBitmap`/`ValidateWithBitmap`, `BitmapChunk`/`BitmapBlock`/
`BitmapSegment`, `SegmentBitmap`, `InMemoryPmmr` + `SegmentGenerator` (полные и
pruned-сегменты), сообщения 21–28. Готов ядровый слой загрузки: `BitmapSegmentAssembler` (фаза 1),
`PibdTreeAssembler<T>`, `PibdDownload` (фазы, запросы, дубликаты), `PibdSession`
(адаптер сообщений 21–28, гейт PIBD_HIST_1), `PibdScheduler` (in-flight лимит,
таймауты, повторы, fallback-таймер 660 с) `PibdSnapshot` (листья по индексам, pruned-хеши) и `PibdTreeRebuilder`
(пересборка корня прореженного дерева — та же арифметика, что пойдёт в hash-файлы).
Запись в файловый PMMR реализована (`PibdStateWriter`, staging-каталог, проверка
перезагрузкой). Атомарная активация реализована (`PibdActivation`), выдача сегментов из
файловых PMMR — тоже (`PibdSegmentService`, цикл serve→download проверен).
G-26 закрыт, `PibdSync` связывает download → snapshot → staging → активацию в
один сервис со статусами и падением в legacy; модель корней сверена с эталоном Rust
5.5.2 (E26) и исправлена. Проводка выполнена: `MessageProcessor` маршрутизирует
сегментные ответы в `PibdSync`, `StateSyncer` запускает PIBD при наличии
`PIBD_HIST_1`-пира, закрывает txhashset, активирует staging и валидирует набор, при
сбое откатывается на txhashset.zip; `Recover` вызывается при старте. Серверная выдача сегментов подключена (запросы 21/23/25/27 обслуживаются из
текущего txhashset), `PIBD_HIST`/`PIBD_HIST_1` рекламируются, `MIN_SYNC_PEERS`
конфигурируем. Живой interop пройден полностью (E31–E34): заголовки testnet
синхронизированы, PIBD-сегменты 22/24/26/28 загружены и провалидированы, staging
записан, активация выполнена, активированный TxHashSet провалидирован (kernel
history/sums/rangeproofs), цепочка догнана до `FULLY_SYNCED` и переживает рестарт.
По ходу найдены и исправлены: горизонт (G-31), wire-формат `BitmapBlock` (G-32),
порядок бит (G-35). Журнальный resume реализован (unit; живой прогон — NOT RUN).
Остаток фазы: живой serving (наш узел отдаёт сегменты Rust-пиру), живой журнальный
resume и G-38 (ложные баны block sync): без него живые проверки упираются в простой.

1. Сообщения 21–28 (уже в `MessageTypes`): `SegmentRequest{block_hash, identifier}`,
   `SegmentResponse<T>`, `OutputSegmentResponse`, `OutputBitmapSegmentResponse`,
   `SegmentIdentifier`, `SegmentProof`, `BitmapSegment` — строго по
   `grin/core/src/core/{segment,segment_identifier}.rs`, `chain/src/txhashset/*`.
2. Параметры сегментации — из `chain/src/pibd_params.rs` (batch size, segment height,
   bitmap segment height, peer limits), без «своих» размеров.
3. Desegmenter: применение bitmap/output/rangeproof/kernel сегментов к PMMR,
   prune shifts, проверка proofs и итоговых roots, финальная валидация TxHashSet.
4. Планировщик: horizon, in-flight лимиты, таймауты, retry, перераспределение между
   пирами, отбрасывание поздних ответов, resume после рестарта.
5. Хранение прогресса: staging-каталог, привязка сегментов к horizon/block hash,
   очистка недостоверного состояния.

Критерий: пир Grin v5.5.2 отдаёт сегменты; собранный TxHashSet даёт те же
output/kernel/rangeproof roots и height/tip, что и у Rust-ноды на том же horizon.

---

## Фаза 3 — PIBD serving + capabilities (P0)

* Ответы на 21/23/25/27 из локального состояния; отказ (`Consumed::None`), если
  состояние не позволяет.
* Только после прохождения interop-тестов включить `PIBD_HIST|PIBD_HIST_1` в
  `Capabilities::ADVERTISED`.
* Legacy TxHashSet сохраняется как fallback (нет PIBD-пиров, повторные ошибки,
  несовместимые capabilities) с ограничением попыток и без сокрытия причины.

---

## Фаза 4 — PIHD (расширение сверх v5.5.2)

`GetHeaderSegment=29`, `HeaderSegment=30`, `PIHD_HIST=0x80`, детерминированная
сегментация заголовков, параллельная загрузка, проверка непрерывности/PoW/cumulative
difficulty, безопасное применение, обработка fork/reorg, fallback на legacy header sync.
До готовности сервера — download-only и без рекламы `PIHD_HIST`.

**Статус: не требуется для v5.5.2; реализовано upstream в ветке `staging`.**
В эталоне v5.5.2 нет сообщений PIHD (enum заканчивается на `KernelSegment = 28`),
бит `0x80` только детектируется и не рекламируется (E33). В ветке `staging`
(`83ff9af`, на 63 коммита впереди тега) PIHD влит полностью — PR #3879 (протокол)
и #3882 (синхронизация с фолбэком на legacy); разбор — `PIHD_UPSTREAM.md`.
Сеть на v5.5.2 его не требует, у `staging` остаётся legacy-путь. Возвращаться к
фазе имеет смысл только если сеть перейдёт на релиз, собранный из `staging`.

**Подготовка (вариант B, `GAP_MATRIX` G-44):** заранее делается только не зависящая
от сети часть — `PihdParams.h`, чистая `PihdPolicy.h`, конфиг
`PIHD_MODE = off | client | full` и юнит-тесты; сообщения 29/30, сервер и
клиентская интеграция остаются на потом (см. `PIHD_UPSTREAM.md` §7). Так включение
после выхода PIHD в релиз сведётся к конфигу и прогону тестов, а не к двум неделям
работы.

---

## Фаза 5 — БД: staging и атомарная активация (P0)

* Проверенный TxHashSet применяется в staging-область и активируется атомарно;
  рабочая БД не переводится в непроверенное состояние.
* Rollback при любой ошибке; crash-тесты: kill во время записи/PIBD/компакции,
  нехватка места, повреждённый сегмент/временный файл, reorg во время синхронизации,
  повторное открытие старой БД (версии 2/3).
* Бэкап перед миграцией (G-22), проверка MMR при открытии по флагу (G-23).

---

## Фаза 6 — безопасность и fuzz (P1)

* Fuzz-цели: все сетевые десериализаторы (включая новые PIBD), bitmap, proofs,
  counts/allocations; проверка checked arithmetic и лимитов.
* ASan/UBSan на поддерживаемой сборке; data-race проверка (TSan) для pipeline.
* Политика rate limiting/банов как в Rust (G-17), лимит размера TxHashSet и
  контроль диска (G-16).

---

## Фаза 7 — тестирование и приёмка

* Differential: одинаковые fixtures (заголовки/блоки/сегменты) → одинаковые
  accept/reject, roots, MMR sizes.
* Interop: Grin++ ↔ Rust Grin v5.5.2 (handshake, headers, blocks, PIBD, relay, fallback).
* Chain integrity после синхронизации (height, tip, TD, roots, MMR sizes).
* Reorg-сценарии; регресс кошелька/API/GUI (ручные окна + существующие тесты).
* Performance — только после функциональной эквивалентности. Конкретика по
  самому дорогому участку (валидация rangeproofs, ~28 из ~31 мин): инкрементальная
  проверка с резюмом, параллельные батчи, scratch — `PERF_RANGEPROOF.md`
  (`GAP_MATRIX` G-49).
* **Финальный гейт вне RC (G-45): mainnet dry-run** по `docs/ACCEPTANCE.md` §M-01 —
  изолированный `--mainnet` data root, `--no-wallet`, без средств; заголовки → PIBD →
  активация → валидация → догон → рестарт → проверка чистоты окружения, сверка
  tip/TD с эталонной Rust-нодой.

---

## Риски

* Без сборки (Фаза 1) любые изменения остаются «по чтению кода» — это главный риск.
* PIBD — единственная крупная отсутствующая подсистема (≈2–4 тыс. строк C++ с тестами);
  оценка «за один заход» нереалистична, требует итераций.
* Rust-нода для differential/interop тоже требует сборки (Cargo + crates offline).
