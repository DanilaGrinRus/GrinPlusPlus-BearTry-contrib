# AUDIT_DELTA (RU) — Grin++ 1.2.9 → совместимость с Grin v5.5.2

Статус: **черновик-скелет**, создан 31.10.2026. Пока не финальный артефакт аудита.
Базовая точка: `5f511e6` (снапшот upstream Grin++ 1.2.9). Диапазон: `5f511e6..HEAD`.

Охват: все коммиты, менявшие код. Коммиты «только документация» карточками не
считаются — они перечислены в карте доказательств. `Было`/`Стало`/`Риск` намеренно
оставлены как `TODO`: их надо заполнить по реальному diff до передачи.

## Как заполнять карточку

1. Заменить каждый `TODO` реальным содержимым (не пересказом: вставлять фрагменты
   diff с файлом и номерами строк).
2. `Ссылка на Rust` — точный `file:line` в grin v5.5.2.
3. `Доказательство` — имя теста, номер `E` или живой артефакт; иначе `NOT VERIFIED`.
4. `Риск, откат` — что может сломаться и как вернуть.

Правила: канон — английская версия; немецкая (`AUDIT_DELTA.de.md`) и русская —
переводы и обязаны сохранять те же ID карточек. Комментарии в коде — только English.

## Карточки

### G-24 — Воспроизводимая сборка (offline-sysroot)
- Статус: DONE
- Коммиты: 89eb22c, d034a2f
- Файлы: `tools/deps/build_sysroot.py`, `tools/build/build.ps1`, `CMakeLists.txt`, `src/CMakeLists.txt`
- Ссылка на Rust: n/a (сборочная инфраструктура)
- Было: TODO
- Стало: TODO
- Доказательство: `BUILD.md`, E6–E9 (чистая сборка, `/W4 /WX`)
- Риск, откат: TODO

### G-19 — Безопасная десериализация (проверки границ)
- Статус: DONE
- Коммиты: fc9e988
- Файлы: `include/Core/Serialization/ByteBuffer.h`, `tests/src/Core/Serialization/Test_ByteBuffer.cpp`
- Ссылка на Rust: `core/src/ser.rs` (проверки границ в reader)
- Было: TODO
- Стало: TODO
- Доказательство: E2 (воспроизведено до/после), `Test_ByteBuffer`
- Риск, откат: TODO

### G-08 / G-10 / G-11 — Таблица сообщений, capabilities, неизвестные типы
- Статус: DONE
- Коммиты: a5c88bf
- Файлы: `include/P2P/Capabilities.h`, `src/P2P/Messages/MessageTypes.h`, `src/P2P/Messages/MessageHeader.h`, `tests/src/Net/Test_P2PCompat.cpp`
- Ссылка на Rust: `p2p/src/msg.rs:67-97`, `p2p/src/types.rs:381-399`
- Было: TODO
- Стало: TODO
- Доказательство: E3, `Test_P2PCompat`
- Риск, откат: TODO

### G-12 / G-14 / G-15 — Genesis в handshake, отправка/приём архива
- Статус: DONE
- Коммиты: e5287f2
- Файлы: `src/P2P/Seed/HandShake.cpp`, `src/P2P/MessageProcessor.cpp`, `src/P2P/Pipeline/TxHashSetPipe.cpp`
- Ссылка на Rust: `p2p/src/handshake.rs`, `p2p/src/msg.rs:735-789`, `p2p/src/protocol.rs:254-265`
- Было: TODO
- Стало: TODO
- Доказательство: E4/E5 (по коду), живая проверка — `NOT VERIFIED`
- Риск, откат: TODO

### G-25 — Версия 1.2.9
- Статус: DONE
- Коммиты: 348d87e
- Файлы: `include/GrinVersion.h`, `include/P2P/Common.h`
- Ссылка на Rust: n/a
- Было: TODO
- Стало: TODO
- Доказательство: тест `GrinVersion::MatchesRelease`, `/v1/status` показывает `Grin++ 1.2.9`
- Риск, откат: TODO

### G-29 — Accept-loop выживает после отклонённого пира
- Статус: DONE
- Коммиты: bfc31a0
- Файлы: `src/P2P/Seed/Seeder.cpp`
- Ссылка на Rust: `p2p/src/serv.rs` (цикл accept)
- Было: TODO
- Стало: TODO
- Доказательство: E29 (`0.0.0.0:13414 LISTENING`), живой handshake
- Риск, откат: TODO

### G-30 — Реклама capabilities в handshake + настраиваемый P2P-порт
- Статус: DONE
- Коммиты: 72c94f2
- Файлы: `src/P2P/Seed/HandShake.cpp`, `include/Core/Config.h`, `src/Core/Config.cpp`, `src/Core/Config/ConfigProps.h`, `src/Core/Config/P2PConfig.h`
- Ссылка на Rust: `p2p/src/msg.rs` (поля Hand/Shake)
- Было: TODO
- Стало: TODO
- Доказательство: E30 (Rust-пир видит `PIBD_HIST | PIBD_HIST_1`)
- Риск, откат: TODO

