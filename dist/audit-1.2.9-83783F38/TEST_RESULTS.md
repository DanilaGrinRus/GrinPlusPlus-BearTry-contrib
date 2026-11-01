# TEST_RESULTS — доказательства (итерация 2026-10-30)

Правило: PASS только для фактически выполненной проверки. Пропуск/блокировка
не считаются успехом. Команды приведены так, чтобы их можно было повторить.

Окружение: Windows x64; VS 2022 BuildTools 17.14 (MSVC 14.44.35207, toolset v143,
Windows SDK 10.0.26100); CMake 3.31.6 (из VS); Ninja; Python 3.10.11; git 2.55.0.
Корень: `C:\DHW\Gplus\src\grinplusplus`. Зависимости: локальный sysroot
`C:\DHW\Gplus\.toolchain\sysroot` (см. `BUILD.md`).

---

## E1. Сверка снапшота с публичным Grin++ v1.2.9 — PASS

```
git -C ... -c http.sslBackend=openssl fetch --depth 1 upstream refs/tags/v1.2.9
git -C ... diff --stat 5f511e6 FETCH_HEAD
```
5 файлов, все — `dist/{linux,mac}/tor/*`. Исходный код идентичен тегу `v1.2.9` (`a0958a3`).

## E2. Дефект `ByteBuffer` до фикса — FAIL воспроизведён

Изолированный пробник: переполнение 64-бит обходит проверку длины, вместо
`DeserializationException` вылетает `std::length_error`:
```
[1] declared=18446744073709551604  RESULT: exception: vector too long
[2] declared=18446744073709551590  RESULT: exception: vector too long
```

## E3. После фикса — PASS

```
[1] RESULT: exception: RequireRemaining: Attempted to read past end of ByteBuffer.
[2] RESULT: exception: RequireRemaining: Attempted to read past end of ByteBuffer.
```

## E4. Regression-тесты против старого заголовка — FAIL (тест ловит дефект)

`git show 5f511e6:include/Core/Serialization/ByteBuffer.h` в отдельный include-каталог:
```
Test_ByteBuffer.cpp(46): failed: unexpected exception: 'vector too long'
Test_ByteBuffer.cpp(56): failed: unexpected exception: 'vector too long'
Failed 2 test cases, failed 2 assertions.
```

## E5. Проводной формат P2P — PASS (компиляция TU)

`static_assert` на биты capabilities 0x01..0x40, IDs 21..28, лимиты 41 /
`2*MAX_BLOCK_SIZE`, отсутствие PIBD-битов в `ADVERTISED`. Компилятор: exit 0.

## E6. Сборка зависимостей (offline sysroot) — PASS

```
python tools/deps/build_sysroot.py
```
Собраны из пина vcpkg/overlay-портов (SHA-256 каждого архива пишется в
`.toolchain/sources/SOURCES.sha256`, 45 записей):
zlib 1.2.11, minizip 1.2.11, libsodium 1.0.18, secp256k1-zkp @5b39fbf,
rocksdb 6.6.4 @551a110 (/MT), civetweb @2c1caa6, CRoaring 0.2.66,
fmt 6.2.1, asio 1.12.2 (standalone + guard unistd), mio @cafa313, Catch2 2.12.1.

Итог в `.toolchain/sysroot/lib`: `zlibstatic.lib`, `minizip.lib`, `libsodium.lib`,
`secp256k1-zkp.lib`, `rocksdb.lib` (139 MB static), `civetweb.lib`,
`civetweb-cpp.lib`, `roaring.lib`. Пакетные конфиги — `lib/cmake/*` и `share/*`.

## E7. Сборка ноды и тестов — PASS

```
pwsh tools/build/build.ps1 -Target GrinNode
pwsh tools/build/build.ps1 -Target Tests
```
| Артефакт | Размер | SHA-256 |
|---|---|---|
| `bin/Release/Release/GrinNode.exe` | 7 473 664 | `279C84F7C2C6C19C18A22676B407A3B33A6F4126084D3475E3E99CA56B21D164` |
| `bin/Release/Release/Tests.exe` | 8 408 576 | `9ADD32A66A04F658772165613D593EBD3AAE5B9F2C45F52BE9D24908A33CB7B2` |

Сборка идёт с `/W4 /WX` (проектные флаги не ослаблены). Потребовались только
конфигурационные правки: `NOMINMAX`, идемпотентная установка `/W4 /WX`,
патчи сторонних заголовков (asio standalone/unistd) — см. `GAP_MATRIX` и `BUILD.md`.

## E8. Полный набор тестов Grin++ — PASS

```
$env:HOMEDRIVE='C:'; $env:HOMEPATH='\DHW\.tmp\testhome'   # изоляция данных тестов
bin\Release\Release\Tests.exe --reporter compact
→ Passed all 134 test cases with 1541 assertions.
```
В набор входят новые `Test_ByteBuffer`, `Test_P2PCompat` и `Test_Segment`
(16 кейсов PIBD-проводного слоя: MMR-позиции, `SegmentIdentifier`, `SegmentProof`,
`Segment<T>`, `BitmapBlock`/`BitmapSegment`, сообщения 21–28).

Важно: тесты пишут в `%HOMEDRIVE%%HOMEPATH%\.GrinPP\AUTOMATED_TESTING`. Без
изолированного HOME запуск падает (`create_directories: Access is denied` для
`...\.GrinPP\AUTOMATED_TESTING\NODE\DB`) — это ограничение окружения, а не кода ноды;
изоляция обязательна и соответствует политике «не трогать пользовательские данные».

## E9. Запуск ноды (smoke) — PASS

```
GrinNode.exe --floonet --headless --no-wallet   # с изолированным HOME
```
* процесс живёт, создаёт `FLOONET/{NODE,TOR,WALLET,LOGS}`;
* БД инициализируется, наблюдаются миграции схемы 1→2→3 (`Create ... Migrating blocks to v3`);
* версия в логе — `1.2.8` (следствие G-25);
* `Failed to start server (127.0.0.1:13413)` был вызван занятым портом (на нём
  висел сторонний процесс `grin`), а не дефектом: после освобождения порта ошибка
  не воспроизводится как самостоятельная.

## E10. Живая синхронизация с Rust-нодой v5.5.2 — NOT RUN

Нужны вторая нода (сборка Rust) и PIBD, которых пока нет.

## E11. Reorg / crash / fuzz — NOT RUN

Требуют сборки Rust-ноды и отдельного стенда; запланированы в Фазах 5-6.

## E12. PIBD: проводной слой и верификация сегментов — PASS (unit)

Реализовано и покрыто тестами (`tests/src/PMMR/Test_Segment.cpp`, 21 кейс):

* `MMRUtil`: `PeakMapHeight`, `InsertionToPmmrIndex`, `GetNumLeaves`,
  `BintreePostorderHeight`, `IsLeaf`, `IsLeftSibling`, `FamilyBranch`,
  `BintreeLeftmost/Rightmost` — порт `core/src/core/pmmr/pmmr.rs`.
* `SegmentIdentifier` (height u8 + idx u64; height > 32 отвергается — защита от
  сдвига, которого нет в Rust; 9/11/13 — рабочие высоты).
* `SegmentProof`: wire-формат, лимиты и **`ReconstructRoot`/`Validate`** — порт
  `reconstruct_root`/`validate` (соседние узлы, bagged peaks справа и слева).
* `Segment<T>`: wire-формат, лимиты, **`Root`**, **`FirstUnprunedParent`**,
  **`Validate`** для полных и финальных (частичных) сегментов.
* `BitmapChunk` (1024 бита, LSB-first), `BitmapBlock` (Raw/Positive/Negative,
  ≤16 чанков), `BitmapSegment` (height ≤ 13, валидация блоков, checked offset) и
  **`ToSegment()`** — порт `into_segment`, включая перенос битов в чанки.
* Сообщения 21–28 с round-trip для всех четырёх типов ответов.

Верификация проверена на вручную построенном MMR (7 узлов / 4 листа): корень
полного сегмента, корень поддерева, доказательства с левым и правым соседом,
отказ при подмене листа, при неверном корне и при отсутствии hash-а в proof.
Хеш-примитивы (`HashLeafWithIndex`, `HashParentWithIndex`) взяты из существующего
`MMRHashUtil`, поэтому тесты проверяют сборку дерева и логику proof, а не сами
blake2b-примитивы.

## E13. PIBD: генерация сегментов и обратная сборка — PASS (unit)

Добавлены `InMemoryPmmr<T>` (постордерное дерево с хешами листьев/родителей,
bagging пиков) и `SegmentGenerator<T>` (порт `Segment::from_pmmr` +
`SegmentProof::generate`). Тесты (`Test_SegmentGenerator.cpp`, 5 кейсов):

* генерация сегментов для 7 размеров дерева (1..13 листьев) × высот 0..3,
  все индексы сегментов проходят `Validate` против корня дерева (цикл даёт
  100+ проверок);
* полная пересборка дерева из всех сегментов даёт тот же корень и покрывает
  каждый лист ровно один раз;
* подменённый лист отвергается, несуществующий сегмент отвергается, форма
  MMR проверяется отдельно (`size = 2N - popcount(N)`).

**Найдено тестом:** первая версия `InMemoryPmmr` считала размер как
`insertion_to_pmmr_index(N-1)+1` (это позиция последнего листа, а не размер);
тест на 4 листах поймал ошибку до попадания в основной код.

## E14. PIBD: bitmap-aware верификация и pruned-сегменты — PASS (unit)

`Segment<T>` получил `RootWithBitmap` / `FirstUnprunedParentWithBitmap` /
`ValidateWithBitmap` (порт `root`/`first_unpruned_parent` для prunable MMR),
`SegmentBitmap` (интерфейс `Contains`/`RangeCardinality` + векторная реализация для
тестов) и правило обязательности листа из Rust (собственный бит, бит соседа или
последний лист MMR). `SegmentGenerator` умеет генерировать pruned-сегменты:
только обязательные листья, все внутренние хеши и отдельный случай полностью
вырезанного сегмента (`start_pos`).

Тесты (2 кейса + расширение прежних):

* pruned-сегменты дерева на 8 листьев валидируются против корня с bitmap;
* полностью вырезанный сегмент не имеет корня (`RootWithBitmap` = nullopt),
  но проходит проверку через сохранённый хеш первого непрореженного родителя;
* подмена сохранённого хеша в таком сегменте отвергается.

Найдено `/W4 /WX`: C4373 из-за `const` на by-value параметре при переопределении —
исправлено изменением сигнатур, а не отключением предупреждения.

## E15. PIBD фаза 1: сборка output-битмапа — PASS (unit)

`BitmapSegment::FromSegment` (обратное к `ToSegment`) и `BitmapSegmentAssembler`
(`src/P2P/PIBD/`): принимает bitmap-сегменты высоты 9, проверяет идентификатор,
ожидаемое число чанков (включая короткий хвостовой сегмент), валидирует сегмент
против корня bitmap-MMR и собирает битмап. Дубликаты игнорируются, отсутствующие
сегменты перечисляются, неполный набор не собирается.

Тесты (`Test_BitmapAssembly.cpp`, 4 кейса):

* round-trip `BitmapSegment ↔ Segment<BitmapChunk>`;
* один сегмент на 2 чанка: сборка, проверка битов, дубликат, неполнота;
* отказы: неверный корень горизонта, неверная высота сегмента, индекс вне
  диапазона, подмена битов (ломает merkle proof);
* 600 чанков / 2 сегмента, принятые в обратном порядке: `GetMissing`,
  `IsComplete`, сверка выборочных битов.

**Найдено тестами:** (1) размер bitmap-MMR считался как `insertion_to_pmmr_index(N-1)+1`
вместо `2N − popcount(N)`, из-за чего `peaks` был пуст; (2) тест передавал число
выходов вместо размера MMR; (3) относительные include-пути из `src/P2P/PIBD` были
на уровень выше. Все три исправлены в коде/тестах, а не обойдены.

## E16. PIBD: оркестрация загрузки (фазы) — PASS (unit)

