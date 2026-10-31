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

---

## Сводка

| Проверка | Статус |
|---|---|
| Идентичность baseline исходникам v1.2.9 | PASS |
| Дефект `ByteBuffer` воспроизведён и исправлен | PASS |
| Проводной формат capabilities/ID/лимитов | PASS (compile-check) |
| Сборка зависимостей (offline, static CRT) | PASS |
| Сборка `GrinNode.exe` + `Tests.exe` (`/W4 /WX`) | PASS |
| Полный набор тестов (140 / 1603) | PASS |
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
| PIBD resume между рестартами | NOT IMPLEMENTED |
| Живой PIBD: загрузка+активация состояния | PASS (E32) |
| Валидация активированного TxHashSet (kernel history/sums/rangeproofs) | PASS (E33) |
| Догон блоков после активации | PASS с обходом G-38 (очистка банов + 12 пиров): `FULLY_SYNCED` на 3316561 (E34) |
| PIBD resume (journal) | PASS (unit); live NOT RUN |
| Сохранение состояния между рестартами | PASS (E34: FULLY_SYNCED без нового PIBD) |
| PIHD | N/A: в эталоне v5.5.2 нет сообщений PIHD (enum до 28); бит 0x80 — только детекция, не рекламируется |
| Версия ПО | PASS: `Grin++ 1.2.9` (G-25), закреплено тестом |
| Legacy-фолбэк txhashset.zip | IN PROGRESS: конфиг `PIBD_ENABLED=false`, живой узел `pp-fallback` |
