# GAP_MATRIX — Grin++ 1.2.9 → Grin v5.5.2

Дата: 2026-10-30. Эталон: `Gplus/reference/grin-5.5.2` (тег v5.5.2).
Приоритеты: **P0** — консенсус/совместимость с текущей сетью/целостность данных;
**P1** — безопасность, отказоустойчивость, совместимость второго порядка;
**P2** — косметика, расширения сверх эталона.

Статусы: `OPEN` — не сделано; `DONE-C` — исправлено и проверено компиляцией
(изолированный TU); `DONE-P` — исправлено и проверено исполняемым пробником;
`DONE-R` — исправлено, проверено только чтением кода (полная сборка недоступна).

---

## A. Консенсус

| ID | Компонент | Эталон (Rust) | Расхождение | Влияние | P | Действие | Проверка | Статус |
|---|---|---|---|---|---|---|---|---|
| G-01 | Block header version | `consensus::valid_header_version` = `min(5, 1+h/262080)` (`core/src/consensus.rs:170`) | нет: `Consensus::GetHeaderVersion` даёт ту же лестницу для mainnet/floonet | — | — | — | кодовое сравнение | ЗАКРЫТО |
| G-02 | Difficulty | DMA до HF4, WTEMA после (`next_difficulty`) | нет: `DifficultyCalculator.cpp:22-28` переключается по `version < 5` | — | — | — | кодовое сравнение | ЗАКРЫТО |
| G-03 | Secondary PoW scaling | проверяется только при `version < 5` (`chain/src/pipe.rs:445`) | нет: для v5 возвращается scaling 0 | — | — | — | кодовое сравнение | ЗАКРЫТО |
| G-04 | PoW edge-bits sanity | `validate_pow_only`: `is_primary()/is_secondary()` + `verify_size` (`chain/src/pipe.rs`, `core/src/pow/cuckatoo.rs`) | Grin++ проверяет только алгоритм по `edge_bits==29`, без явной валидации допустимых размеров (primary ≥32 после HF4, secondary 29 до HF4) | Некорректный/дешёвый PoW не проходит total-difficulty, но класс ошибок = «битый» заголовок принимается дальше по конвейеру; расхождение с эталоном в диагностике/бане | P1 | Добавить `PoWValidator`-проверку размеров и `is_primary/is_secondary`; differential-тест на граничные `edge_bits` | unit + differential (нужна сборка) | OPEN |
| G-05 | Вес блока | `1/21/3`, `MAX_BLOCK_WEIGHT=40000` | нет | — | — | — | кодовое сравнение | ЗАКРЫТО |
| G-06 | Coinbase/kernel sums/rangeproofs/cut-through/lock heights | `block.validate`, `TxBodyValidator`, `KernelSumValidator` | нет (структурно совпадает) | — | — | — | кодовое сравнение | ЗАКРЫТО |
| G-07 | Варианты ядер (NRD) | `TODO - NRD` (`BlockValidator.cpp:22`) — у Grin++ нет проверки kernel feature/variant | Если на сети появятся NRD-ядра (не в v5.5.2), проверка будет неполной; для v5.5.2 вреда нет | P2 | Держать как явный TODO; фиксировать при появлении HF | — | OPEN (осознанно) |

---

## B. P2P / протокол

