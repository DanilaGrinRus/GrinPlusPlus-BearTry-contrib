# FULL_ASAN — полная ASan-сборка проекта, TestsMini и найденный UAF (линия 3)

Дата: 2026-11-03. Репо HEAD `e0a5d2d…` + чужие незакоммиченные изменения (см.
`git_status` в манифестах). Отдельное дерево и build dir, shared build не
использовался, production-код не изменялся.

## 1. Полная ASan-сборка проекта — PASS

| Параметр | Значение |
|---|---|
| copied tree | `C:\DHW\.tmp\asan-line3\gpp-tree` (robocopy; исключены `.git`, `bin`, `dist`, `evidence`, `vcpkg`) |
| source manifest | 1155 файлов include/src/tests/deps/cmake, sha256 `E73103AB48A72304334325DFB3B029A53C0EB77D5FB9CC609F9D771557AD4150` |
| build dir | `C:\DHW\.tmp\asan-line3\gpp-build` |
| targets | `Common;Core;Crypto;PMMR` (без Database → без неинструментированного rocksdb.lib) |
| toolchain | MSVC 14.44.35207, VS 2022 BuildTools 17.14, Windows SDK 10.0.26100, sysroot `C:\DHW\Gplus\.toolchain\sysroot` |
| flags | проект форсирует `/W4 /WX` и `/MT`; добавлено только `/fsanitize=address /Zi` + `/DEBUG` |
| результат | **BUILD PASS**, 75.6 s; build manifest sha256 `210FBAB776AA73FB2F54FC982850F3D771EFB2BC2D56595675BFB11E81FC650E` |
| logs | `evidence/line3/full-asan-20261103-031306/` (build_manifest.json, configure/build logs) |

Собранные ASan-библиотеки (`…\gpp-tree\bin\Release\Release`): Common, Core,
Crypto, PMMR, jsoncpp, bitcoin, sha3, scrypt, rfc6234, chachapoly, Net, Tor.

**LNK2038 resolution.** Ранее harness падал на `LNK2038: annotate_string /
annotate_vector mismatch` при линковке ASan-TU с обычными `/MT`-библиотеками.
Теперь ВСЕ project-TU в отдельном дереве собраны с ASan, поэтому
`_DISABLE_STRING_ANNOTATION/_DISABLE_VECTOR_ANNOTATION` согласованы; харнессы
линкуются с ASan-библиотеками успешно. Неинструментированными остаются только
C-библиотеки sysroot (zlib, minizip, libsodium, secp256k1-zkp, roaring) и CRT;
`rocksdb.lib` (C++, без ASan) намеренно не линкуется.

Первичный configure-сбой (C4530 в `deps/reproc++`) был вызван заменой дефолтных
`CMAKE_CXX_FLAGS` — потерялся `/EHsc`. Исправлено сохранением дефолтов MSVC
(`/DWIN32 /D_WINDOWS /W3 /GR /EHsc`) перед добавлением ASan-флагов; проектный
CMake сам заменяет `/W3` на `/W4 /WX`.

## 2. Production-тесты под ASan (TestsMini) — PASS

`TestsMini.exe` sha256 `A34F068ABD36EBC4756FE223B3249AB40253BE6C4564C7B245435A892CF6C4CD`,
7 140 352 B, линкован с ASan-библиотеками; скомпилированы с ASan:
`Test_NrdDifferential.cpp`, `Test_NrdIndexRebuild.cpp`, `Test_Segment.cpp`,
`Test_P2PCompat.cpp`, `BlockValidator.cpp`; entry point —
`tests/fuzz/tests_mini_main.cpp` (локальный runtime в `main`, см. §4).
Один символ `Chain::GetByHeight` закрыт тестовым stub'ом
(`tests/fuzz/stub_chain_getbyheight.cpp`), т.к. он тянет Database/rocksdb;
покрытые тесты его не вызывают.

Прогоны (изолированный HOME/TMP, `GRINPP_NRD_CASES` → repo cases json sha
`AD2B77AC…`, `ASAN_OPTIONS=detect_leaks=0`, профиль пользователя не изменён,
stderr пуст, exit 0):

| run | filter | результат | manifest sha256 |
|---|---|---|---|
| `asan-testsmini-nrd-02` | `Nrd*` | **12 cases / 329 assertions PASS** | `7EAB64BE…` |
| `asan-testsmini-segment-01` | `Segment*` | **16 cases / 68 assertions PASS** | `05CF5FEA…` |
| `asan-testsmini-p2p-02` | `Capabilities::*,MessageTypes::*,GrinVersion::*` | **5 cases / 46 assertions PASS** | `BEB62E36…` |

`Nrd*` включает `Test_NrdDifferential` (категории NRD) и
`Test_NrdIndexRebuild` (rebuild/negative сценарии) — т.е. под ASan реально
исполнены NRD-правила, перестройка kernel-индекса и segment/P2P-совместимость.
Это НЕ весь набор Tests.exe (подмножество).

## 3. Linked fuzz targets — PASS (bounded)

| target | exe sha256 | покрытие |
|---|---|---|
| `p2p_responses` | `67A769E16D365D9153CC7A2B16327EB4E2842D15909A4FA17F9973048A4B055D` | реальный `SegmentResponseMessage::Deserialize` для 22/24/26/28 + `SerializePayload` + bounded Root/Validate |
| `segment_validate` | `17A35EF382551341B63A56813D8A2F85C1B47001916494F237B7EA35011261C2` | `Segment<T>`/`SegmentProof` Root/Validate/ReconstructRoot (Hasher инструментирован) |

