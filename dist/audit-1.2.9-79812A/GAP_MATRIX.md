# GAP_MATRIX — Grin++ 1.2.9 → Grin v5.5.2

Дата: 2026-10-30. Эталон: `Gplus/reference/grin-5.5.2` (тег v5.5.2).
Приоритеты: **P0** — консенсус/совместимость с текущей сетью/целостность данных;
**P1** — безопасность, отказоустойчивость, совместимость второго порядка;
**P2** — косметика, расширения сверх эталона.

Статусы: `OPEN` — не сделано; `DONE-C` — исправлено и проверено компиляцией
(изолированный TU); `DONE-P` — исправлено и проверено исполняемым пробником;
`DONE-R` — исправлено, проверено только чтением кода (полная сборка недоступна).

| **G-44** | PIHD: спящий каркас (вариант B) | `staging`: `chain/src/pihd_params.rs`, `servers/src/grin/sync/header_sync.rs`, PR #3879/#3882 | Полная реализация PIHD сейчас (вариант C) — это 2–3 недели под флагом, но протокол живёт только в `staging` (не в релизе) и живых пиров с `PIHD_HIST` нет, то есть проверять нечего. При этом дешёвая часть (параметры, политика, конфиг, тесты) от будущего формата почти не зависит | Запланировано (вариант B, без сетевого кода): `PihdParams.h` с константами Rust (in-flight 8/2 на пира, таймаут 20 с, cooldown 30 с, 3 провала → block, 3 таймаута + 120 с без прогресса → fallback, сегмент 512 заголовков, `start = idx*512+1`); `PihdPolicy.h` — чистая политика по образцу `BlockSyncPolicy`; `PIHD_MODE = off \| client \| full` (default `off`, по образцу `PIBD_ENABLED`); уточнить комментарий в `Capabilities.h`. Сеть не подключается, `PIHD_HIST` не рекламируется. Детали — `PIHD_UPSTREAM.md` §7 | unit-тесты сегментной арифметики, лимитов и решений политики; сборка `/W4 /WX` | **OPEN (запланировано, не начато)** |
| **G-45** | Mainnet dry-run — финальный гейт | `docs/ACCEPTANCE.md` §M-01; эталон — Rust v5.5.2 на mainnet | Живых прогонов против mainnet нет: все доказательства получены на testnet/floonet, а mainnet-константы (header version, difficulty) сверены только по коду (G-01/G-02) | Вне RC: изолированный `--mainnet` data root, `--no-wallet`, без средств; шаги M-01.1–M-01.7 (заголовки → PIBD → активация → валидация → догон → рестарт → чистота окружения), сверка tip hash/TD с эталонной Rust-нодой | живой прогон + diff каталогов + статус-JSON | **OPEN (запланировано, вне RC)** |
---

## A. Консенсус

| ID | Компонент | Эталон (Rust) | Расхождение | Влияние | P | Действие | Проверка | Статус |
|---|---|---|---|---|---|---|---|---|
| G-01 | Block header version | `consensus::valid_header_version` = `min(5, 1+h/262080)` (`core/src/consensus.rs:170`) | нет: `Consensus::GetHeaderVersion` даёт ту же лестницу для mainnet/floonet | — | — | — | кодовое сравнение | ЗАКРЫТО |
| G-02 | Difficulty | DMA до HF4, WTEMA после (`next_difficulty`) | нет: `DifficultyCalculator.cpp:22-28` переключается по `version < 5` | — | — | — | кодовое сравнение | ЗАКРЫТО |
| G-03 | Secondary PoW scaling | проверяется только при `version < 5` (`chain/src/pipe.rs:445`) | нет: для v5 возвращается scaling 0 | — | — | — | кодовое сравнение | ЗАКРЫТО |
| G-04 | PoW edge-bits sanity | `validate_pow_only`: `is_primary()/is_secondary()` + `verify_size` (`chain/src/pipe.rs`, `core/src/pow/cuckatoo.rs`) | Grin++ не отвергал явно недопустимые `edge_bits` (29 secondary, 31–63 primary); значения ≥64 давали сдвиг 64-битного слова | Добавлен `PoWValidator::IsSupportedEdgeBits` и проверка **до** диспетчеризации в Cuckoo; unit-таблица границ | unit `Test_PoWEdgeBits` (11 проверок) | **DONE** |
| G-05 | Вес блока | `1/21/3`, `MAX_BLOCK_WEIGHT=40000` | нет | — | — | — | кодовое сравнение | ЗАКРЫТО |
| G-06 | Coinbase/kernel sums/rangeproofs/cut-through/lock heights | `block.validate`, `TxBodyValidator`, `KernelSumValidator` | нет (структурно совпадает) | — | — | — | кодовое сравнение | ЗАКРЫТО |
| G-07 | Варианты ядер (NRD) | `TODO - NRD` (`BlockValidator.cpp:22`) — у Grin++ нет проверки kernel feature/variant | Если на сети появятся NRD-ядра (не в v5.5.2), проверка будет неполной; для v5.5.2 вреда нет | P2 | Держать как явный TODO; фиксировать при появлении HF | — | OPEN (осознанно) |