| ID | Компонент | Эталон | Расхождение | Влияние | P | Действие | Проверка | Статус |
|---|---|---|---|---|---|---|---|---|
| G-08 | Набор сообщений | IDs 0..28 (`p2p/src/msg.rs:67-97`) | IDs 21/22 заняты несовместимыми `GetKernels/Kernels` (никуда не отправляются); 23-28 отсутствуют | Любая попытка использовать 21/22 будет принята Rust-нодой как PIBD-запрос | P0 | Убрать коллизию, завести таблицу 21-28 с размерами из Rust | `static_assert`/компиляция TU | **DONE-C** |
| G-09 | PIBD (клиент+сервер) | `pibd.rs`, `desegmenter.rs`, `txhashset.rs`, `p2p/src/protocol.rs:294+`, `core/.../pmmr/segment.rs` | **Проводной слой отсутствовал полностью** | Нода не может синхронизировать состояние с современными нодами | **P0** | Сделано: MMR-математика, `SegmentIdentifier`, `SegmentProof`, `Segment<T>` (root/validate + bitmap-aware), `BitmapChunk`/`BitmapBlock`/`BitmapSegment` (+`FromSegment`), сообщения 21–28, `InMemoryPmmr` + `SegmentGenerator`, `SegmentBitmap`, `BitmapSegmentAssembler` (фаза 1), `PibdTreeAssembler<T>`, `PibdDownload` (фазы, запросы, дубликаты), `PibdSession` (capability-гейт по PIBD_HIST_1, сообщения 21–28 ↔ сессия, отсечение чужого horizon) и `PibdScheduler` (in-flight лимит, таймауты, повторы, fallback-таймер), `PibdSnapshot` (листья по индексам, pruned-хеши из сегментов, сверка множеств), `PibdTreeRebuilder` (пересборка корня прореженного дерева из листьев + сохранённых хешей — та же арифметика, что пойдёт в hash-файлы PMMR). `PibdStateWriter` (запись pmmr_hash/data/leafset/prun в staging-каталог тем же форматом, что у ноды, с проверкой перезагрузкой). `PibdActivation` (атомарная активация staging через маркер + два rename, восстановление после сбоя в любую сторону), `PibdSegmentService` (выдача сегментов из файловых PMMR; прореженная, G-26 закрыт) и `PibdSync` (полный цикл download → snapshot → staging → атомарная активация, статусы, падение в legacy). **Важно:** модель проверки приведена к эталону Rust v5.5.2: `header.output_root = H(output_mmr_size ‖ output_pmmr_root ‖ bitmap_root)`, bitmap-сегмент несёт output-PMMR root и проверяется через `validate_with`, output-сегмент — комбинацией с собранным bitmap-root, rangeproof — против `range_proof_root`, ядра — против `kernel_root` (ранее output-сегменты ошибочно проверялись прямо против `output_root` заголовка; исправлено). Проводка сделана: `MessageProcessor` маршрутизирует 22/24/26/28 в `PibdSync` и **обслуживает** запросы 21/23/25/27 из текущего txhashset (только для своего flushed-заголовка), `PIBD_HIST`/`PIBD_HIST_1` включены в `ADVERTISED` (и handshake теперь реально отправляет `ADVERTISED`, а не константу `FAST_SYNC_NODE`; проверено живым Rust-пиром: он видит оба PIBD-бита); `StateSyncer` запускает PIBD при наличии пира с `PIBD_HIST_1` (горизонт — candidate tip), по завершении закрывает txhashset, активирует файлы и валидирует их через `ProcessExistingTxHashSet`; при сбое — legacy txhashset.zip с паузой 120 минут; `PibdActivation::Recover` при старте; `MIN_SYNC_PEERS` конфигурируем (нужно для interop с одним пиром). Осталось: живой interop, resume, PIHD | 51 unit-кейс; полный набор 134/1541 PASS; smoke-старт PASS | **PARTIAL (клиент+сервер+ads DONE, live interop NOT RUN)** |
| G-10 | Capabilities | `HEADER_HIST..PIBD_HIST_1` (`p2p/src/types.rs:381-399`) | Объявлены только 0x01/0x02/0x04; 0x08/0x10/0x20/0x40 отсутствуют | Невозможно корректно негодовать возможности; PIBD/BLOCK_HIST не рекламируются | P0 | Добавлен полный набор битов + `ADVERTISED` (PIBD/PIBD_1/BLOCK_HIST выключены до реализации) | `static_assert` в TU | **DONE-C** |
| G-11 | Неизвестные типы сообщений | читать и отбрасывать (`default_max_msg_size`) | Grin++ рвал соединение на любом неизвестном типе (лимит 0) | Прямая несовместимость с новыми сообщениями; дисконнект от Rust-пиров при будущих расширениях | P1 | `MessageHeader`: неизвестный тип → лимит `MAX_BLOCK_SIZE*4` (как Rust) | компиляция TU | **DONE-C** |
| G-12 | Проверка genesis в handshake | `BadHandshake` при ином genesis (`p2p/src/handshake.rs`) | Grin++ читал genesis пира и **не сравнивал** | Подключение к peer другой сети → поток «плохих» блоков/баны честных пиров | P1 | Сравнение с `Global::GetGenesisHash()` в Hand и Shake | только чтение кода (DONE-R) | **DONE-R** |
| G-13 | Формат Hand/Shake | `msg.rs:405-490` | нет: поля/порядок/кодирование совпадают | — | — | — | кодовое сравнение | ЗАКРЫТО |
| G-14 | TxHashSet archive | `hash+height+bytes` + attachment `msg.rs:735-789` | нет по формату; **но** сервер терял последний неполный чанк (`while(file.read)`) | Пир получает меньше байт, чем объявлено → таймаут, бан честной ноды | P1 | Цикл по `gcount()`, отправка последнего чанка | только чтение кода (DONE-R) | **DONE-R** |
| G-15 | Непрошеный TxHashSet | Rust: `txhashset_receive_ready()` + `state_sync_requested` иначе `BadMessage` (`p2p/src/protocol.rs:254-265`) | Grin++ писал на диск и обрабатывал архив, даже если не запрашивал (бан был закомментирован) | Любой peer может подсунуть снапшот: подмена состояния/DoS диска | P1 | Требовать статус `SYNCING_TXHASHSET`, иначе бан | только чтение кода (DONE-R) | **DONE-R** |
| G-16 | Ограничение размера TxHashSet | Rust пишет `bytes` из сообщения (без явного cap) | Grin++ также не ограничивает `zipped_size` и не проверяет место на диске | Исчерпание диска одним сообщением | P1 | Ввести предел размера + проверку свободного места (после замера типового размера) | интеграционный тест | OPEN |
| G-17 | Rate limiting / abuse | `Tracker`: ≥150 мс между сообщениями, бан за >500 msg/min (`p2p/src/msg.rs:249-263`) | В Grin++ есть `RateCounter`, но политика банов/лимитов сообщений не соответствует эталону | Ресурс-exhaustion со стороны пира | P1 | Сверить и внедрить политику (порог, бан, счётчики) | нагрузочный тест | OPEN |
| G-18 | Peer addresses / discovery | `MAX_PEER_ADDRS=256`, capability-фильтр | совпадает | — | — | — | кодовое сравнение | ЗАКРЫТО |

