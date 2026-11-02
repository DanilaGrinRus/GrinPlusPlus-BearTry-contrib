# NRD handoff audit и reuse (line 2 → line 3)

Дата: 2026-11-03. Аудитор: line3-fuzz. Исходный HEAD: `e0a5d2d…` (дерево с
незакоммиченными изменениями линий 1/2/аудитора). Задача — проверить привязку
evidence line2 к SHA, зафиксировать пригодность seed-материала и встроить его
в ASan-fuzz линию без изменения production-кода.

## 1. Проверка заявленного evidence line2

| Run | Заявлено | Проверка | Вердикт |
|---|---|---|---|
| `nrd-20261103-025613` | 17 cases / 350 assertions PASS, 14 differential cases | manifest `status=PASS`, exit `0x00000000`, stdout SHA совпадает с manifest (`2354B2A5…`), HOME=`C:\DHW\.tmp\line2-nrd\iso\…\home`, `user_profile_unchanged=true`, `test_data_dir_created=true`, args `Nrd*` | **PASS** (привязка к SHA есть) |
| `kernidx-20261103-025622` | 3 cases / 48 assertions PASS | manifest `status=PASS`, exit 0, stdout SHA `580D0CB1…` совпадает, изоляция подтверждена, args `KernelIndex*` | **PASS** |
| `nrd-20261103-024909` | не заявлен в handoff | manifest `status=FAIL`, exit `0x00000003`, 3 failed на старом SHA `168AF3C2…` | исторический FAIL, перекрыт более поздним PASS |
| `cpp-nrd-differential-report.json` | 14 differential cases | 14 записей, `actual_cpp_category == expected_cpp_category` во всех 14 (mismatches=0), sha256 `EA44A390A37AAB5F53D045F3F63BB6E1CF2DE39BE37710F8CFCD67AE502B5FCB` | **PASS (формально)** |
| `cases/nrd_cases.json` | 14 chain cases + категории | файл читается, 14 cases, sha256 `AD2B77AC958AD3BD4C853B4A74CE12B278430FE22BC5B0E3DFA1E4EA8F08766B` | **PASS** |

Заявленный SHA `0EDC0F81…235FC7` подтверждён: текущий
`bin\Release\Release\Tests.exe` (сборка line2, mtime 2026-11-03 02:56:09,
PE ts `0x6AE8EAD8`) имеет ровно этот SHA-256.

### Найденный процессный gap

В evidence line2 (`tools/audit/nrd/evidence/isolated/*`) **нет копии бинарника**
(`copy_kept=False`): привязка держится только на изменяемом
`bin\Release\Release\Tests.exe`. Любая следующая сборка разорвёт связь с логами.

Исправлено независимыми прогонами line3 (тот же SHA, изолированный HOME,
`-KeepBinary`, расширенный provenance-список с NRD-файлами):

| Run line3 | Результат | Manifest sha256 | Бинарник сохранён |
|---|---|---|---|
| `evidence/line3/nrd-audit-20261103-01` (`KernelIndex*`) | PASS, 3 cases / 48 assertions | `E0D1F842…` | `0EDC0F81…` |
| `evidence/line3/nrd-audit-20261103-02` (`Nrd*`) | PASS, 17 cases / 350 assertions | `EC0CC9D7…` | `0EDC0F81…` |

Оба прогона: exit 0, `user_profile_unchanged=true`, data dir создан в
одноразовом HOME, Application Error events = 0. `run-tests-isolated.ps1` теперь
хеширует и NRD-исходники (`NrdRules.h`, `KernelIndex.*`, `Test_Nrd*`) — см.
`source_hashes` в манифестах.

Важно: SHA `0EDC0F81…` — актуальный бинарник линии 2; более ранний SHA
`4F75DCD4…` (line3 baseline) сохранён в
`evidence/line3/baseline-e0a5d2d-001/Tests.exe` и не конфликтует с этим.

### Закрытие gap (проверено line3, 2026-11-03 ~03:0x)

Line2 сохранил неизменяемую копию тестированного бинарника:
`tools/audit/nrd/evidence/bin/Tests-0EDC0F810E7D.exe`.

Проверка line3 (независимое чтение):