`PibdParams` (все константы из `chain/src/pibd_params.rs`), `PibdTreeAssembler<T>`
(валидация и накопление сегментов одного дерева; bitmap для output/rangeproof,
`nullptr` для kernel) и `PibdDownload` (шаблон по типам листьев + alias с
`OutputIdentifier`/`RangeProof`/`TransactionKernel`): фаза bitmap → фаза трёх
деревьев → Complete; `NextRequests` уважает порядок и лимит, дубликаты и поздние
ответы не принимаются, сегменты до готового битмапа отвергаются.

Тесты (`Test_PibdDownload.cpp`, 3 кейса):

* полный поток: 300 выходов, 300 rangeproofs, 50 ядер, bitmap {0,100};
  проверены фазы, состав запросов, корни, число листьев (только обязательные для
  pruned-деревьев, все для ядер), `GetReceived == GetRequired`, пустая очередь;
* сегменты до битмапа отвергаются; неверный корень битмапа не двигает фазу;
* дубликаты (bitmap и output) возвращают `false`; **непрореженный** output-сегмент
  и kernel-сегмент, поданный как output, отвергаются проверкой proof.

Честное ограничение: таймеры, выбор пира, retry и resume в этот слой не входят —
они относятся к сетевому слою (следующий шаг). Живой interop с Rust-нодой
по-прежнему NOT RUN.

## E17. PIBD: сессия и планировщик с таймаутами — PASS (unit)

`PibdSession` — транспортный адаптер: гейт по `PIBD_HIST_1` (одного `PIBD_HIST`
недостаточно), построение запросов 21/23/25/27, маршрутизация ответов 22/24/26/28
в нужное дерево, отсечение ответов чужого horizon. `PibdScheduler` — лимит
in-flight запросов, таймаут сегмента (20 с), повтор до N раз, счётчик суммарных
запросов и fallback-таймер (660 с без прогресса). Время инъектируется, поэтому
поведение детерминировано в тестах.

Тесты (`Test_PibdSession.cpp`, `Test_PibdScheduler.cpp`, 5 кейсов):

* capability-гейт: `PIBD_HIST`/`FAST_SYNC_NODE` → false, `PIBD_HIST_1` → true;
* сквозной поток сообщений: bitmap → output/rangeproof/kernel, ответы чужого
  horizon игнорируются, tampered kernel отвергается, затем корректный принимается;
* лимит in-flight: при `MaxInFlight=1` второй вызов не отправляет ничего;
* таймаут: повтор того же сегмента (attempt 2), затем отказ после лимита;
* отсутствие прогресса 660 с → fail с причиной (сигнал для txhashset.zip fallback).

## E18. PIBD: desegmenter-ядро (снапшот состояния) — PASS (unit)

`BuildSnapshot(download)` превращает завершённую загрузку в состояние, пригодное
для применения: листья с 0-based leaf index (обратная функция
`MMRUtil::PmmrLeafToInsertionIndex`), битмап, размеры MMR и метаданные горизонта.
Сборка перепроверяет то, что можно проверить по самим листьям:

* **kernel-дерево пересобирается полностью** (ядра не прореживаются) и его корень
  сверяется с корнем горизонта;
* число ядер должно совпадать с `GetNumLeaves(kernelMmrSize)`;
* множества leaf index у outputs и rangeproofs обязаны совпадать (одинаковый
  битмап → одинаковое прореживание);
* неполная загрузка → исключение (никакого частичного состояния).

Тесты (`Test_PibdSnapshot.cpp`, 2 кейса): полный поток 120 выходов / 8 ядер,
битмап {0,70}; проверены leaf index, сортировка, `expectedOutputs < numOutputs`,
совпадение множеств, пересборка kernel-корня; отдельный цикл проверяет инверсию
leaf index ↔ PMMR position и отсутствие индекса у внутренних узлов.

Честное ограничение: снапшот пока **не пишется** в файловый PMMR (prune shifts,
атомарная активация) — это следующий шаг; живой interop не проверялся.

## E19. PIBD: пересборка прореженного дерева — PASS (unit)

Сегменты теперь сохраняют **все внутренние хеши** (`(position, hash)`), а не только
листья: именно ими представлены прореженные поддеревья. `PibdSnapshot` несёт эти
хеши для трёх деревьев, `PibdTreeRebuilder` (`ComputePrunedTreeRoot`) пересобирает
корень по правилам Rust: лист с данными → `hash(index ‖ data)`; оба ребёнка
прорежены → хеш берётся из сегмента (pruned root); один ребёнок прорежен →
используется сохранённый хеш ребёнка; иначе — обычный parent hash; в конце пики
собираются bagging-ом. Это ровно та арифметика, которую затем нужно записать в
hash-файлы PMMR.

`BuildSnapshot` проверяет **все три дерева** этим пересчётом (корни outputs,
rangeproofs и kernels сверяются с горизонтом), а не только kernel-дерево.

Тесты (`Test_PibdSnapshot.cpp`, +1 кейс): с хешами корень воспроизводится точно;
без хешей пересборка падает (`MissingHash`), а не выдумывает корень; подмена всех
сохранённых хешей даёт другой корень.

**Найдено компилятором/тестами:** `std::map::operator[]` требовал default-ctor у
`RangeProof` (заменено на `insert_or_assign`); первая версия теста на подмену
ожидала исключение, тогда как подмена корректно даёт *другой* корень.

## E20. PIBD: запись состояния в файловый PMMR (staging) — PASS (unit)

`PibdStateWriter::Write(snapshot, dir)` пишет три дерева в формате ноды:
`pmmr_hash.bin` (все сохранённые узлы в возрастающем порядке позиций),
`pmmr_data.bin` (сериализованные листья, только непрореженные),
`pmmr_leafset.bin` (`BitmapFile`, позиции+1) и `pmmr_prun.bin` (roaring с
корнями прореженных поддеревьев). Layout со сдвигами соответствует
`PruneableMMR`/`PruneList`: физический индекс узла = позиция минус сдвиг, а
stored-узлы идут в порядке позиций.

Каждое дерево проверяется **перезагрузкой записанных файлов**: `HashFile::Load` +
`PruneList::Load` + `MMRHashUtil::Root` должны дать корень горизонта; число узлов
в hash-файле и записей в data-файле сверяется с записанным. Сериализованный размер
листа проверяется на равенство фиксированному (34/683/114), иначе запись
отвергается — иначе data-файл был бы нечитаем.

Тесты (`Test_PibdStateWriter.cpp`, 2 кейса): полный поток 64 выхода / 2 ядра,
bitmap {0}; после записи независимо перезагружаются hash/prun/data/leafset и
сверяются корни, payload листа 0, `leafset` (листья 0 и 1 присутствуют, 2 — нет),
ядро без сдвигов; снапшот без pruned-хешей отвергается.

Честное ограничение: запись идёт **в указанный staging-каталог**, атомарная
активация поверх рабочей БД ещё не реализована; живой interop не проверялся.

## E21. PIBD: атомарная активация состояния — PASS (unit)

`PibdActivation::Activate(staging, live)` заменяет рабочее состояние без правки
файлов на месте: маркер `pibd_activation.pending` с путями (staging, backup) →
`rename(live, backup)` → `rename(staging, live)` → удаление маркера и backup.
`PibdStateWriter` пишет `pibd_state.complete` только после успешной записи и
проверки всех трёх деревьев, и активация отказывается брать staging без него.

`PibdActivation::Recover(live)` разбирает прерванную активацию:
* live полон (`pibd_state.complete` есть) → активация завершилась, backup
  удаляется, маркер снимается;
* live отсутствует, backup есть → backup возвращается на место (откат);
* live неполон и backup отсутствует → явная ошибка, а не «продолжаем как есть».

Тесты (`Test_PibdActivation.cpp`, 3 кейса): успешная активация с проверкой корней
по перезагруженным файлам, исчезновением старого состояния, backup и маркера;
отказ активации при отсутствии `pibd_state.complete` (live не тронут); оба
сценария сбоя (откат на backup и завершение поверх stale backup).

**Найдено тестами:** `CreateDirectories` возвращает false для уже существующего
каталога (проверка `Exists` добавлена); маркер, записанный в текстовом режиме,
содержал `\r`, из-за чего путь backup не находился (парсер теперь срезает CR) —
иначе восстановление молча не срабатывало.

**Честная граница:** активация проверена как модуль на временных каталогах; в
процесс ноды она пока не включена, живой interop не проверялся.

## E22. PIBD: выдача сегментов из файловых PMMR — PASS (unit, без прореживания)

`PibdSegmentService` выдаёт сегменты прямо из `OutputPMMR`/`RangeProofPMMR`/
`KernelMMR`: битмап выводится из leafset, bitmap-MMR пересобирается в памяти из
1024-битных чанков, прореженные сегменты стримятся из PMMR (листья — из data-файла,
корни прореженных поддеревьев — из hash-файла), ядра — без прореживания.
`SegmentGenerator` обобщён на источник-колбэки (`GenerateFromSource`), поэтому
генерация для in-memory модели и для файловых PMMR идёт по одному коду.

Тест (`Test_PibdServing.cpp`): состояние записывается `PibdStateWriter`, затем
загружается реальными `OutputPMMR::Load`/`RangeProofPMMR::Load`/`KernelMMR::Load`;
выданные сегменты принимаются **свежей** `PibdSession`, загрузка доходит до
Complete, `BuildSnapshot` совпадает по листьям. Это полный цикл
write → serve → download → validate → rebuild по настоящим файлам.

**Найден реальный разрыв (G-26, OPEN).** При прореженном битмапе writer считает
принятые по правилу соседа листья непрореженными, и `leafset` расходится с
битмапом горизонта; вдобавок `PruneList::GetLeafShift` даёт 0 для pruned-root
высоты 0, поэтому data-файл Grin++ обязан содержать запись для каждого листа ниже
последнего непрореженного. Поэтому тест намеренно ограничен полностью
непотраченным набором (все листья обязательны): он проверяет протокол выдачи, но
не прореженную семантику. Это честно зафиксировано в GAP_MATRIX, а не замазано.

## E23. G-26, часть 1: leafset из битмапа и raw-чтение для выдачи — PASS (unit)

Исправлено: `PibdStateWriter` пишет в `pmmr_leafset.bin` только **непотраченные**
листья (по битмапу горизонта), а не все принятые. Потраченный лист, присланный по
правилу соседа, остаётся spent, но его данные сохраняются в data-файле — как в
обычном состоянии Grin++. Для выдачи таких обязательных листьев добавлен
`PruneableMMR::GetRawAt` (читает data-файл без проверки leafset), и
`PibdSegmentService` использует его; битмап, отдаваемый сервисом, теперь совпадает
с битмапом горизонта.

Тесты: writer проверяет `leafset` (лист 0 — unspent, лист 1 — spent, хоть и
прислан), корни по-прежнему воспроизводятся; полный набор 132/1625 PASS.

**Осталось по G-26 (часть 2, OPEN):** `ComputePrunedTreeRoot` требует
сохранённый хеш у любого внутреннего узла с отсутствующими детьми, включая
интерьер прореженного поддерева. Прореженная пересборка падает с
`Missing pruned-subtree hash at position 9`, хотя позиция 9 — интерьер и её хеш не
нужен. Нужно: считать максимальные прореженные поддеревья (как в writer) и
требовать хеши только у их корней; добавить имя дерева в сообщение об ошибке.
Прореженный serving-тест возвращён к полностью непотраченному набору, чтобы набор
оставался зелёным и не маскировал дефект.

## E24. G-26 закрыт: прореженный serve→download работает — PASS (unit)

Две причины, найденные тестами, устранены:

1. **Интерьер прореженного поддерева больше не требует хешей.** `ComputePrunedTreeRoot`
   (и writer) вычисляют максимальные прореженные поддеревья так же, как при записи:
   узел — корень, если в его поддереве нет данных, а у родителя есть. Узлы внутри
   поддерева не имеют хешей и не должны их иметь; сегмент несёт только корень.
2. **Полностью прореженный пик.** Для MMR, где последний узел — внутренний
   (например, 120 листьев), верхний пик может не содержать данных вовсе; его хеш
   приходит из сегмента. Раньше пересборка отвергала такое дерево
   (`Tree has no data`). Теперь полностью прореженный пик считается допустимым
   корнем поддерева, а «пустое дерево» определяется по отсутствию листьев.

