# AUDIT_DELTA (RU) — Grin++ 1.2.9 → совместимость с Grin v5.5.2
**Актуальный срез 02.11.2026.** База 1.2.9 `5f511e6`; рабочее дерево HEAD `508d2e1`. Метрики исходников:

- добавлено C++: **+17 093 строки**; удалено **−594**; 187 файлов;
- объём diff/новых C++ файлов: **≈ +721.7 KiB**;
- итоговый `GrinNode.exe`: **7 744 512 B = 7 563.0 KiB = 7.38 MiB**;
- **прирост exe относительно 1.2.9: NOT MEASURED** — исходный бинарник отсутствует; 7.7 MB это итоговый размер, не дельта;
- артефакты: `GrinNode A3E35660…`, `Tests EC80635F…`; suite **203 / 3676 PASS**.
- **G-09 stage 1:** identity-checked P2P + Rust-прогон на H=3 315 600 — PASS (7/7, 1673/1673, 1673/1673, 1637/1637); roots взяты из C++ REST, независимое подтверждение H и обычная Rust-нода — NOT VERIFIED.
- **PIBD/PIHD guard:** height-cap 13, overflow-check `idx*capacity`, привязка PIHD-ответа к connection/peer, fallback при отсутствии роста chain height; PIBD-ответ строится по одному `ApiStateSnapshot` с retry при смене sequence, bitmap/root кешируются по output root, лимит 500 запросов/мин на connection. PIHD experimental, `PIHD_HIST` не рекламируется; конкурентный apply/reorg-тест snapshot ещё открыт.

Базовая точка: Grin++ 1.2.9 (`5f511e6`). Дельта обновлена: 02.11.2026, HEAD `508d2e1` (G-09/G-23/G-53/G-07/G-24).
Сопутствующие документы: `AUDIT_PACKAGE.md` (правила пакета), `GAP_MATRIX.md`
(статус каждой лазейки), `TEST_RESULTS.md` (доказательства E1–E84),
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
- Дифф: `tools/build/build.ps1`, коммит `89eb22c` — offline sysroot/`/W4 /WX` entry point.
- Доказательство: `BUILD.md`, E6–E9 (чистая сборка, предупреждения не отключались), SHA-256 бинарей в `BUILD.md`.
- Риск, откат: риск — устаревший/неполный sysroot; откат — вернуть скрипты/CMake и собирать с системными зависимостями.

### G-19 — Безопасная десериализация (проверки границ)
- Статус: DONE
- Коммиты: `fc9e988`
- Файлы: `include/Core/Serialization/ByteBuffer.h`, `tests/src/Core/Serialization/Test_ByteBuffer.cpp`
- Ссылка на Rust: `core/src/ser.rs` (проверки границ чтения)
- Было: чтение varint/vector/string могло выйти за пределы payload; арифметика размеров не проверялась против остатка буфера.
- Стало: каждое чтение проверяется по концу буфера (с учётом версии протокола); усечённые и чрезмерные входы аккуратно отвергаются; unit-тесты покрывают оба случая.
- Дифф: `include/Core/Serialization/ByteBuffer.h`, коммит `fc9e988`
  ```diff
  --- a/include/Core/Serialization/ByteBuffer.h
  +++ b/include/Core/Serialization/ByteBuffer.h
  -        if (m_index + sizeof(T) > m_bytes.size()) {
  -            throw DESERIALIZATION_EXCEPTION("Attempted to read past end of ByteBuffer.");
  -        }
  +        RequireRemaining(sizeof(T));
  ```
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
- Дифф: `src/P2P/Seed/HandShake.cpp`, коммит `72c94f2`
  ```diff
  --- a/src/P2P/Seed/HandShake.cpp
  +++ b/src/P2P/Seed/HandShake.cpp
  -        Capabilities::FAST_SYNC_NODE,
  +        Capabilities::ADVERTISED,
  -        Capabilities::FAST_SYNC_NODE,
  +        Capabilities::ADVERTISED,
  ```
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
- Дифф: `src/P2P/PIBD/PibdActivation.h`, коммит `fcea044`
  ```diff
  --- /dev/null
  +++ b/src/P2P/PIBD/PibdActivation.h
  +// Atomic activation of a downloaded PIBD state.
  +//
  +// The writer produces a complete state in a staging directory. Activation swaps
  +// it in with two renames and a marker file:
  +//
  +//   1. write <live parent>/pibd_activation.pending with (staging, backup) paths
  +//   2. rename live -> backup            (if a live state exists)
  +//   3. rename staging -> live
  ```
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
- Дифф: `src/P2P/Sync/StateSyncer.cpp`, коммит `39b77a9`
  ```diff
  --- a/src/P2P/Sync/StateSyncer.cpp
  +++ b/src/P2P/Sync/StateSyncer.cpp
  +        // archive_height = (header_head - STATE_SYNC_THRESHOLD) rounded down to a
  +        // multiple of TXHASHSET_ARCHIVE_INTERVAL (grin txhashset_archive_header_header_only).
  +        uint64_t archiveHeight = pTip->GetHeight();
  +        archiveHeight = archiveHeight > Consensus::STATE_SYNC_THRESHOLD
  +            ? archiveHeight - Consensus::STATE_SYNC_THRESHOLD
  +            : 0;
  +        archiveHeight -= archiveHeight % Consensus::TXHASHSET_ARCHIVE_INTERVAL;
  ```
