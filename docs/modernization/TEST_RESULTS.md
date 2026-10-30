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
| `bin/Release/Release/GrinNode.exe` | 7 225 344 | `A66125C33C3EBCD4B40360C9A35A473738C37DEB7D62E01C32D17D13CE1DB0F4` |
| `bin/Release/Release/Tests.exe` | 7 856 640 | `6BD54A96C337363984153A024665B00974BB0E7DB77DAA788283A443BC68A9E8` |

Сборка идёт с `/W4 /WX` (проектные флаги не ослаблены). Потребовались только
конфигурационные правки: `NOMINMAX`, идемпотентная установка `/W4 /WX`,
патчи сторонних заголовков (asio standalone/unistd) — см. `GAP_MATRIX` и `BUILD.md`.

## E8. Полный набор тестов Grin++ — PASS

```
$env:HOMEDRIVE='C:'; $env:HOMEPATH='\DHW\.tmp\testhome'   # изоляция данных тестов
bin\Release\Release\Tests.exe --reporter compact
→ Passed all 96 test cases with 656 assertions.
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

## E12. PIBD: проводной слой — PASS (unit)

Реализовано и покрыто тестами (`tests/src/PMMR/Test_Segment.cpp`, 16 кейсов):

* `MMRUtil`: `PeakMapHeight`, `InsertionToPmmrIndex`, `GetNumLeaves`,
  `BintreePostorderHeight`, `IsLeaf`, `IsLeftSibling`, `FamilyBranch`,
  `BintreeLeftmost/Rightmost` — порт `core/src/core/pmmr/pmmr.rs`.
* `SegmentIdentifier` (height u8 + idx u64; height > 32 отвергается — защита от
  сдвига, которого нет в Rust; 9/11/13 — рабочие высоты).
* `SegmentProof` (count ≤ 1 000 000 и count ≤ remaining/32).
* `Segment<T>` (позиции на проводе 1-based, строго возрастающие; лимиты из Rust).
* `BitmapBlock` (режимы Raw/Positive/Negative, chunk-count ≤ 16, индексы < n_bits)
  и `BitmapSegment` (height ≤ 13, ≥1 блок, полные промежуточные блоки, checked offset).
* Сообщения 21–28 (`SegmentRequestMessage`, `SegmentResponseMessage`) с round-trip
  для всех четырёх типов ответов.

Осталось по G-09: `Segment::root/first_unpruned_parent/validate`,
`BitmapSegment → Segment<BitmapChunk>`, desegmenter, планировщик загрузки, resume,
серверная выдача сегментов и включение `PIBD_HIST`.

---

## Сводка

| Проверка | Статус |
|---|---|
| Идентичность baseline исходникам v1.2.9 | PASS |
| Дефект `ByteBuffer` воспроизведён и исправлен | PASS |
| Проводной формат capabilities/ID/лимитов | PASS (compile-check) |
| Сборка зависимостей (offline, static CRT) | PASS |
| Сборка `GrinNode.exe` + `Tests.exe` (`/W4 /WX`) | PASS |
| Полный набор тестов (96 / 656) | PASS |
| PIBD проводной слой (модели + сообщения 21–28) | PASS (unit) |
| Smoke-запуск ноды (floonet) | PASS |
| Живая синхронизация с Rust v5.5.2 | NOT RUN |
| PIBD verify/desegmenter/planner/serving | NOT IMPLEMENTED |
| PIHD | NOT IMPLEMENTED |