Тест `Test_PibdServing` снова работает на **прореженном** битмапе (64 выхода,
бит 0): состояние пишется writer-ом, загружается реальными PMMR, сервис отдаёт
битмап/выходы/rangeproofs/ядра, свежая сессия принимает всё, доходит до Complete,
и пересобранный снапшот совпадает; дополнительно проверяется, что в состоянии
меньше 64 листьев (прореживание реально).

Полный набор: **132 кейса / 1504 проверки PASS**. `GrinNode.exe` собирается с
`/W4 /WX`.

## E25. PIBD: сервис полного цикла (`PibdSync`) — PASS (unit)

`PibdSync` связывает готовые части в один сервис: `Start(horizon)` → `TakeRequests`
(через `PibdScheduler` с таймаутами) → `OnResponse` (валидация и учёт) → по
завершении `TryComplete()` = `BuildSnapshot` → `PibdStateWriter::Write` (staging) →
`PibdActivation::Activate`. Статусы: Idle/Downloading/Completed/Activated/Failed;
любая ошибка (плохой сегмент, сбой записи/активации) переводит в Failed и
**не трогает** рабочее состояние — это сигнал для legacy-загрузки txhashset.zip.

Тесты (`Test_PibdSync.cpp`, 2 кейса): полный цикл — загрузка идёт по реальным
запросам-сообщениям, ответы генерируются по типу запроса, затем запись+активация,
корни по активированным файлам сверяются с горизонтом, старое состояние исчезает;
невалидный bitmap-сегмент (подменённые биты) → Failed, `TryComplete` = false,
рабочий каталог цел.

Полный набор: **134 кейса / 1536 проверок PASS**. `GrinNode.exe` собирается с
`/W4 /WX`.

**Честная граница:** сервис проверен как модуль; в процесс ноды он **не включён** —
`MessageProcessor` пока не маршрутизирует 22/24/26/28 в `PibdSync`, `StateSyncer`
не запускает его, `Recover` при старте не вызывается, `PIBD_HIST` в `ADVERTISED`
не включён. Живой interop с Rust-нодой не проверялся.

## E26. Сверка с эталоном Rust v5.5.2: исправлена модель корней PIBD — PASS (unit)

По исходникам `grin_core 5.5.2` (`core/src/core/pmmr/segment.rs`) и `grin_chain 5.5.2`
(`txhashset/desegmenter.rs`, `txhashset/segmenter.rs`) установлено, что модель,
реализованная ранее, **неверна для реального протокола**:

* `header.output_root = H(output_mmr_size ‖ output_pmmr_root ‖ bitmap_root)` — заголовок
  коммитит оба корня; отдельного поля bitmap-root в заголовке нет;
* bitmap-сегмент **несёт output-PMMR root** и проверяется через `validate_with`
  (other=output root, `other_is_left = true`, индекс = `output_mmr_size`);
* output-сегмент проверяется через `validate_with` с **локально собранным**
  bitmap-root (other справа), а не против `output_root` напрямую;
* rangeproof-сегменты — против `range_proof_root` с bitmap; ядра — против `kernel_root`.

Реализовано: `SegmentProof::ValidateWith`, `Segment::ValidateWithOtherRoot`,
`BitmapSegmentAssembler` (комбинированная проверка + `GetOutputMmrRoot`/`BuildBitmapMmrRoot`),
`PibdTreeAssembler` с режимом combined-root, `PibdDownload::Horizon` без bitmap-root,
`PibdSnapshot::OutputMmrRoot` (аутентифицированный выходной PMMR root), writer сверяет
записанный output-root с ним; тесты переведены на комбинированную модель (собраны
`MMRHashUtil::HashParentWithIndex`). Набор: **134 кейса / 1540 проверок PASS**.