---

## B. P2P / протокол

| ID | Компонент | Эталон | Расхождение | Влияние | P | Действие | Проверка | Статус |
|---|---|---|---|---|---|---|---|---|
| G-08 | Набор сообщений | IDs 0..28 (`p2p/src/msg.rs:67-97`) | IDs 21/22 заняты несовместимыми `GetKernels/Kernels` (никуда не отправляются); 23-28 отсутствуют | Любая попытка использовать 21/22 будет принята Rust-нодой как PIBD-запрос | P0 | Убрать коллизию, завести таблицу 21-28 с размерами из Rust | `static_assert`/компиляция TU | **DONE-C** |
| G-09 | PIBD (клиент+сервер) | `pibd.rs`, `desegmenter.rs`, `txhashset.rs`, `p2p/src/protocol.rs:21-28` | Живая загрузка/активация/догон PASS (E42); **live serving FAIL**: rangeproof-сегмент idx 1100/1673 требует отсутствующий sibling hash 4513785 (E56), клиент получает desync/`Proof of size ...`. Wire/live-state round-trip PASS, значит дефект prune-state/serving | Сохранять sibling hashes или отдавать из полного snapshot, затем повторить живой serving | E32–E34/E41/E42/E55/E56 | **PARTIAL** |
| G-10 | Capabilities | `HEADER_HIST..PIBD_HIST_1` (`p2p/src/types.rs:381-399`) | Объявлены только 0x01/0x02/0x04; 0x08/0x10/0x20/0x40 отсутствуют | Невозможно корректно негодовать возможности; PIBD/BLOCK_HIST не рекламируются | P0 | Добавлен полный набор битов + `ADVERTISED` (PIBD/PIBD_1/BLOCK_HIST выключены до реализации) | `static_assert` в TU | **DONE-C** |
| G-11 | Неизвестные типы сообщений | читать и отбрасывать (`default_max_msg_size`) | Grin++ рвал соединение на любом неизвестном типе (лимит 0) | Прямая несовместимость с новыми сообщениями; дисконнект от Rust-пиров при будущих расширениях | P1 | `MessageHeader`: неизвестный тип → лимит `MAX_BLOCK_SIZE*4` (как Rust) | компиляция TU | **DONE-C** |
| G-12 | Проверка genesis в handshake | `BadHandshake` при ином genesis (`p2p/src/handshake.rs`) | Grin++ читал genesis пира и **не сравнивал** | Подключение к peer другой сети → поток «плохих» блоков/баны честных пиров | P1 | Сравнение с `Global::GetGenesisHash()` в Hand и Shake | только чтение кода (DONE-R) | **DONE-R** |
| G-13 | Формат Hand/Shake | `msg.rs:405-490` | нет: поля/порядок/кодирование совпадают | — | — | — | кодовое сравнение | ЗАКРЫТО |
| G-14 | TxHashSet archive | `hash+height+bytes` + attachment `msg.rs:735-789` | нет по формату; **но** сервер терял последний неполный чанк (`while(file.read)`) | Пир получает меньше байт, чем объявлено → таймаут, бан честной ноды | P1 | Цикл по `gcount()`, отправка последнего чанка | только чтение кода (DONE-R) | **DONE-R** |
| G-15 | Непрошеный TxHashSet | Rust: `txhashset_receive_ready()` + `state_sync_requested` иначе `BadMessage` (`p2p/src/protocol.rs:254-265`) | Grin++ писал на диск и обрабатывал архив, даже если не запрашивал (бан был закомментирован) | Любой peer может подсунуть снапшот: подмена состояния/DoS диска | P1 | Требовать статус `SYNCING_TXHASHSET`, иначе бан | только чтение кода (DONE-R) | **DONE-R** |
| G-16 | Лимит размера TxHashSet | Rust: проверка размера архива | Не было верхнего предела `txhashset.zip` — пир мог заставить писать произвольный объём | `TxHashSetLimits::MAX_ZIPPED_SIZE = 8 GiB`, проверка в `TxHashSetPipe` до записи на диск, бан `BadTxHashSet` | unit `TxHashSetLimits` | **DONE** |
| G-17 | Rate limiting на соединении | Rust: rate limit | Проверено, что ограничение уже есть: `RateCounter` + `ExceedsRateLimit()` (>500 байт/мин в любую сторону) → бан `Abusive` в цикле соединения | Отдельная доработка не требуется; порог документирован | код + обзор | **DONE (существующее)** |
| G-18 | Peer addresses / discovery | `MAX_PEER_ADDRS=256`, capability-фильтр | совпадает | — | — | — | кодовое сравнение | ЗАКРЫТО |

