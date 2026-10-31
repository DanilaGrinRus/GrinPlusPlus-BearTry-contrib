# AUDIT_DELTA (RU) — Grin++ 1.2.9 → совместимость с Grin v5.5.2

Базовая точка: Grin++ 1.2.9 (`5f511e6`). Дельта обновлена: 31.10.2026, HEAD `256cbe4`.
Сопутствующие документы: `AUDIT_PACKAGE.md` (правила пакета), `GAP_MATRIX.md`
(статус каждой лазейки), `TEST_RESULTS.md` (доказательства E1–E47),
`DB_CONSISTENCY.md` (устройство chain-хранилищ).

Область: одна карточка на изменение кода относительно базовой точки либо на
неразделимую группу изменений. В каждой карточке — коммит(ы), файлы, ссылка на
эталон Rust, фрагмент «было/стало», доказательство, риск и откат. Пункты без
изменений кода исключены (см. конец); чисто документальные коммиты учитываются
в карте доказательств, а не как карточки.

## Карточки

### G-24 — Воспроизводимая сборка (offline-sysroot)
- Статус: DONE
- Коммиты: `89eb22c`, `d034a2f`
- Файлы: `tools/deps/build_sysroot.py`, `tools/build/build.ps1`, `CMakeLists.txt`, `src/CMakeLists.txt`
- Ссылка на Rust: n/a (инфраструктура сборки)
- Было: configure тянул зависимости из сети/системного vcpkg и использовал тот MSVC/Ninja, что оказался в `PATH`; сборка не была воспроизводимой офлайн.
- Стало: `.toolchain/sysroot` даёт заголовки и библиотеки офлайн; генератор/тулсет закреплены; `build.ps1 -Target GrinNode|Tests` — единая точка входа; предупреждения собираются с `/W4 /WX`.
- Доказательство: `BUILD.md`, E6–E9 (чистая сборка, предупреждения не отключались), SHA-256 бинарей в `BUILD.md`.
- Риск, откат: риск — устаревший/неполный sysroot; откат — вернуть скрипты/CMake и собирать с системными зависимостями.

### G-19 — Безопасная десериализация (проверки границ)
- Статус: DONE
- Коммиты: `fc9e988`
- Файлы: `include/Core/Serialization/ByteBuffer.h`, `tests/src/Core/Serialization/Test_ByteBuffer.cpp`
- Ссылка на Rust: `core/src/ser.rs` (проверки границ чтения)
- Было: чтение varint/vector/string могло выйти за пределы payload; арифметика размеров не проверялась против остатка буфера.
- Стало: каждое чтение проверяется по концу буфера (с учётом версии протокола); усечённые и чрезмерные входы аккуратно отвергаются; unit-тесты покрывают оба случая.
- Доказательство: E2 (воспроизведено до/после), `Test_ByteBuffer`.
- Риск, откат: строгие проверки могут отвергнуть погранично-валидные сообщения; версионный гейт ограничивает это; откат — вернуть `ByteBuffer`.

### G-08 / G-10 / G-11 — Таблица сообщений, capabilities, неизвестные типы
- Статус: DONE
- Коммиты: `a5c88bf`
- Файлы: `include/P2P/Capabilities.h`, `src/P2P/Messages/MessageTypes.h`, `src/P2P/Messages/MessageHeader.h`, `tests/src/Net/Test_P2PCompat.cpp`
- Ссылка на Rust: `p2p/src/msg.rs:67-97`, `p2p/src/types.rs:381-399`
- Было: таблица сообщений и биты capabilities не соответствовали grin v5.5.2; типов PIBD-сегментов не было, неизвестные типы явно не отвергались.
- Стало: полная таблица `EMessageType`, включая segment request/response 21–28; значения capabilities как в Rust; неизвестные типы отвергаются до dispatch; тесты совместимости закрепляют оба аспекта.
- Доказательство: E3, `Test_P2PCompat`.
- Риск, откат: расхождение enum ломает handshake/диспетчеризацию; тесты закрепляют wire-значения; откат — вернуть таблицу и заголовки capabilities вместе.