| Поле | Значение |
|---|---|
| size | 9 012 736 bytes |
| SHA-256 | `0EDC0F810E7D9BB864F27266EEB0872DC12288B6F08A9E79293B44D912235FC7` |
| PE ts | `0x6AE8EAD8` (2026-11-02T19:56:08Z) |
| binding-файл | `Tests-0EDC0F81.sha256.txt` (рядом), привязывает файл к `nrd-025613`, `kernidx-025622` и к line3 reruns `nrd-audit-20261103-01/02` |
| repo binary | по-прежнему тот же SHA и mtime 2026-11-03T02:56:09 (пересборки не было) |

Gap «бинарник не сохранён» — **CLOSED**. Итоговая evidence-цепочка для SHA
`0EDC0F81…`: immutable copy + `.sha256.txt` + manifests прогонов line2 и
независимых line3-прогонов.

## 2. Rust oracle provenance

`nrd_cases.json` честно фиксирует `commit_provenance: NOT VERIFIED` (у reference
tree нет `.git`, версия из Cargo.toml; есть tree SHA-256 manifest
`grin-5.5.2-reference-sha256.txt`). Этот статус сохраняется: line3 не
перепроверял Rust-коммит и не считает его доказанным.

## 3. Встраивание seed-материала: target `nrd_rules` — PASS

Выбрана production-логика, которую можно инструментировать без полной ASan
пересборки проекта: `include/PMMR/NrdRules.h` — header-only чистые правила
(`IsRelativeHeightSatisfied`, `RecordBlockKernel`).

- Новый target: `tests/fuzz/target_nrd_rules.cpp`, собран как `fuzz_nrd_rules.exe`
  (sha256 `4D7275E7F93AAE7742A8A1AB6BD2472E09AA35E5ACAFEAE757F77507ED7E8367`,
  профиль `/W4 /WX /Zi /MT /fsanitize=address`).
- Corpus: 26 seed-файлов `tests/fuzz/corpus/nrd_rules/` = 12 граничных
  (`relative_height` 0/1/2/WEEK_HEIGHT/WEEK_HEIGHT+1/0xFFFF/UINT64_MAX,
  `currentHeight < previousHeight`, `cutoff > previousHeight`, duplicate- и
  mixed-excess) + 14, производных от `nrd_cases.json`
  (relative_height и excess целевых ядер; генератор
  `tools/fuzz/gen_corpus.ps1`, layout описан в target-файле).
- Bounded smoke: seed `20261103`, 500 итераций + 26 one-shot в дочерних
  процессах: **PASS**, `ok=500`, ASan=0, timeout=0, slowest 0.062 ms.
  Manifest `evidence/line3/fuzz-nrd-smoke-20261103-01/manifest.json`
  (sha256 `E4789A722968442653C7FEBA8C2394FEBD4C379FCD341730724BDD24F90C3327`).

### Что НЕ покрыто и не переинтерпретируется

- Target проверяет отсутствие OOB/UB/hang в чистых правилах и duplicate-детекции,
  но **не** строит KernelMMR/исторический индекс: rebuild, missing-middle-header,
  multi-kernel attribution остаются у line2 differential/rebuild тестов.
- JSON-парсер — инструмент аудита, а не production-поверхность; мутации JSON не
  являются product fuzz target. Seed-и переводятся в бинарный layout высот/ядер.
- `KernelMMR`/`KernelIndex.cpp` под ASan требуют полной ASan-сборки проекта
  (LNK2038 при смешивании с обычными libs) — ограничение задокументировано в
  `docs/modernization/SANITIZERS.md`.
- `missing_local_history` — fail-closed локальное состояние, не доказательство
  невалидности peer (согласовано с line2; в invalid-evidence не используется).

## 4. Итог

- Evidence line2: **PASS** (17/350 и 3/48 воспроизведены независимо; SHA
  подтверждён), с устранённым gap по сохранению бинарника.
- Differential 14/14 категорий совпадают с ожидаемыми; формальная проверка
  отчёта, oracle-коммит остаётся NOT VERIFIED.
- ASan reuse: **PASS** (target `nrd_rules`, 26 seeds, 500 итераций + 26 one-shot,
  0 находок в рамках бюджета).

## 5. Обновление task-5: новый бинарник 2032536D (2026-11-03 03:35)

Line2 передал новый Tests.exe после pre-commit NRD gate / fail-closed callers /
D2 policy helper. Проверено line3:

