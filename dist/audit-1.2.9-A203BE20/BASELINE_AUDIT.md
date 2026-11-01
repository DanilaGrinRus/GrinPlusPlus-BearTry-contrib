# BASELINE_AUDIT — Grin++ 1.2.9 против эталона Grin v5.5.2

Дата: 2026-10-30. Статус: **PASS (аудит исходников)**; сборка baseline — **BLOCKED** (см. §9).
Числа прогонов ниже — на дату аудита; актуальные — в последней записи `TEST_RESULTS.md`.

Документ фиксирует фактическое состояние C++-ноды Grin++ на момент начала работ.
Все утверждения ниже проверены по коду в `Gplus/src/grinplusplus` и по исходникам
эталона `Gplus/reference/grin-5.5.2`, а не по README/релизам.

---

## 1. Провенанс и проверимость

| Артефакт | Значение |
|---|---|
| `Gplus/GrinPlusPlus-1.2.9.zip` | SHA-256 `404A935A66922370D26C11BEFF041E9073EED32214428179391FD25F16E0B115` |
| `Gplus/grin-5.5.2.zip` | SHA-256 `C60BE3BB38C66FCAD3BC9D057F8C98D8A193D8B65E44AFC71F43E4A270FD9349` |
| Рабочий корень C++ | `Gplus/src/grinplusplus` (отдельный git-репозиторий) |
| Baseline commit | `5f511e6 Import Grin++ 1.2.9 upstream source snapshot` |
| Эталон Rust | `Gplus/reference/grin-5.5.2` (тег v5.5.2, релизный коммит `dbaff34`) |

Сверка с публичным репозиторием (git, тег `v1.2.9` = `a0958a3`, добавлен remote `upstream`):

* `git diff 5f511e6 FETCH_HEAD` → **5 файлов, все — бинарники Tor в `dist/*/tor`** (LFS-объекты в архиве и в git различаются).
* Исходный код локального снапшота **побайтово совпадает** с тегом v1.2.9.
  Локальных «неизвестных» правок в исходниках на входе не было.

Размер кодовой базы: 782 файла `*.cpp/*.h`, 85 454 строки; в git-tracked — 1 045 файлов.

---

## 2. Идентичность версии и протокола

| Параметр | Grin++ 1.2.9 | Grin v5.5.2 | Совпадение |
|---|---|---|---|
| Версия ПО | `include/GrinVersion.h:5` → `1.2.8` (при теге 1.2.9) | 5.5.2 | нет (косметика UA) |
| User agent | `"Grin++ 1.2.8"` (`include/P2P/Common.h:11`) | `MW/Grin 5.5.2.<sha>` | формат иной, допустимо |
| Protocol version | `V1000` (`include/Core/Enums/ProtocolVersion.h:22`) | `PROTOCOL_VERSION = ProtocolVersion(1_000)` (`core/src/global.rs:47`) | **да** |
| Negotiation | `min(local, peer)` (`src/P2P/Seed/HandShake.cpp:48,82`) | `min(self, other)` (`p2p/src/handshake.rs:90`) | **да** |
| Network magic | 2 байта из `Global::GetMagicBytes()` | `OTHER/TESTNET/MAINNET_MAGIC` (`p2p/src/msg.rs:57-60`) | да по длине/семантике |
| Wire header | 2 magic + type(1) + len(u64) = 11 байт | `MsgHeader::LEN = 2+1+8` (`p2p/src/msg.rs:311`) | **да** |

Расхождение версии в `GrinVersion.h` — реальный дефект: UA и `VERSION` в БД будут
врать «1.2.8» для сборки 1.2.9. Кандидат на P2-фикс (не влияет на консенсус).

---

## 3. Структура кода