### G-12 / G-14 / G-15 — Genesis в handshake, отправка/приём архива
- Статус: DONE (код); живой перенос архива NOT VERIFIED
- Коммиты: `e5287f2`
- Файлы: `src/P2P/Seed/HandShake.cpp`, `src/P2P/MessageProcessor.cpp`, `src/P2P/Pipeline/TxHashSetPipe.cpp`
- Ссылка на Rust: `p2p/src/handshake.rs`, `p2p/src/msg.rs:735-789`, `p2p/src/protocol.rs:254-265`
- Было: handshake не проверял genesis-hash; отправка архива шла циклом по `istream::read` и молча теряла последний неполный блок; у получателя не было проверки размера/очистки.
- Стало: несовпадение genesis отвергает пира; кадрирование архива использует `gcount()` и отправляет каждый байт, с проверкой размера и удалением неполных файлов.
- Доказательство: E4/E5 (уровень кода); живой legacy-перенос архива по-прежнему NOT VERIFIED (G-52).
- Риск, откат: отказ при другом genesis — намеренный; изменения ограничены `TxHashSetPipe`; откат — вернуть оба файла.

### G-25 — Версия 1.2.9
- Статус: DONE
- Коммиты: `348d87e`
- Файлы: `include/GrinVersion.h`, `include/P2P/Common.h`
- Ссылка на Rust: n/a
- Было: `GRINPP_VERSION`/user agent сообщали 1.2.8.
- Стало: версия и UA — 1.2.9; `GrinVersion::MatchesRelease("Grin++ 1.2.9")` закрепляет это, `/v1/status` отдаёт ту же строку.
- Доказательство: unit-тест, живой `/v1/status` (E30).
- Риск, откат: версия влияет на баннеры пиров; откат — одна строка.

### G-29 — Accept-loop выживает после отклонённого пира
- Статус: DONE
- Коммиты: `bfc31a0`
- Файлы: `src/P2P/Seed/Seeder.cpp`
- Ссылка на Rust: `p2p/src/serv.rs` (accept-loop)
- Было: `Seeder::Accept` возвращался после отклонения пира, не перезапуская `async_accept`; listener умирал после первого отказа.
- Стало: обработчик accept всегда перезапускает listener; отклонённое соединение больше не убивает входящий сервис.
- Доказательство: E29 (`0.0.0.0:13414 LISTENING` после отказов), живые handshake.
- Риск, откат: постоянная ошибка accept могла бы крутиться; путь ошибки защищён; откат возвращает прежнее поведение.

### G-30 — Реклама capabilities в handshake + настраиваемый P2P-порт
- Статус: DONE
- Коммиты: `72c94f2`
- Файлы: `src/P2P/Seed/HandShake.cpp`, `include/Core/Config.h`, `src/Core/Config.cpp`, `src/Core/Config/ConfigProps.h`, `src/Core/Config/P2PConfig.h`
- Ссылка на Rust: `p2p/src/msg.rs` (поля Hand/Shake)
- Было: handshake рекламировал жёстко зашитый `FAST_SYNC_NODE`, из-за чего PIBD-биты не были видны; порт прослушивания использовался и для исходящих соединений.
- Стало: handshake рекламирует `ADVERTISED` (`FAST_SYNC_NODE | PIBD_HIST | PIBD_HIST_1`); `P2P_PEER_PORT` задаёт порт дозвона; оба параметра конфигурируются и тестируются.
- Доказательство: E30 (живой Rust-пир видит `TXHASHSET_HIST | PEER_LIST | PIBD_HIST | PIBD_HIST_1`).
- Риск, откат: пиры могут иначе трактовать лишние биты; PIBD-биты отключаются политикой; откат возвращает прежнее значение.

### G-09a — Проводной слой PIBD, верификация сегментов, генератор
- Статус: DONE
- Коммиты: `62a146c`, `8e2f493`, `de3f1a5`, `c390ee9`
- Файлы: `src/PMMR/Models/Segment*.h`, `src/PMMR/Models/Bitmap*.h`, `src/P2P/Messages/Segment*Message.h`, `tests/src/PMMR/Test_Segment.cpp`, `tests/src/PMMR/Test_SegmentGenerator.cpp`
- Ссылка на Rust: `core/src/core/pmmr/segment.rs`, `chain/src/txhashset/*`
- Было: не было ни проводных сообщений PIBD, ни моделей сегментов; существовал только legacy zip.
- Стало: segment request/response 21–28, модели bitmap chunk/block и сегментов с верификацией, детерминированный генератор; unit-тесты сравнивают корни и байты wire.
- Доказательство: E26, unit-наборы `Test_Segment`, `Test_SegmentGenerator`.
- Риск, откат: несовпадение wire делает PIBD нерабочим, legacy остаётся; флаг `P2P.PIBD_ENABLED=false` отключает путь; откат удаляет сообщения/модели.