---

## C. Безопасность десериализации

| ID | Компонент | Эталон | Расхождение | Влияние | P | Действие | Проверка | Статус |
|---|---|---|---|---|---|---|---|---|
| G-19 | `ByteBuffer` (все парсеры) | Rust `Reader` — проверки с ограничением остатка | Все проверки вида `m_index + N > size`: переполнение 64-бит обходит проверку. Воспроизведено на handshake `user_agent`: вместо `DeserializationException` вылетал `std::length_error` (не ловится в `MessageProcessor`) | Пир одним 128-байтным Hand может вызвать UB/раскрутку исключения вне ожидаемого типа; класс уязвим для всех будущих PIBD-парсеров | P1 | `RequireRemaining()` с вычитанием; все чтения переведены на него | исполняемый пробник (до/после) | **DONE-P** |
| G-20 | Ограничения размеров в парсерах | строгие лимиты на counts/allocations | PIBD-парсеры уже имели `MAX_SEGMENT_READ_ITEMS`, checked arithmetic и проверки размера; не хватало явной проверки, что список листьев помещается в payload до reserve | `Segment::Deserialize` отвергает `numLeaves > remaining` до резервирования; лимиты `SegmentProof`/`BitmapSegment`/`BitmapBlock` и message-size cap подтверждены | unit `Test_Segment` (21 кейс, включает huge counts) | **DONE (PIBD/segment)**; fuzz вне матрицы |

---

## D. Хранилище и восстановление

| ID | Компонент | Эталон | Расхождение | Влияние | P | Действие | Проверка | Статус |
|---|---|---|---|---|---|---|---|---|
| **G-21** | Атомарная активация legacy txhashset | `TxHashSetProcessor`, `TxHashSetManager`, `TxHashSetZip` | ZIP распаковывался прямо в живое состояние до валидации; при ошибке/крахе state терялся | Staging-распаковка в `TXHASHSET.staging`, валидация до установки, atomic swap `live↔staging` с marker `txhashset_activation.pending`, восстановление backup на старте, переоткрытие live при ошибке | unit `Test_TxHashSetStaging` (3 кейса) | **DONE** |
| G-22 | Бэкап перед миграцией | Rust/процедура миграции | Миграция меняла БД без резервной копии | Реализован `DbBackup` (recursive stream copy, LOCK skipped) перед миграцией v1/v2 → v2/v3; ошибка копии прерывает миграцию; fresh v0 не бэкапится | unit `DbBackup` (2 кейса) | **DONE** |
| G-23 | Проверка MMR и chain-хранилищ при загрузке | `BlockChainImpl.cpp`, `HeaderMMR`, `MmrStructure` | Структурная проверка размера MMR (`2L−popcount(L)`); при отставании MMR — **перестройка из block DB** (заголовки не теряются), при опережении — усечение лишних листьев; recovery-копия обязательна и проверяется по размеру; цепочка позади checkpoint → отказ старта (без молчаливого усечения) | Хеш/корневая ревалидация (`--validate-on-load`) OPEN | unit `MmrStructure`/`ChainConsistency`/`DbRecovery`; live: MMR −2000 листьев → rebuild, header 3 317 117 (E48) | **PARTIAL (structural/rebuild DONE)** |