- Доказательство: E31 (живой сегмент сходится с `header.output_root`), E42.
- Риск, откат: off-by-one в интервале ломает валидацию сегментов; оракул/живые тесты это закрепляют; откат возвращает запрос по tip (известно неверный).

### G-32 / G-35 — Размер блока битмапа (64 чанка) и порядок бит MSB-first
- Статус: DONE
- Коммиты: `39b77a9`, `c6ab7ff`
- Файлы: `src/PMMR/Models/BitmapBlock.h`, `src/PMMR/Models/BitmapChunk.h`, `src/P2P/PIBD/BitmapSegmentAssembler.h`, `tests/src/PMMR/Test_Segment.cpp`
- Ссылка на Rust: `chain/src/txhashset/bitmap_accumulator.rs:470,213`
- Было: `BitmapBlock` содержал 16 чанков и индексировал биты LSB-first; sparse (positive/negative) сегменты читались неверно, реальные сегменты не десериализовались.
- Стало: 2^16 бит = 64 чанка × 1024; бит `n` лежит в байте `n/8`, бит `7−(n%8)` (MSB-first, как `bitvec::to_bytes`); raw-чанки пишут 1024 байта без префикса длины.
- Дифф: `src/PMMR/Models/BitmapChunk.h`, коммит `c6ab7ff`
  ```diff
  --- a/src/PMMR/Models/BitmapChunk.h
  +++ b/src/PMMR/Models/BitmapChunk.h
  -        return (m_bytes[idx / 8] & (uint8_t)(1u << (idx % 8))) != 0;
  +        return (m_bytes[idx / 8] & (uint8_t)(1u << (7 - (idx % 8)))) != 0;
  -        const uint8_t mask = (uint8_t)(1u << (idx % 8));
  +        const uint8_t mask = (uint8_t)(1u << (7 - (idx % 8)));
  ```
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
- Коммиты: `4dbae1b`, `c06888b`, `7997ca5`
- Файлы: `src/P2P/Sync/BlockSyncPolicy.h`, `src/P2P/Sync/BlockSyncer.cpp`, `src/P2P/Sync/HeaderSyncer.cpp`, `src/P2P/Seed/Seeder.cpp`, `include/P2P/SyncStatus.h`, `src/P2P/MessageProcessor.cpp`, `tests/src/Net/Test_BlockSyncPolicy.cpp`
- Ссылка на Rust: `servers/src/grin/sync/body_sync.rs:149,191-204`, `header_sync.rs:135-141`
- Было: `BlockSyncer` банил пира за `FraudHeight` после двух таймаутов, `m_slowPeers` не истекал, окно запросов игнорировало высоты пиров, seeder/syncer молчали после банов.
- Стало: таймаут считается и блок переназначается; запросы ограничены `min(candidate tip, network height, per-connection height)` и batch ≤100; retry 6/1 с + backoff; медленный пир получает cooldown 60 с; `FraudHeight` теперь точно как Rust: строго больший **total difficulty** + 120 с без прогресса заголовков (прежнее условие «высота ИЛИ TD» дало живьём три ложных бана; исправлено, E48); seeder логирует причину; `/v1/status` отдаёт waiting reason и счётчики.
- Дифф: `src/P2P/Sync/BlockSyncPolicy.h`, коммит `4dbae1b`
  ```diff
  --- /dev/null
  +++ b/src/P2P/Sync/BlockSyncPolicy.h
  +// Behaviour mirrored from grin v5.5.2 body_sync.rs / header_sync.rs:
  +//   * a timeout is never fraud: reassign the block and count it;
  +//   * FraudHeight requires a peer that claims more work than we have and has
  +//     made no progress for a long time;
  ```