### G-09a — Проводной слой PIBD, верификация сегментов, генератор
- Статус: DONE
- Коммиты: 62a146c, 8e2f493, de3f1a5, c390ee9
- Файлы: `src/PMMR/Models/Segment*.h`, `src/PMMR/Models/Bitmap*.h`, `src/P2P/Messages/Segment*Message.h`, `tests/src/PMMR/Test_Segment.cpp`, `Test_SegmentGenerator.cpp`
- Ссылка на Rust: `core/src/core/pmmr/segment.rs`, `chain/src/txhashset/*`
- Было: TODO
- Стало: TODO
- Доказательство: unit-наборы, E26 (модель корней)
- Риск, откат: TODO

### G-09b — Сборка битмапа, оркестрация загрузки, сессия/планировщик, снапшот
- Статус: DONE
- Коммиты: 621efad, ec006a9, 21df0b9, df81cc3, e167e8a
- Файлы: `src/P2P/PIBD/BitmapSegmentAssembler.h`, `PibdDownload.h`, `PibdSession.h`, `PibdScheduler.h`, `PibdSnapshot.h`, `PibdTreeAssembler.h`, `PibdTreeRebuilder.h`
- Ссылка на Rust: `chain/src/txhashset/desegmenter.rs`, `segmenter.rs`, `p2p/src/protocol.rs:294+`
- Было: TODO
- Стало: TODO
- Доказательство: unit-тесты по модулям (E24/E25/E26)
- Риск, откат: TODO

### G-09c — Запись staging-PMMR и атомарная активация
- Статус: DONE
- Коммиты: b681c2e, fcea044
- Файлы: `src/P2P/PIBD/PibdStateWriter.h`, `src/P2P/PIBD/PibdActivation.h`
- Ссылка на Rust: `chain/src/txhashset/txhashset.rs` (снапшот), семантика активации
- Было: TODO
- Стало: TODO
- Доказательство: `Test_PibdStateWriter`, `Test_PibdActivation` (сбой в обе стороны)
- Риск, откат: TODO

### G-09d — Выдача PIBD-сегментов из PMMR ноды (включая прореженные)
- Статус: DONE
- Коммиты: 49942ab, 30012d6, 4f9a569, 177c97a
- Файлы: `src/P2P/PIBD/PibdSegmentService.h`, `src/P2P/MessageProcessor.cpp`, `include/BlockChain/BlockChain.h`, `src/BlockChain/BlockChainImpl.cpp`
- Ссылка на Rust: `chain/src/txhashset/segmenter.rs`, `p2p/src/protocol.rs` (21/23/25/27)
- Было: TODO
- Стало: TODO
- Доказательство: `Test_PibdServing` (прореженный битмап), живой serving — `NOT VERIFIED`
- Риск, откат: TODO

### G-09e — Проводка в ноду: StateSyncer, P2P-сервер, обработка TxHashSet
- Статус: DONE
- Коммиты: 7ed7ffb, edd6151, 921ca42
- Файлы: `src/P2P/Sync/StateSyncer.cpp`, `src/P2P/P2PServerImpl.cpp`, `src/BlockChain/Processors/TxHashSetProcessor.cpp`, `src/BlockChain/BlockChainImpl.cpp`
- Ссылка на Rust: `chain/src/chain.rs` (применение txhashset), `p2p/src/protocol.rs`
- Было: TODO
- Стало: TODO
- Доказательство: E32–E34 (живая активация, валидация, `FULLY_SYNCED`)
- Риск, откат: TODO

### G-31 — Детерминированный archive-горизонт
- Статус: DONE
- Коммиты: 39b77a9, 9027ebf
- Файлы: `src/P2P/Sync/StateSyncer.cpp`, `include/Consensus.h`
- Ссылка на Rust: `chain/src/chain.rs:985` (`txhashset_archive_header`)
- Было: TODO
- Стало: TODO
- Доказательство: E31 (живой сегмент сходится с `header.output_root`), E42
- Риск, откат: TODO

### G-32 / G-35 — Размер блока битмапа (64 чанка) и порядок бит MSB-first
- Статус: DONE
- Коммиты: 39b77a9, c6ab7ff
- Файлы: `src/PMMR/Models/BitmapBlock.h`, `src/PMMR/Models/BitmapChunk.h`, `src/P2P/PIBD/BitmapSegmentAssembler.h`, `tests/src/PMMR/Test_Segment.cpp`
- Ссылка на Rust: `chain/src/txhashset/bitmap_accumulator.rs:470,213`
- Было: TODO
- Стало: TODO
- Доказательство: живой bitmap-сегмент проходит, оракул 7/7 корней (G-35), E32
- Риск, откат: TODO