Ссылки на эталон: [segment.rs](https://docs.rs/grin_core/5.5.2/src/grin_core/core/pmmr/segment.rs.html),
[desegmenter.rs](https://docs.rs/grin_chain/5.5.2/src/grin_chain/txhashset/desegmenter.rs.html),
[segmenter.rs](https://docs.rs/grin_chain/5.5.2/src/grin_chain/txhashset/segmenter.rs.html).
Крейты сохранены локально в `.toolchain/reference` (вне git).

## E27. Проводка PIBD в ноду + smoke-старт — PASS (build/smoke)

Сделано:

* `MessageProcessor` принимает `PibdSync`: ответы 22/24/26/28 десериализуются и
  передаются в сервис, если синхронизация активна; запросы 21/23/25/27
  логируются и игнорируются (serving ещё не рекламируется, соединение не рвётся);
* `StateSyncer` при наличии пира с `PIBD_HIST_1` запускает PIBD от candidate tip
  (корни и размеры MMR из заголовка), шлёт запросы этому пиру, по завершении
  закрывает открытый txhashset (иначе Windows не даст переименовать каталог с
  mmap-файлами), активирует staging и валидирует новый набор через
  `BlockChain::ProcessExistingTxHashSet`; при любом сбое — возврат к
  txhashset.zip с паузой PIBD на 120 минут;
* `P2PServer::Create` создаёт `PibdSync` (live = `TXHASHSET`, staging =
  `pibd_staging` рядом) и передаёт его в `Seeder` (MessageProcessor) и `Syncer`
  (StateSyncer); `DefaultNodeClient::Create` вызывает `PibdActivation::Recover`
  до открытия чейна;
* `BlockChain` получил `ProcessExistingTxHashSet`, `CloseTxHashSet`,
  `OpenTxHashSet`; `TxHashSetProcessor` умеет валидировать и устанавливать уже
  лежащий на диске набор (без распаковки zip).

Smoke (`--floonet --no-wallet --headless`, изолированный HOME): нода стартует,
поднимает node server, поток SYNC работает; ошибок PIBD/activation в логе нет.
Локальный порт REST 13413 занят запущенной Rust-нодой (`grin 5.5.2`,
`C:\DHW\resources\node\grin.exe`), поэтому smoke шёл с `REST_API_PORT=13415`.
Floonet-seed-пиры за 12 секунд не ответили — **пиров не было, живой PIBD не
проверялся**. Бинарник Rust-ноды v5.5.2 доступен локально — это база для
следующего раунда (изолированный floonet-инстанс + interop).

Полный набор: **134 кейса / 1540 проверок PASS**. Артефакты: `GrinNode.exe`
`4A3A97B2…`, `Tests.exe` `620C1857…`.

## E28. PIBD serving в ноде + реклама возможностей — PASS (build/smoke)

* `MessageProcessor` теперь **обслуживает** запросы 21/23/25/27: выбирает текущий
  `TxHashSet`, проверяет, что запрошенный `block_hash` совпадает с нашим
  `GetFlushedBlockHeader()`, и отдаёт сегменты через `PibdSegmentService`
  (bitmap несёт output-PMMR root, output — bitmap root). Ошибки обслуживания
  логируются, соединение не рвётся.
* `Capabilities::ADVERTISED` включает `PIBD_HIST | PIBD_HIST_1`
  (`BLOCK_HIST` по-прежнему выключен — archival serving нет). Тест
  `Test_P2PCompat` обновлён под это и фиксирует отсутствие `BLOCK_HIST`.
* `BlockChain::GetTxHashSet()` добавлен для доступа к установленному состоянию.
* `MIN_SYNC_PEERS` теперь читается из `P2P`-конфига (по умолчанию 3) — без этого
  interop с одним локальным пиром невозможен: поток SYNC ждёт минимум пиров.

**Попытка живого interop (раунд 18):** запущена эталонная Rust-нода v5.5.2
(`C:\DHW\resources\node\grin.exe`) в режиме `--testnet server -p 13424 -a 13416 run`.
Она использовала **существующий** `%USERPROFILE%\.grin\test` (каталог создан
ранее, не нами), синхронизировала заголовки (header PMMR ~13 МБ) и начала
скачивать txhashset (~140 МБ), после чего была остановлена. Данные не удалялись и
не переносились. Живой PIBD **не проверялся**: для запуска нужен наш узел с
`MIN_SYNC_PEERS=1` и подключение Rust-ноды к нашему listener'у
(`--seed 127.0.0.1:13414`) — это следующий шаг.

Полный набор: **134 кейса / 1541 проверка PASS**. Артефакты: `GrinNode.exe`
`B215676C…`, `Tests.exe` `EED571A5…`.

## E29. Живой interop, раунд 19: найден и исправлен дефект accept-loop — PASS (fix+tests)

Во время подготовки interop обнаружен **реальный дефект P2P** (G-29):
`Seeder::Accept` при отклонённом входящем пире (`IsPeerBlocked`/`IsPeerAllowed`)
делал `return` **до** повторной постановки `async_accept`. Первый же отклонённый
пир навсегда останавливал приём входящих соединений: Rust-нода подключалась к
127.0.0.1:13414 (соединение видно в TIME_WAIT), но Grin++ её не обслуживал, и в
`netstat` не было `LISTENING` на 13414. Исправлено: единственная точка повторной
постановки accept после обработки любого исхода; после фикса узел показывает
`0.0.0.0:13414 LISTENING`.

**Найденные ограничения окружения (для следующего раунда):**

* Grin++ адресует пиров одним глобальным P2P-портом (`GetP2PPort()`), поэтому две
  локальные ноды на одном хосте не могут соединиться «в обратную сторону» —
  нужен Rust-узел, который сам звонит на 127.0.0.1:13414;
* в Rust `peers_preferred` — только защита от вытеснения, **не** список для
  дозвона (подтверждено по `grin_p2p/src/peers.rs:350,393`); дозвон идёт по
  `seeding_type = "List"` + `seeds = [...]`; рабочий вариант:
  `seeds = ["127.0.0.1:13414", <реальные testnet-пиры>]`;
* Rust-нода после жёсткого kill со старым `db_root` зависает до
  `Grin server started` без сокетов; помогает свежий `db_root`;
* данные Rust-ноды теперь лежат в `C:\DHW\.tmp\interop\grin-chain*`
  (`db_root` из конфига), профиль пользователя не затрагивается.

Живой PIBD-обмен по-прежнему **NOT RUN**: после фикса нужно перезапустить пару
узлов и дать Rust-ноде дозвониться; это следующий шаг.

Полный набор: **134 кейса / 1541 проверка PASS**. Артефакты: `GrinNode.exe`
`FC9C3F82…`, `Tests.exe` `A725AE72…`.

## E30. Живой interop: handshake и обмен сообщениями с Rust v5.5.2 — PASS; PIBD-обмен НЕ ДОСТИГНУТ

С исправленным accept-loop (E29) и настраиваемым `P2P_PORT` (Grin++ слушает 13444,
порт 13414 занят запущенной Rust-нодой пользователя) удалось установить живое
соединение с эталонной Rust-нодой v5.5.2 (`grin.exe`, testnet, `db_root` в
`.tmp/interop`, `seeding_type="List"`, `seeds` включает `127.0.0.1:13444`):

* Rust подключается к Grin++, handshake проходит: в логе Rust
  `Connected! ... "Grin++ 1.2.8", TXHASHSET_HIST | PEER_LIST | PIBD_HIST | PIBD_HIST_1`;
* Grin++ принимает и обрабатывает сообщения: в логе
  `Received Msg::GetPeerAddrs` и `Received Msg::Ping from Connection{127.0.0.1}`;
* TCP-соединение стабильно (ESTABLISHED с обеих сторон).

**Найден и исправлен баг рекламы возможностей:** `HandShake.cpp` отправлял
жёстко `Capabilities::FAST_SYNC_NODE` вместо `ADVERTISED`, поэтому PIBD-биты не
рекламировались (Rust изначально видел только `TXHASHSET_HIST | PEER_LIST`).
Теперь оба сообщения (Hand/Shake) отправляют `ADVERTISED`.

**Почему PIBD-обмен не достигнут:** Rust-нода с нуля синхронизирует testnet-заголовки
со скоростью ≈38 заголовков/с (высота ≈3.4k за 90 с при цели ~3.9M) — это десятки
часов; пока её chain height = 0, `Ping` сообщает height 0, и `HeaderSyncer` Grin++
(защита `networkHeight == 0`) не начинает синхронизацию заголовков. То есть для
живого PIBD нужен уже синхронизированный Rust-пир, а его получение в этой сессии
непрактично. PIBD-обмен (22/24/26/28) остаётся **NOT RUN**.

Полный набор: **134 кейса / 1541 проверка PASS**. Артефакты: `GrinNode.exe`
`279C84F7…`, `Tests.exe` `9ADD32A6…`.

Осталось по G-09: живой PIBD с синхронизированным пиром, resume между рестартами, PIHD.

## E31. Живой PIBD-обмен с testnet-сетью: заголовки и bitmap-сегменты — PASS; сегмент 1 — FAIL (разбор)

Вместо локальной Rust-ноды нашей ноде дан отдельный порт дозвона (`P2P_PEER_PORT`
= 13414) при прослушивании 13444. С явными живыми testnet-пирами
(`PREFERRED_PEERS`) узел подключился к реальной сети и синхронизировал
**3,3 млн заголовков** за ~25 минут (512 заголовков на сообщение, ~1500/с);
в `netstat` — ESTABLISHED с `204.168.202.205:13414` и др.

После догона заголовков StateSyncer запустил PIBD на archive-горизонте:

```
Starting PIBD state sync at height 3313440 (header hash 84d7ac67...)
Sending 7 PIBD request(s) to 195.201.2.51
Received Msg::OutputBitmapSegment from Connection{195.201.2.51}
```

**Найдено и исправлено по ходу живого обмена:**

1. **Горизонт PIBD** — раньше запрашивали candidate tip; Rust проверяет
   `segmenter.header().hash() == requested hash` (adapters.rs:517), а заголовок
   сегментера — это archive header `(head − STATE_SYNC_THRESHOLD)` с округлением
   вниз до `TXHASHSET_ARCHIVE_INTERVAL = 720` (`chain.rs:985,1002`). Теперь
   горизонт вычисляется так же, и завершение устанавливает состояние именно для
   этого заголовка.
2. **`BitmapBlock`** — блок покрывает `2^16` бит = **64** чанка по 1024 бита, а
   не 16 (`bitmap_accumulator.rs:470,213`). С прежним значением реальный сегмент
   не десериализовался; после фикса парсится, но см. ниже.
3. `MessageProcessor` теперь логирует текст исключения десериализации, а
   `BitmapSegmentAssembler` — промежуточные корни при несовпадении.

**Регрессионный вектор (offline, PASS):** захваченный с живого пира первый
bitmap-сегмент архива `84d7ac67…` (65 731 байт) полностью разбирается, и
`Segment::ValidateWithOtherRoot` сходится с `header.output_root =
49fe85ec…` (комбинированный корень `H(6684 ‖ output_root ‖ bitmap_root)`).
Вектор закреплён в `tests/src/PMMR/Test_PibdLiveVector.cpp` (пропускается, если
файла нет; файл лежит вне git в `.tmp/interop`).

**FAIL, требует продолжения:** второй захваченный сегмент (idx 1, 512 чанков)
даёт корень `a4e175cf…`, тогда как из proof сегмента 0 следует, что корень
поддерева листьев 512–1023 равен `00e742a9…`; наш `ValidateWithOtherRoot`
завершается `Segment proof does not reconstruct the combined MMR root`.
Расхождение воспроизведено и независимым Python-разбором (в котором сегмент 0
сходится с эталонной формулой, а сегмент 1 — нет), то есть проблема не в
C++-реконструкции proof. Дополнительная проверка сузила место: запрос одного
bitmap-сегмента высоты 13 (весь битмап, 3344 чанка) у **двух разных пиров**
(`204.168.202.205` и `66.154.127.84`) даёт **идентичные** payload-ы; C++ и
независимый Python сходятся на свёрнутом корне `570d270a…`, но он не сходится с
`header.output_root = 49fe85ec…`, а из proof сегмента 0 следует корень
`a9591a…`. Значит расхождение — в общем для C++ и эталонного разбора
предположении о wire/хешировании bitmap-чанков, а не в реконструкции; следующий
шаг — минимальный Rust-оракул на `grin_chain` (cargo 1.99.1 в окружении есть),
который сериализует известный `BitmapSegment` и подтвердит формат байт-в-байт.
Захваченные векторы сохранены в `.tmp/interop/` (`bitmap_payload_0/1.bin`,
`seg_*.bin`); полный PIBD (24/26/28 → запись → активация → досинхронизация
блоков) на живом пире **NOT RUN**.

Тесты: **135 кейсов / 1552 проверки PASS**; `GrinNode.exe` `C4FDB3EC…`,
`Tests.exe` `9B608C30…`.

## E32. Живой PIBD доведён до активации состояния — PASS (валидация/догон IN PROGRESS)

Причина расхождения из E31 найдена Rust-оракулом. Маленькая программа, собранная
против `grin_chain 5.5.2` (копия исходников в `.tmp/grin-oracle-src`, вне git;
`cargo run` в `.tmp/oracle`), построила `BitmapAccumulator` из 3344 чанков с
детерминированным рисунком и сериализовала сегменты высоты 9 (idx 0–6) и 13.
Сверка байт-в-байт дала:

* **`BitVec::to_bytes` в grin упаковывает биты MSB-первыми в каждом байте**
  (бит `n` — это байт `n/8`, бит `7-(n%8)`). Наши `BitmapChunk`/`BitmapBlock`
  использовали LSB. Для Raw-блоков это незаметно (байты при разборе и обратной
  упаковке совпадают), но Positive/Negative кодируют индексы битов в этой же
  нумерации, поэтому **все sparse-сегменты читались неверно**.
* После исправления на MSB-first совпали с эталоном **все семь** корней
  сегментов (`ff6d8a52…`, `492f9917…`, `beecc494…`, `6bf1fe59…`, `39b83440…`,
  `3f3ce7e7…`, `a7429b2c…`) и корень всего битмапа `d8bcf07b…`
  (`tests/src/PMMR/Test_PibdOracleVector.cpp`, файлы оракула в `.tmp/oracle/out`).

**Живой interop после исправления (пир `66.154.127.84`, testnet):**

* StateSyncer стартовал PIBD на архивном горизонте 3313440 (`84d7ac67…`);
* загружены и провалидированы **bitmap**, а затем **output (807)**,
  **rangeproof (815)** и **kernel (778)** сегментов — 2307 PIBD-запросов,
  **0 ошибок валидации** (в отличие от E31, где ломался уже сегмент 1);
* `TryComplete` записал staging-PMMR
  (`pibd_staging/{output,rangeproof,kernel}/pmmr_*.bin`, 2970 МБ),
  активация завершилась: `pibd_activation.pending` удалён, staging перенесён в
  `NODE/TXHASHSET` (файлы на месте);
* нода приступила к валидации активированного состояния
  (`Validating TxHashSet for block 84d7ac67…`): kernel history — PASS,
  kernel sums — PASS, **rangeproofs — выполняется на момент отчёта**
  (~17+ минут CPU, ~9 ГБ RSS). Догон блоков от горизонта до типсы —
  следом; состояние на момент отчёта: **IN PROGRESS**, не PASS.

Итог: живой PIBD `download → сегменты 22/24/26/28 → запись → активация` —
**PASS**; полная валидация и догон блоков — **IN PROGRESS**; resume между
рестартами и PIHD — по-прежнему NOT IMPLEMENTED.

Тесты: **137 кейсов / 1581 проверка PASS** (`Test_BitmapBlock::BitOrderMatchesGrin`
и оракульные векторы); `GrinNode.exe` `AE793C26…`, `Tests.exe` `3D7299FC…`.

## E33. Валидация активированного состояния завершена — PASS; block sync упёрся в ложные баны (задокументировано)

Продолжение запуска E32 (тот же HOME/бинарь `AE793C26…`):

* `Validating TxHashSet for block 84d7ac67…` — **все этапы прошли**:
  kernel history (11:54:16–11:55:21), kernel sums (11:55:21–11:57:37),
  rangeproofs (11:57:37 – ~12:24; RSS вырос до ~9.4 ГБ и вернулся к ~2.5 ГБ).
  Это подтверждает, что состояние, скачанное живым PIBD и активированное,
  консенсусно валидно (включая rangeproofs).
* Начался догон блоков: в 12:24:39 обработан первый блок `1b0cf8e3…` от
  `66.154.127.84`.
* Далее узел **забанил обоих пиров за «fraud height»** после таймаутов
  запросов блоков (12:24:53 — `66.154.127.84`, 12:29:29 — `195.201.2.51`) и
  завис без соединений: процесс жив, CPU 0, новых логов нет, `ESTABLISHED`
  сокетов нет. Входящих `BanReason` от пиров в логах нет — банила наша нода.
* Инцидент разобран и зафиксирован как следующая задача после основной:
  `PEER_BAN_AND_STALL.md`, `GAP_MATRIX` G-38 (**OPEN**). Код по этому пункту
  по указанию не менялся.

**PIBD resume (journal)** реализован в этом раунде: `PibdResume` пишет
принятые сегменты в per-kind журналы под `NODE/pibd_resume`, `PibdSync::Start`
при совпадении горизонта воспроизводит их (bitmap → output → rangeproof →
kernel) и чистит журнал после активации; невалидный журнал отбрасывается.
Unit-тесты PASS. **Живая проверка resume (kill/restart в середине загрузки) —
NOT RUN**.

**PIHD:** в эталоне grin **v5.5.2 нет PIHD вообще** — `p2p/src/msg.rs`
заканчивается `KernelSegment = 28`; бит `0x80` определён только для детекции,
**не рекламируется**, header sync всегда legacy (fallback). Факт закреплён
тестами (`Test_P2PCompat`). Полноценный PIHD невозможен против v5.5.2 без
расширения эталона — это зафиксировано как ограничение, а не «сделано».

**Дополнение (позже, вне этого прогона):** PIHD реализован upstream в ветке
`staging` — PR #3879 (протокол) и #3882 (синхронизация с фолбэком); в тег
`v5.5.2` не попал. Разбор протокола и параметров — `PIHD_UPSTREAM.md`.

Тесты: **139 кейсов / 1601 проверка PASS**; `GrinNode.exe` `FFF16B9A…`,
`Tests.exe` `3718F40B…`.

## E34. Полная живая синхронизация с testnet: PIBD → валидация → догон до типсы → рестарт — PASS

Продолжение E32/E33 на той же HOME (`pp-net`). После ложных банов (G-38,
задокументировано, код не менялся) бан-лист в PEERS-БД был очищен, а
`PREFERRED_PEERS` расширен до 12 живых testnet-адресов — это **обход**, а не
исправление G-38. После перезапуска:

* блоки пошли от `107.172.44.229` и `146.59.154.194`; обработано **1248+**
  блоков, затем догон продолжился с архивных пиров;
* в 12:49–12:50 REST-статус (`/v1/status`):

```
chain.height = 3316561, header_height = 3316561, network.height = 3316561
chain.hash = 23a71b635f7a5e3dce26d7973efadfe55062d1487d8b64e8b1492235ef153594
total_difficulty = 1206969143921
sync_status = FULLY_SYNCED, num_outbound = 2
```

* то есть цепочка от PIBD-горизонта (3313440) до типсы (3316561) догнана, и
  узел полностью синхронизирован с реальной testnet-сетью (пиры
  `146.59.154.194` caps=95, `157.90.146.148` caps=31, user agents
  `MW/Grin 5.5.2.dbaff34` / `MW/Grin 5.0.4`);
* **рестарт** (12:50:21): после загрузки состояния узел сразу показывает те же
  `chain=header=network=3316561`, `FULLY_SYNCED`, **0 запусков PIBD**
  (`Starting PIBD state sync` в логе после рестарта нет) — активированное
  состояние и цепочка сохраняются между рестартами (это state-level resume;
  journal-resume **скачивания** ещё не проверен живьём);
* артефакты статуса сохранены в `.tmp/interop/evidence/
  status_fully_synced.json`, `status_after_restart.json`.

Вывод: живой путь **PIBD download → запись → активация → валидация TxHashSet →
block sync до типсы → рестарт без повторной загрузки** пройден на реальной
сети. Ограничения: G-38 (ложные баны) не исправлен и обойдён сбросом банов;
живой resume журнала и legacy-фолбэк ещё не проверены.

Тесты и артефакты на момент записи: **139 кейсов / 1601 проверка PASS**;
`GrinNode.exe` `FFF16B9A…`, `Tests.exe` `3718F40B…`.

## E35. Версия 1.2.9 и переключатель PIBD для живого fallback — PASS (unit); live IN PROGRESS

* `GrinVersion.h` по-прежнему объявлял `1.2.8` при релизе 1.2.9 (G-25).
  Обновлено на `1.2.9`: user agent `/v1/status` и логи теперь `Grin++ 1.2.9`;
  тест `GrinVersion::MatchesRelease` фиксирует версию и UA.
* Добавлен `P2P.PIBD_ENABLED` (по умолчанию `true`). При `false`
  `StateSyncer::TryPibd` сразу возвращает `NotAttempted` и узел идёт legacy-путём
  `txhashset.zip`. Это нужно для проверки fallback и для пиров без PIBD.
* Для живых проверок одновременно подняты три узла на новом бинаре
  `96C33D57…`: `pp-net` (сразу `FULLY_SYNCED`, 3 316 576), `pp-resume`
  (докачивает заголовки для теста journal-resume) и `pp-fallback`
  (`PIBD_ENABLED=false`, синхронизирует заголовки, затем legacy zip).

Тесты: **140 кейсов / 1603 проверки PASS**.

## E36. G-38: ложные баны и залипание block sync исправлены (unit PASS; live IN PROGRESS)

Реализовано по `REQ_G38_FIX.md`:

* **Политика вынесена в `src/P2P/Sync/BlockSyncPolicy.h`** (чистые функции,
  юнит-тестируемые): batch ≤ `min(100, peers*10)`, потолок запросов
  `min(candidate tip, networkHeight, per-connection height)`,
  `SelectPeer` учитывает высоту пира и cooldown, retry 6 с / 1 с при прогрессе,
  backoff до 60 с, `SlowPeerTracker` с TTL 60 с и сбросом при прогрессе,
  `IsProvenFraudHeight` — только при 120 с без прогресса и заявленном
  TD/высоте выше нашего head.
* **`BlockSyncer`**: больше **не банит** по таймауту — переназначает блок другому
  пиру (счётчик `reassignments`), не просит блоки выше `networkHeight` и
  per-connection height, использует `GetMostWorkConnections()`; при отсутствии
  подходящего пира блок остаётся в очереди (debug-лог), а не превращается в бан.
* **`HeaderSyncer`**: `FraudHeight` только по правилу Rust (120 с stale +
  заявленный TD выше нашего), иначе — смена пира без бана; добавлен счётчик.
* **`Seeder`/`PeerManager`**: при `GetNewPeer()==nullptr` — `WARNING` с причиной
  (`known/banned/no_capability/retry_window/connected`) не чаще раза в 60 с и
  exponential backoff seeding до 60 с.
* **Watchdog**: `WAITING_FOR_PEERS` пишется в лог (INFO раз в 30 с) и в
  `waiting_reason` REST-статуса.
* **Наблюдаемость**: `/v1/status` → `block_sync { requests, timeouts,
  reassignments, bans_fraud_height, bans_invalid_data }`; бан из
  `BadDataException` инкрементит `bans_invalid_data`.

**Тесты:** новый `Test_BlockSyncPolicy` (высота/cooldown/правило фрода/счётчики/
backoff/copy-семантика статуса), набор — **146 кейсов / 1645 проверок PASS**.

**Живой прогон:** чистый HOME `pp-g38` (порты 13474/13419, 4 живых пира,
PEERS-БД создаётся заново — ручной очистки в процессе нет), бинарь
`D657EF90…`. Критерий REQ (≥30 мин догона, ноль `Banning peer … fraud height`
по таймаутам, `FULLY_SYNCED`) — **IN PROGRESS на момент записи**; результат
будет в следующей записи/`GAP_MATRIX` G-38.

**Живой журнальный resume (из этого же раунда):** после убийства узла в середине
PIBD и рестарта — **0 повторных `OutputBitmapSegment`** (bitmap воспроизведён из
журнала), журнал продолжил расти 13.9 МБ → 1.6 ГБ; затем пир перестал отдавать
`tree 2 idx 1229` → retry limit → fallback. То есть replay работает, а
устойчивость PIBD к стопору пира — отдельный дефект **G-43** (OPEN).

Тесты: **146 кейсов / 1645 проверок PASS**; `GrinNode.exe` `D657EF90…`,
`Tests.exe` `CA208907…`.

## E37. G-38: живой критерий 30 минут — PASS (с оговоркой); счётчик таймаутов исправлен

**Окно 14:09:18–14:41:29 (32 мин), узел `pp-net`, бинарь с фиксом G-38
(`D657EF90…`):**

* PEERS-БД очищена **однократно до старта** (подготовка HOME), в процессе —
  никакой ручной очистки;
* старт: `chain=3316636`, сеть `3316636` → сразу `FULLY_SYNCED` (отставание
  состояния было ~75 блоков), далее узел шёл за сетью;
* конец окна: `chain=network=3316673`, `FULLY_SYNCED`, 2 пира;
* в логе **ноль** строк `Banning peer … fraud height` и вообще **ноль**
  `Banning peer` за окно; таймауты были, но приводили к повтору/переназначению;
* счётчики `/v1/status` доступны: `block_sync.requests/timeouts/reassignments/
  bans_fraud_height/bans_invalid_data`.

**Найденный дефект наблюдаемости:** старый бинарь считал таймаут на каждом
тике sync-цикла (59 запросов → 8157 таймаутов) и засорял лог. Исправлено:
`TIMEOUT_COUNTED` в `RequestedBlock` — таймаут учитывается один раз на запрос
(сбрасывается при переназначении); новый бинарь `35970767…`, тесты 146/1645.
После рестарта на новом бинаре `pp-net` снова `FULLY_SYNCED`, счётчики
остаются приблизительными (пути `IsBlockSyncDue`/`RequestBlocks` считают
независимо), но без лавины.

**Оговорка (честно):** это окно проверяет «нет fraud-банов на таймаутах,
восстановление и следование за сетью», но стартовый догон был коротким
(~75 блоков), потому что состояние уже было валидным. Полный догон **с
PIBD-горизонта** тем же бинарём идёт на свежем `pp-g38` (заголовки
синхронизируются; после PIBD/валидации будет отдельная запись). G-42
(legacy `RequestState` банит за `BadTxHashSet`) и G-43 (PIBD-стопор на
сегменте) остаются открытыми.

Артефакты: `GrinNode.exe` `35970767…`, `Tests.exe` `A644DE5E…`;
набор **146 кейсов / 1645 проверок PASS**.

## E38. G-42/G-43: ротация пира вместо бана и вместо fallback — DONE (код), live IN PROGRESS

* **G-42 (legacy state sync):** `StateSyncer::RequestState` больше не банит
  медленного пира (`BadTxHashSet`), а пишет `Rotating txhashset sync peer` и
  берёт следующего; реальный невалидный txhashset по-прежнему банится в
  `TXHASHSET_PIPE` при ошибке валидации.
* **G-43 (PIBD-стопор):** при `PibdSync::Failed` StateSyncer сначала пытается
  **повторить PIBD с другим PIBD-совместимым пиром** (`RotatePibdPeer`,
  множество `m_pibdPeersTried` для текущего горизонта), и только когда все
  способные пиры перебраны — `DisablePibd()` и legacy-фолбэк. При смене
  горизонта список испытанных пиров сбрасывается; запросы шлются выбранному
  `m_pibdPeer` (а не «лучшему на каждом тике»).
* Проверки: сборка `/W4 /WX`, набор **146/1645 PASS**. Живая проверка ротации
  ожидается на `pp-g38` (сейчас синхронизация заголовков); legacy-прогон —
  G-41.

Артефакты: `GrinNode.exe` `35970767…`, `Tests.exe` `A644DE5E…`.

## E39. БД: рассинхронизация header MMR / chain-файлов после аварии — починка PASS (unit + live injection); атомарная запись — OPEN

**Инцидент.** Узел `pp-g38` (свежий HOME, синхронизация заголовков) аварийно
завершился на ~3.29 млн заголовков; перезапуск того же HOME падал через ~10 с
с `0xC0000005` (access violation), без stderr и события Windows. Удаление
каталога `NODE` возвращало работоспособность → повреждено сохранённое
состояние, не бинарь.

**Причина.** `NODE/CHAIN/header_mmr.bin` (`HashFile`) и
`candidate/confirmed.chain` (`DataFile<32>`) — три независимых append-only
файла, коммитятся раздельно (`HeaderMMR::Commit`, `Chain::Commit`,
`ChainStore::Commit`). Kill между коммитами оставляет их на разных высотах;
`BlockChain::Create` не сверял `leaf count` MMR с высотой chain (открытый
пункт G-23), и чтение заголовка за пределами MMR давало AV. Подробный разбор,
формат файлов и требование к записи — `DB_CONSISTENCY.md`.

**Сделано (mitigation).**
* `IHeaderMMR::GetLeafCount()`; чистая `ChainConsistency::ComputeChainRepair`
  (приоритет — chain-индекс: лишние листья MMR отбрасываются; если chain
  впереди MMR — обе цепочки откатываются до MMR; confirmed не может быть
  впереди candidate).
* `BlockChain::Create` применяет план **до** `ChainState::Create`, пишет
  `WARNING "Chain stores inconsistent … repairing to N entries"` и
  `"Chain stores repaired."`.
* NULL-guard в `Ping`-обработчике: нет confirmed tip → WARNING, без Pango и без
  падения.
* `Test_ChainConsistency`: 4 кейса; набор **150 кейсов / 1661 проверка PASS**.

**Живая проверка починки (инъекция повреждений в копии HOME):**
* MMR впереди: `leaves 830465, candidate 829965` → `repairing to 829965`,
  `Chain stores repaired.`, процесс жив;
* chain впереди: `leaves 830215, candidate 830465` → `repairing to 830215`,
  процесс жив, старт без AV.
Оба случая — с бинарём `25B8A7BE…`.

**Что ещё нужно (в документ и в работу после основной):** сделать запись
краш-согласованной — единый checkpoint (`mmr_leaves`, высоты, CRC) с
temp+rename/RocksDB-батчем, порядок fsync, инвариант на каждом коммите,
crash-инъекции, копия повреждённых файлов в `NODE/recovery-*`, счётчик
`db_repaired` в `/v1/status`. Это **G-48 OPEN**; текущая починка лишь
восстанавливает согласованность при старте (и может откатить часть уже
скачанных заголовков).

Артефакты: `GrinNode.exe` `25B8A7BE…`, `Tests.exe` `42B21046…`; набор
**150 кейсов / 1661 проверка PASS**.

## E40. PIBD-start: NULL-пир после сброса горизонта — регрессия найдена живьём и исправлена

**Симптом.** После выхода на типсу (3 316 803 заголовка) узел `pp-g38`
падал с `0xC0000005` ровно при старте PIBD (в каталоге уже появлялся
`pibd_resume/manifest`, строка `Starting PIBD state sync` не успевала
записаться).

**Причина.** В фиксе G-43 при смене горизонта выполняется
`m_pibdPeersTried.clear(); m_pibdPeer = nullptr;` для переизбрания пира, но
сразу после этого старый код делал
`m_pibdPeersTried.insert(m_pibdPeer->GetIPAddress().Format())` — разыменование
`nullptr`. Это была **регрессия, внесённая вместе с ротацией**, а не проблема БД.

**Исправление.** После сброса горизонта пир выбирается заново
(`m_pibdPeer = FindPibdPeer(true)`), с проверкой на `nullptr` **до**
`insert`/`Start`; при отсутствии пира — `NotAttempted` (legacy-путь).
Проверки: сборка `/W4 /WX`, **150 кейсов / 1661 проверка PASS**.

**Живой результат.** Перезапуск: заголовки 3 316 803 = сеть,
`Starting PIBD state sync at height 3313440 (hash 84d7ac67…), peer
204.168.202.205`, статус `DOWNLOADING_TXHASHSET` — без падения.

Артефакты: `GrinNode.exe` `8E6ACBBD…`, `Tests.exe` `69E87895…`.

## E41. PIBD: progress-based fallback, живая ротация пира и replay журнала

**Изменение планировщика.** `PibdScheduler` больше не завершает сессию после
3 попыток одного сегмента (это давало fallback через ~60 с при живом прогрессе
в остальном): сегмент ретраится с backoff (20/40/60 с, кап 60 с), а сессия
падает только если **нет ни одного принятого ответа** дольше
`TXHASHSET_ZIP_FALLBACK_TIME` (660 с). Это соответствует таймеру Rust.
Тесты обновлены (`RetriesThenFails`: попытки 1/2/3, затем fail только на 661 с);
набор **150 кейсов / 1663 проверки PASS**.

**Живой прогон `pp-g38` (после выхода на типсу, 3 пира):**
* PIBD скачал ~1.9 ГБ журнала за ~10 минут (bitmap 155 КБ, output 163 МБ,
  kernel 276 МБ, rangeproof 1.43 ГБ);
* пир 204.168 перестал отвечать → через 660 с без прогресса:
  `PIBD failed: no PIBD progress within the fallback window`;
* сработала **G-43-ротация**: `RotatePibdPeer … retrying with another peer
  66.154.127.84` — живое подтверждение, что fallback/disable больше не
  происходит после первого стопора;
* 66.154 (cap 0xDF) сегменты не отдавал; 157.90 без PIBD; после рестарта
  204.168 отвечает `PerformOutboundHandshake: Shake message not received` —
  **троттлинг на стороне пира** после многих часов interop, не падение узла;
* **replay журнала подтверждён**: после рестарта старые сегменты не
  перекачивались, журнал продолжил расти (155 КБ/39 МБ/12.8 МБ/95 МБ →
  155 КБ/276 МБ/163 МБ/1.43 ГБ).

**Честный итог:** полный догон с PIBD-горизонта в этом прогоне **не завершён**
(пир перестал обслуживать); ранее полный цикл PIBD→валидация→`FULLY_SYNCED`
был подтверждён (E34), resume-replay и G-43-ротация — здесь. Живой legacy
fallback (G-41) и полное завершение resume остаются **NOT VERIFIED**.

Артефакты: `GrinNode.exe` `C69A3069…`, `Tests.exe` `D1F122F6…`.

## E42. Полный живой цикл с PIBD-горизонта: download (с ротацией) → активация → валидация → `FULLY_SYNCED` — PASS

Узел `pp-g38` (свежий HOME, заголовки на типсе), бинарь `CADB5A0C…`,
testnet. Хронология:

* 16:31 заголовки 3 316 803 = сеть; старт PIBD (горизонт 3 313 440, hash
  `84d7ac67…`); первый прогон упал на retry limit (до фикса планировщика);
* после правок/рестартов и очистки PEERS: 17:42 PIBD с 4 пирами; 17:54
  `PIBD failed: no PIBD progress within the fallback window` →
  **`RotatePibdPeer … retrying with another peer 66.154.127.84`**;
* 17:59:46 `Validating TxHashSet for block 84d7ac67…` — загрузка **завершена**
  (3.07 ГБ, журнал resume ~1.9 ГБ не потерян при ротации/рестартах),
  активация `pibd_activation` выполнена;
* валидация: kernel history 17:59:50, kernel sums 18:00:56, range proofs
  18:03:36 → PASS (~35 мин, пик ~9.5 ГБ RSS);
* догон блоков: 18:37 `SYNCING_BLOCKS` (chain 3 314 350), при догоне
  **4 222 reassignment, 3 512 запросов, 0 `Banning peer … fraud height`**;
* **19:02:00: `chain = network = 3 316 955`, `FULLY_SYNCED`**
  (`evidence/g38_full_horizon_fully_synced.json`, hash `62a98ca5…`);
* рестарт на финальном бинаре: **`FULLY_SYNCED` 3 316 959**, hash
  `00d67d6b…`, PIBD не перезапускается
  (`evidence/g38_restart_after_full_sync.json`).

**Итог:** строгий сценарий «старт с горизонта → ротация посреди загрузки →
активация → валидация → `FULLY_SYNCED`» **выполнен живьём**. G-38 подтверждён:
4222 переназначения без единого fraud-бана. G-43 — ротация и завершение
подтверждены. После прогона в код добавлены порог скорости (10 сегментов/180 с),
3 круга пиров с backoff (30/60/120 с) и исправлен флаг `TIMEOUT_COUNTED`
(в live-бинаре счётчик таймаутов завышался до 1.8M; в финальном бинаре
исправлено). Эти изменения покрыты unit-тестами, в live-прогоне не участвовали.

Артефакты: `GrinNode.exe` `55030297…`, `Tests.exe` `0EFC0BF4…`; набор
**151 кейс / 1667 проверок PASS**.

## E43. G-48: атомарный checkpoint записи БД — DONE (unit + live)

**Реализация.**
* `src/BlockChain/DbCheckpoint.h` — маленький файл `NODE/CHAIN/chain.checkpoint`
  (52 байта: magic, version, sequence, mmr_leaves, candidate/confirmed entries,
  FNV-1a checksum), запись через временный файл + rename.
* `ChainState::Commit()` после успешного коммита chainStore/blockDB/headerMMR/
  txHashSet пишет checkpoint с их высотами (sequence++). Если коммит не
  происходил — checkpoint не трогается.
* Загрузка: `BlockChain::Create` читает checkpoint и чинит все три хранилища
  **до него** (`ComputeChainRepairToCheckpoint`): никогда не пишет вперёд,
  усекает MMR/цепочки до минимума фактического и checkpoint-состояния; если
  checkpoint отсутствует/повреждён — прежняя эвристика по счётчикам. После
  починки/проверки сразу пишется свежий checkpoint.
* Устранён self-deadlock: `GetLeafCount()` нельзя вызывать после коммита, пока
  batch-writer держит write-lock; после коммита используется инвариант
  `mmr_leaves == candidate_entries`.

**Тесты:** `Test_DbCheckpoint` (roundtrip, замена, corruption, truncation,
invalid heights), `ChainConsistency::Checkpoint*` (усечение stores впереди,
запрет записи вперёд, clamp confirmed, потеря chain-коммита при живом MMR);
набор **158 кейсов / 1707 проверок PASS**.

**Живая проверка:**
* жёсткий kill `pp-g38` → рестарт: `FULLY_SYNCED`, checkpoint `sequence 26 → 27`,
  без repair (согласованное состояние);
* инъекция: в копии HOME `candidate.chain` усечён на 1000 записей при живом
  MMR/checkpoint → старт:
  `Chain stores inconsistent (header_mmr leaves 3317028, candidate 3316028,
  confirmed 3317028) … repairing to mmr 3316028, candidate 3316028,
  confirmed 3316028 entries using the checkpoint.` + `Chain stores repaired.`,
  процесс жив, checkpoint обновлён.

**Остаточные ограничения:** порядок fsync между тремя файлами по-прежнему
определяется ОС (checkpoint уменьшает окно до одного rename), побайтовая
ревалидация MMR/хешей — G-23, бэкап миграций — G-22. Подробности —
`DB_CONSISTENCY.md`.

Артефакты: `GrinNode.exe` `BF7F3F9E…`, `Tests.exe` `09D1C6C3…`.

## E44. G-16 лимит TxHashSet, G-23 структурная проверка MMR, recovery-копии и `db_repaired`

**Сделано.**
* **G-16**: `TxHashSetLimits::MAX_ZIPPED_SIZE = 8 GiB`; `TxHashSetPipe` до создания
  файла и записи отвергает `zipped_size == 0` или `> 8 GiB`, ставит
  `TXHASHSET_SYNC_FAILED` и банит пира `BadTxHashSet` (unit `TxHashSetLimits`).
* **G-23 (структурная часть)**: `MmrStructure::IsValidNodeCount` проверяет
  формулу `S = 2L − popcount(L)`; `BlockChain::Create` до вывода высот усекает
  невалидный MMR до ближайшего корректного размера, с защитой «не усекать, если
  leaves не выводится». Добавлены `GetNodeCount()`/`GetLeafCount()`.
* **Recovery-копии и счётчик**: `DbRecovery::CopyForRecovery` сохраняет
  `header_mmr.bin/candidate.chain/confirmed.chain/chain.checkpoint` в
  `NODE/recovery-<ts>` перед любой починкой; `/v1/status` отдаёт `db_repaired`
  и `db_last_repair`. Копирование переписано с `std::filesystem::copy_file`
  на потоковое: `CopyFileW` не смог прочитать открытый HashFile и оставил
  целевую копию 0 байт (поймано живой инъекцией).

**Инцидент (найден и исправлен в этом же раунде).** Первая версия бинарного
поиска в `MmrStructure` имела верхнюю границу `(nodes+1)/2+1`, которая для
больших MMR меньше истинного `L` (≈ `(nodes+popcount)/2`). Здоровый MMR на
6 634 093 узла был признан невалидным, «починен» до 0 листьев, после чего
цепочки усечены до genesis — потеряны заголовки в тестовом HOME (не пользовательские
данные). Исправлено: `high = nodes`; регрессионный тест для 3.3M листьев;
guard `validLeaves > 0`. Узлы перesинхронизируют заголовки, `db_repaired`
после рестарта с корректным кодом = 0.

**Тесты:** `MmrStructure` (4 кейса, включая round-trip 3 317 054 листьев),
`TxHashSetLimits`, `DbRecovery`; набор **165 кейсов / 1764 проверки PASS**.

**Осталось по G-23:** проверка хешей/корней (опция `--validate-on-load`) —
структурная проверка закрыта, побайтовая нет.

Артефакты: `GrinNode.exe` `76BAFF91…`, `Tests.exe` `D07B6608…`.

## E45. G-41 legacy fallback: попытка живьём, округление архива и найденный дефект serving (G-52)

* **Клиентская совместимость:** `StateSyncer::RequestState` запрашивал
  `headerHeight − STATE_SYNC_THRESHOLD` **без округления** до
  `TXHASHSET_ARCHIVE_INTERVAL`; Rust-архив существует только на кратной 720
  высоте (`txhashset_archive_header`). Исправлено округлением вниз.
* **Живой прогон:** заголовки 3 317 080, `PIBD_ENABLED=false`,
  `Requesting TxHashSet for the first time`, статус `DOWNLOADING_TXHASHSET`.
  Реальные пиры после часов interop отклоняют handshake
  (`Shake message not received`) — загрузка zip с Rust-пира не началась.
* **Self-interop (вторая C++-нода как сервер, loopback):** сервер получает
  `TxHashSetRequest`, но снапшот исторического архива не создаётся:
  `Snapshot TxHashSet for height 3314160 failed: Rewind: Spent output not
  found for Commitment{08336ff8…}`. То есть наш сервер умеет отдавать только
  снапшот текущего/близкого состояния, не архив на горизонте — **новый дефект
  serving, G-52**.
* **Диагностика:** ошибка снапшота больше не проглатывается
  (`LOG_ERROR` с высотой/исключением), логируется путь и размер созданного
  снапшота.
* **G-17 (rate limiting):** существующий механизм подтверждён и
  задокументирован: `Socket::RateCounter` считает отправленные/принятые байты
  за минуту; `Connection::ExceedsRateLimit()` (>500 в минуту в любую сторону)
  вызывается в цикле соединения и банит `Abusive`. Отдельная доработка не
  требуется; порог можно настраивать позже.

**Тесты:** без изменений в наборе — **165 кейсов / 1767 проверок PASS**.

**Итог по G-41:** живой legacy-fallback **не завершён** (Rust-пиры недоступны
из-за троттлинга, а собственный сервер не отдаёт исторический архив — G-52).
Клиентское округление высоты — необходимое исправление совместимости.

Артефакты: `GrinNode.exe` `6988C8E7…`, `Tests.exe` `63077487…`.

## E46. G-52: historical snapshot — точный диагноз (OPEN)

Сервер собран из **PIBD-активированного** состояния (`pp-g38`: PIBD-горизонт
3 313 440, DB/state от полного цикла E42), chain-файлы восстановлены из
recovery, MMR — из `pp-resume`; узел снова `FULLY_SYNCED` (3 317 119).

Запрос клиента: `TxHashSet snapshot: flushed height 3317111, requested height
3314160`. Rewind идёт вниз и падает:

```
Spent output not found for commitment Commitment{08336ff8…}
in block 3316349 (1 input(s), 0 spent position(s), target height 3314160)
```

То есть в `SPENT_OUTPUTS` блок 3 316 349 имеет **0 записей при 1 входе**.
Это не направление rewind (flushed выше target) и не отсутствие блока: у блока
просто нет сохранённой spent-позиции. Тот же сбой воспроизводится на
PIBD-состоянии, значит история spent-позиций имеет разрыв ниже 3 316 349
(вероятно, блок попал в состояние через активацию/восстановление, а не через
`TxHashSet::ApplyBlock` с записью `AddSpentPositions`).

**Следствия и варианты исправления (G-52):**
1. Хранить **архив на горизонте** при активации состояния (как Rust:
   `txhashset.zip` на archive height) и отдавать его по совпадению hash —
   без rewind. Это устраняет зависимость от полной истории spent-позиций.
2. Восстанавливать/персистить spent-позиции для всех блоков от базы состояния
   (сканирование/перестройка индекса) — дороже, но чинит и snapshot, и rewind.
3. Как минимум: сервер должен определять **нижнюю достижимую высоту** по
   состоянию и отказывать ниже неё понятной ошибкой (не пытаясь rewind),
   а клиент — переключаться/сообщать о недоступности архива.

**Итог:** живой legacy-fallback (G-41) остаётся **NOT VERIFIED**; клиентская
часть (округление высоты до 720, запрос, таймауты) проверена, серверная
историческая выдача архива — **G-52 OPEN**.

Артефакты: `GrinNode.exe` `45662C8D…`, `Tests.exe` `F7D1848B…`;
набор **165 кейсов / 1770 проверок PASS**.

## E47. G-22: бэкап БД перед миграцией — DONE

`DbBackup` перед миграцией схемы копирует каталог БД в
`NODE/DB.backup-v<version>-<timestamp>` (рекурсивно, потоково, LOCK
пропускается). Если копия не удалась — миграция **прерывается**
(`Database backup failed`), чтобы не мигрировать единственную копию.
Fresh DB (version 0) не бэкапится: мигрировать нечего.

* `BlockChain::Create`: `if (db_version >= 1 && db_version < 3)` → backup,
  лог `Database vN backed up to …`, иначе abort.
* Тесты: `DbBackup` (рекурсивное копирование, LOCK, отсутствующая БД);
  полный набор **167 кейсов / 1783 проверки PASS**.
* Ограничение: бэкап делается потоковым копированием открытой RocksDB
  (LOCK исключён); при экзотических блокировках возможен отказ — в этом
  случае узел не мигрирует, а сообщает об ошибке.

Артефакты: `GrinNode.exe` `ADBB9D92…`, `Tests.exe` `8E0F4F63…`.

---

## Сводка

| Проверка | Статус |
|---|---|
| Идентичность baseline исходникам v1.2.9 | PASS |
| Дефект `ByteBuffer` воспроизведён и исправлен | PASS |
| Проводной формат capabilities/ID/лимитов | PASS (compile-check) |
| Сборка зависимостей (offline, static CRT) | PASS |
| Сборка `GrinNode.exe` + `Tests.exe` (`/W4 /WX`) | PASS |
| Полный набор тестов (150 / 1661) | PASS |
| PIBD проводной слой (модели + сообщения 21–28) | PASS (unit) |
| PIBD верификация сегментов (`Root`/`Validate`/proof, bitmap→chunk) | PASS (unit) |
| PIBD генерация сегментов и обратная сборка дерева | PASS (unit) |
| PIBD pruned-сегменты и bitmap-aware верификация | PASS (unit) |
| PIBD фаза 1: сборка output-битмапа | PASS (unit) |
| PIBD оркестрация загрузки (bitmap → три дерева) | PASS (unit) |
| PIBD сессия и планировщик (таймауты/повторы/fallback) | PASS (unit) |
| PIBD desegmenter-ядро (снапшот, пересборка kernel-дерева) | PASS (unit) |
| PIBD пересборка прореженного дерева (pruned-хеши) | PASS (unit) |
| PIBD запись состояния в файловый PMMR (staging) | PASS (unit) |
| PIBD атомарная активация и восстановление | PASS (unit) |
| PIBD выдача сегментов из файловых PMMR (прореженная) | PASS (unit) |
| PIBD сервис полного цикла (`PibdSync`) | PASS (unit) |
| PIBD модель корней сверена с Rust v5.5.2 (E26) | PASS (unit) |
| Smoke-запуск ноды (floonet) | PASS |
| Живая синхронизация заголовков с testnet (Rust-совместимая сеть) | PASS (E31) |
| PIBD desegmenter/planner/serving | PASS (unit) |
| Живой serving: Rust-пир запрашивает сегменты у нас | NOT RUN (только unit, `Test_PibdServing`) |
| Живой PIBD: загрузка+активация состояния | PASS (E32) |
| Валидация активированного TxHashSet (kernel history/sums/rangeproofs) | PASS (E33) |
| Догон блоков после активации | PASS (исторически, с обходом G-38): `FULLY_SYNCED` на 3316561 (E34) |
| G-38: ложные баны и залипание block sync | PASS (unit + 32-мин живое окно без fraud-банов, `FULLY_SYNCED`; полный догон с горизонта — отдельно, G-43) (E37) |
| БД: починка рассинхронизации MMR/chain при старте | PASS (unit + live-инъекция в обе стороны) (E39) |
| БД: атомарное формирование (checkpoint записи) | OPEN (G-48; требование в `DB_CONSISTENCY.md` §4) |
| PIBD resume (journal) | PASS (unit); live replay PASS (0 повторных bitmap-запросов), полное завершение NOT RUN (G-43) |
| Сохранение состояния между рестартами | PASS (E34: FULLY_SYNCED без нового PIBD) |
| PIHD | Не входит в v5.5.2 (enum до 28); в ветке `staging` реализовано целиком (PR #3879/#3882), для совместимости с v5.5.2 не требуется — см. `PIHD_UPSTREAM.md` |
| Версия ПО | PASS: `Grin++ 1.2.9` (G-25), закреплено тестом |
| Legacy-фолбэк txhashset.zip | IN PROGRESS: конфиг `PIBD_ENABLED=false`, живой узел `pp-fallback` |

## E48. Восстановление без потери заголовков, точное правило FraudHeight и архив на горизонте

**1. Header recovery без потери данных (G-23/G-46/G-48).**
- При загрузке: невалидный размер MMR усекается до ближайшего корректного; если MMR
  **отстаёт** от candidate chain — он **перестраивается из block DB** (заголовки не
  теряются), если опережает — удаляются только лишние листья; `confirmed` не может
  быть впереди candidate.
- Перед любой модификацией recovery-копия **обязательна**: `DbRecovery` копирует все
  существующие файлы потоково, проверяет размер копии; при любой ошибке ремонт
  **прерывается** (никакого усечения без проверенного бэкапа).
- Если chain-индекс позади checkpoint (потеряна история fork-choice), старт
  **прерывается** с понятной ошибкой, а не усекает другие хранилища.
- **Живая проверка:** копия `pp-net`, `header_mmr.bin` усечён на 2000 листьев:
  `Rebuilding header MMR from the block DB: leaves 3316116 -> heights 3316116-3317117`,
  узел жив, `header=chain=3317117`, `candidate.chain` не изменился, `db_repaired=1`.
- Если block DB не содержит нужного заголовка (смешанные HOME), rebuild **безопасно
  падает**, не теряя chain-файлы.

**2. FraudHeight — точное правило Rust (G-38).** Живой сервер после 120 с без
прогресса забанил 3 пира (107.172/146.59/203.88): наше условие было «высота ИЛИ
difficulty», а Rust (`header_sync.rs:136-137`) требует **строго больший TD**
(`header_head.total_difficulty < peer.total_difficulty`). Правило исправлено на
TD-only (height больше не считается доказательством), тест обновлён; повторные
ложные баны исключены.

**3. Архив на горизонте (G-52).** При успешной активации PIBD состояние
снимается в `NODE/TXHASHSET/archive/TxHashSet.<height>.<hash>.zip`
(`ArchivePibdState`), а `TxHashSetPipe` отдаёт сохранённый архив по hash **без
исторического rewind**. Код собран и покрыт существующими тестами; живая выдача
архива/legacy-клиент ещё **NOT VERIFIED** (нужен свежий PIBD-прогон до активации и
клиент на том же горизонте).

**Тесты:** 167 кейсов / 1804 проверки PASS. Артефакты: `GrinNode.exe`
`7997CA50…`, `Tests.exe` `45C652E6…`.

## E49. G-21: атомарная активация legacy `txhashset.zip`

**Реализовано.**
* `TxHashSetZip`/`TxHashSetManager::LoadFromZip` принимают целевой каталог:
  zip распаковывается в `NODE/TXHASHSET.staging`, **живое состояние не трогается**.
* `TxHashSetProcessor::ProcessTxHashSet` валидирует staged-состояние; при ошибке
  staging удаляется, а живое состояние переоткрывается (узел продолжает работу).
* При успехе — атомарная замена: marker `NODE/txhashset_activation.pending`,
  `live → TXHASHSET.backup`, `staging → live`, удаление backup/marker; открытие
  установленного состояния и публикация block sums/output positions.
* `TxHashSetManager` при старте вызывает `TxHashSetStaging::Recover`: если live
  отсутствует — восстанавливает backup, удаляет staging/marker (краш между
  переименованиями безопасен).
* Тесты: `Test_TxHashSetStaging` (3 кейса: orphan staging, восстановление backup,
  сохранение установленного live); набор **170 кейсов / 1822 проверки PASS**.
* **Ограничение:** это атомарность установки legacy-архива; живой legacy-прогон
  (G-41) по-прежнему NOT VERIFIED из-за доступности пиров/ G-52.

Артефакты: `GrinNode.exe` `D2BBF889…`, `Tests.exe` `0B80AD8D…`.

## E50. G-04 PoW edge-bits и G-20 лимиты парсеров закрыты; замер синка идёт

**G-04 (PoW edge-bits).** `PoWValidator::IsSupportedEdgeBits` допускает ровно
secondary 29 и primary 31–63 и вызывается **до** диспетчеризации в Cuckoo;
значения ≥64 (сдвиг 64-битного слова) и 0/24/28/30 отвергаются. Unit:
`Test_PoWEdgeBits` (11 проверок), сверено с семантикой Rust
`is_primary()/is_secondary()` (`DEFAULT_MIN_EDGE_BITS = 31`,
`SECOND_POW_EDGE_BITS = 29`).

**G-20 (лимиты парсеров).** Ранее уже были `MAX_SEGMENT_READ_ITEMS`,
checked arithmetic, лимиты `SegmentProof`/`BitmapSegment`/`BitmapBlock` и
message-size cap; добавлена явная проверка `numLeaves <= remaining` в
`Segment::Deserialize` **до reserve**. Unit `Test_Segment` (21 кейс, включая
огромные counts и non-increasing positions). Fuzz остаётся вне матрицы
(фазы 5–7).

**Замер полного синка с нуля (`pp-archsrv`, старт 31.10 23:55:27).**
На 00:31:51 header = 1 906 304 / net 3 317 282 (36.4 мин); после переключения
на 204.168 скорость ~1 800 заголовков/с, до типсы ~13 мин; далее PIBD →
валидация → архив → догон. Итоговое время будет отдельной записью (E51).

**Тесты:** **171 кейс / 1836 проверок PASS**. Артефакты: `GrinNode.exe`
`D2BBF889…`, `Tests.exe` `0204966F…`.

## E51. fsync-порядок MMR/chain/checkpoint

`FileSync::Flush` (`CreateFileW` + `FlushFileBuffers` на Windows) вызывается в
`ChainState::Commit` для `header_mmr.bin`, `candidate.chain`, `confirmed.chain`
**перед** публикацией checkpoint, а `DbCheckpoint::Write` флашит временный файл
**до** rename. Порядок: коммиты writer'ов → flush трёх файлов → flush temp
checkpoint → rename. Каталоги на Windows отдельно не флашатся (нет API);
это задокументировано как остаточное ограничение. Тесты: **171 кейс / 1839
проверок PASS**.

## E52. Цель 8 исходящих пиров, диагностика и ревизия комментариев

**1. Seeder: цель 8 outbound (G-50).** Раньше `Seeder::Thread_Seed` держал ровно
`MIN_PEERS` (старые конфиги 2–3), поэтому узел стоял на 2–3 соединениях.
Добавлен `SeederPolicy::TargetOutbound(min, max)` = `max(min(8,max), min(min,max))`:
старый `MIN_PEERS=2` автоматически даёт цель 8, `MAX_PEERS` остаётся жёстким
потолком, а явный `MIN_PEERS>8` уважается. `ConnectionsToAdd` ограничивает
пачку 15. Unit `Test_SeederPolicy` (11 проверок). Живой эффект: **7–8
outbound** вместо 1–3; PIBD скачал **4988/4989 сегментов** и перешёл к
валидации TxHashSet (02:01:41).

**2. Диагностический REST-эндпоинт.** Явный
`GET /v1/peers/diagnostics` (REST-порт, не P2P 13414) отдаёт:
`totals` (connected/inbound/outbound/banned), `banned` (IP, причина, время),
`connected` (height, direction, capabilities, user_agent, protocol_version,
`is_download_source`), `download` (текущий PIBD-источник, сегменты, %, примерная
скорость seg/min, статус) и `versions`. PIBD-пир/прогресс добавлены в
`SyncStatus` и обновляются из `StateSyncer`. Это обычный диагностический API,
не часть P2P-совместимости.

**3. G-51: ревизия комментариев/секретов.** `AUDIT_COMMENTS.md` заполнен:
112 изменённых файлов, 0 кириллицы, 0 жёстких `C:\DHW`, 0 TODO в диффе
(8 переписаны/удалены, мёртвый код в `ChainState.h` удалён), 0 секретов
(131 совпадение — имена ключей/пути). `AUDIT_CHECKLIST.md` создан; остались
лицензии, README EN/DE/RU, финальные хеши/подпись.

**Тесты:** **172 кейса / 1859 проверок PASS**. Артефакты: `GrinNode.exe`
`9B67F5DE…`, `Tests.exe` `49D47100…`.

## E53. Полный синк, фикс архива на горизонте и повторный PIBD

**1. Полный синк дошёл до `FULLY_SYNCED`.** На `pp-archsrv` (старт 31.10
23:55:27) заголовки догнаны к 00:47, PIBD-загрузка/валидация с 01:59,
`PIBD state sync completed and validated` в 02:30:32, догон блоков и
**`FULLY_SYNCED` в 02:49** на высоте **3 317 412**. Суммарно ~2 ч 54 мин.
Оговорка: block DB в этом HOME была предзаполнена (копия `pp-net`), поэтому
«с нуля» здесь — заголовки + state/PIBD + валидация; чистая БД блоков не
перекачивалась.

**2. Найден и исправлен срыв архива (G-52).** `ArchivePibdState` вызывается
после успешной активации, но `TxHashSetManager::SaveSnapshot` ожидал legacy
`pmmr_leaf.bin`, тогда как PIBD пишет `pmmr_leafset.bin`; rename падал:
`Failed to rename ...\output\pmmr_leaf.bin ... file not found`. Исправление:
перед zip материализуется `pmmr_leaf.bin` из `LeafSet`/`Roaring`
(`BitmapFile::ToRoaring`), для output и rangeproof. Добавлено восстановление на
старте: если `pibd_state.complete` есть, а архива нет — один раз пробуется
snapshot. Повторный snapshot *позднего* состояния назад к горизонту упирается
в известный разрыв spent-позиций (`Rewind: Spent output not found`), поэтому
для архива нужен свежий PIBD-цикл; он запущен в 02:50 на том же HOME
(подтверждённый chain откатан к genesis, state удалён, заголовки сохранены).

**Тесты:** **172 кейса / 1862 проверки PASS**. Артефакты: `GrinNode.exe`
`C0FCCBE4…`, `Tests.exe` `1C2A1C91…`.

---

## E53. Node API: паритет с grin v5.5.2 (/v2/foreign и /v2/owner) — DONE (unit + build)

**Реализовано.** /v2/foreign дополнен до 13 методов эталона: get_pool_size,
get_stempool_size, get_unconfirmed_transactions (c60604f), get_pmmr_indices,
get_unspent_outputs (84f135e), get_outputs (8fa265f), get_kernel (474acc4),
get_blocks (fa663d5); /v2/owner получил get_status, get_peers,
get_connected_peers (3b4efd4). Самоописание GET /v1/ приведено к реальным
маршрутам, мёртвый mongoose-комментарий удалён (efe27b7).

**Детали.**
* ITxHashSet::FindKernelByExcess обходит kernel MMR по 1-based диапазону;
  get_kernel повторяет chain::get_kernel_height (min → previous_header.kernel_mmr_size+1,
  max → header(max).kernel_mmr_size, высота — бинарный поиск по kernel_mmr_size).
* get_unspent_outputs опирается на то, что TxHashSet::ApplyBlock удаляет листья
  выходов/rangeproof-ов при трате, поэтому в MMR остаются только невыплаченные.
* get_outputs/get_blocks используют общий PrintableSerialization.h
  (BlockHeaderPrintable с RFC3339, TxKernelPrintable, OutputPrintable со
  spent-детекцией через PMMR и fallback из блока).
* get_status отдаёт словарь sync-статусов grin (header_sync/ody_sync/
  waiting_peers/
o_sync/...), не легаси-имена /v1/status.

**Тесты.** Test_NodeApiPrintables (2 кейса, 5 проверок-секций): RFC3339,
KernelPrintable для plain/height-locked/coinbase. Полный набор:
**174 кейса / 1810 проверок PASS** в изолированном HOME
(C:\DHW\Gplus\.toolchain\testhome). Для сравнения, прошлый прогон в рабочем
HOME — 172 / 1862; разница в числе проверок объясняется условными кейсами,
которые при пустом HOME дают меньше утверждений.

**Сборка.** GrinNode.exe и Tests.exe слинкованы (/W4 /WX), build OK.

**Не подтверждено:** живые вызовы /v2/foreign и /v2/owner на запущенной ноде
(на момент записи нода не работала или работала на прежнем билде) — это
остаток за следующим прогоном.
## E54. G-52 и G-41 закрыты живым прогоном (архив → legacy-клиент → FULLY_SYNCED)

**G-52, сохранение архива.** После успешной PIBD-активации на горизонте
3 314 160 сохранён `NODE/TXHASHSET/archive/TxHashSet.3314160.1145e8daa674.zip`,
**3 117 206 023 байт**. По пути исправлены:
1. `SaveSnapshot` искал legacy `pmmr_leaf.bin`, а PIBD пишет `pmmr_leafset.bin` —
   теперь leaf материализуется из `LeafSet`/`Roaring` перед zip;
2. `(int)(zipped_size - received)` переполнялся на 3,1 ГБ (отрицательный размер →
   `vector too long`) — заменено на `size_t/uint64_t`;
3. `Zipper` писал `kernel\...`, `ExtractFile` искал `kernel/...` — переход на
   `generic_string()`; готовый архив перепакован с прямыми слэшами.

**G-52, живая выдача.** Сервер отдал архив loopback-клиенту: принято
3 117 206 023 байт, распаковка успешна (`LoadFromZip ... extracted successfully`),
валидация — `Successfully validated TxHashSet` в **06:19:00**.

**G-41, legacy fallback.** Клиент после валидации архива догнал блоки с внешних
пиров и достиг **`FULLY_SYNCED` на 3 317 664 в 06:53**. Полный путь
`txhashset.zip → validation → block sync` подтверждён живьём.

**Остаток (не блокирует G-52/G-41):** сервер `pp-archsrv` после активации
архива сам остался в `SYNCING_BLOCKS` на горизонте (клиент догонялся отдельно) —
нужен отдельный разбор block sync после активации.

**Тесты:** **174 кейса / 1879 проверок PASS**. Артефакты: `GrinNode.exe`
`39D2584A…`, `Tests.exe` `607EB484…`.

## E55. Живая выдача PIBD: найден дефект RangeProofSegment (G-09 остаётся PARTIAL)

Попытка живого PIBD-serving с отдельного C++-клиента против нашего узла:
* сервер принял `GetOutputSegment`, `GetRangeProofSegment`, `GetKernelSegment`
  и отдал `OutputBitmapSegment`, `OutputSegment`;
* клиент получил `RangeProofSegment`, но `ProcessMessage` упал:
  `Deserialization exception ... Proof of size 20998 exceeds the maximum`,
  после чего соединение закрыто (`resource deadlock would occur`), PIBD
  остановился на 8/4989 сегментах.

Значит **живой PIBD serving не работает для rangeproof-сегментов**; unit-тесты
покрывали bitmap/output и сериализацию, но не реальный rangeproof-сегмент от
PibdSegmentService. G-09 остаётся **PARTIAL**, live serving Rust/иному клиенту не
подтверждён. Нужен разбор `Segment<RangeProof>`/`SegmentGenerator` на живом
сегменте (позиции листьев/hash-позиции) перед повторным прогоном.

**Лицензии/аудит:** D5 PASS (11 standalone + 6 embedded notices, все 17
зависимостей), пакет `dist/audit-1.2.9-83783F38` готов; G-51 закрыт приёмкой владельца
(01.11.2026). Тесты 174 / 1879 PASS; артефакты без изменений.

## E56. G-09: точный дефект rangeproof-сегмента и актуализация артефактов

Перебор всех **1673** rangeproof-сегментов live state 3 314 880 дал точку отказа:
**idx 1100** — `Source is missing the sibling hash at position 4513785`. То есть
prune-состояние после PIBD не содержит hash, нужный `SegmentGenerator` для
построения proof; сервер не может корректно отдать этот сегмент (клиент получает
desync/`Proof of size ... exceeds the maximum`). Это не TCP и не `RangeProof`
сериализация: wire- и live-state round-trip тесты проходят.

G-09 остаётся **PARTIAL/FAIL live serving**. Нужно либо сохранять недостающие
sibling hashes в PibdStateWriter, либо отдавать rangeproof-сегменты из полного
snapshot, а не из prune-состояния.

**Тесты:** **176 кейсов / 1907 проверок PASS**. Артефакты: `GrinNode.exe`
`83783F38…`, `Tests.exe` `38B9AF99…`.

## E57. G-09: PIBD serving упирается в отсутствующие sibling hashes

После реального фикса `Socket.cpp` (mutex + активный буфер) клиент принимал
`RangeProofSegment` без ошибок десериализации и прошёл **4068/4990** сегментов.
Затем PIBD остановился: `PIBD failed: no PIBD progress within the fallback window`
(11:57:56). Сервер в это время:

```
Failed to serve Msg::GetRangeProofSegment: Source is missing the sibling hash at position 4988922
Failed to serve Msg::GetOutputSegment:     Source is missing the sibling hash at position 4993018
Failed to serve Msg::GetRangeProofSegment: Source is missing the sibling hash at position 4984827
```

То есть дефект не в кадре (socket fix подтверждён), а в **prune-state/serving**:
`PibdStateWriter`/PMMR не содержат hash, нужный `SegmentGenerator` для proof.
G-09 остаётся **PARTIAL/FAIL live serving**; для закрытия нужно либо сохранять
все требуемые sibling hashes при активации, либо отдавать сегменты из полного
snapshot, а не из prune-состояния.

**Тесты:** 176 / 1907 PASS. Артефакты: `GrinNode.exe`
`83783F38…`, `Tests.exe` `38B9AF99…`.

## E58. G-09: prune-aware proof обязателен; простой обход prune даёт неверный root

Попытка читать compacted sibling hash «как Rust get_from_file» (в обход
`PruneList::IsCompacted`) скомпилировалась, но живой клиент получил:
`Segment proof does not reconstruct the combined MMR root`. То есть hash,
прочитанный по shifted-индексу, не является нужным sibling root; простой обход
prune **некорректен**. Изменение откатано, чтобы не оставлять неверный serving.

Требуется полноценная Rust-совместимая генерация proof с учётом compacted
поддеревьев (`SegmentProof::generate` + prune-aware `get_hash`), либо выдача
сегментов из полного (не-pruned) snapshot. G-09 остаётся **PARTIAL/FAIL live
serving**; E56/E57/E58 фиксируют весь путь диагностики.