- Доказательство: E37 (32-мин живое окно), E42 (4 222 переназначения, 0 fraud-банов), E48 (TD-only после живых ложных банов).
- Риск, откат: без банов неотвечающий пир может замедлить догон; cooldown/ротация ограничивают; откат возвращает прежний путь бана.

### G-42 — Ротация пира legacy state sync вместо бана
- Статус: DONE
- Коммиты: `bbf9a56`
- Файлы: `src/P2P/Sync/StateSyncer.cpp`
- Ссылка на Rust: `servers/src/grin/sync/syncer.rs`, `state_sync.rs`
- Было: `RequestState` банил пира `BadTxHashSet`, если zip не давал прогресса 10 с или истекали 120 минут; нода могла остаться без пиров.
- Стало: медленный пир ротируется (`Rotating txhashset sync peer`); действительно невалидный архив по-прежнему банится пайплайном после ошибки валидации.
- Дифф: `src/P2P/Sync/StateSyncer.cpp`, коммит `bbf9a56`
  ```diff
  -        LOG_WARNING_F("Banning peer: {}", m_pPeer);
  -        m_pPeer->Ban(EBanReason::BadTxHashSet);
  +        LOG_INFO_F("Rotating txhashset sync peer: {}", m_pPeer);
  ```
- Доказательство: E41/E42.
- Риск, откат: сломанный пир может быть повторён; ротация логируется; откат возвращает бан.

### G-46 / G-47 — Починка chain-хранилищ и NULL-guard Ping
- Статус: DONE (mitigation при загрузке)
- Коммиты: `0331ea1`
- Файлы: `src/BlockChain/ChainConsistency.h`, `src/BlockChain/BlockChainImpl.cpp`, `src/P2P/MessageProcessor.cpp`, `tests/src/BlockChain/Test_ChainConsistency.cpp`
- Ссылка на Rust: n/a (инварианты загрузки)
- Было: после грязного завершения header MMR и candidate/confirmed chain могли разойтись; загрузка читала за пределы MMR (0xC0000005). Обработчик Ping разыменовывал отсутствующий confirmed tip.
- Стало: `ComputeChainRepair` сравнивает количество листьев/записей и усекает хранилища до согласованной цели до открытия `ChainState`; Ping проверяет tip и отвечает либо пишет WARNING вместо падения.
- Дифф: `src/BlockChain/ChainConsistency.h`, коммит `0331ea1` — load-time counts repair and Ping null guard.
- Доказательство: E39 (unit + живая инъекция в обе стороны).
- Риск, откат: починка может отрезать незакоммиченные заголовки; recovery-копии и checkpoint ограничивают потерю; откат — восстановить `NODE/recovery-*`.

### G-48 — Атомарный checkpoint chain-хранилищ
- Статус: DONE (checkpoint; хеш-валидация — G-23)
- Коммиты: `fdef6e6`, `1c4d020`, `7997ca5`
- Файлы: `src/BlockChain/DbCheckpoint.h`, `src/BlockChain/ChainState.*`, `src/BlockChain/BlockChainImpl.cpp`, `include/BlockChain/DbRecovery.h`
- Ссылка на Rust: n/a (дизайн краш-согласованности; `DB_CONSISTENCY.md` §4)
- Было: `header_mmr.bin`, `candidate.chain` и `confirmed.chain` коммитились независимо; падение между коммитами оставляло разные высоты, а загрузка доверяла сырым счётчикам.
- Стало: `ChainState::Commit` пишет `chain.checkpoint` (52 байта, FNV-1a) через temp+rename после всех коммитов; при загрузке проверяются инварианты, отстающий MMR перестраивается из block DB, удаляются только лишние листья, восстанавливается хвост confirmed, требуется проверенная по размеру recovery-копия, а при цепочке позади checkpoint старт прерывается; `db_repaired`/`db_last_repair` отдаются в `/v1/status`. Живьём: hard kill seq 26→27; MMR −2000 листьев перестроен, header сохранён (E43/E44/E48).
- Дифф: `src/BlockChain/DbCheckpoint.h`, коммит `fdef6e6`
  ```diff
  --- /dev/null
  +++ b/src/BlockChain/DbCheckpoint.h
  +struct DbCheckpointData
  +{
  +    uint64_t Sequence = 0;
  +    uint64_t MmrLeaves = 0;
  +    uint64_t CandidateEntries = 0;
  +    uint64_t ConfirmedEntries = 0;
  +};
  +// "GRINPPCK" magic, version, FNV-1a checksum; written via temp + rename
  ```