| Модуль | Каталог | Содержимое |
|---|---|---|
| P2P | `src/P2P`, `include/P2P` | Connection/ConnectionManager, MessageProcessor, Seed/HandShake/PeerManager, Pipeline (Block/Transaction/TxHashSet), Sync (Header/State/Block) |
| Consensus/Chain | `src/BlockChain`, `include/BlockChain` | BlockChainImpl, Chain, ChainState, ChainStore, CoinView, Validators/{Block,BlockHeader}Validator, ChainResyncer |
| PMMR | `src/PMMR`, `include/PMMR` | HeaderMMR, KernelMMR, OutputPMMR, RangeProofPMMR, TxHashSet(+Validator, +Manager), примитивы DataFile/PruneList/LeafSet/HashFile |
| Storage | `src/Database`, `include/Database` | RocksDB-обёртка (BlockDB, PeerDB, Database) |
| Crypto | `src/Crypto`, `include/Crypto` | secp256k1-zkp, Pedersen, Bulletproofs, kernel signatures, HASH |
| TxPool | `src/TxPool` | Mempool/StemPool, Dandelion++ (`src/P2P/Dandelion.cpp`) |
| API/Server | `src/API`, `src/Server` | REST API (civetweb), node/wallet endpoints |
| Wallet | `src/Wallet`, `include/Wallet` | кошелёк (slate, keys, accounts), SQLite |
| PoW | `src/PoW` | Cuckaroo/Cuckarood/Cuckaroom/Cuckarooz/Cuckatoo + DifficultyCalculator |

---

## 4. Система сборки и зависимости

* CMake `>= 3.8`, C++17 (`CMakeLists.txt:1,16`), MSVC: `/W4 /WX` (`CMakeLists.txt:28-32`),
  static CRT (`MultiThreaded`, `CMakeLists.txt:34`), без Clang (`:19-22`).
* Зависимости берутся из **vcpkg** (классические порты): `vcpkg/packages.txt` —
  libsodium, rocksdb, zlib, civetweb, minizip, roaring, asio, fmt, secp256k1-zkp, mio, catch2.
* Overlay-порты в репозитории (`vcpkg/custom_ports/`):
  secp256k1-zkp (`mimblewimble/secp256k1-zkp@5b39fbf`), rocksdb `6.6.4-1`,
  civetweb `2019-07-05-1`, roaring `2019-03-05-2`, libsodium `1.0.18-2`.
* В дереве (`deps/`): sqlite3, jsoncpp, uint128, scrypt, sha3, reproc, bitcoin,
  chachapoly, cppcodec, rfc6234, stduuid, spdlog, klib, ghc (`filesystem.h`), `Roaring.h`.
* CI (`.github/workflows/ci.yml`): windows-2019 (`VS16Win64`, triplet `x64-windows-static`),
  ubuntu-20.04 (gcc-10, Ninja), macos-12; vcpkg запинен **только в CI**:
  коммит `6185aa76504a5025f36754324abf307cc776f3da` (2020.06-эпоха).

**Дефект воспроизводимости:** пин vcpkg и версии портов не зафиксированы в самом
репозитории (нет `vcpkg.json`/`builtin-baseline`); локальная сборка «сегодня» не
воспроизводит CI 2020 года. Требует отдельного решения (см. §9 и GAP G-19).

---

## 5. P2P: фактическое состояние

Типы сообщений (`src/P2P/Messages/MessageTypes.h:9-34`) — значения 0..22.
Значения 0..20 совпадают с Grin v5.5.2 (`p2p/src/msg.rs:67-88`), **21 и 22 — нет**:

| ID | Grin++ 1.2.9 | Grin v5.5.2 |
|---|---|---|
| 21 | `GetKernels` (объявлен, нигде не используется) | `GetOutputBitmapSegment` |
| 22 | `Kernels` (объявлен, нигде не используется) | `OutputBitmapSegment` |
| 23-28 | отсутствуют | `GetOutputSegment`, `OutputSegment`, `GetRangeProofSegment`, `RangeProofSegment`, `GetKernelSegment`, `KernelSegment` |