### G-09b — Сборка битмапа, оркестрация загрузки, сессия/планировщик, снапшот
- Статус: DONE
- Коммиты: `621efad`, `ec006a9`, `21df0b9`, `df81cc3`, `e167e8a`
- Файлы: `src/P2P/PIBD/BitmapSegmentAssembler.h`, `PibdDownload.h`, `PibdSession.h`, `PibdScheduler.h`, `PibdSnapshot.h`, `PibdTreeAssembler.h`, `PibdTreeRebuilder.h`
- Ссылка на Rust: `chain/src/txhashset/desegmenter.rs`, `segmenter.rs`, `p2p/src/protocol.rs:294+`
- Было: не было сборки битмапа, оркестрации загрузки и планировщика запросов; PIBD нельзя было вести.
- Стало: bitmap-сегменты собираются и валидируются, загрузка планируется с лимитом in-flight, таймаутами, ретраями и fallback, деревья собираются/пересобираются из сегментов, есть snapshot API.
- Доказательство: E24/E25/E26, unit-тесты по модулям.
- Риск, откат: память/CPU и штормы ретраев ограничены настройками планировщика; отключение через `PIBD_ENABLED` или откат модуля.

### G-09c — Запись staging-PMMR и атомарная активация
- Статус: DONE
- Коммиты: `b681c2e`, `fcea044`
- Файлы: `src/P2P/PIBD/PibdStateWriter.h`, `src/P2P/PIBD/PibdActivation.h`
- Ссылка на Rust: `chain/src/txhashset/txhashset.rs` (snapshot), семантика активации
- Было: для загруженного состояния PIBD не было краш-безопасного staging/активации.
- Стало: `PibdStateWriter` пишет staging-PMMR; активация использует временный каталог/маркер и переключается после валидации; тесты инжектируют падения в обе стороны и проверяют восстановление.
- Доказательство: `Test_PibdStateWriter`, `Test_PibdActivation`.
- Риск, откат: частичный staging нельзя устанавливать; маркер и тесты это обеспечивают; откат удаляет staging и сохраняет живое состояние.

### G-09d — Выдача PIBD-сегментов из PMMR ноды (включая прореженные)
- Статус: DONE (unit); живой serving NOT VERIFIED
- Коммиты: `49942ab`, `30012d6`, `4f9a569`, `177c97a`
- Файлы: `src/P2P/PIBD/PibdSegmentService.h`, `src/P2P/MessageProcessor.cpp`, `include/BlockChain/BlockChain.h`, `src/BlockChain/BlockChainImpl.cpp`
- Ссылка на Rust: `chain/src/txhashset/segmenter.rs`, `p2p/src/protocol.rs` (21/23/25/27)
- Было: нода не умела отвечать на `GetSegment`; выдачи сегментов не существовало.
- Стало: `PibdSegmentService` отдаёт bitmap/output/rangeproof/kernel-сегменты из PMMR ноды, включая прореженные пути; MessageProcessor маршрутизирует запросы, BlockChain предоставляет сегментер; тест с прореженным битмапом покрывает сложный случай.
- Доказательство: `Test_PibdServing`; живая выдача Rust-клиенту NOT VERIFIED.
- Риск, откат: неверный сегмент или DoS при регрессе проверки горизонта; unit-тесты закрепляют пути; откат убирает маршрутизацию и сервис.

### G-09e — Проводка в ноду: StateSyncer, P2P-сервер, обработка TxHashSet
- Статус: DONE
- Коммиты: `7ed7ffb`, `edd6151`, `921ca42`
- Файлы: `src/P2P/Sync/StateSyncer.cpp`, `src/P2P/P2PServerImpl.cpp`, `src/BlockChain/Processors/TxHashSetProcessor.cpp`, `src/BlockChain/BlockChainImpl.cpp`
- Ссылка на Rust: `chain/src/chain.rs` (применение txhashset), `p2p/src/protocol.rs`
- Было: StateSyncer использовал только `txhashset.zip`; у P2P-сервера не было ресурсов PIBD; установка/валидация PIBD-состояния отсутствовала.
- Стало: `TryPibd` ведёт PIBD-сессию до legacy-fallback, сервер подключает resume/serving, `ProcessExistingTxHashSet` валидирует и устанавливает загруженное состояние.
- Доказательство: E32–E34, E42 (живая активация, валидация, `FULLY_SYNCED`).
- Риск, откат: неверная установка может повредить состояние; валидация идёт до активации; `PIBD_ENABLED=false` — безопасный откат.