- Доказательство: E43, E44, E48.
- Риск, откат: слишком консервативный checkpoint отбрасывает незакоммиченные заголовки; recovery-копии сохраняются; откат — удалить `chain.checkpoint`, вернуться к эвристике по счётчикам.

### G-50 — Повторное DNS-обнаружение пиров
- Статус: DONE
- Коммиты: `b37caa0`
- Файлы: `src/P2P/Seed/Seeder.cpp`
- Ссылка на Rust: `p2p/src/peers.rs` (seeding)
- Было: DNS опрашивался один раз (`m_usedDNS`); при устаревших seed-адресах нода навсегда оставалась без пиров.
- Стало: пока нет кандидатов, DNS опрашивается повторно раз в пять минут; количество полученных пиров логируется и добавляется, с существующим backoff и логом причины.
- Дифф: `src/P2P/Seed/Seeder.cpp`, коммит `b37caa0`
  ```diff
  -    } else if (!m_usedDNS.exchange(true)) {
  -        std::vector<SocketAddress> peerAddresses = DNSSeeder::GetPeersFromDNS();
  +    } else if (m_lastDnsAttempt + std::chrono::minutes(5) <= now) {
  +        m_lastDnsAttempt = now;
  +        std::vector<SocketAddress> peerAddresses = DNSSeeder::GetPeersFromDNS();
  ```
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
- Дифф: `include/P2P/Pipeline/TxHashSetLimits.h`, коммит `1c4d020` — 8 GiB archive bound before writing.
- Доказательство: E44, unit `TxHashSetLimits`.
- Риск, откат: легитимный архив >8 GiB был бы отвергнут (архивы mainnet заметно меньше); константа в одном заголовке; откат убирает проверку.

### G-04 — Проверка edge-bits PoW
- Статус: DONE
- Коммиты: `9d9f1ae`
- Файлы: `include/PoW/PoWValidator.h`, `src/PoW/PoWValidator.cpp`, `tests/src/Consensus/Test_PoWEdgeBits.cpp`
- Ссылка на Rust: `chain/src/pipe.rs` (`validate_pow_only`, `is_primary`/`is_secondary`), `core/src/pow/cuckatoo.rs` (`verify_size`)
- Было: только `edge_bits == 29` выбирал вариант Cuckoo; любое другое значение (0, 24, 30, 64, 255) попадало в `Cuckatoo::Validate`, где сдвиг 64-битного слова на `edge_bits` не определён.
- Стало: `PoWValidator::IsSupportedEdgeBits` допускает secondary 29 и primary 31–63; `IsPoWValid` отвергает остальные размеры до диспетчеризации; таблица границ покрыта unit-тестом.
- Дифф: `src/PoW/PoWValidator.cpp`, коммит `9d9f1ae`
  ```diff
  +    if (!IsSupportedEdgeBits(header.GetEdgeBits())) {
  +        LOG_WARNING_F("Unsupported PoW edge bits {} for block {}", header.GetEdgeBits(), header);
  +        return false;
  +    }
  ```