---

## C. Безопасность десериализации

| ID | Компонент | Эталон | Расхождение | Влияние | P | Действие | Проверка | Статус |
|---|---|---|---|---|---|---|---|---|
| G-19 | `ByteBuffer` (все парсеры) | Rust `Reader` — проверки с ограничением остатка | Все проверки вида `m_index + N > size`: переполнение 64-бит обходит проверку. Воспроизведено на handshake `user_agent`: вместо `DeserializationException` вылетал `std::length_error` (не ловится в `MessageProcessor`) | Пир одним 128-байтным Hand может вызвать UB/раскрутку исключения вне ожидаемого типа; класс уязвим для всех будущих PIBD-парсеров | P1 | `RequireRemaining()` с вычитанием; все чтения переведены на него | исполняемый пробник (до/после) | **DONE-P** |
| G-20 | Ограничения размеров в парсерах | строгие лимиты на counts/allocations | До PIBD-парсеров ограничений нет (их и нет); для block/tx — ограничения задаются заголовком сообщения | — | P1 | Реализовать при добавлении PIBD-структур: max counts, checked arithmetic, отказ до аллокации | fuzz + unit | OPEN |

---

## D. Хранилище и восстановление

| ID | Компонент | Эталон | Расхождение | Влияние | P | Действие | Проверка | Статус |
|---|---|---|---|---|---|---|---|---|
| G-21 | Атомарная активация TxHashSet | Rust: временный каталог + переключение после валидации | Grin++ пишет в рабочую БД напрямую (`ProcessTransactionHashSet`) | Частичная запись/сбой = повреждённое состояние | **P0** | Staging-область + атомарная активация, rollback | crash-тесты | OPEN |
| G-22 | Версии БД/миграции | — | Есть версии 2/3 и отказ для >3, миграция при открытии; резервная копия перед миграцией не делается | Потеря данных при неудачной миграции | P1 | Бэкап + проверка восстановления перед миграцией | тест миграции | OPEN |
| G-23 | Проверка MMR при загрузке | re-validate on load (TODO в Grin++ `BlockChainImpl.cpp:442`) | Не выполняется | Повреждение индекса не детектируется | P1 | Добавить проверку корней при открытии (по флагу) | тест повреждения | OPEN |