### G-31 — Детерминированный archive-горизонт
- Статус: DONE
- Коммиты: `39b77a9`, `9027ebf`
- Файлы: `src/P2P/Sync/StateSyncer.cpp`, `include/Consensus.h`
- Ссылка на Rust: `chain/src/chain.rs:985` (`txhashset_archive_header`)
- Было: PIBD запрашивал hash candidate tip, и пиры отвечали `SegmenterHeaderMismatch`.
- Стало: горизонт `archive_height = (head − STATE_SYNC_THRESHOLD)`, округлённый вниз до `TXHASHSET_ARCHIVE_INTERVAL = 720`; PIBD и legacy-запрос архива используют эту высоту.
- Доказательство: E31 (живой сегмент сходится с `header.output_root`), E42.
- Риск, откат: off-by-one в интервале ломает валидацию сегментов; оракул/живые тесты это закрепляют; откат возвращает запрос по tip (известно неверный).

### G-32 / G-35 — Размер блока битмапа (64 чанка) и порядок бит MSB-first
- Статус: DONE
- Коммиты: `39b77a9`, `c6ab7ff`
- Файлы: `src/PMMR/Models/BitmapBlock.h`, `src/PMMR/Models/BitmapChunk.h`, `src/P2P/PIBD/BitmapSegmentAssembler.h`, `tests/src/PMMR/Test_Segment.cpp`
- Ссылка на Rust: `chain/src/txhashset/bitmap_accumulator.rs:470,213`
- Было: `BitmapBlock` содержал 16 чанков и индексировал биты LSB-first; sparse (positive/negative) сегменты читались неверно, реальные сегменты не десериализовались.
- Стало: 2^16 бит = 64 чанка × 1024; бит `n` лежит в байте `n/8`, бит `7−(n%8)` (MSB-first, как `bitvec::to_bytes`); raw-чанки пишут 1024 байта без префикса длины.
- Доказательство: E32/E35, оракульные векторы (`grin_chain 5.5.2`): 7/7 корней сегментов и корень всего битмапа совпадают.
- Риск, откат: неверный порядок бит молча портит корни; оракул/unit-векторы это ловят; откат — только с отключённым PIBD.

### G-09f — Журнал resume для PIBD
- Статус: DONE (unit + живой replay)
- Коммиты: `f73c7e8`
- Файлы: `src/P2P/PIBD/PibdResume.h`, `src/P2P/PIBD/PibdSync.h`
- Ссылка на Rust: `chain/src/txhashset/desegmenter.rs` (позиция resume)
- Было: рестарт выбрасывал все скачанные сегменты, PIBD начинался с нуля.
- Стало: append-only журналы по типам (`bitmap.log`, `output.log`, `rangeproof.log`, `kernel.log`, `manifest`) с u32 BE рамками длины воспроизводятся до новых запросов; manifest привязывает журнал к горизонту и очищается только после активации.
- Доказательство: E41/E42 (после рестарта нет повторных bitmap-запросов; журнал рос, загрузка продолжилась).
- Риск, откат: устаревший/битый журнал может дать неверные сегменты; проверки горизонта/десериализации его отбрасывают; откат — удалить `NODE/pibd_resume`.

### G-43 — Ротация пиров, backoff, порог скорости, сохранение журнала
- Статус: DONE
- Коммиты: `47b6bf1`, `5de693d`, `a5a976a`, `b377220`
- Файлы: `src/P2P/PIBD/PibdScheduler.h`, `src/P2P/Sync/StateSyncer.cpp`
- Ссылка на Rust: `p2p/src/peers.rs` (выбор пиров), таймерная семантика `pibd.rs`
- Было: первый стопор сегмента прерывал PIBD и отключал его на 120 минут; медленный, но живой пир не ротировался; после сброса горизонта указатель пира мог быть null (падение).
- Стало: сегмент повторяется до 660 с без принятого ответа; `RotatePibdPeer` пробует других пиров, затем до трёх кругов по набору с backoff 30/60/120 с; `RateWindow` ротирует пиров, принявших менее 10 сегментов за 180 с; `Reset()` сохраняет журнал; после смены горизонта пир выбирается заново.
- Доказательство: E40 (регрессия null-пира исправлена), E41/E42 (живая ротация 204→66, полный цикл).
- Риск, откат: churn ротаций в медленной сети; пороги — константы; откат возвращает планировщик/ротацию, legacy-fallback остаётся.