| Пункт | Проверка |
|---|---|
| preserved binary | `tools/audit/nrd/evidence/bin/Tests-2032536D810F.exe`: size **9 141 248**, SHA-256 **`2032536D810F9DD78E57658FDEC39DCA06B71368551B2B1C6CAB08B79592140F`**, PE ts **`0x6AE8F3B3`** (2026-11-02T20:33:55Z) — совпало с заявленным |
| sha256 manifest | `Tests-2032536D810F.sha256.txt` присутствует, привязывает бинарник к `nrd-precommit-20261103-033401`, `kernidx-20261103-033401`, `full-20261103-033401` |
| build log | `cpp-build-20261103-032722.log` заканчивается `build OK`, Tests.vcxproj → repo Tests.exe |
| previous binaries | `Tests-0EDC0F810E7D.exe` (0EDC0F81…) и `Tests-06309AF0D81C.exe` (06309AF0…) сохранены рядом |
| `nrd-precommit-20261103-033401` | manifest PASS, exit 0, SHA 2032536D…, stdout SHA привязан; `20 cases / 375 assertions` — совпало |
| `kernidx-20261103-033401` | manifest PASS, exit 0, `3 / 48` — совпало |
| `full-20261103-033401` | manifest **FAIL**: `231 cases / 3917 assertions`, 230 pass, 1 failed; сохранён `Tests.exe` (`-KeepBinary`) |
| `lease-recheck-20261103-033526` | manifest **FAIL** детерминированно: 1 case / 14 assertions, 13 pass, 1 failed |
| differential report | sha256 `EA44A390…` не изменился (14/14 категорий) |
| изоляция | во всех прогонах `user_profile_unchanged=true`, `test_data_dir_created=true`, HOME под `C:\DHW\.tmp\line2-nrd\iso\...`, 0 crash events |

### Найденный FAIL — внешний для NRD (line1)

`PibdArchiveServer::LeaseOutlivesHorizonSwitch` (`tests/src/PMMR/Test_PibdServing.cpp:611`):
`REQUIRE( destroyed == 1 )` → `0 == 1`, 14 assertions (13 pass). Детерминированно
воспроизводится одиночным фильтром. Независимое воспроизведение line3 на
preserved binary (свой изолированный runner, профиль не изменён, бинарник
сохранён): `evidence/line3/line3-audit-pibd-lease-01/manifest.json` —
status FAIL, exit 1, тот же REQUIRE, SHA 2032536D…. Это **FAIL теста линии 1
(PibdArchiveServer lease fixture)**, не NRD/crypto и не ошибка runner'а.
`full`-прогон корректно классифицирован как FAIL; NRD-скоуп (pre-commit gate,
KernelIndex, differential) — PASS.

Замечание по процессу: для `nrd-precommit`/`kernidx`/`lease-recheck` запуски
сделаны без `-KeepBinary`; SHA↔логи держатся на глобальном preserved binary +
`.sha256.txt`. Line2 закрыл это финальными keepbin-прогонами (ниже).

### Финальное подтверждение line2 (keepbin) и классификация line1

- `nrd-final-keepbin-20261103-033742`: PASS 20/375, собственный `Tests.exe` в
  dir, SHA копии = `2032536D…` — проверено.
- `kernidx-final-keepbin-20261103-033742`: PASS 3/48, копия с тем же SHA —
  проверено.
- `cpp-nrd-differential-report-final.json`: sha `EA44A390…`, 14/14 — не
  изменился.
- Line1 root-caused lease failure как **test fixture bug**, не production
  lifetime: лишний сильный owner `pFirst`; fix в
  `tests/src/PMMR/Test_PibdServing.cpp` (sha
  `4646F4DF…`) добавляет `pFirst.reset()`/`pSecond.reset()` и
  `REQUIRE(lease.use_count() == 1)`, assertion `destroyed == 1` не ослаблен;
  нормальный бинарник с фиксом ещё не собран (build slot pending).
- Line3 подтвердил fix под ASan: archive subset из copied tree (source manifest
  `857AA38E…`, TestsMini `2DA4727E…`) — lease 1/16 PASS, `PibdArchiveServer::*`
  7/168 PASS, ASan-отчётов нет; см.
  `evidence/line3/asan-line1-archive-subset-20261103.md`.