**PIBD в локальном дереве отсутствует полностью**: нет `Segment.h`, `SegmentProof.h`,
`BitmapSegment.h`, нет `SegmentRequestMessage`/`SegmentResponseMessage`, нет обработчиков
в `MessageProcessor.cpp`. (Утверждение промта о наличии этих классов в публичном
Grin++ к снапшоту 1.2.9 не относится — проверено поиском по дереву.)

Capabilities (`include/P2P/Capabilities.h`): объявлены только `0x01`, `0x02`, `0x04`.
Отсутствуют `TX_KERNEL_HASH=0x08`, `PIBD_HIST=0x10`, `BLOCK_HIST=0x20`, `PIBD_HIST_1=0x40`
из `p2p/src/types.rs:381-399`. Рекламируется `FAST_SYNC_NODE = TXHASHET_HIST|PEER_LIST`
(`src/P2P/Seed/HandShake.cpp:94,112`).

Handshake: порядок полей Hand/Shake совпадает с Rust (`p2p/src/msg.rs:405-490`),
адреса — `u8 variant + 4/16 байт + u16 port`, длина строки — `u64` prefix
(`include/Net/IPAddress.h`, `include/Core/Serialization/Serializer.h`).
**Не проверялся** genesis-хеш пира — исправлено (см. GAP G-04).

TxHashSet archive: `TxHashSetRequest{hash, height}` и `TxHashSetArchive{hash, height, bytes}`
совпадают с Rust (`p2p/src/msg.rs:735-789`); zip передаётся потоком после тела сообщения
(`src/P2P/Pipeline/TxHashSetPipe.cpp`), что соответствует механизму attachment в Rust
(`p2p/src/msg.rs:249-286`).

Синхронизация: `HeaderSyncer` (по 512 заголовков), `StateSyncer` (TxHashSet на высоте
`headerHeight - STATE_SYNC_THRESHOLD`, 2 дня), `BlockSyncer`/`BlockPipe`.
Параметр `CUT_THROUGH_HORIZON` = 1 неделя (`include/Consensus.h`).

Обработка неизвестных типов сообщений: Grin++ **рвёт соединение** (лимит 0*4),
Rust — читает и отбрасывает до `default_max_msg_size()*4` (`p2p/src/msg.rs:366-378`).
Исправлено (GAP G-03).

---

## 6. Консенсус: что реализовано

Проверено по `src/BlockChain/Validators/*`, `src/Core/Validation/*`, `include/Consensus.h`,
`src/PoW/*`:

* height = prev+1; timestamp > prev и ≤ now+5 мин; version == `header_version(height)`;
  previous_root == локальный Header MMR root (`BlockHeaderValidator.cpp`).
* `header_version`: mainnet `min(5, 1 + height/262080)`; floonet 185040/298080/552960/642240 —
  совпадает с `core/src/consensus.rs:header_version` для mainnet и testnet-констант.
* PoW: Cuckaroo(v1)/Cuckarood(v2)/Cuckaroom(v3)/Cuckarooz(v4+)/Cuckatoo(иные edge_bits),
  total difficulty, max difficulty, точное равенство `targetDifficulty == next_difficulty`.
* Difficulty: DMA до v5, WTEMA начиная с v5 (`DifficultyCalculator.cpp:22-28`), что
  соответствует Rust (`consensus::next_difficulty`, WTEMA после HF4); secondary scaling
  для v5 = 0 — как в Rust, где scaling проверяется только при `version < 5`
  (`chain/src/pipe.rs:445`).
* Вес блока `1/21/3`, `MAX_BLOCK_WEIGHT = 40000` — совпадает (`include/Consensus.h`).
* Тело блока: сортировка, дубликаты, cut-through, rangeproofs, batch-проверка подписей
  ядер (`TxBodyValidator.cpp`), суммы ядер с overage и kernel offset
  (`KernelSumValidator.h`), coinbase-сумма, lock heights, reward+fees (`BlockValidator.cpp`).
* Coinbase maturity 1440 (`Consensus::COINBASE_MATURITY`), cut-through horizon 10080.

**Не проверено фактическими тестами:** эквивалентность корней MMR, поведения на reorg,
консенсус на исторических блоках — для этого нужна сборка (см. §9).