### G-38 — Политика block sync: никаких банов за таймаут
- Статус: DONE
- Коммиты: `4dbae1b`, `c06888b`
- Файлы: `src/P2P/Sync/BlockSyncPolicy.h`, `src/P2P/Sync/BlockSyncer.cpp`, `src/P2P/Sync/HeaderSyncer.cpp`, `src/P2P/Seed/Seeder.cpp`, `include/P2P/SyncStatus.h`, `src/P2P/MessageProcessor.cpp`
- Ссылка на Rust: `servers/src/grin/sync/body_sync.rs:149,191-204`, `header_sync.rs:135-141`
- Было: `BlockSyncer` банил пира за `FraudHeight` после двух таймаутов, `m_slowPeers` не истекал, окно запросов игнорировало высоты пиров, seeder/syncer молчали после банов.
- Стало: таймаут считается и блок переназначается, без бана; запросы ограничены `min(candidate tip, network height, per-connection height)` и batch ≤100; retry 6 с (1 с при прогрессе) + backoff; медленный пир получает cooldown 60 с; `FraudHeight` требует большей заявленной работы и 120 с без прогресса заголовков; seeder логирует причину/backoff; `/v1/status` отдаёт waiting reason и счётчики.
- Доказательство: E37 (32-минутное живое окно), E42 (4 222 переназначения, ноль fraud-банов).
- Риск, откат: без банов неотвечающий пир может замедлить догон; cooldown/ротация ограничивают; откат возвращает прежний путь бана.

### G-42 — Ротация пира legacy state sync вместо бана
- Статус: DONE
- Коммиты: `bbf9a56`
- Файлы: `src/P2P/Sync/StateSyncer.cpp`
- Ссылка на Rust: `servers/src/grin/sync/syncer.rs`, `state_sync.rs`
- Было: `RequestState` банил пира `BadTxHashSet`, если zip не давал прогресса 10 с или истекали 120 минут; нода могла остаться без пиров.
- Стало: медленный пир ротируется (`Rotating txhashset sync peer`); действительно невалидный архив по-прежнему банится пайплайном после ошибки валидации.
- Доказательство: E41/E42.
- Риск, откат: сломанный пир может быть повторён; ротация логируется; откат возвращает бан.

### G-46 / G-47 — Починка chain-хранилищ и NULL-guard Ping
- Статус: DONE (mitigation при загрузке)
- Коммиты: `0331ea1`
- Файлы: `src/BlockChain/ChainConsistency.h`, `src/BlockChain/BlockChainImpl.cpp`, `src/P2P/MessageProcessor.cpp`, `tests/src/BlockChain/Test_ChainConsistency.cpp`
- Ссылка на Rust: n/a (инварианты загрузки)
- Было: после грязного завершения header MMR и candidate/confirmed chain могли разойтись; загрузка читала за пределы MMR (0xC0000005). Обработчик Ping разыменовывал отсутствующий confirmed tip.
- Стало: `ComputeChainRepair` сравнивает количество листьев/записей и усекает хранилища до согласованной цели до открытия `ChainState`; Ping проверяет tip и отвечает либо пишет WARNING вместо падения.
- Доказательство: E39 (unit + живая инъекция в обе стороны).
- Риск, откат: починка может отрезать незакоммиченные заголовки; recovery-копии и checkpoint ограничивают потерю; откат — восстановить `NODE/recovery-*`.