- Доказательство: E50, `Test_PoWEdgeBits` (11 проверок).
- Риск, откат: будущие primary-размеры >63 потребуют изменения консенсуса; набор задан константой; откат убирает guard.
### G-20 — Лимиты размеров в парсерах
- Статус: DONE (PIBD/segment-парсеры; fuzz вне матрицы)
- Коммиты: `9d9f1ae`
- Файлы: `src/PMMR/Models/Segment.h`, `tests/src/PMMR/Test_Segment.cpp`, `src/PMMR/Models/BitmapSegment.h`, `src/PMMR/Models/SegmentProof.h`
- Ссылка на Rust: `core/src/ser.rs` (границы), `core/src/core/pmmr/segment.rs` (`MAX_SEGMENT_READ_ITEMS`)
- Было: счётчики сегментов ограничивались `MAX_SEGMENT_READ_ITEMS`, checked arithmetic и лимитами сообщений, но вектор листьев мог зарезервировать память до лимита, ещё не проверив, что список помещается в payload.
- Стало: `numLeaves > remaining` отвергается до резервирования; лимиты `SegmentProof`/`BitmapSegment`/`BitmapBlock`, checked arithmetic и message-cap сохранены; тесты покрывают огромные counts, неубывающие позиции и слишком высокие сегменты.
- Дифф: `src/PMMR/Models/Segment.h`, коммит `9d9f1ae`
  ```diff
  +        if (numLeaves > byteBuffer.GetRemainingSize()) {
  +            throw DESERIALIZATION_EXCEPTION("Segment leaf list does not fit in the payload");
  +        }
  ```
- Доказательство: E50, `Test_Segment` (21 кейс, включая огромные counts).
- Риск, откат: некорректные payload отвергаются раньше; валидные сегменты не затронуты; откат убирает guard.
### G-21 — Атомарная активация legacy-txhashset
- Статус: DONE
- Коммиты: `2122e32`
- Файлы: `src/PMMR/Zip/TxHashSetZip.*`, `include/PMMR/TxHashSetManager.h`, `src/PMMR/TxHashSetManager.cpp`, `src/BlockChain/Processors/TxHashSetProcessor.cpp`, `include/PMMR/TxHashSetStaging.h`, `tests/src/PMMR/Test_TxHashSetStaging.cpp`
- Ссылка на Rust: `chain/src/txhashset/txhashset.rs` (установка состояния), `servers/src/grin/sync/state_sync.rs`
- Было: `TxHashSetZip::Extract` удалял и перезаписывал живой `NODE/TXHASHSET` до валидации; неудачный или прерванный legacy-sync оставлял частичное состояние.
- Стало: zip распаковывается в `TXHASHSET.staging` и валидируется там, живое состояние не трогается; при ошибке staging удаляется и живое состояние переоткрывается; при успехе — `live → backup` / `staging → live` под `txhashset_activation.pending`, а `TxHashSetStaging::Recover` при старте восстанавливает backup или удаляет устаревший staging.
- Дифф: `src/BlockChain/Processors/TxHashSetProcessor.cpp`, коммит `2122e32`
  ```diff
  -        ITxHashSetPtr pTxHashSet = TxHashSetManager::LoadFromZip(m_config, path, pHeader);
  +        const fs::path stagingPath = TxHashSetStaging::Staging(livePath);
  +        ITxHashSetPtr pStaged = TxHashSetManager::LoadFromZip(m_config, path, pHeader, stagingPath);
  ```
- Доказательство: E49, `Test_TxHashSetStaging` (3 кейса).
- Риск, откат: ошибка rename восстанавливает backup; падение между переименованиями восстанавливается по маркеру; откат убирает staging-путь.
### G-22 — Бэкап БД перед миграцией
- Статус: DONE
- Коммиты: `9c6bb34`
- Файлы: `include/Database/DbBackup.h`, `src/BlockChain/BlockChainImpl.cpp`, `tests/src/Database/Test_DbBackup.cpp`
- Ссылка на Rust: n/a (операционная безопасность)
- Было: миграции v1→v2 и v2→v3 переписывали БД на месте без предыдущей копии.
- Стало: `DbBackup` рекурсивно копирует открытый каталог RocksDB потоковыми копиями (без `LOCK`) в `DB.backup-v<version>-<timestamp>` перед миграцией; неудачная копия прерывает миграцию; свежая БД (version 0) не копируется.
- Дифф: `include/Database/DbBackup.h`, коммит `9c6bb34` — recursive stream backup before migration.
- Доказательство: E47, `Test_DbBackup`.
- Риск, откат: дополнительные диск/время при миграциях; копируются только реальные миграции; откат убирает backup и вызов.

