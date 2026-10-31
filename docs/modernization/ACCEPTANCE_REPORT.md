# ACCEPTANCE_REPORT — фактический статус по мастер-заданию

Дата: 31.10.2026 (раунд 30/32). Репозиторий: `C:\DHW\Gplus\src\grinplusplus`
(Grin++ 1.2.9, эталон — Rust Grin v5.5.2 `dbaff34`).

**Итоговый вердикт: НЕ ЗАВЕРШЕНО (обновлено 19:10; полный живой цикл с горизонта PASS).** Реализован и проверен живой путь PIBD →
валидация → догон до `FULLY_SYNCED` на testnet, исправлен ряд реальных
дефектов совместимости, но часть требований остаётся открытой (см. §5).
Цель не помечается выполненной.

## 1. Как проверялось

* Сборка MSVC `/W4 /WX`, полный набор unit-тестов, `git`-история по шагам.
* Живые прогоны на реальной testnet-сети (Rust-совместимые пиры 5.4/5.5.x),
  REST `/v1/status`, `netstat`, логи узла.
* Rust-оракул: маленькая программа против `grin_chain 5.5.2`
  (`.tmp/grin-oracle-src`, `.tmp/oracle`), сверка байт-в-байт.
* Прочитанный эталон: `grin_core`, `grin_chain`, `grin_p2p`, `grin_servers`
  (msg.rs, pmmr/segment.rs, bitmap_accumulator.rs, body/header_sync.rs,
  adapters.rs, chain.rs).

## 2. Статусы по пунктам мастер-задания

| # | Требование | Статус | Доказательства / ограничения |
|---|---|---|---|
| 1 | Baseline-аудит и собираемый baseline | **PASS** | `BASELINE_AUDIT.md`, `BUILD.md`; baseline = upstream v1.2.9; сборка `/W4 /WX` без отключения предупреждений |
| 2 | GAP_MATRIX с фактическими расхождениями | **PASS** | `GAP_MATRIX.md` (G-01…G-48, статусы DONE/PARTIAL/OPEN с file:line и проверками) |
| 3 | P0: консенсус | **PASS-live / PARTIAL** | Живая валидация 3.3M заголовков, PIBD-состояния (kernel history/sums/rangeproofs), догон блоков до типсы (`chain=header=network=3316673`, `FULLY_SYNCED`, E34/E37). Не сделаны дифференциальные/fuzz-тесты, NRD (G-07), PoW edge-bits (G-04) |
| 4 | P0: P2P capabilities | **PASS** | Handshake теперь рекламирует `ADVERTISED` (`PIBD_HIST|PIBD_HIST_1`), живой Rust-пир это видит (E30); `Capabilities` тесты; PIHD-бит не рекламируется (E33) |
| 5 | P0: безопасность десериализации | **PARTIAL** | Исправлены `ByteBuffer`/парсеры и лимиты части сообщений; PIBD-парсеры с проверками; открыты G-16 (размер TxHashSet), G-20 (частичные лимиты) |
| 6 | P0: надёжность БД | **PARTIAL** | Найдена и воспроизведена рассинхронизация `header_mmr.bin`/`chain` после аварии (0xC0000005); добавлена загрузочная починка `ChainConsistency` + live-инъекции (E39). Атомарное формирование БД (checkpoint) — **G-48 OPEN**; миграции/бэкапы — G-22, ревалидация MMR — G-23 |
| 7 | PIBD download → desegmenter | **PASS (live, полный цикл)** | Сегменты 22/24/26/28, staging, активация, валидация; **19:02 `FULLY_SYNCED` после старта с горизонта, с ротацией 204→66 посреди загрузки** (E42) |
| 8 | PIBD resume | **PASS (unit + live)** | Журнал переживает рестарт и смену пира (rotations 204→66), не теряет принятые сегменты; полный цикл с горизонта дошёл до активации и `FULLY_SYNCED` (E41/E42) |
| 9 | PIBD serving | **PASS (unit)** | `PibdSegmentService`, `Test_PibdServing`; живой Rust-клиент сегменты у нас не запрашивал — **live NOT VERIFIED** |
| 10 | Legacy txhashset.zip fallback | **PARTIAL** | Код, конфиг `P2P.PIBD_ENABLED=false`, ротация пира вместо бана (G-42); **живой fallback-прогон не завершён** (G-41). В текущем прогоне после отказа PIBD 204.168 перестал принимать handshake (`Shake message not received`) — троттлинг пира после длительного interop, не падение узла |
| 11 | PIHD как расширение с fallback | **N/A (эталон)** | В grin v5.5.2 нет PIHD-сообщений (enum p2p заканчивается KernelSegment=28). Бит 0x80 только детектируется, не рекламируется, header sync всегда legacy; закреплено тестом (E33, G-27) |
| 12 | Тесты/доказательства | **PASS** | 150 кейсов / 1661 проверка PASS; oracle-векторы; живые логи/статусы; `/W4 /WX` |
| 13 | Честный отчёт | **PASS** | этот документ + `TEST_RESULTS.md` (E1…E39), `PEER_BAN_AND_STALL.md`, `DB_CONSISTENCY.md` |

