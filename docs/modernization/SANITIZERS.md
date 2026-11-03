# SANITIZERS — отдельный ASan/fuzz harness Grin++ (линия 3)

Статус на 2026-11-03. Профиль AddressSanitizer собран **отдельно** от обычной
сборки; production-флаги `/W4 /WX` и общий build directory не менялись.

## Toolchain (фактический)

| Компонент | Значение |
|---|---|
| Host | Windows 11 x64 (10.0.26300), VS 2022 BuildTools 17.14 |
| Компилятор | MSVC 14.44.35207 (toolset v143) |
| Windows SDK | 10.0.26100 |
| ASan runtime | `clang_rt.asan_dynamic-x86_64.dll` из MSVC 14.44.35207, SHA-256 `3AA742521E8CBD23252F0545AA51D5A15624A3E4430FBE2F81567AFAEB3FC59B` |
| Зависимости | локальный sysroot `C:\DHW\Gplus\.toolchain\sysroot` (rocksdb, secp256k1-zkp, libsodium, roaring, minizip, zlib — **без ASan**) |
| libFuzzer | недоступен: в VS BuildTools нет `clang-cl` (только clang-format/clang-tidy), MSVC `/fsanitize=address` не содержит fuzzer-рантайма |

## Профиль сборки

```
MSVC: /nologo /W4 /WX /O1 /Zi /MT /EHsc /std:c++17 /fsanitize=address
link: /DEBUG /INCREMENTAL:NO
project defines: WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE
                 SPDLOG_WCHAR_FILENAMES FMT_HEADER_ONLY=1 SODIUM_STATIC
                 HAS_MEMSET_S ssize_t=SSIZE_T _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
```

Обязательные детали:

- `/Zi`: без отладочной информации MSVC выдаёт `C5072` («ASan enabled without
  debug information»), и с `/WX` сборка падает. `/Zi` не ослабляет production
  флаги; `/WX` остаётся включённым.
- ASan поставляется как **динамический** runtime DLL. `build_asan.ps1` копирует
  `clang_rt.asan_dynamic-x86_64.dll` рядом с exe; без неё процесс завершается
  `0xC0000135` (DLL not found).
- `/MT` (static CRT) совместим с MSVC ASan — проверено на probe и всех targets.

## Сборка

```powershell
pwsh tools/fuzz/build_asan.ps1                  # все 4 target
pwsh tools/fuzz/build_asan.ps1 -Only zip        # один target
pwsh tools/fuzz/build_asan.ps1 -Clean
```