### G-23 — Структурная валидация header MMR при загрузке
- Статус: PARTIAL (структурная проверка DONE; хеш/корневая валидация открыта)
- Коммиты: `1c4d020`, `0331ea1`, `7997ca5`
- Файлы: `include/PMMR/Common/MmrStructure.h`, `include/PMMR/HeaderMMR.h`, `src/PMMR/HeaderMMRImpl.h`, `src/BlockChain/BlockChainImpl.cpp`, `include/BlockChain/DbRecovery.h`, `tests/src/PMMR/Test_MmrStructure.cpp`
- Ссылка на Rust: n/a (инвариант MMR `N = 2L − popcount(L)`; см. `DB_CONSISTENCY.md`)
- Было: сырое число узлов в `header_mmr.bin` считалось доверенным; усечённый append-only файл мог загрузиться как валидный MMR.
- Стало: `IsValidNodeCount` проверяет `N = 2L − popcount(L)`; при загрузке невозможный размер усекается до наибольшего валидного, отстающий MMR **перестраивается из block DB** (заголовки сохраняются), удаляются только лишние листья; любая модификация требует проверенной по размеру recovery-копии; цепочка позади checkpoint прерывает старт. Живая инъекция: MMR −2000 листьев → header 3 317 117 сохранён, candidate.chain без изменений (E48).
- Дифф: `include/PMMR/Common/MmrStructure.h`, коммит `1c4d020` — `N = 2L − popcount(L)`; rebuild lagging MMR from block DB (E48).
- Доказательство: E44 (структурная регрессия/guard), E48 (rebuild без потерь; отказ при отсутствии заголовка в DB).
- Риск, откат: ложный вердикт «невалидно» может усечь данные (тот самый инцидент); guard, регрессионный тест и recovery-копии защищают; откат отключает структурную проверку, оставляя счётчики/checkpoint.

### G-52 — Диагностика исторического TxHashSet-снапшота (OPEN)
- Статус: OPEN (сохранение/выдача архива частично реализованы; живой перенос NOT VERIFIED)
- Коммиты: `7973d46`, `7997ca5`
- Файлы: `src/PMMR/TxHashSetManager.cpp`, `src/PMMR/TxHashSetImpl.cpp`, `src/P2P/Pipeline/TxHashSetPipe.cpp`, `include/P2P/TxHashSetArchive.h`, `src/P2P/Sync/StateSyncer.cpp`
- Ссылка на Rust: `chain/src/txhashset/txhashset.rs` (архивный снапшот)
- Было: ошибка исторического снапшота проглатывалась (`catch (...) { return; }`), rewind сообщал только о commitment; причина была невидима.
- Стало: при активации состояние горизонта сохраняется в `NODE/TXHASHSET/archive/TxHashSet.<height>.<hash>.zip` (`ArchivePibdState`), а `TxHashSetPipe` отдаёт этот архив по hash без исторического rewind; ошибки snapshot логируют высоты flushed/requested, блок, число входов и spent-позиций. Разрыв spent-позиций остаётся для произвольных исторических snapshot, живой перенос по-прежнему NOT VERIFIED.
- Дифф: `src/PMMR/TxHashSetManager.cpp`, коммит `7973d46`
  ```diff
  --- a/src/PMMR/TxHashSetManager.cpp
  +++ b/src/PMMR/TxHashSetManager.cpp
  +            LOG_INFO_F(
  +                "TxHashSet snapshot: flushed height {}, requested height {}.",
  +                pFlushedHeader != nullptr ? pFlushedHeader->GetHeight() : 0,
  +                pHeader->GetHeight());
  ```
- Доказательство: E45/E46 (диагноз), E48 (код сохранения архива; live pending).
- Риск, откат: только логирование; откат убирает строки лога.
## Известные открытые пункты (не карточки)

- **G-52 (OPEN)**: исторический `TxHashSet` snapshot/serving падает на разрыве spent-позиций (`Rewind: Spent output not found … block 3 316 349`, ноль записей при одном входе). Legacy serving и G-41 остаются NOT VERIFIED; варианты — в `TEST_RESULTS.md` E46 и `REMAINING.md`.
- **G-41 (OPEN)**: живой legacy-`txhashset.zip` fallback не завершён (пиры троттлят handshake; серверная сторона заблокирована G-52). Клиент теперь запрашивает архив на округлённой высоте 720.
- **Живой PIBD serving** Rust-клиенту: только unit-тесты.
- **G-23 хеш-уровень** ревалидации MMR; **G-21** атомарная legacy-активация; **G-20** оставшиеся лимиты парсеров.
- **G-04** PoW edge-bits, **G-07** NRD, **G-49** ускорение валидации rangeproofs, дифференциальные/fuzz-наборы, mainnet (вне этого аудита).
- **PIHD**: N/A для grin v5.5.2; detection-only by design.