- ASan-результаты привязаны к source manifest/TestsMini SHA, а не к normal
  `2032536D…`; после пересборки normal Tests.exe нужен новый прогон/SHA.

### Финальная привязка к интегрированному FD7C0492 (2026-11-03)

- Line1 final normal binary: SHA
  `FD7C04922A2E016723531EC389E21D3AC1E8C0F99134F003595DB18149AB16E5`
  (9 154 048 B, PE ts `0x6AE8F63F`); repo binary, line1 preserved copy и line2
  `tools/audit/nrd/evidence/bin/Tests-FD7C04922A2E.exe` совпадают по SHA.
- Line3 independent isolated reruns (SHA-bound, `-KeepBinary`, профиль не
  изменён, exit 0): full suite **232/3926 PASS**, `Nrd*` **20/375 PASS**,
  `KernelIndex*` **3/48 PASS**, archive **7/168**, lease **1/16**; manifests
  `C4373967…`, `AA121B36…`, `13AEFAA1…`, `AD146118…`, `9F665959…`.
- Line2 binding на тот же SHA проверен: `nrd-bind-FD7C0492-20261103-034652`
  (20/375, своя копия FD7C0492…), `kernidx-bind-FD7C0492-…` (3/48, копия та же);
  `cpp-nrd-differential-report-FD7C0492.json` sha `EA44A390…` (14/14).
- Итог: task-5 evidence теперь SHA-bound к интегрированному зелёному бинарнику
  FD7C0492…; NRD и archive subsets подтверждены; подробности —
  `evidence/line3/normal-binary-fd7c-audit-20261103.md`.

### D2 mainnet NRD gate build DDC21F45 (2026-11-03, финальная привязка)

- Preserved binary `Tests-DDC21F45EBFE.exe`: SHA
  `DDC21F45EBFE6FA4846FFFB0FA5E376297EED5BFFEFAA5583E3A6B04B0BCF714`,
  9 172 480 B, PE ts `0x6AE8F9CD`; repo binary и preserved copy совпадают;
  `.sha256.txt` + build log BUILD OK; earlier FD7C0492/2032536D/0EDC0F81/
  06309AF0 сохранены.
- Line2 evidence проверен: `nrd-d2-…` 23/382 PASS, `kernidx-d2-…` 3/48 PASS,
  `full-d2-…` 236/3952 PASS — keepbin copy SHA DDC21F45…;
  `cpp-nrd-differential-report-D2-DDC21F45.json` `EA44A390…` (14/14).
- Line3 independent isolated reruns на preserved binary: Nrd* 23/382 PASS,
  KernelIndex* 3/48 PASS, archive 8/187 PASS, full 236/3952 PASS
  (manifests `DB5856FF…`, `859029CE…`, `D47CF517…`, `C92E2CA3…`).
- Provenance: +4 full-suite cases = +3 `Test_NrdPolicy` (D2) и +1 archive case
  из правки line1 `Test_PibdServing.cpp` 03:54:55 (sha `EDB1BA61…`).
- ASan D2 subset (source manifest `5A769673…`, TestsMini `4AB0CDDD…`):
  Nrd* 21/362 (NrdRules+NrdPolicy+Differential+IndexRebuild), archive 8/187,
  Segment 16/68, P2P 5/46, teardown PASS, zero ASan reports.
  `Test_NrdPreCommit` и `Test_KernelIndex` под ASan **BLOCKED** (link:
  BlockChain→Database→rocksdb; `asan-d2-blocked-kernidx/linker_errors.txt`).
- Итог: DDC21F45… — нормальный authoritative SHA (D2 gate); task-5 NRD evidence
  связан с ним; ASan остаётся source-bound.

### Run-only binding 8F10BFD6 (Lead-approved, финальная)

- Preserved `tools/audit/nrd/evidence/bin/Tests-8F10BFD67047.exe` +
  `.sha256.txt`: SHA
  `8F10BFD67047DD25FAB3C201C29FA125096EF4DD7E8EFFFE33B61627CF1D73D6`,
  9 172 480 B, PE ts `0x6AE8FC0B`; repo binary и line1-build-05 copy совпадают;
  SHA стабилен до/после прогонов.
- Line2 run-only: `nrd-bind-8F10BFD6-…` 23/382 PASS, `kernidx-bind-8F10BFD6-…`
  3/48 PASS; keepbin per-dir copies = 8F10BFD6…; differential `EA44A390…` 14/14.