---

## E. Сборка и воспроизводимость

| ID | Компонент | Эталон | Расхождение | Влияние | P | Действие | Проверка | Статус |
|---|---|---|---|---|---|---|---|---|
| G-24 | Пин зависимостей | — | vcpkg-пин только в CI; сам vcpkg-путь несовместим с VS2022 и недоступен в этой среде | Неповторяемая сборка | P0 (для проверяемости) | Реализован offline-путь: `tools/deps/build_sysroot.py` (те же версии, SHA-256 исходников) + `tools/build/build.ps1`; пин vcpkg в репозиторий пока не перенесён | сборка с нуля — PASS (E6-E7) | **DONE (частично)** |
| G-25 | Версия ПО | — | `GrinVersion.h` = 1.2.8 при теге 1.2.9 | Исправлено на 1.2.9 (UA/REST/логи), тест `GrinVersion::MatchesRelease` | unit/CI | **DONE** |
| G-26 | `/W4 /WX` на MSVC 14.4x | — | Проверено: проект компилируется под `/W4 /WX` без правок кода; понадобились `NOMINMAX` и идемпотентная установка флагов | — | P1 | Конфигурационные правки CMakeLists внесены | сборка — PASS | ЗАКРЫТО |
| G-28 | Изоляция данных тестов | — | Тесты пишут в `%HOME%\.GrinPP\AUTOMATED_TESTING`; без изолированного HOME падают с `Access is denied` (окружение), и в общем случае задевают пользовательский каталог | Риск для пользовательских данных при прогоне | P1 | Документировано в `BUILD.md`; в CI/скриптах задавать изолированный `HOMEDRIVE`/`HOMEPATH` (явный test-root — следующий шаг) | прогон тестов с изоляцией — PASS (E8) | **DONE (документация)** |

---

## F. Расширения сверх v5.5.2