### G-09f — Журнал resume для PIBD
- Статус: DONE (unit), живой replay PASS
- Коммиты: f73c7e8
- Файлы: `src/P2P/PIBD/PibdResume.h`, `src/P2P/PIBD/PibdSync.h`
- Ссылка на Rust: `chain/src/txhashset/desegmenter.rs` (позиция возобновления)
- Было: TODO
- Стало: TODO
- Доказательство: E41/E42 (replay журнала, нет повторных bitmap-запросов)
- Риск, откат: TODO

### G-43 — Ротация пиров, backoff, порог скорости, сохранение журнала
- Статус: DONE
- Коммиты: 47b6bf1, 5de693d, a5a976a, b377220
- Файлы: `src/P2P/PIBD/PibdScheduler.h`, `src/P2P/Sync/StateSyncer.cpp`
- Ссылка на Rust: `p2p/src/peers.rs` (выбор пира), таймеры `pibd.rs`
- Было: TODO
- Стало: TODO
- Доказательство: E40 (регрессия NULL-пира исправлена), E41, E42 (ротация 204→66, полный цикл)
- Риск, откат: TODO

### G-38 — Политика block sync: никаких банов за таймаут
- Статус: DONE
- Коммиты: 4dbae1b, c06888b
- Файлы: `src/P2P/Sync/BlockSyncPolicy.h`, `src/P2P/Sync/BlockSyncer.cpp`, `src/P2P/Sync/HeaderSyncer.cpp`, `src/P2P/Seed/Seeder.cpp`, `include/P2P/SyncStatus.h`, `src/P2P/MessageProcessor.cpp`
- Ссылка на Rust: `servers/src/grin/sync/body_sync.rs:149,191-204`, `header_sync.rs:135-141`
- Было: TODO
- Стало: TODO
- Доказательство: E37 (окно 32 мин), E42 (4 222 переназначения, 0 fraud-банов)
- Риск, откат: TODO

### G-42 — Ротация legacy-пира вместо бана
- Статус: DONE (код), живая ротация PASS
- Коммиты: bbf9a56
- Файлы: `src/P2P/Sync/StateSyncer.cpp`
- Ссылка на Rust: `servers/src/grin/sync/syncer.rs` / `state_sync.rs`
- Было: TODO
- Стало: TODO
- Доказательство: E41/E42 (`Rotating txhashset sync peer`)
- Риск, откат: TODO

### G-46 / G-47 — Починка согласованности chain-хранилищ и NULL-guard в Ping
- Статус: DONE (mitigation)
- Коммиты: 0331ea1
- Файлы: `src/BlockChain/ChainConsistency.h`, `src/BlockChain/BlockChainImpl.cpp`, `src/P2P/MessageProcessor.cpp`, `tests/src/BlockChain/Test_ChainConsistency.cpp`
- Ссылка на Rust: n/a (инварианты при загрузке)
- Было: TODO
- Стало: TODO
- Доказательство: E39 (unit + живая инъекция в обе стороны)
- Риск, откат: TODO

### G-48 — Атомарный чекпоинт chain-хранилищ
- Статус: OPEN (реализация в работе; `DbCheckpoint` не закоммичен на момент черновика)
- Коммиты: TODO (заполнить после слияния)
- Файлы: `src/BlockChain/DbCheckpoint.h`, `src/BlockChain/ChainState.*`, `src/BlockChain/BlockChainImpl.cpp`
- Ссылка на Rust: n/a (проектирование crash-consistency; см. `DB_CONSISTENCY.md` §7)
- Было: TODO
- Стало: TODO
- Доказательство: ожидают crash-инъекции
- Риск, откат: TODO

### G-50 — Повторное DNS-обнаружение пиров
- Статус: DONE (код)
- Коммиты: b37caa0
- Файлы: `src/P2P/Seed/Seeder.cpp`
- Ссылка на Rust: `p2p/src/peers.rs` (seeding)
- Было: TODO
- Стало: TODO
- Доказательство: живой прогон, два DNS-опроса, адреса добавлены
- Риск, откат: TODO

### G-27 — PIHD: только детекция (без протокола)
- Статус: N/A для v5.5.2 (бит детектируется, не рекламируется)
- Коммиты: a5c88bf (бит), f73c7e8, 348d87e
- Файлы: `include/P2P/Capabilities.h`, `tests/src/Net/Test_P2PCompat.cpp`
- Ссылка на Rust: `p2p/src/msg.rs` (в v5.5.2 заканчивается на `KernelSegment = 28`)
- Было: TODO
- Стало: TODO
- Доказательство: E33, `Test_P2PCompat`
- Риск, откат: TODO

## Что исключено из дельты

- G-44 (спящий каркас PIHD), G-45 (mainnet dry-run), G-49 (производительность
  rangeproofs), G-51 (этот пакет): запланированы, кода пока нет.
- Коммиты «только документация» (`docs: …`): перечислены в карте доказательств,
  карточками не считаются.