- Line3 independent: `Nrd*` 23/382 (`4D0860F0…`), `KernelIndex*` 3/48
  (`B5CECC9A…`), archive 8/187 (`7463675E…`), full **236/3952** (`06B30BE1…`),
  все PASS, isolated, profile unchanged.
- **Provenance:** между сборками DDC21F45 (03:59:57) и 8F10BFD6 (04:09:31)
  изменённых компилируемых файлов **0** → 8F10 = source-identical rebuild;
  счётчики совпадают. DDC21F45 сохранён историческим; 8F10BFD6 — текущий
  authoritative normal SHA для NRD/archive evidence.
- ASan остаётся source-bound: copied tree файл-в-файл == текущие компилируемые
  исходники (mismatch=0), source manifest `5A769673…`/TestsMini `4AB0CDDD…`;
  `Test_NrdPreCommit`+`Test_KernelIndex` ASan-BLOCKED (rocksdb), покрыты
  normal-suite 8F10BFD6.

### FINAL build-07 C8C386BE — финальная привязка (2026-11-03)

- Tests
  `C8C386BE576A62604404E0FA02E56EE9E8F06B50EA0B15AD109F36F5AFF5B7FE`
  (9 241 600 B, PE ts `0x6AE90EE9`), GrinNode
  `F9EF06B4CB0962B50E6DDBD186E1C61F5872FAFF9E41A779EE7C317A2ED25BDA`;
  repo == line1-build-07 == line2 preserved copy; SHA стабилен.
- Line2 evidence на C8C проверено: Nrd* 30/428, NrdFalseBan* 5/17,
  KernelIndex* 3/48 (kept copies C8C…); paired oracle v3 11 computed /
  3 not_computed / 11 paired / 0 mismatches, report `57978B2E…`;
  oracle `00D0AA51…`.
- Line3 independent: full **3/3 PASS 245/4013**, archive 8/187, Nrd 30/428,
  KernelIndex 3/48, advert 2/15, Slatepack* 5/5.
- Флейк 930 (2/6 full fail) объяснён фиксированным wallet-портом 3421 и
  параллельными прогонами; на C8C сериализованно 4/4 full PASS (включая
  pipeline `ci-c8c-01`).
- **C8C386BE — финальный normal authoritative SHA** для NRD/archive evidence;
  930/8F10/DDC/FD7C — historical. ASan остаётся source-bound и для C8C
  **STALE** (mismatch=19), без PASS-claims.

### BUILD-09 0A3B7D66 — final binding WITH EXPLICIT EXCEPTION (Lead decision b)

- Tests
  `0A3B7D66E78F5E70A6D95616F0138D994AEB820796C0C77E98AEC7EAC4F2735B`
  (9 335 808 B, PE `0x6AE91AB1`), GrinNode
  `F90EEF389D1B35B7A17BD8E3C35F06AECFD9C39410C4F755713E1CD1F448A2F8`;
  repo == `C:\DHW\.tmp\build-09` copies; BUILD OK; frozen source (0 changes after
  build start; aggregate `832C378C…`; production `964FE6D7…`).
- Line2 evidence проверено: preserved `Tests-0A3B7D66E78F.exe`; BlockPipe 3/16,
  Nrd* 30/428, KernelIndex 3/48 PASS; bodyless FAIL (test-only short-chain header
  version); kill NOT VERIFIED (test-only fs restore).
- Line3 independent: full **249 cases / 248 passed / 1 failed** (4033/4032 ×2),
  единственный провал — test-only `Test_NrdBodylessRestart`; PASS: archive 8/187,
  Nrd 30/428, KernelIndex 3/48, advert 2/15, BlockPipe 3/16.
- ASan refresh: source manifest `CF78C188…`, TestsMini `12C3D16C…`, Nrd* 21/365,
  archive 8/187, Segment 16/68, P2P 7/61, teardown PASS, 0 reports; bodyless/kill/
  BlockPipe normal-only.
- **Binding:** build-09 — normal authoritative **WITH EXPLICIT EXCEPTION**
  (не fully green; bodyless-NRD + kill-recovery NOT VERIFIED; production 0 fails);
  C8C/build-08 и ранее — historical; ASan source-bound к build-09 production
  sources. Build-10 — backlog по отдельному approval.