---

## E. Сборка и воспроизводимость

| ID | Компонент | Эталон | Расхождение | Влияние | P | Действие | Проверка | Статус |
|---|---|---|---|---|---|---|---|---|
| G-24 | Воспроизводимая сборка (offline sysroot) | — | Сборка offline-sysroot и `/W4 /WX` работает (`BUILD.md`); **CI-проводка и перенос пина vcpkg не сделаны** | CI + пин vcpkg | `BUILD.md`, E6–E9 | **PARTIAL** |
| G-25 | Версия ПО | — | `GrinVersion.h` = 1.2.8 при теге 1.2.9 | Исправлено на 1.2.9 (UA/REST/логи), тест `GrinVersion::MatchesRelease` | unit/CI | **DONE** |
| G-26 | `/W4 /WX` на MSVC 14.4x | — | Проверено: проект компилируется под `/W4 /WX` без правок кода; понадобились `NOMINMAX` и идемпотентная установка флагов | — | P1 | Конфигурационные правки CMakeLists внесены | сборка — PASS | ЗАКРЫТО |
| G-28 | Изоляция данных тестов | — | Тесты пишут в `%HOME%\.GrinPP\AUTOMATED_TESTING`; без изолированного HOME падают с `Access is denied` (окружение), и в общем случае задевают пользовательский каталог | Риск для пользовательских данных при прогоне | P1 | Документировано в `BUILD.md`; в CI/скриптах задавать изолированный `HOMEDRIVE`/`HOMEPATH` (явный test-root — следующий шаг) | прогон тестов с изоляцией — PASS (E8) | **DONE (документация)** |

---

## F. Расширения сверх v5.5.2