Smoke seed `20261103`, 200 итераций/target, ≤30 s + каждый corpus-вход
отдельным процессом с timeout:

- `p2p_responses`: ok=1, rejected_deserialization=199, ASan=0, timeout=0, 36/36 one-shot;
- `segment_validate`: ok=20, rejected_deserialization=174, rejected_segment=6, ASan=0, timeout=0, 84/84 one-shot.

Manifest: `evidence/line3/fuzz-linked-smoke-20261103-02/manifest.json`
(sha256 `E440A0CF…`). Ограничение: большинство мутаций отбрасывается на
десериализации; coverage не измерялась.

## 4. FINDING: exit-time heap-use-after-free в TestMain/Global — FAIL (не исправлено)

При линковке **исходного** `tests/src/TestMain.cpp` (`static TestRuntime
RUNTIME;`, exe sha256 `8DDB1C09…`) ASan на каждом завершении процесса, **даже
без единого теста**, сообщает:

```
ERROR: AddressSanitizer: heap-use-after-free
WRITE ... std::shared_ptr<Context>::{dtor} ~TestRuntime() tests/src/TestMain.cpp:31
freed by ... Global::Shutdown() src/Core/Global.cpp:59 (SHARED_CONTEXT.reset())
```

Воспроизведения: `evidence/line3/asan-testsmini-nrd-01/` (12/329 PASS, exit 1
из-за UAF) и `evidence/line3/asan-testsmini-empty-01/` (фильтр не совпал,
`No tests ran`, UAF тот же). Диагностический прогон
`tests/fuzz/diag_runtime_lifetime.cpp` (exe sha256 `AA7F09D4…`, логи
`diag_runtime_lifetime.{stdout,stderr}.log`) даёт refcount-картину:

```
[ctor] after create: m_pContext.use_count=1
[ctor] after Global::Init: m_pContext.use_count=2 (+temp), global=3
[dtor] step1 with weak lock: m_pContext=2, weak_lock=2   <- SHARED_CONTEXT уже пуст
[dtor] step2 before Shutdown: m_pContext=1
Global::Shutdown(); -> ASan UAF при последующем обращении к control block
```

Класс: **static deinitialization order fiasco**. `TestMain.cpp` держит
`TestRuntime` как статический объект; статические strong/weak-держатели
`SHARED_CONTEXT`/`GLOBAL_CONTEXT` из `src/Core/Global.cpp` (Core.lib) к моменту
его деструктора уже вне времени жизни, и `Global::Shutdown()`/`GetContext()`
работают с уже освобождённым control block (двойной decref). Точная
последовательность decref'ов внутри MSVC `shared_ptr` не инструментирована, но
класс подтверждён воспроизведением с нулём тестов и refcount-логами.

- **Production impact:** NOT VERIFIED. Наблюдался только teardown тестового
  executable; латентная опасность — использование `Global::*` после статической
  деинициализации/`Shutdown`. Production-код не менялся.
- **Workaround (только harness):** `tests/fuzz/tests_mini_main.cpp` создаёт
  runtime локально в `main()`; global-статики живы на момент деструктора. Все
  ASan-прогоны §2 сделаны с этим main. Обычный `Tests.exe` не изменён.
- **Рекомендация владельцу:** заменить статические держатели Global на
  Meyers-singleton/явный lifetime, либо перевести TestMain на
  `CATCH_CONFIG_RUNNER` + локальный runtime; добавить ASan-run в CI как
  regression. Без согласования Lead production-код не правился.

## 5. Ограничения

- Database/ChainState/rocksdb не инструментированы и не линкуются (C++, без
  ASan) — DB-персистентность не покрыта.
- `TxHashSet::SaveOutputPositions` заменён stub'ом только для линковки; покрытые
  тесты его не вызывают (output-position persistence не проверялся).
- TestsMini — подмножество (33 теста), не полный Tests.exe; полный Tests.exe
  под ASan потребует ASan-пересборки rocksdb/C++ sysroot.
- ASan leak detection отключён (`detect_leaks=0`); утечки отдельно не оценивались.
- Coverage не измерялась; fuzz — bounded smoke, отсутствие находок не означает
  безопасность.
- Найденный UAF не исправлен; production/test-код не изменялся.

## 6. Артефакты и команды

- Сборка: `pwsh tools/fuzz/build_full_asan.ps1` (+ `-Reconfigure`), логи/манифест
  в `evidence/line3/full-asan-20261103-031306/`.
- Linked targets: `pwsh tools/fuzz/build_linked_targets.ps1`, артефакты и логи —
  `evidence/line3/full-asan-linked-artifacts/` (бинарники p2p_responses,
  segment_validate, TestsMini, diag; build logs; build_manifest).
- Тесты: `run-tests-isolated.ps1 -ExePath …\TestsMini.exe -ExeArgs 'Nrd*'
  -KeepBinary` (аналогично Segment*, Capabilities::*,MessageTypes::*).
- Fuzz: `run_fuzz_smoke.ps1 -BuildDir …\fuzz-linked -Targets p2p_responses,segment_validate`.