---

## 7. Хранилище и надёжность

* Основная БД — **RocksDB** (`src/Database/BlockDBImpl.cpp`), не LMDB.
* Индекс цепи — плоский `DataFile<32>` хешей (`src/BlockChain/Chain.cpp`), PMMR — собственные
  `DataFile`/`PruneList`/`LeafSet`/`HashFile` в `src/PMMR`.
* Версия схемы БД: `VERSION` в RocksDB, поддерживаются 2 и 3, миграция v2→v3 при открытии,
  БД новее 3 — отказ (`src/BlockChain/BlockChainImpl.cpp:60-84`).
* Атомарность: batch-записи (`OnInitWrite`/`OnEndWrite`, `Chain::Commit/Rollback`),
  но **staging-области и атомарной активации проверенного TxHashSet нет** — состояние
  пишется в рабочую БД (`ProcessTransactionHashSet`), это открытый P0-вопрос (GAP G-20).
* В коде остались TODO/ограничения: `TODO - NRD: Verify kernel variants`
  (`BlockValidator.cpp:22`), «It would also be a good idea to re-validate the MMRs on load»
  (`BlockChainImpl.cpp:442`).

---

## 8. Тесты

46 файлов `tests/src/**/*.cpp` (Catch2): API 6, BlockChain 2, Common 1, Consensus 3,
Core 5, Crypto 4, Database 1, Net 5, PMMR 5, Wallet 9.
Состояние прогонов на baseline — **PASS: 80 test cases / 558 assertions** (см. §9 и
`TEST_RESULTS.md` E8).

---

## 9. Состояние сборки baseline: PASS (через локальный sysroot)

Окружение: Windows x64, VS 2022 BuildTools 17.14 (MSVC 14.44, toolset v143, SDK 10.0.26100),
CMake 3.31.6 и Ninja (из VS), Python 3.10, git 2.55.

История и итог:

1. Штатный путь (vcpkg, запиненный в CI, версия 2020.02) непригоден: он не знает
   VS2022/toolset v143 (требует v142/VS2019), а загрузка пакетов через WinHTTP падала
   (`WinHttpSendRequest() failed: 12185`).
2. Собран `vcpkg.exe` из `toolsrc` этого пина через MSBuild/v143 — успешно, но
   установка пакетов всё равно упиралась в загрузку и поддержку тулчейна.
3. Решение: зависимости тех же зафиксированных версий собираются локально в
   `.toolchain/sysroot` скриптом `tools/deps/build_sysroot.py` (11 библиотек,
   статический CRT, SHA-256 исходников в `SOURCES.sha256`).
4. `tools/build/build.ps1` конфигурирует проект (VS2022, `CMAKE_PREFIX_PATH` = sysroot)
   и собирает цели. Результат: **`GrinNode.exe` (7 225 344 B) и `Tests.exe`
   (7 747 584 B) собраны с `/W4 /WX`**, полный набор тестов — **80 кейсов / 558 проверок PASS**.
5. Понадобились только конфигурационные правки: `NOMINMAX`, идемпотентная установка
   `/W4 /WX` и два патча сторонних заголовков (asio standalone/unistd, rocksdb
   `thirdparty.inc`). Исходный код ноды под `/WX` не потребовал правок.

Инструкции: `BUILD.md`; доказательства: `TEST_RESULTS.md` (E6-E9).

---

## 10. Что нельзя утверждать по итогам baseline

* Что Grin++ синхронизируется с текущей сетью: собран и запускается, но живая
  синхронизация с Rust-нодой v5.5.2 **не выполнялась** (нет PIBD и второй ноды).
* Что обработка исторических блоков/MMR/reorg эквивалентна Rust (нужны differential-тесты).
* Что кошелёк и API не регрессировали: тесты зелёные, но ручной GUI/кошелёк-прогон
  и live-сценарии не выполнялись.
* Что PIBD/PIHD совместимы — их в коде нет.