| ID | Компонент | Источник | Состояние |
|---|---|---|---|
| G-27 | PIHD (`GetHeaderSegment=29`, `HeaderSegment=30`, `PIHD_HIST=0x80`) | Ветка `staging` (PR #3879 — протокол, #3882 — синхронизация; влиты 29.06 и 14.07.2026) | **Не входит в v5.5.2** (тег `dbaff34` от 09.10.2026: enum до `KernelSegment = 28`). В `staging` реализовано полностью: `PIHD_HIST = 0x80` в default-capabilities, сегмент 512 заголовков при `height = 9` и `start = idx*512+1`, сервер с троттлингом 120 запросов/60 с, клиент с in-flight 8/2 на пира, таймаутом 20 с и фолбэком на legacy через 3 таймаута и 120 с без прогресса. Для совместимости с v5.5.2 не требуется; детали — `PIHD_UPSTREAM.md` |

---

## G. Дефекты и проверки, найденные живым interop

| ID | Компонент | Эталон/источник | Расхождение | Действие | Проверка | Статус |
|---|---|---|---|---|---|---|
| **G-33** | PIBD serving прореженного состояния | `chain/src/txhashset/txhashset.rs`, `segmenter.rs` | `leafset` не строился из битмапа горизонта; `ComputePrunedTreeRoot` требовал хеши интерьера и падал на полностью прореженном пике | `leafset` пишется из битмапа (потраченные листья остаются spent), `GetRawAt` отдаёт данные, корень считается по максимальным прореженным поддеревьям | тест serve→download на прореженном битмапе ({0} из 64 листьев) | **CLOSED** |
| **G-29** | Приём входящих P2P-соединений | `Seeder::Accept` | При отклонённом пире (`blocked`/`not allowed`) функция делала `return` до повторного `async_accept`; первый отказ навсегда останавливал listener (в interop Rust-нода не могла подключиться, `LISTENING` отсутствовал) | Исправлено: accept переставляется после любого исхода; после фикса `0.0.0.0:13414 LISTENING` | сборка, набор на тот момент 134/1541, listener проверен netstat | **DONE** |
| **G-30** | Реклама возможностей в handshake | `msg.rs` Hand/Shake | `HandShake.cpp` отправлял `Capabilities::FAST_SYNC_NODE` вместо `ADVERTISED`, поэтому PIBD-биты не доходили до пиров (Rust видел только TXHASHSET_HIST\\|PEER_LIST) | Отправляется `ADVERTISED`; проверено живым Rust-пиром: `PIBD_HIST \\| PIBD_HIST_1` в его handshake-логе | live handshake-лог Rust | **DONE** |
| **G-31** | Горизонт PIBD | `chain.rs:985,1002` (`txhashset_archive_header*`) | Запрашивали сегменты для candidate tip; Rust проверяет запрошенный hash против header сегментера (archive header = (head − STATE_SYNC_THRESHOLD) с округлением до 720), поэтому реальный пир не отвечал | Горизонт вычисляется так же; завершение ставит состояние на этот header | live bitmap-сегмент сходится с `header.output_root` (E31) | **DONE** |
| **G-32** | Wire-формат `BitmapBlock` | `bitmap_accumulator.rs:470,213` | Блок = 2^16 бит = 64 чанка по 1024, у нас было 16 → реальные сегменты не десериализовались | Исправлено; парсинг живого сегмента PASS | живой interop + unit | **DONE** |
| **G-34** | Live-сверка bitmap-корня PIBD | `bitmap_accumulator.rs`, `segment.rs` | Сегменты (height 9 и 13) от двух независимых живых пиров дают один и тот же свёрнутый bitmap-корень `570d270a…`, но он не сходится с `header.output_root`, а proof сегмента 0 даёт `a9591a…`; C++ и независимый Python совпадают между собой | **CLOSED (снят G-35)**: оракул `grin_chain 5.5.2` подтвердил формат и хеширование чанков — 7/7 корней сегментов и корень всего битмапа совпали; исходное расхождение объяснено порядком бит (MSB-first) | живые векторы `.tmp/interop`, оракул G-35 | **CLOSED** |
| **G-35** | Порядок бит в bitmap-структурах | `bitvec::to_bytes` (MSB-first) | `BitmapChunk`/`BitmapBlock` индексировали биты LSB-first; Raw-блоки сходились, но Positive/Negative (sparse) кодируют индексы в MSB-нумерации → все sparse-сегменты PIBD читались неверно | Исправлено на MSB-first; сверено с Rust-оракулом (`grin_chain 5.5.2`): 7/7 корней сегментов + корень всего битмапа совпали | оракульные векторы + unit-тест + живой PIBD (E32) | **DONE** |
| **G-36** | Живой PIBD download+активация | `pibd.rs`, `desegmenter.rs` | Проверено на реальном пире: bitmap/output/rangeproof/kernel сегменты 22/24/26/28 загружены без ошибок, staging записан, активация выполнена | — | живой лог E32 | **PASS** |
| **G-37** | Валидация активированного состояния и догон блоков | `txhashset.rs` | kernel history, kernel sums и rangeproofs прошли; догон до типсы выполнен (с обходом G-38) | — | живой лог E33/E34 | **PASS** (зависимость от G-38 отмечена) |
| **G-39** | Полная живая синхронизация до типсы | `pibd.rs`, `block_sync` | Проверено на реальной testnet: PIBD → активация → валидация TxHashSet → догон блоков до `chain=header=network=3316561`, `FULLY_SYNCED` | — | живой REST-статус + логи E34 | **PASS** |
| **G-40** | Сохранение состояния между рестартами | `chain`/`txhashset` load | После рестарта узел сразу `FULLY_SYNCED` на той же высоте/хеше, PIBD не перезапускается (state-level resume; journal-resume скачивания ещё не проверен) | — | живой лог E34 | **PASS** |
| **G-41** | Живой legacy txhashset.zip fallback | `StateSyncer`, `TxHashSetPipe`, `TxHashSetZip` | Архивный сервер отдал 3,1 ГБ, клиент скачал/распаковал/валидировал и догнал блоки до `FULLY_SYNCED` на 3 317 664 (06:53) | `FULLY_SYNCED` по legacy | E54 | **DONE** |
| **G-38** | Ложные баны пиров и залипание block sync | `servers/src/grin/sync/body_sync.rs:149,191-204`, `header_sync.rs:135-141` | Подтверждено: `BlockSyncer` банил за таймаут, `m_slowPeers` не очищался, окно не ограничивалось высотой пира, seeder/sync молчали. Исправлено: `BlockSyncPolicy` (batch≤100, `min(tip,network,peer height)`, retry 6/1 с + backoff, cooldown 60 с); `HeaderSyncer` — FraudHeight только по правилу Rust; seeder reason/backoff; waiting_reason/счётчики. **Live-инцидент:** 3 ложных бана (107.172/146.59/203.88) из-за условия «высота ИЛИ TD» → правило приведено к точному Rust (только строго больший TD, `header_sync.rs:136-137`), тест обновлён | unit; E37/E42/E48 | **DONE** |
| **G-42** | Legacy state sync банит пиров за таймаут | `StateSyncer::RequestState` | При `txhashset.zip` 10–120 с без прогресса пир банился с `BadTxHashSet` и узел оставался без пиров | Исправлено: медленный пир теперь **ротируется** (`Rotating txhashset sync peer`), а не банится; реальный плохой txhashset по-прежнему банится в `TXHASHSET_PIPE` при ошибке валидации | unit/сборка; живой legacy-прогон — G-41 | **DONE (код)** |
| **G-43** | PIBD: стопор пира на сегменте | `PibdScheduler`/`PibdSync` | Живой прогон resume: пир перестал отдавать `tree 2 idx 1229` → retry limit → PIBD failed → fallback; журнал replay при этом сработал (0 повторных bitmap-запросов) | Исправлено: `PibdScheduler` ретраит сегмент с backoff 20/40/60 с и падает только при 660 с без прогресса (как таймер Rust); `StateSyncer` при `PibdSync::Failed` перебирает других PIBD-пиров (`RotatePibdPeer`, `m_pibdPeersTried` на горизонт), `DisablePibd()` только после исчерпания; запросы идут выбранному пиру | unit/сборка; живой прогон: 17:13:30 `PIBD failed: no PIBD progress…` → 17:13:31 `RotatePibdPeer … another peer 66.154.127.84` → 17:14 новый старт; затем legacy-ротация `RequestState` 66.154→195.201→157.90 (G-42); replay журнала без повторных bitmap-запросов (E41). Остаток: порог ротации по скорости (сейчас только нулевой прогресс) и повторный обход набора после backoff — не реализованы; полный догон с ротацией прерван троттлингом пира | **DONE**, полный live-догон — отдельно |


| **G-46** | Краш-согласованность chain-хранилищ | `HeaderMMR`/`Chain`/`ChainStore` | Три независимых append-only файла; kill между коммитами оставляет разные высоты; загрузка читала за MMR → `0xC0000005`. Исправлено: `chain.checkpoint`, **MMR отстаёт → rebuild из block DB** (без потери заголовков), опережает → усечение лишних листьев; recovery-копия обязательна (проверка размера), цепочка позади checkpoint → отказ старта. Live: MMR −2000 листьев → rebuild, header сохранён (E48) | unit; live E39/E43/E48 | **DONE (no-loss rebuild)** |
| **G-47** | NULL-deref в обработчике Ping | `MessageProcessor.cpp` | При отсутствии confirmed tip `pTipHeader->GetTotalDifficulty()` падал (0xC0000005) | Проверка на nullptr: `WARNING "Cannot answer Ping: no confirmed tip header."`, Pong не отправляется | сборка/тесты | **DONE** |
| **G-48** | Атомарное формирование БД (чекпоинт записи) | `Chain::Commit`, `ChainStore::Commit`, `HeaderMMR::Commit` | Три файла коммитятся по отдельности | Реализовано: `DbCheckpoint` (52 Б, temp+rename, FNV-1a) после всех коммитов; загрузка: checkpoint + инварианты, rebuild MMR из DB, проверенный backup, отказ при цепочке позади checkpoint; `db_repaired` в `/v1/status` | unit; live E43/E44/E48 | **DONE (checkpoint)** |
| **G-49** | Валидация rangeproofs: ~28 мин из ~31 | `TxHashSetValidator::ValidateRangeProofs`, `Bulletproofs::VerifyBulletproofs` | Проверка однопоточная, батчи 1000 по ~0,46 с; scratch 256 МБ на вызов; нет инкрементальной проверки и резюма (Rust проверяет по мере PIBD, `desegmenter.rs:317`), рестарт начинает 28 минут заново | **Запланировано после приёмки:** (1) инкрементальная проверка + `last_rangeproof_validation_pos` и остановка; (2) параллельные батчи (8 воркеров, ожидаемо ~4–4,5 мин); (3) scratch на воркер и правильный размер; (4) опционально asm secp. Расчёты, план и критерии — `PERF_RANGEPROOF.md` | bench до/после: proofs/s, время фаз, RSS, идентичные roots/sums | **OPEN (запланировано, после приёмки)** |

| G-50 | Повторный DNS-discovery и целевое число пиров | `Seeder.cpp`, `PeerManager.cpp` | DNS повторялся, но сидер держал ровно `MIN_PEERS` (2–3 в старых конфигах) и не набирал запас для ротации | Цель `max(min(8,MAX_PEERS), min(MIN_PEERS,MAX_PEERS))`; старые конфиги получают 8 без ручной миграции; `Test_SeederPolicy` | unit + живой прогон 7–8 outbound (E52) | **DONE** |
| **G-51** | Аудиторский пакет и комментарии кода | `docs/modernization/*`, `dist/audit-1.2.9-79812A` | Дельта: 29 карточек, 8 полей, 18 diff-блоков, паритет EN/DE/RU; ревизия комментариев/секретов, README EN/DE/RU, пакет `dist/audit-1.2.9-79812A` + SHA256SUMS, лицензии/notices (11 standalone + 6 embedded) готовы; **осталась только внешняя подпись чек-листа** | независимый аудитор подписывает `AUDIT_CHECKLIST.md` | `check_audit_delta.ps1` PASS; D5 PASS | **IN PROGRESS (ready for sign-off)** |

| **G-52** | Исторический TxHashSet archive: сохранение/выдача на горизонте | `TxHashSetManager`, `TxHashSetZip`, `TxHashSetPipe`, `TxHashSetArchive` | Архив на горизонте не сохранялся (leafset/leaf, int overflow, `\` в zip), исторический rewind падал на spent-gap. Теперь архив сохраняется при активации, отдаётся по hash без rewind и успешно принимается legacy-клиентом | живая выдача + валидация (E54) | **DONE** (архив на горизонте; произвольные более старые snapshot по-прежнему не поддерживаются) |

| **G-53** | Node/API: недостающие методы grin v5.5.2 | `src/API/Node/NodeServer.cpp`, `src/API/Node/Handlers/*`, `src/TxPool/*`, `src/PMMR/TxHashSet*`, `src/Server/Node/API/*` | JSON-RPC `/v2/foreign` содержал 7 методов из 13 у grin v5.5.2, `/v2/owner` пуст; REST v1 — легаси Grin++, его каталог `GET /v1/` отставал от реальных маршрутов | Реализованы все 13 методов `/v2/foreign` (`get_pool_size`, `get_stempool_size`, `get_unconfirmed_transactions`, `get_pmmr_indices`, `get_unspent_outputs`, `get_outputs`, `get_kernel` + `ITxHashSet::FindKernelByExcess`, `get_blocks` + общий `PrintableSerialization.h`; c60604f/84f135e/8fa265f/474acc4/fa663d5) и 3 метода `/v2/owner` (`get_status`, `get_peers`, `get_connected_peers`; 3b4efd4); каталог `GET /v1/` обновлён (efe27b7). Unit-тесты — `Test_NodeApiPrintables` (a76dfd5) | E53: полная сборка `GrinNode.exe`/`Tests.exe`, 174 кейса / 1810 проверок PASS; живые вызовы `/v2` на запущенной ноде — не подтверждены | **DONE (unit + build)** |

---

## Сводка

**Итого по матрице: 53 записи** — **44 закрыто**, 3 частично (G-09, G-23, G-24),
1 в работе (G-51), 0 открыто, 3 запланировано (G-44, G-45, G-49),
1 осознанно отложен (G-07), 1 снят по объёму (G-27, PIHD не в v5.5.2).
44 + 3 + 1 + 0 + 3 + 1 + 1 = 53.

* **G-52 и G-41 закрыты живьём (E54):** архив 3 117 206 023 байт сохранён и
  отдан, клиент скачал/распаковал/валидировал его и достиг `FULLY_SYNCED`
  на 3 317 664 (06:53).
* В работе: G-51 (аудит-пакет: 29 карточек, 18 diff-блоков, паритет EN/DE/RU;
  ревизия комментариев/секретов выполнена; остались лицензии, README,
  финальные hashes/provenance и подпись чек-листа).
* Закрыто по API: **G-53** — все 13 методов `/v2/foreign` и 3 метода `/v2/owner`,
  каталог `GET /v1/` приведён к реальным маршрутам; unit-тесты
  (`Test_NodeApiPrintables`) и полная сборка — E53. Живые вызовы `/v2` на
  запущенной ноде остаются за следующим прогоном.
* Частично: G-09 (PIBD: живая загрузка/активация/догон PASS, resume replay PASS;
  живой serving Rust-клиентом и полное завершение resume не подтверждены),
  G-23 (структурная проверка/rebuild MMR DONE — E48; hash-level ревалидация
  OPEN), G-24 (offline build DONE; CI-проводка и пин vcpkg — вне матрицы, фазы 5–7).
* Запланировано: G-44 (спящий каркас PIHD), G-45 (mainnet dry-run), G-49
  (ускорение rangeproof-валидации; `PERF_RANGEPROOF.md`).
* Отложено: G-07 (NRD). N/A: G-27 (PIHD в v5.5.2).
* Закрыто с живой проверкой: G-25 (версия 1.2.9), G-31 (горизонт), G-32
  (`BitmapBlock`), G-35 (порядок бит), G-36 (download+активация), G-37 (валидация
  состояния), G-39 (догон до `FULLY_SYNCED`), G-40 (сохранение состояния между
  рестартами), G-38 (4 222 переназначения без fraud-банов в полном догоне, E42),
  G-43 (E42: полный цикл с горизонта — ротация 204→66, активация, валидация,
  `FULLY_SYNCED`, рестарт без PIBD), G-46 (починка рассинхрона MMR/chain
  подтверждена инъекциями, E39), G-47 (NULL-deref в `Ping`, E39), G-48 (атомарный
  `chain.checkpoint`, hard kill seq 26→27 и инъекции, E43), G-50
  (DNS-обнаружение повторяется, адреса добавляются). G-42 исправлен в коде и
  проверен живой ротацией legacy-пиров (E42); полный legacy-догон — G-41.
  G-16 (лимит `txhashset.zip` 8 GiB) и G-23-структурная (проверка размера MMR)
  закрыты unit-тестами (`TxHashSetLimits`, `MmrStructure`) — E44/E45.
* PIHD (G-27) — не входит в v5.5.2 (enum до `KernelSegment = 28`); в ветке `staging`
  реализовано целиком (PR #3879/#3882), для совместимости с v5.5.2 не требуется —
  разбор в `PIHD_UPSTREAM.md`.
* Сборка и тесты проверяемы исполнением: `/W4 /WX`, актуальный прогон — последняя
  запись `TEST_RESULTS.md` (E47: 167 кейсов / 1783 проверки PASS).
* Не подтверждено: живой serving Rust-клиенту, legacy-фолбэк (G-41), reorg,
  crash/fuzz, **mainnet (G-45)**. Живой журнальный resume: replay подтверждён,
  полное завершение — G-43.