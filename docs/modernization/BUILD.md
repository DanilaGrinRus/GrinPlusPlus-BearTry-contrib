# BUILD.md — сборка и тесты Grin++ в этом дереве

Штатная инструкция проекта (`BUILD.md` в корне) предполагает vcpkg 2020.02 и VS2019.
В этом окружении такой путь не работает: vcpkg-пин не знает VS2022/v143, а загрузка
пакетов через WinHTTP/Schannel изначально была заблокирована. Поэтому зависимости
собираются локально в sysroot из тех же зафиксированных версий, что и в
`vcpkg/` (пин `6185aa7` + overlay-порты проекта).

## 1. Зависимости (один раз)

```powershell
python tools/deps/build_sysroot.py                 # все зависимости
python tools/deps/build_sysroot.py --only rocksdb  # точечно
```

* Исходники качаются в `C:\DHW\Gplus\.toolchain\sources`, SHA-256 каждого архива
  дописывается в `SOURCES.sha256`.
* Версии совпадают с пином vcpkg: zlib/minizip 1.2.11, libsodium 1.0.18,
  secp256k1-zkp `5b39fbf`, rocksdb 6.6.4 `551a110`, civetweb `2c1caa6`,
  CRoaring 0.2.66, fmt 6.2.1, asio 1.12.2, mio `cafa313`, Catch2 2.12.1.
* Все библиотеки собираются со статическим CRT (`/MT`) — нода линкуется статически.
* Патчи сторонних исходников минимальны и объяснены в скрипте: asio standalone +
  `unistd.h` только вне Windows; rocksdb — отключён `thirdparty.inc` (NuGet-пути)
  и включён `ROCKSDB_INSTALL_ON_WINDOWS`/`WITH_MD_LIBRARY=OFF`.

## 2. Сборка

```powershell
pwsh tools/build/build.ps1                  # GrinNode + Tests
pwsh tools/build/build.ps1 -Target GrinNode
pwsh tools/build/build.ps1 -ConfigureOnly
```

* CMake-генератор: Visual Studio 17 2022, x64; `CMAKE_PREFIX_PATH` → sysroot.
* Флаги проекта не ослаблены: `/W4 /WX`, статический CRT.
* MSBuild запускается без `/m` и с `/p:TrackFileAccess=false`: в песочнице
  параллельные узлы MSBuild используют именованные каналы, а file tracker падает
  с `TRK0002`. Это ограничения сборочного хоста, а не проекта.
* Результат: `bin/Release/Release/GrinNode.exe`, `bin/Release/Release/Tests.exe`.

## 3. Тесты

```powershell
$env:HOMEDRIVE = 'C:'
$env:HOMEPATH  = '\DHW\.tmp\testhome'   # изоляция: тесты пишут в %HOME%\.GrinPP
New-Item -ItemType Directory -Force 'C:\DHW\.tmp\testhome' | Out-Null
bin\Release\Release\Tests.exe --reporter compact
```

Ожидаемый результат на текущем срезе: `Passed all 80 test cases with 558 assertions.`

## 4. Ограничения

* Скрипты рассчитаны на Windows x64 + MSVC 14.4x; Linux/macOS-ветка CI не проверялась.
* vcpkg-путь не удалён: при наличии VS2019/vcpkg файлы проекта остаются совместимыми.
* Пин vcpkg по-прежнему хранится только в CI (`.github/workflows/ci.yml`) — это
  открытая позиция G-24 в `GAP_MATRIX.md`.