### G-48 — Атомарный checkpoint chain-хранилищ
- Статус: DONE (checkpoint; хеш-валидация — G-23)
- Коммиты: `fdef6e6`, `1c4d020`
- Файлы: `src/BlockChain/DbCheckpoint.h`, `src/BlockChain/ChainState.*`, `src/BlockChain/BlockChainImpl.cpp`, `include/BlockChain/DbRecovery.h`
- Ссылка на Rust: n/a (дизайн краш-согласованности; `DB_CONSISTENCY.md` §4)
- Было: `header_mmr.bin`, `candidate.chain` и `confirmed.chain` коммитились независимо; падение между коммитами оставляло разные высоты, а загрузка доверяла сырым счётчикам.
- Стало: после всех четырёх коммитов `ChainState::Commit` пишет `chain.checkpoint` (52 байта: magic/version/sequence/`mmr_leaves`/candidate/confirmed/FNV-1a) через temp+rename; `BlockChain::Create` чинит каждое хранилище до `min(actual, checkpoint)` и пишет свежий checkpoint; четыре файла копируются в `NODE/recovery-*` перед починкой, `db_repaired` отдаётся в `/v1/status`.
- Доказательство: E43 (hard kill, рестарт seq 26→27; живая починка рассинхронизации), E44 (recovery/`db_repaired`).
- Риск, откат: слишком консервативный checkpoint отбрасывает незакоммиченные заголовки; recovery-копии сохраняются; откат — удалить `chain.checkpoint`, вернуться к эвристике по счётчикам.

### G-50 — Повторное DNS-обнаружение пиров
- Статус: DONE
- Коммиты: `b37caa0`
- Файлы: `src/P2P/Seed/Seeder.cpp`
- Ссылка на Rust: `p2p/src/peers.rs` (seeding)
- Было: DNS опрашивался один раз (`m_usedDNS`); при устаревших seed-адресах нода навсегда оставалась без пиров.
- Стало: пока нет кандидатов, DNS опрашивается повторно раз в пять минут; количество полученных пиров логируется и добавляется, с существующим backoff и логом причины.
- Доказательство: живой прогон (два DNS-опроса, адреса добавлены; доступность зависит от сетевого пути).
- Риск, откат: повторные DNS-запросы; пятиминутный throttle ограничивает; откат возвращает однократный опрос.

### G-27 — PIHD только детекция (без протокола)
- Статус: N/A для grin v5.5.2 (бит детектируется, не рекламируется)
- Коммиты: `a5c88bf`, `f73c7e8`, `348d87e`
- Файлы: `include/P2P/Capabilities.h`, `tests/src/Net/Test_P2PCompat.cpp`
- Ссылка на Rust: `p2p/src/msg.rs` (v5.5.2 заканчивается на `KernelSegment = 28`)
- Было: PIHD считался реализуемым режимом header sync, хотя в эталоне нет PIHD-сообщений.
- Стало: бит `PIHD_HIST` (`0x80`) детектируется, но не рекламируется, PIHD никогда не согласуется; header sync всегда legacy; тест закрепляет поведение.
- Доказательство: E33, `Test_P2PCompat`.
- Риск, откат: для совместимости риска нет (эталон не умеет PIHD); оставить detection-only.

### G-16 — Лимит размера архива TxHashSet
- Статус: DONE
- Коммиты: `1c4d020`
- Файлы: `include/P2P/Pipeline/TxHashSetLimits.h`, `src/P2P/Pipeline/TxHashSetPipe.cpp`, `tests/src/Net/Test_TxHashSetLimits.cpp`
- Ссылка на Rust: n/a (защитная граница; эталон сам валидирует архив)
- Было: `zipped_size` из `TxHashSetArchiveMessage` напрямую использовался для создания/записи файла архива; нулевой или произвольный размер мог заполнить диск.
- Стало: `IsPlausibleSize` требует `0 < size ≤ 8 GiB`; pipeline отвергает неправдоподобный архив до создания файла, ставит `TXHASHSET_SYNC_FAILED` и банит `BadTxHashSet`.
- Доказательство: E44, unit `TxHashSetLimits`.
- Риск, откат: легитимный архив >8 GiB был бы отвергнут (архивы mainnet заметно меньше); константа в одном заголовке; откат убирает проверку.

### G-22 — Бэкап БД перед миграцией
- Статус: DONE
- Коммиты: `9c6bb34`
- Файлы: `include/Database/DbBackup.h`, `src/BlockChain/BlockChainImpl.cpp`, `tests/src/Database/Test_DbBackup.cpp`
- Ссылка на Rust: n/a (операционная безопасность)
- Было: миграции v1→v2 и v2→v3 переписывали БД на месте без предыдущей копии.
- Стало: `DbBackup` рекурсивно копирует открытый каталог RocksDB потоковыми копиями (без `LOCK`) в `DB.backup-v<version>-<timestamp>` перед миграцией; неудачная копия прерывает миграцию; свежая БД (version 0) не копируется.
- Доказательство: E47, `Test_DbBackup`.
- Риск, откат: дополнительные диск/время при миграциях; копируются только реальные миграции; откат убирает backup и вызов.