## Исключено из дельты

- G-44 (спящий каркас PIHD), G-45 (mainnet dry-run), G-49 (производительность rangeproofs), G-51 (этот пакет): закрыт (аудит-пакет; приёмка владельца 01.11.2026).
- Чисто документальные коммиты (`docs: …`): перечислены в карте доказательств, не как карточки.
## Актуализация 02.11.2026 — HEAD `508d2e1`

Базовая точка прежняя (`5f511e6`). Новый срез включает G-09-candidate merge,
G-23 hash-ревалидацию, KernelIndex/get_kernel, NRD-правила и локальный CI-gate.
Артефакты: `GrinNode E16A014E…`, `Tests CC6472B1…`; suite **194 / 2227 PASS**.

### G-09 — PIBD serving (PARTIAL, live in progress)

- Слит кандидатный патч: `GetRawAt` проверяет compaction/границы; required leaf
  без данных → `MissingLeaf`; optional leaf → prune-aware hash; fully pruned
  сегмент обходит **всю** `familyBranch` до первого сохранённого предка.
- `PibdStateWriter`/`PibdTreeRebuilder` сохраняют hash-only boundary leaves.
- Read-path: `AppendOnlyFile`, `MMRHashUtil`, `DataFile` проверяют границы;
  `IsCompacted` учитывает leaf-set; prune-ignoring fallback удалён.
- Serving gate: только `NOT_SYNCING` и non-genesis flushed state.
- Offline sweep `pp-ckpt` H=3 317 027: output **0/1674**, rangeproof
  **0/1674**, kernel **0/1638** ошибок; total **4993/4993 PASS**.
- Output/rangeproof fixture (11,1100) — побайтно совпали с Rust-validated
  fixture; kernel-fixture был неполным, полный sweep корректен.
- Live: C++ PIBD-клиент качает state на горизонт 3 315 600; Rust-клиент будет
  запущен после активации. **Live G-09 NOT VERIFIED** — offline PASS не считать
  live-приёмкой.

### G-23 — hash/root ревалидация (PARTIAL)

- При наличии checkpoint startup проверяет hash/height/previous каждого
  заголовка и leaf hash header MMR; открытый TxHashSet обязан совпасть с
  flushed header roots.
- Live startup на цепи 3.3M: API готов через 75.4 с, mismatch нет.
- Есть тест порчи одного байта `header_mmr.bin`; fault-injection полного
  startup-abort и полный PMMR-node sweep — **NOT VERIFIED**.

### G-53 — KernelIndex/get_kernel (PARTIAL)

- Persistent generation-based `excess → OutputLocation` индекс с tip/root/count/
  checksum; `get_kernel` fast path + bounded direct-scan fallback; N-06 исправлен
  на 1-based PMMR positions.
- 3 теста индекса / 48 assertions. Live `/v2`, reorg и pagination — NOT VERIFIED.

### G-07 — NRD (PARTIAL)

- duplicate NRD в блоке и `diff < relative_height` отклоняются; cutoff 2 недели;
  Rewind/startup/PIBD rebuild; до HF3 NRD запрещён.
- Нет persisted incremental index, full block/reorg NRD-теста и live NRD peer.

### G-24 — сборка (PARTIAL)

- Добавлен `tools/build/ci.ps1` (build + isolated test + manifest) и
  `BUILD_PINS.md`; hosted CI и pin vcpkg на чистом checkout — NOT VERIFIED.

### N-находки

- Исправлены N-01 (commit threads), N-02 (flush/checkpoint replace), N-03
  (recovery branch), N-04 (async rate counter), N-05 (16 MiB write queue),
  N-06 (get_kernel positions), N-07 (get_outputs errors).
- Fault-injection для N-01/N-02/N-05 и нагрузочные тесты — NOT VERIFIED.

### Остаётся

- **G-09 live**: завершить Rust-клиент/PIBD и зафиксировать peer/H/число ответов.
- G-23 fault injection/full PMMR node sweep.
- G-53 live `/v2`/reorg/pagination.
- G-07 persisted index/full block/reorg.
- G-24 hosted CI; G-49 ускорение rangeproofs отложено.