| ID | Компонент | Источник | Состояние |
|---|---|---|---|
| G-27 | PIHD (`GetHeaderSegment=29`, `HeaderSegment=30`, `PIHD_HIST=0x80`) | `staging` (PR #3879/#3882) | **OPEN**: не начато; требуется после PIBD, с fallback на legacy header sync и без рекламирования до готовности сервера |

---

## Сводка

* P0: G-09 (PIBD), G-21 (атомарная активация состояния); G-24 закрыт по существу на
  этом хосте (offline sysroot), но пин vcpkg в репозиторий ещё не перенесён.
* P1: G-04, G-16, G-17, G-20, G-22, G-23, G-28.
* Закрыто: G-08, G-10, G-11, G-12, G-14, G-15, G-19 (проверены прогоном полного
  набора тестов после сборки), G-26 (сборка `/W4 /WX`).
* Baseline теперь проверяем исполнением: `GrinNode.exe` + `Tests.exe` собраны,
  `Tests.exe` — 80 кейсов / 558 проверок PASS (см. `TEST_RESULTS.md`).
* Не подтверждено: живая синхронизация с Rust v5.5.2, reorg, crash/fuzz, PIBD/PIHD.
| **G-33** | PIBD serving прореженного состояния | Закрыто. `leafset` пишется из битмапа горизонта (потраченные листья, присланные по правилу соседа, остаются spent), `GetRawAt` отдаёт их данные. `ComputePrunedTreeRoot` теперь считает максимальные прореженные поддеревья (как writer), не требует хешей интерьера и корректно обрабатывает полностью прореженный пик (его хеш приходит из сегмента). Тест serve→download проходит на прореженном битмапе ({0} из 64 листьев). | P1 | **CLOSED** |
| **G-29** | Приём входящих P2P-соединений | `Seeder::Accept` | При отклонённом пире (`blocked`/`not allowed`) функция делала `return` до повторного `async_accept`; первый отказ навсегда останавливал listener (в interop Rust-нода не могла подключиться, `LISTENING` отсутствовал) | Исправлено: accept переставляется после любого исхода; после фикса `0.0.0.0:13414 LISTENING` | сборка, набор 134/1541, listener проверен netstat | **DONE** |
| **G-30** | Реклама возможностей в handshake | `msg.rs` Hand/Shake | `HandShake.cpp` отправлял `Capabilities::FAST_SYNC_NODE` вместо `ADVERTISED`, поэтому PIBD-биты не доходили до пиров (Rust видел только TXHASHSET_HIST\\|PEER_LIST) | Отправляется `ADVERTISED`; проверено живым Rust-пиром: `PIBD_HIST \\| PIBD_HIST_1` в его handshake-логе | live handshake-лог Rust | **DONE** |
| **G-31** | Горизонт PIBD | `chain.rs:985,1002` (`txhashset_archive_header*`) | Запрашивали сегменты для candidate tip; Rust проверяет запрошенный hash против header сегментера (archive header = (head − STATE_SYNC_THRESHOLD) с округлением до 720), поэтому реальный пир не отвечал | Горизонт вычисляется так же; завершение ставит состояние на этот header | live bitmap-сегмент сходится с `header.output_root` (E31) | **DONE** |
| **G-32** | Wire-формат `BitmapBlock` | `bitmap_accumulator.rs:470,213` | Блок = 2^16 бит = 64 чанка по 1024, у нас было 16 → реальные сегменты не десериализовались | Исправлено; парсинг живого сегмента PASS | живой interop + unit | **DONE** |
| **G-34** | Live-сверка bitmap-корня PIBD | `bitmap_accumulator.rs`, `segment.rs` | Сегменты (height 9 и 13) от двух независимых живых пиров дают один и тот же свёрнутый bitmap-корень `570d270a…`, но он не сходится с `header.output_root`, а proof сегмента 0 даёт `a9591a…`; C++ и независимый Python совпадают между собой | **OPEN**: нужен Rust-оракул на `grin_chain` (сериализация известного `BitmapSegment`), чтобы подтвердить формат/хеширование чанков | живые векторы `.tmp/interop`, offline-разбор | OPEN |
| **G-35** | Порядок бит в bitmap-структурах | `bitvec::to_bytes` (MSB-first) | `BitmapChunk`/`BitmapBlock` индексировали биты LSB-first; Raw-блоки сходились, но Positive/Negative (sparse) кодируют индексы в MSB-нумерации → все sparse-сегменты PIBD читались неверно | Исправлено на MSB-first; сверено с Rust-оракулом (`grin_chain 5.5.2`): 7/7 корней сегментов + корень всего битмапа совпали | оракульные векторы + unit-тест + живой PIBD (E32) | **DONE** |
| **G-36** | Живой PIBD download+активация | `pibd.rs`, `desegmenter.rs` | Проверено на реальном пире: bitmap/output/rangeproof/kernel сегменты 22/24/26/28 загружены без ошибок, staging записан, активация выполнена | — | живой лог E32 | **PASS** |
| **G-37** | Валидация активированного состояния и догон блоков | `txhashset.rs` | kernel history и kernel sums прошли; rangeproofs валидируются (IN PROGRESS на момент отчёта) | — | живой лог E32 | IN PROGRESS |
| **G-39** | Полная живая синхронизация до типсы | `pibd.rs`, `block_sync` | Проверено на реальной testnet: PIBD → активация → валидация TxHashSet → догон блоков до `chain=header=network=3316561`, `FULLY_SYNCED` | — | живой REST-статус + логи E34 | **PASS** |
| **G-40** | Сохранение состояния между рестартами | `chain`/`txhashset` load | После рестарта узел сразу `FULLY_SYNCED` на той же высоте/хеше, PIBD не перезапускается (state-level resume; journal-resume скачивания ещё не проверен) | — | живой лог E34 | **PASS** |
| **G-41** | Переключатель PIBD для fallback | — | Не было способа проверить legacy-путь на живом узле | Добавлен `P2P.PIBD_ENABLED` (default true); при false StateSyncer сразу идёт в txhashset.zip | unit + живой `pp-fallback` (IN PROGRESS) | **DONE (код)**, live IN PROGRESS |