### G-23 — Структурная валидация header MMR при загрузке
- Статус: PARTIAL (структурная проверка DONE; хеш/корневая валидация открыта)
- Коммиты: `1c4d020`, `0331ea1`
- Файлы: `include/PMMR/Common/MmrStructure.h`, `include/PMMR/HeaderMMR.h`, `src/PMMR/HeaderMMRImpl.h`, `src/BlockChain/BlockChainImpl.cpp`, `tests/src/PMMR/Test_MmrStructure.cpp`
- Ссылка на Rust: n/a (инвариант MMR `N = 2L − popcount(L)`; см. `DB_CONSISTENCY.md`)
- Было: сырое число узлов в `header_mmr.bin` считалось доверенным; усечённый append-only файл мог загрузиться как валидный MMR.
- Стало: `IsValidNodeCount` проверяет `N = 2L − popcount(L)`, `LargestValidNodeCount`/`LeavesForNodes` выводят безопасную цель; при загрузке невозможный размер усекается до наибольшего валидного, с guard, запрещающим усечение, если число листьев не выводится; регрессионный тест покрывает 3.3M листьев.
- Доказательство: E44 (первая версия имела неверную границу бинарного поиска и ложно усекла здоровый MMR; исправлено, добавлены guard и регрессионный тест).
- Риск, откат: ложный вердикт «невалидно» может усечь данные (тот самый инцидент); guard, регрессионный тест и recovery-копии защищают; откат отключает структурную проверку, оставляя счётчики/checkpoint.

### G-52 — Диагностика исторического TxHashSet-снапшота (OPEN)
- Статус: OPEN (только диагностика; функционального исправления нет)
- Коммиты: `7973d46`
- Файлы: `src/PMMR/TxHashSetManager.cpp`, `src/PMMR/TxHashSetImpl.cpp`, `src/P2P/Pipeline/TxHashSetPipe.cpp`
- Ссылка на Rust: `chain/src/txhashset/txhashset.rs` (архивный снапшот)
- Было: ошибка исторического снапшота проглатывалась (`catch (...) { return; }`), rewind сообщал только о commitment; причина была невидима.
- Стало: отправитель логирует высоты flushed/requested, путь и размер снапшота; rewind логирует высоту блока, число входов и число spent-позиций; функциональная лазейка остаётся открытой.
- Доказательство: E45/E46 (`flushed 3317111 → requested 3314160`, блок `3316349`, 1 вход, 0 spent-позиций).
- Риск, откат: только логирование; откат убирает строки лога.
## Известные открытые пункты (не карточки)

- **G-52 (OPEN)**: исторический `TxHashSet` snapshot/serving падает на разрыве spent-позиций (`Rewind: Spent output not found … block 3 316 349`, ноль записей при одном входе). Legacy serving и G-41 остаются NOT VERIFIED; варианты — в `TEST_RESULTS.md` E46 и `REMAINING.md`.
- **G-41 (OPEN)**: живой legacy-`txhashset.zip` fallback не завершён (пиры троттлят handshake; серверная сторона заблокирована G-52). Клиент теперь запрашивает архив на округлённой высоте 720.
- **Живой PIBD serving** Rust-клиенту: только unit-тесты.
- **G-23 хеш-уровень** ревалидации MMR; **G-21** атомарная legacy-активация; **G-20** оставшиеся лимиты парсеров.
- **G-04** PoW edge-bits, **G-07** NRD, **G-49** ускорение валидации rangeproofs, дифференциальные/fuzz-наборы, mainnet (вне этого аудита).
- **PIHD**: N/A для grin v5.5.2; detection-only by design.

## Исключено из дельты

- G-44 (спящий каркас PIHD), G-45 (mainnet dry-run), G-49 (производительность rangeproofs), G-51 (этот пакет): запланированы, изменений кода пока нет.
- Чисто документальные коммиты (`docs: …`): перечислены в карте доказательств, не как карточки.