Выход: `C:\DHW\.tmp\asan-line3\fuzz\` (exe, `logs\build_<target>.log`,
`build_manifest.json`). Скрипт отказывается писать в
`C:\DHW\Gplus\.toolchain\grinpp-build` и в `bin/` репозитория; общий build
directory не используется.

## Доказательство инструментации (PASS)

```powershell
fuzz_bytebuffer.exe --asan-selftest
→ AddressSanitizer: heap-buffer-overflow ... fuzz_driver.cpp:222, exit 1
```

Сырые логи: `evidence/line3/asan_instrumentation_selftest.{stdout,stderr}.log`,
`asan_instrumentation_selftest.exit.txt`. Отдельно проверен чистый путь: probe
без нарушения памяти завершается 0. Подтверждено, что ASan реально
инструментирует TU harness, а не только подключён линковкой.

## Targets и corpus

Все targets — standalone exe с общим детерминированным драйвером
`tests/fuzz/fuzz_driver.cpp` (xorshift64*, фиксированный seed, границы размера,
категории исхода, сохранение текущего входа до вызова).

| target | Что проверяется |
|---|---|
| `bytebuffer` | `ByteBuffer`: 8/16/32/64-bit big/little endian, `ReadVarStr`, 64-битные length-префиксы (`ReadVector`/`ReadString`), `ReadBigInteger<32/64>`, `ReadArray`, `ReadRemainingBytes`, границы переполнения |
| `p2p_body` | реальный `SegmentRequestMessage::Deserialize` (wire ids 21/23/25/27), тело `OutputBitmapSegment` (22) через `BitmapSegment::Deserialize`, усечённые тела |
| `segment` | `SegmentIdentifier`, `SegmentProof`, `BitmapBlock`, `BitmapChunk`, `BitmapSegment`, `Segment<BitmapChunk>` (parse-only), лимиты `MAX_SEGMENT_READ_ITEMS`/`MAX_HASHES`, overflow счётчиков |
| `zip` | `ZipFile::Load/ListFiles/ExtractFile` над атакующим архивом: CRC mismatch, обрезанный central directory, повторные/пустые имена, path-traversal имя, мусор, чрезмерные размеры; запись только в `GRINPP_FUZZ_STAGING` |
| `nrd_rules` | production-правила `include/PMMR/NrdRules.h`: `IsRelativeHeightSatisfied` (relative_height 0/1/2/WEEK_HEIGHT±1/0xFFFF/UINT64_MAX, `currentHeight<previousHeight`, cutoff), `RecordBlockKernel` duplicate-excess. Seed-материал — из line-2 handoff `tools/audit/nrd/cases/nrd_cases.json` |
| `p2p_responses` | реальный `SegmentResponseMessage::Deserialize` 22/24/26/28 + `SerializePayload` + bounded Root/Validate; линкуется с полной ASan-сборкой проекта |
| `segment_validate` | `Segment<T>`/`SegmentProof` Root/Validate/ReconstructRoot, Hasher инструментирован (полная ASan-сборка) |

## Полная ASan-сборка проекта (LNK2038 resolved)

Скрипт `tools/fuzz/build_full_asan.ps1`: копия дерева в `C:\DHW\.tmp\asan-line3\gpp-tree`,
отдельный CMake build, все project-TU с `/fsanitize=address /Zi` (проект сам
форсирует `/W4 /WX` и `/MT`). Собраны Common/Core/Crypto/PMMR (+ jsoncpp,
bitcoin, sha3, scrypt, rfc6234, chachapoly, Net, Tor); `Database`/`rocksdb.lib`
намеренно не линкуются, поэтому annotate-mismatch исчезает: **LNK2038 решён**
(все линкуемые C++ project-объекты ASan-согласованы, sysroot — только C).
`build_full_asan.ps1` при замене `CMAKE_CXX_FLAGS` обязан сохранять дефолты
MSVC (`/DWIN32 /D_WINDOWS /W3 /GR /EHsc`), иначе `deps/reproc++` падает C4530.

Скрипт `tools/fuzz/build_linked_targets.ps1` собирает `p2p_responses`,
`segment_validate` и `TestsMini` (Catch2: `Test_NrdDifferential`,
`Test_NrdIndexRebuild`, `Test_Segment`, `Test_P2PCompat` + `BlockValidator.cpp`)
против ASan-библиотек. Без Database один символ
(`Chain::GetByHeight` из `TxHashSet::SaveOutputPositions`) закрыт тестовым
stub'ом `tests/fuzz/stub_chain_getbyheight.cpp`; покрытые тесты его не вызывают.

Результаты: полная сборка **PASS** (75.6 s); TestsMini под ASan: `Nrd*` 12/329,
`Segment*` 16/68, `Capabilities/MessageTypes/GrinVersion` 5/46 — exit 0, без
ASan-отчётов; linked fuzz smoke: `p2p_responses` 200 итераций (1 ok / 199
deserialize-reject), `segment_validate` 200 (20 ok / 174 reject / 6
segment-reject), 120/120 one-shot, 0 ASan/timeout. Детали:
`evidence/line3/full-asan-audit-20261103.md`.

**Archive-server subset линии 1 под ASan (после обновления источников):**
copied tree пересинхронизирован с line1/line2 изменениями (source manifest
`857AA38E…`), TestsMini sha `2DA4727E…` включает `Test_PibdServing.cpp`:
`PibdArchiveServer::LeaseOutlivesHorizonSwitch` 1 case/16 assertions PASS,
`PibdArchiveServer::*` 7/168 PASS, `Nrd*` 12/329, `Segment*` 16/68, P2P 5/46 —
exit 0, без ASan; zero-test teardown regression PASS; ASan-бинарники сохранены
в evidence. Pre-fix fixture bug линии 1 (`REQUIRE(destroyed == 1)` → 0 == 1)
воспроизведён line3 на нормальном SHA `2032536D…`
(`evidence/line3/line3-audit-pibd-lease-01`), классифицирован line1 как test
fixture bug (не production lifetime) и подтверждён под ASan после фикса.
Детали: `evidence/line3/asan-line1-archive-subset-20261103.md`.

**Authoritative normal binary (build-07 final integration candidate):**
Tests `C8C386BE576A62604404E0FA02E56EE9E8F06B50EA0B15AD109F36F5AFF5B7FE`
(9 241 600 B, PE ts `0x6AE90EE9`) + GrinNode
`F9EF06B4CB0962B50E6DDBD186E1C61F5872FAFF9E41A779EE7C317A2ED25BDA`
(7 805 440 B, PE ts `0x6AE90E4D`); repo == preserved copies.
Независимые изолированные прогоны line3: full **3/3 PASS (245/4013
assertions)**, `PibdArchiveServer::*` **8/187**, `Nrd*` **30/428**,
`KernelIndex*` **3/48**, `P2P::AdvertisedTip*` **2/15**, `Slatepack*`
**5/5 PASS**; всё `-KeepBinary`, профиль не изменён (детали:
`evidence/line3/normal-binary-c8c-audit-20261103.md`). CI pipeline `ci-c8c-01`
подтвердил full 245/4013 и regressions, overall PASS_WITH_UNCOVERED.
**Provenance:** build-07 — финальный source set (atomic advertised-tip P0 +
line2 task-6/round-2 + archive). Предыдущие 8F10BFD6 (236/3952),
DDC21F45 (236/3952), FD7C0492 (232/3926) сохранены как исторические и к
C8C386BE не привязываются.
**Environment flake:** на 930148B6 (non-final) 2/6 full FAIL; причина части сбоев —
фиксированный wallet-порт `127.0.0.1:3421` при параллельных прогонах. На C8C при
сериализации и port-preflight — **8/8 full PASS** (3 line3 + pipeline + line2 +
3 stress), `Slatepack*` 5/5 PASS. Редкий Slatepack-decrypt FAIL (Unknown
exception/DecryptSlatepack) **не имеет доказанной причины** — остаётся открытым
риском, не списывается автоматически на окружение; CI классифицирует как FAIL,
если в провале есть Slatepack-путь.
ASan **обновлён под build-07 sources** (refresh 05:42): source manifest
`57590D1ED05CDC59E68D38C66F34F40D0CC24556D58BB2F1D76691055B3D8000`, TestsMini
`19FE60057312CDC6D28E430F1247E5069E7DE136D1388482FCC3668936244ACF`. Subset PASS:
`Nrd*` **21/365**, archive **8/187**, Segment 16/68, P2P **7/61** (включая оба
advert-теста), zero-test teardown PASS, 0 ASan reports.
`Test_NrdPreCommit`, `Test_NrdFalseBan`, `Test_NrdBatchSafety`, `Test_KernelIndex`
остаются ASan-**BLOCKED** (BlockProcessor/BlockChain→Database→non-ASan rocksdb
`LNK2019`); покрыты normal-suite C8C.
**Post-build drift (05:50):** после build-07 в repo добавлены
`tests/src/P2P/Test_BlockPipeBan.cpp`, `tests/src/P2P/CMakeLists.txt`,
изменён `tests/CMakeLists.txt`; бинарник C8C их не содержит → текущий tree снова
расходится с ASan (pipeline `mismatch=3`, ASan `NOT RUN (STALE)`). Refresh
приостановлен до заморозки дерева/build-08; никаких ASan-PASS к незамороженному
tree нет. Normal authoritative evidence остаётся за `C8C386BE…` (его собственный
source snapshot); предыдущие `8F10BFD6…`, `DDC21F45…`, `FD7C0492…` — исторические.

**Build-08 / build-09 (2026-11-03):** build-08 `7994F191…` — historical FAIL
(249/237/12: 11× wallet-port env + 1× test-only bodyless); ASan refresh для него
отменён. Build-09 `0A3B7D66…` — **normal authoritative WITH EXPLICIT EXCEPTION**
(Lead decision b): full **249 cases / 248 passed / 1 failed** (4033/4032; только
test-only `Test_NrdBodylessRestart` short-chain header-version); bodyless-NRD и
kill-recovery **NOT VERIFIED**; production/agreed suites 0 fails; PASS: Nrd 30/428,
KernelIndex 3/48, archive 8/187, advert 2/15, BlockPipe 3/16. **Не fully green.**
ASan refresh build-09 (ONE): source manifest `CF78C188…` (1290 files), TestsMini
`12C3D16C…`; Nrd* 21/365, archive 8/187, Segment 16/68, P2P 7/61 (incl. advert),
teardown PASS, 0 reports; BlockPipe/bodyless/kill/PreCommit/FalseBan/BatchSafety/
KernelIndex — normal-only (rocksdb). Pipeline `ci-0a3-01` overall **FAIL** honest
(full-suite test-only); asan-subset PASS.

**Build-10 final candidate (2026-11-03):** Tests
`C17C3BC0…` + GrinNode `60D7F7A6…`; clean serialized full **254 cases /
253 passed / 1 failed** (bodyless-only), all pre-registered filters PASS.
ASan refresh **PASS (subset)**: source manifest `776B0DB3…` (1292 files),
full ASan BUILD_PASS 98.6 s, TestsMini `497353EC…`; Nrd* 21/365, archive 8/187,
Segment 16/68, P2P+advert 7/61, teardown PASS, **0 ASan reports**;
ASan-BLOCKED normal-only PreCommit/FalseBan/BatchSafety/KernelIndex/BlockPipe/
bodyless/kill; rocksdb and sysroot C libs uninstrumented. Optional
`tests_mini_prefix` not built (missing legacy source). Details:
`evidence/line3/normal-binary-build10-audit-20261103.md`,
`evidence/line3/asan-build10-refresh-20261103.md`,
`evidence/line3/build10-asan-refresh-20261103.json`.

**Build-11 final candidate (2026-11-03, retry):** Tests
`02DC3D9C…` + GrinNode `565BC2DB…`; full **255 cases / 254 passed / 1 failed**
(bodyless-only); all groups PASS including `P2P::ArchivePublication*` 1/83.
ASan refresh **PASS (subset)**: source manifest `69B01EE6…` (1293 files), full
ASan BUILD_PASS 101.1 s, TestsMini `C76CCCF2…`; copied tree matches build-11
freeze 1003/1003; Nrd* 21/365, archive 8/187, Segment 16/68, P2P+advert 7/61,
ArchivePublication 1/83, teardown PASS, **0 ASan reports**; ASan-BLOCKED
normal-only PreCommit/FalseBan/BatchSafety/KernelIndex/BlockPipe/bodyless/kill;
rocksdb/sysroot uninstrumented. Failed first attempt C1083 `ACED40F9…`
excluded; retry log `B274B34A…`; freeze `E5F7519C…`. Details:
`evidence/line3/normal-binary-build11-audit-20261103.md`,
`evidence/line3/asan-build11-refresh-20261103.md`,
`evidence/line3/build11-asan-refresh-20261103.json`.

**Build-11 post-audit bug / build-12 pending:** build-11 contains the production
Release `SpentOutputs::Deserialize` NDEBUG assert-side-effect bug (old
`SpentOutput.h F3F38B44…`; raw 52 B/count1 reads as 0). Its normal/ASan PASS does
not validate the fix. Build-12 must audit `SpentOutput.h D27A607D…` + regression
`Test_SpentOutputs.cpp 6157AC56…`, then live H=3313440/H=3315600 recovery.
Evidence: `task10-11-spent-deserialize-rootcause-20261103/`,
`build12-preaudit-20261103.md`.

**Build-12 final candidate (2026-11-03):** Tests `D1103F34…` + GrinNode
`833CCBFD…`; full normal **256/255/1** (bodyless-only), `SpentOutputs*` 1/6
PASS, exact preflight PASS, all groups incl. ArchivePublication 1/83 PASS.
ASan refresh **PASS (subset)**: source manifest `3DB12364…` (1293 files), full
ASan BUILD_PASS 96.4 s, TestsMini `CB09CEE8…`; freeze match 1003/1003; Nrd
21/365, archive 8/187, Segment 16/68, P2P+advert 7/61, ArchivePublication 1/83,
teardown PASS, **0 ASan reports**. `Test_SpentOutputs.cpp` is not in TestsMini;
Release regression covered normal only. Live H=3313440/H=3315600 recovery still
pending. Details: `normal-binary-build12-audit-20261103.md`,
`asan-build12-refresh-20261103.md`, `build12-asan-refresh-20261103.json`.

**Build-13 final candidate (2026-11-03):** Tests `001DF4BC…` + GrinNode
`F39BDFE5…`; full normal **257/256/1** (bodyless-only), `SpentOutputs*` 1/6,
exact preflight 1/4, **boundary regression 1/8**, ArchivePublication 1/83 and
all groups PASS. ASan refresh **PASS (subset)**: source manifest `BFBF2186…`
(1293 files), full ASan BUILD_PASS 87.5 s, TestsMini `ECDD6D5E…`; freeze match
1003/1003; Nrd 21/365, archive 8/187, Segment 16/68, P2P+advert 7/61,
ArchivePublication 1/83, teardown PASS, **0 ASan reports**. SpentOutputs/
boundary tests not in TestsMini; normal Release PASS. Recovery H3313440
observed PASS (archive 3,116,449,317 B SHA `7FB23058…`, LoadAndValidate before
publish) and H3315600 snapshot/serving observed (3,118,756,420 B SHA
`DA23A1C7…`); line1 formal report received/pass. Details:
`normal-binary-build13-audit-20261103.md`,
`asan-build13-refresh-20261103.md`,
`build13-asan-refresh-20261103.json`,
`task10-11-build13-recovery-audit-20261103.md`.

**Build-13 full 7B live (2026-11-03): PASS** — test/floonet, final live
binding. Tip 3318655/`2945348e…`; C++ `FULLY_SYNCED`, Rust `no_sync`; roots
pre==post. PIBD leaves 10,203,442 @H3315600, range 2,943,034, kernels
3,351,542; duration ≈1h58m59s; H3315600 archive 3,118,756,420 B SHA `DA23A1C7…`.
Non-blocking: admin DACL refusal, initial loopback connect failure, stale C++
`waiting_reason`, Rust 50,056 WARN/0 ERROR. Crash/publication/resource matrix
prepared, **NOT RUN**; not Mainnet/full G-09. Evidence:
`build13-7b-full-audit-20261103.md`, `build13-7b-full-20261103/`
(hash manifest `6CAE425C…`).

**Находка ASan — исправлено в тестовом runtime (Lead-approved option b):**
исходный `tests/src/TestMain.cpp` (`static TestRuntime`) давал exit-time
heap-use-after-free: статические `SHARED_CONTEXT`/`GLOBAL_CONTEXT` из
`src/Core/Global.cpp` уже вне времени жизни к моменту `~TestRuntime()`,
`Global::Shutdown()` двойственно освобождал control block (воспроизводилось без
единого теста). Фикс: `CATCH_CONFIG_RUNNER` + `TestRuntime` как локальный объект
в `main()` (sha256 `6BAC1744…`), Global.cpp **не менялся**. После фикса:
`Nrd*` 12/329, `Segment*` 16/68, P2P 5/46 — PASS, exit 0, без ASan-отчётов;
zero-test teardown regression PASS (script
`tools/fuzz/run_asan_teardown_regression.ps1`), на pre-fix negative control та
же регрессия корректно падает с UAF. Pre-fix манифесты сохранены
(`asan-testsmini-nrd-01`, `asan-testsmini-empty-01`, `diag_runtime_lifetime.*`).
Детали: `evidence/line3/testmain-uaf-fix-20261103.md`.
Residual (production) — **NOT VERIFIED**: `Global` статики по-прежнему опасны при
использовании после статической деинициализации/`Shutdown`; отдельная
production-lifetime задача не выполнялась.

Corpus: `tests/fuzz/corpus/<target>/` (189 синтетических файлов, без
пользовательских данных; 26 `nrd_rules`, 36 `p2p_responses`, 84
`segment_validate`), генерируется детерминированно:

```powershell
pwsh tools/fuzz/gen_corpus.ps1 -Force
```

## Ограниченный прогон (smoke)

```powershell
pwsh tools/fuzz/run_fuzz_smoke.ps1 -Iterations 200 -MaxSeconds 30 -RunId <id>
```

Прогон `fuzz-smoke-20261103-01` (seed `20261103`, 200 итераций на target,
≤30 с, каждый corpus-файл отдельно в дочернем процессе с timeout):

| target | executed | ok | controlled rejection | ASan | timeout | slowest |
|---|---|---|---|---|---|---|
| bytebuffer | 200 | 42 | 158 (`DeserializationException`) | 0 | 0 | 0.137 ms |
| p2p_body | 200 | 34 | 166 (`DeserializationException`) | 0 | 0 | 0.156 ms |
| segment | 200 | 45 | 155 (`DeserializationException`) | 0 | 0 | 0.165 ms |
| zip | 200 | 25 | 175 (`FileException`) | 0 | 0 | 197.763 ms |

Плюс 43/43 one-shot corpus-входа без crash/timeout/ASan. Манифест:
`evidence/line3/fuzz-smoke-20261103-01/manifest.json`.

Отдельный NRD-прогон после handoff line2 (`fuzz-nrd-smoke-20261103-01`, seed
`20261103`, 500 итераций + 26 one-shot): `ok=500`, ASan=0, timeout=0, slowest
0.062 ms; exe sha256 `4D7275E7…`; манифест
`evidence/line3/fuzz-nrd-smoke-20261103-01/manifest.json`. Аудит evidence и
ограничения — `evidence/line3/nrd-handoff-audit-20261103.md`.

**Ограничение доказательства:** отсутствие crash за 200 итераций/30 с не
доказывает отсутствие ошибок. Это smoke-прогон, не исчерпывающий fuzz.

## Что инструментировано, а что нет

Инструментировано ASan:

- harness-драйвер и target-TU (включая header-шаблоны `ByteBuffer`, `Segment*`,
  `SegmentRequestMessage`, `Bitmap*`);
- `src/PMMR/Zip/ZipFile.cpp`, `src/Common/Logger.cpp`, `src/Common/GrinStr.cpp`,
  `src/Common/Util/FileUtil.cpp`, `src/Core/Traits/Serializable.cpp` — собраны
  из исходников в ASan-профиле для zip/p2p/segment targets.

НЕ инструментировано:

- обычные static-библиотеки `bin\Release\Release\*.lib` (Release `/MT`, без
  ASan) — не линкуются в harness, кроме явно перечисленных TU;
- sysroot-библиотеки: rocksdb, secp256k1-zkp, libsodium, roaring, minizip,
  zlibstatic (C/C++ без ASan; для zip это значит, что внутренности minizip
  ASan не проверяет, проверяется вызывающий код и CRC-логика `ZipFile.cpp`);
- системный CRT/Windows DLL, кроме ASan runtime.

## Известные ограничения (важно для приёмки)

1. **Смешивание ASan-объектов и обычных MSVC-STL объектов невозможно.** ASan
   включает `_DISABLE_STRING_ANNOTATION`/`_DISABLE_VECTOR_ANNOTATION`, а
   обычные библиотеки собраны с annotations; линковка даёт
   `LNK2038: annotate_string/annotate_vector mismatch` (`/U` не снимает).
   Поэтому standalone harness (`build_asan.ps1`) содержит только
   header-only/ZipFile-цели. Для p2p-ответов 24/26/28, `Segment::Root`/
   `SegmentProof::ReconstructRoot`, NRD index rebuild и полного Tests.exe нужна
   полная ASan-сборка проекта: она выполнена в изолированном дереве
   (`build_full_asan.ps1`, см. раздел выше) — p2p-ответы, proof-валидация,
   NRD differential и KernelIndex rebuild покрыты (TestsMini/linked targets).
   Полный `Tests.exe` под ASan всё ещё требует ASan-пересборки rocksdb/C++
   sysroot и не выполнялся.
2. Нет coverage-guided fuzzing: libFuzzer/clang-cl недоступен, используется
   детерминированный мутационный драйвер. Coverage не измеряется.
3. Только ASan. UBSan/MSan/TSan на Windows/MSVC в этом окружении не
   использовались; многопоточность одним ASan не проверяется.
4. Зависание внутри одного вызова нельзя прервать in-process; детекция —
   `--one-shot` в отдельном процессе с timeout в `run_fuzz_smoke.ps1`.
5. Сетевые проверки выполнялись только на синтетических байтах, без живого
   peer/mainnet/пользовательских данных.
6. Snapshot activation (staging/rename/commit, disk-full) и NRD apply/rewind/
   reorg transitions этим harness **не покрыты**: нужны контролируемые
   filesystem/disk-failure барьеры и детерминированные последовательности
   состояний (зона линий 1/2). ZIP-target покрывает только парсер архива и
   извлечение в staging; семантика path traversal к `ZipFile::ExtractFile`
   неприменима, т.к. путь назначения задаёт вызывающий код, а не имя entry.
   Чистые NRD-правила (`NrdRules.h`) покрыты target `nrd_rules`; построение
   KernelMMR/исторического индекса (rebuild, missing middle header, attribution)
   остаётся у differential-тестов линии 2.
7. Автоматической минимизации входа нет: `current_case.bin` сохраняется до
   вызова, минимизация — ручная (delta-debug) перед добавлением в corpus.
8. ASan-покрытие NRD seed-материала ограничено бинарным layout высот/ядер:
   JSON-парсер `nrd_cases.json` — инструмент аудита, а не production-поверхность,
   поэтому он не является fuzz target.

## Как оформлять находку

1. ASan падение: сохранить `current_case.bin`, `last_case.txt`, stderr с
   отчётом; зафиксировать SHA exe и seed.
2. Воспроизвести `fuzz_<target>.exe --one-shot <case>` в чистом процессе.
3. Минимизировать вход, добавить в `tests/fuzz/corpus/<target>/` как regression,
   передать владельцу кода; не считать компонент безопасным по smoke-прогону.
4. Проверить, не является ли случай контролируемым отказом (исключение
   `DeserializationException`/`FileException`/`SegmentException`) — это PASS по
   политике rejection, а не finding.