## 3. Ключевые исправленные дефекты (реальные, найдены живым interop)

1. **Accept-loop** (`Seeder::Accept`): ранний return при отклонённом пире
   навсегда останавливал входящие соединения (G-29).
2. **Handshake capabilities**: отправлялся хардкод `FAST_SYNC_NODE` вместо
   `ADVERTISED`, PIBD-биты не рекламировались (G-30).
3. **PIBD horizon**: вместо archive-header (`head − 2d`, округление до 720)
   запрашивался candidate tip → пир отвечал `SegmenterHeaderMismatch` (G-31).
4. **BitmapBlock**: 16 чанков вместо 64 → реальные сегменты не парсились
   (G-32); **порядок бит MSB-first** (bit-vec) — sparse-сегменты читались
   неверно (G-35). Подтверждено оракулом `grin_chain`: 7/7 корней сегментов +
   корень всего битмапа совпали.
5. **Ложные баны/залипание block sync** (G-38): таймаут трактовался как
   `FraudHeight`, 3-часовые баны, вечный `m_slowPeers`, seeder/sync молчали.
   Введена `BlockSyncPolicy` (per-peer высота, batch≤100, retry 6с/1с+backoff,
   cooldown 60с, fraud только по правилу Rust), лог/счётчики/waiting_reason.
   Живое окно 32 мин: 0 `Banning peer`, `FULLY_SYNCED` (E37).
6. **PIBD peer stall** (G-43): fallback/disable после первого стопора. Теперь
   перебор PIBD-пиров до исчерпания (`RotatePibdPeer`), затем legacy.
7. **Legacy RequestState** (G-42): бан медленного пира заменён ротацией.
8. **БД** (G-46/G-47): рассинхронизация MMR/chain после аварии → AV; добавлена
   починка на старте (live-инъекции обе стороны), NULL-guard Ping. Требуется
   атомарная запись при формировании БД (G-48).
9. **Версия** (G-25): 1.2.8 → 1.2.9, закреплено тестом.

## 4. Артефакты и доказательства

* Тесты: **151 кейс / 1667 проверок PASS** (`Tests.exe`
  `0EFC0BF4…`), `GrinNode.exe` `55030297…`, `/W4 /WX`.
* Живые доказательства: `TEST_RESULTS.md` E30–E39; статусы
  `.tmp/interop/evidence/status_fully_synced.json`,
  `status_after_restart.json`; логи `.tmp/interop/**/Node.log`.
* Оракул: `.tmp/oracle` + `.tmp/grin-oracle-src`; векторы
  `.tmp/oracle/out/seg_h9_i*.bin`, `seg_h13_i0.bin`.
* Документы: `BASELINE_AUDIT.md`, `BUILD.md`, `GAP_MATRIX.md`,
  `IMPLEMENTATION_PLAN.md`, `TEST_RESULTS.md`, `PEER_BAN_AND_STALL.md`,
  `DB_CONSISTENCY.md`, `PEER_REVIEW.md`, `REQ_G38_FIX.md`.

## 5. Что остаётся (честно, до COMPLETE)

1. **G-48** — атомарное формирование БД: единый checkpoint
   (`mmr_leaves`, высоты, CRC), порядок fsync, crash-инъекции, `db_repaired`,
   копии повреждённых файлов (`DB_CONSISTENCY.md` §4). Это следующая задача
   после основной.
2. **G-41** — живой legacy `txhashset.zip` fallback (код есть, прогон не
   завершён); связанные G-42/G-43 live-проверки.
3. **PIBD serving живьём** — Rust-клиент ни разу не запрашивал сегменты у нас;
   unit-тесты есть.
4. **Resume полного цикла** — PASS (E42): журнал переживает ротацию/рестарт,
   цикл с горизонта завершён; осталось проверить сценарий с падением посреди
   активации (fault injection).
5. **P1-пункты**: G-16 (лимит TxHashSet), G-17 (rate limiting),
   G-20 (лимиты парсеров), G-21 (атомарная активация legacy-пути),
   G-22 (бэкап миграций), G-23 (полная ревалидация MMR при загрузке),
   G-04 (PoW edge-bits), G-07 (NRD).
6. **Дифференциальные/fuzz-тесты** C++ vs Rust и консенсусные векторы сверх
   живого прогона.
7. **PIHD** — невозможно реализовать/проверить против v5.5.2 (нет сообщений);
   оставлено detection-only, зафиксировано.

## 6. Вывод

Функциональная и сетевая совместимость с Rust v5.5.2 **подтверждена живым
прогоном** для заголовков, PIBD-загрузки/активации/валидации состояния,
догона блоков и устойчивости sync (без ложных банов) на testnet. Ряд P0-дефектов
найден и исправлен именно благодаря живому interop. Однако требования
«полноценный PIBD с resume и serving и legacy fallback», «атомарная надёжность
БД», часть P1 и дифференциальные доказательства **не закрыты**, поэтому статус
мастер-задания — **НЕ ЗАВЕРШЕНО**, цель остаётся активной.
