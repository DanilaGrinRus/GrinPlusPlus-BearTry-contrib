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
| `bin/Release/Release/GrinNode.exe` | 7 225 344 | `793E1DC24D92B7B6D7E7FD31BC2DC0CA3DD934C0D6D8171E13B303ACA9EC356B` |
| `bin/Release/Release/Tests.exe` | 8 034 816 | `7B21B7BDBA546D6A254729BF08966FA3017D4382AB1BAF4F7C65EA1578693A65` |

Сборка идёт с `/W4 /WX` (проектные флаги не ослаблены). Потребовались только
конфигурационные правки: `NOMINMAX`, идемпотентная установка `/W4 /WX`,
патчи сторонних заголовков (asio standalone/unistd) — см. `GAP_MATRIX` и `BUILD.md`.

## E8. Полный набор тестов Grin++ — PASS

```
$env:HOMEDRIVE='C:'; $env:HOMEPATH='\DHW\.tmp\testhome'   # изоляция данных тестов
bin\Release\Release\Tests.exe --reporter compact
→ Passed all 115 test cases with 894 assertions.
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

Осталось по G-09: desegmenter с prune shifts и записью в файловый PMMR,
таймауты/retry/resume в сетевом слое, fallback на txhashset.zip, серверная выдача
сегментов и включение `PIBD_HIST`.

---

## Сводка

| Проверка | Статус |
|---|---|
| Идентичность baseline исходникам v1.2.9 | PASS |
| Дефект `ByteBuffer` воспроизведён и исправлен | PASS |
| Проводной формат capabilities/ID/лимитов | PASS (compile-check) |
| Сборка зависимостей (offline, static CRT) | PASS |
| Сборка `GrinNode.exe` + `Tests.exe` (`/W4 /WX`) | PASS |
| Полный набор тестов (115 / 894) | PASS |
| PIBD проводной слой (модели + сообщения 21–28) | PASS (unit) |
| PIBD верификация сегментов (`Root`/`Validate`/proof, bitmap→chunk) | PASS (unit) |
| PIBD генерация сегментов и обратная сборка дерева | PASS (unit) |
| PIBD pruned-сегменты и bitmap-aware верификация | PASS (unit) |
| PIBD фаза 1: сборка output-битмапа | PASS (unit) |
| PIBD оркестрация загрузки (bitmap → три дерева) | PASS (unit) |
| Smoke-запуск ноды (floonet) | PASS |
| Живая синхронизация с Rust v5.5.2 | NOT RUN |
| PIBD desegmenter/planner/resume/serving | NOT IMPLEMENTED |
| PIHD | NOT IMPLEMENTED |
