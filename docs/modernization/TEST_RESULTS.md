# TEST_RESULTS — доказательства итерации 2026-10-30

Правило: PASS только для фактически выполненной проверки. Пропуск/блокировка
не считаются успехом. Все команды приведены так, чтобы их можно было повторить.

Окружение: Windows x64; VS 2022 BuildTools 17.14 (MSVC 14.44.35207, toolset v143,
Windows SDK 10.0.26100); CMake 3.31.6 (из VS); Ninja; Python 3.10.11; git 2.55.0;
DSH sandbox `workspace-write`; рабочая копия `C:\DHW\Gplus\src\grinplusplus`.

---

## E1. Сверка снапшота с публичным Grin++ v1.2.9 — PASS

```
git -C C:\DHW\Gplus\src\grinplusplus remote add upstream https://github.com/GrinPlusPlus/GrinPlusPlus.git
git -C ... -c http.sslBackend=openssl fetch --depth 1 upstream refs/tags/v1.2.9
git -C ... diff --stat 5f511e6 FETCH_HEAD
```
Результат: 5 файлов, все — `dist/{linux,mac}/tor/*` (бинарники/LFS). Исходный код
идентичен тегу `v1.2.9` (`a0958a3`).

## E2. Воспроизведение дефекта `ByteBuffer` (до фикса) — FAIL (ожидаемо, дефект подтверждён)

Изолированный пробник `C:\DHW\Gplus\.toolchain\probe\varstr_probe.cpp`
(ручная сборка `build-probe.bat`, MSVC 14.44):

```
[1] declared=18446744073709551604  RESULT: exception: vector too long
[2] declared=18446744073709551590  RESULT: exception: vector too long
```
Ожидался `DeserializationException`, получен `std::length_error` — проверка
`m_index + length > size` обходится переполнением 64-бит, `MessageProcessor`
такой тип не ловит.

## E3. Проверка фикса `ByteBuffer` — PASS

Тот же пробник после перевода всех проверок на `RequireRemaining()`:

```
[1] RESULT: exception: RequireRemaining: Attempted to read past end of ByteBuffer.
[2] RESULT: exception: RequireRemaining: Attempted to read past end of ByteBuffer.
```

## E4. Новые regression-тесты (после фикса) — PASS

Файлы: `tests/src/Core/Serialization/Test_ByteBuffer.cpp`, `tests/src/Net/Test_P2PCompat.cpp`.
Изолированная сборка с in-tree Catch2 (`deps/Catch2/catch.hpp`), без vcpkg:

```
C:\DHW\Gplus\.toolchain\probe\build-unit.bat
C:\DHW\Gplus\.toolchain\probe\unit\unit_tests.exe --reporter compact
→ Passed all 7 test cases with 35 assertions.
```

## E5. Те же тесты против исходного `ByteBuffer.h` — FAIL (доказательство ценности теста)

`git show 5f511e6:include/Core/Serialization/ByteBuffer.h` подложен в отдельный
include-каталог перед основным:

```
Test_ByteBuffer.cpp(46): failed: unexpected exception with message: 'vector too long'
Test_ByteBuffer.cpp(56): failed: unexpected exception with message: 'vector too long'
Failed 2 test cases, failed 2 assertions.
```

## E6. Проводной формат P2P (capabilities, PIBD ids, лимиты) — PASS (компиляция)

`check-p2p-headers.cpp` + `build-headers.bat`: `static_assert` на биты 0x01..0x40,
IDs 21..28, размеры 41 / `2*MAX_BLOCK_SIZE`, а также на то, что `ADVERTISED` не
содержит PIBD-битов. Компиляция: exit 0.

## E7. Сборка baseline `GrinNode.exe` / `Tests.exe` — BLOCKED

| Шаг | Результат |
|---|---|
| vcpkg-пин из CI (`6185aa7`, 2020.02.04) | требует VS2019/v142; в окружении VS2022/v143 |
| сборка `vcpkg.exe` из `toolsrc` (MSBuild v143, SDK 10.0.26100) | PASS (`vcpkg.exe`, 1 254 912 байт) |
| `vcpkg install zlib --triplet x64-windows-static` | **FAIL**: `WinHttpSendRequest() failed: 12185` |
| диагностика TLS | Schannel: `AcquireCredentialsHandle failed: SEC_E_NO_CREDENTIALS`; git/curl/vcpkg не ходят по HTTPS |
| обходы | `git -c http.sslBackend=openssl` — работает; Python/OpenSSL — работает |

Причина: песочница DSH блокирует доступ Schannel к пользовательским крипто-каталогам
(запись вне workspace запрещена). Зависимости штатным vcpkg-путём не скачиваются.
Следствие: `GrinNode.exe`, `Tests.exe` и `ctest` **не получены**, baseline-тесты
Grin++ не выполнялись.

## E8. Прогон существующего набора тестов Grin++ (46 файлов) — NOT RUN

Требует сборки (см. E7).

## E9. Живая синхронизация с нодой Grin v5.5.2, reorg, crash-тесты — NOT RUN

Требует сборки и второй ноды; в этой итерации недоступно.

---

## Сводка

| Проверка | Статус |
|---|---|
| Идентичность baseline исходникам v1.2.9 | PASS |
| Дефект `ByteBuffer` воспроизведён | PASS (дефект подтверждён) |
| Фикс `ByteBuffer` + regression-тесты | PASS |
| Проводной формат capab./ids/лимитов | PASS (compile-check) |
| Полная сборка Grin++ | BLOCKED |
| Полный набор тестов Grin++, live-сеть | NOT RUN |
