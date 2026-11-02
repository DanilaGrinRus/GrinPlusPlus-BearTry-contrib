# Normal-binary FINAL audit build-07 — C8C386BE (2026-11-03)

Line1 final integration candidate (atomic advertised-tip P0 + line2 task-6 +
round-2 tests). Re-hash/re-audit by line3; no build; SHA-bound evidence.
Lead declared 930148B6 non-final; this document is the **normal authoritative**
binding.

## 1. Binary identity (verified, repo == preserved copies)

| Артефакт | SHA-256 | Размер | PE ts |
|---|---|---|---|
| Tests.exe | `C8C386BE576A62604404E0FA02E56EE9E8F06B50EA0B15AD109F36F5AFF5B7FE` | 9 241 600 B | `0x6AE90EE9` (2026-11-02T22:30:01Z) |
| GrinNode.exe | `F9EF06B4CB0962B50E6DDBD186E1C61F5872FAFF9E41A779EE7C317A2ED25BDA` | 7 805 440 B | `0x6AE90E4D` |

- repo `bin\Release\Release\Tests.exe` == `C:\DHW\.tmp\line1-build-07\Tests-C8C386BE576A.exe`
  == line2 preserved `tools/audit/nrd/evidence/bin/Tests-C8C386BE576A.exe` (+`.sha256.txt`).
- repo GrinNode == preserved `GrinNode-F9EF06B4CB09.exe`.
- `build-console.log` line1-build-07 → BUILD OK; mtime Tests 05:30:01, GrinNode 05:27:25.

## 2. Независимые изолированные прогоны line3 — PASS

`run-tests-isolated.ps1` на preserved Tests copy, isolated HOME/TMP, профиль не
изменён, exit 0; full-01/nrd/kernidx/archive/advert — `-KeepBinary` (kept copy SHA
C8C386BE…):

| run | filter | результат | manifest sha256 |
|---|---|---|---|
| `normal-c8c-full-01` | full | **245 cases / 4013 assertions PASS** | `D2F48904…` |
| `normal-c8c-full-02` | full | **245 / 4013 PASS** | (NoEvidenceCopy) |
| `normal-c8c-full-03` | full | **245 / 4013 PASS** | (NoEvidenceCopy) |
| `normal-c8c-nrd-01` | `Nrd*` | **30 / 428 PASS** | `EFF8DE30…` |
| `normal-c8c-kernidx-01` | `KernelIndex*` | **3 / 48 PASS** | `BDC1AC83…` |
| `normal-c8c-archive-01` | `PibdArchiveServer::*` | **8 / 187 PASS** | `052F6FBE…` |
| `normal-c8c-advert-01` | `P2P::AdvertisedTip*` | **2 cases / 15 assertions PASS** (8+7) | `01D14772…` |
| `normal-c8c-slatepack-01..05` | `Slatepack*` | **5/5 PASS**, по 3 cases / 39 assertions | (NoEvidenceCopy) |

Полный suite: **3/3 PASS** последовательно; подтверждён и в CI pipeline (4-й
прогон). Заявленный line1 набор 245/4013 воспроизведён детерминированно в
сериализованной среде; wallet/Tor флейка на build-07 нет.

Дополнительно **line2 independent full** на этом же SHA после моей сериализации:
`full-c8c386-20261103-053707` — **PASS 245/4013**, exit 0, kept `Tests.exe` SHA
C8C…, isolated HOME/TMP, профиль не изменён, stderr 0 байт, stdout 14 861 B;
manifest sha `A3B9A607…`. Итого на C8C: **5/5 полных прогонов PASS** (3 line3
+ pipeline + line2), без wallet/Tor флейка.

## 3. Line2 evidence на C8C — проверено

- preserved `Tests-C8C386BE576A.exe` (SHA совпал) + `.sha256.txt`.
- `nrd-c8c386-20261103-053139`: Nrd* **30/428 PASS**, kept copy C8C…;
- `falseban-c8c386-…`: NrdFalseBan* **5/17 PASS**; `kernidx-c8c386-…`: 3/48 PASS;
- `nrd-paired-c8c386-…`: paired oracle v3 — 11 computed / 3 not_computed /
  11 paired / 0 mismatches; report
  `paired-differential-report-nrd-paired-c8c386-20261103-053139.json` sha
  `57978B2EC9C1DEF368EF15ADB40506A42A23815EDBC6C83DAD971CF1239C5AF6`;
- oracle binary `nrd-oracle-00D0AA518C36.exe` sha
  `00D0AA518C36C704131ABE203A5C833FC95528BCCD13D47952701CA02FEAAD85`;
- not_computed (не paired): missing-local-history,
  rebuild-rejects-historical-violation, rebuild-rejects-duplicate-history-block;
- fixture checks line2: missing `GRINPP_NRD_CASES` → expected FAIL, positive →
  PASS with report, unwritable report dir → expected FAIL; no false PASS.
  (Проверены наличием/статусами dirs; сами negative EXPECTED-FAIL прогоны
  остаются в evidence line2.)

## 4. CI pipeline финального среза

`tools/build/ci-pipeline.ps1`, run **`ci-c8c-01`** на C8C386BE:

- overall **PASS_WITH_UNCOVERED**;
- provenance PASS (Tests C8C386BE, GrinNode F9EF06B4, SHA до/после);
- normal-build NOT RUN (provided binary; no build);
- unit-regressions PASS (advertised-tip / false-ban / nrd-rollback-reorg /
  rebuild-interruption);
- full-suite PASS (**245 / 4013**);
- ASan-subset **NOT RUN (STALE, source mismatch=19)** — 5A769673…/4AB0CDDD…
  относится к более старому source set; refresh только по разрешению Lead;
- fuzz-smoke PASS (ADVISORY): 100 итераций на target, 0 ASan/timeout;
  coverage не измерялась; source binding stale;
- untested-zones REPORTED (rocksdb/DB ASan, coverage-guided fuzzing, snapshot/
  NRD fault-injection, wallet port determinism, live G-09, 0xC0000409,
  crafted-zip).
- Evidence: `evidence/ci/ci-c8c-01/summary.json`.

## 5. Environment flake resolution (930 → C8C)

На 930: 4 PASS / 2 FAIL полных прогонов, провалы только wallet/slatepack-
decrypt; line2: 14× `mg_start failed 127.0.0.1:3421`. На C8C при **сериализованных**
прогонах: 4/4 full PASS, Slatepack* 5/5 PASS, line2 не запускал full параллельно.
Вывод: 242/3980 vs 242/3944 — следствие фиксированного wallet-manager порта
`127.0.0.1:3421` и перекрывающихся прогонов, а не кода NRD/archive; правило CI —
wallet/full suite только последовательно либо с выделенными портами. Раздельных
HOME/TMP недостаточно.

## 6. Binding

- **Normal authoritative:** Tests `C8C386BE…`, GrinNode `F9EF06B4…`.
- **Historical:** 930148B6… (non-final, 245→ нет, 242/3980 flaky), 8F10BFD6…,
  DDC21F45…, FD7C0492… — сохранены, к финальному SHA не привязываются.
- **ASan:** refresh 05:42 под build-07 source snapshot: source manifest
  `57590D1E…`, TestsMini `19FE6005…`, subset PASS (Nrd* 21/365, archive 8/187,
  Segment 16/68, P2P 7/61, teardown PASS, 0 reports). `Test_NrdPreCommit`/
  `Test_NrdFalseBan`/`Test_NrdBatchSafety`/`Test_KernelIndex` — ASan-BLOCKED
  (BlockProcessor/BlockChain→Database→rocksdb). Этот ASan **привязан только к
  build-07 snapshot**; после post-05:50 WIP он STALE и **не** обновляется до
  freeze gate/build-08.
- Компилируемые источники C8C: atomic-tip set (AdvertisedTip.h, SyncStatus.h,
  MessageProcessor/Connection/P2PServerImpl, ChainState-комментарий,
  Test_P2PCompat) + archive/pibd/line2 изменения; ключевые SHA фиксируются в
  pipeline provenance.

## 8. Freeze gate, post-build WIP и границы доказательств (06:00+)

- C8C386BE/F9EF06B4 остаётся **accepted evidence binary только для своего
  pre-05:50 source snapshot** (сохранённые копии). Post-05:50 файлы
  `tests/src/P2P/Test_BlockPipeBan.cpp`, `tests/src/P2P/CMakeLists.txt`,
  `tests/CMakeLists.txt` — **line2 task-8**, не входят в C8C и **не
  привязываются** к нему (line1 подтвердил: его source changes после build-07
  нет; возможный один файл Ping/Pong — только после Lead approval/слота).
- **ASan 57590D1E/19FE6005 — build-07-only.** Против dirty tree он STALE
  (mismatch=3 на момент флага, вырастет с task-8b); refresh **приостановлен** до
  freeze gate и build-08, затем один ASan refresh по замороженному tree.
- **ci-c8c-02** (PASS_WITH_UNCOVERED, 245/4013, exclusive port-3421 lock +
  preflight) валиден для C8C и его source snapshot, **не** для dirty tree.
- Hosted CI (`/.github/workflows/line3-ci.yml` — self-hosted Windows template)
  **NOT VERIFIED**: runner не подключён, repo содержит uncommitted cross-line
  изменения.
- Flake-дисциплина сохранена: mg_start/Receive-класс объясняется фиксированным
  портом 3421; редкий Slatepack-decrypt FAIL причинно **не доказан** и остаётся
  FAIL/open risk, не относится автоматически к окружению.
- Freeze gate: ждём подтверждения line2 task-8 (freeze) и line1 task-7
  (нужны ли source edits); после Lead-команды — build-08, затем один ASan
  refresh и re-audit normal/ASan. Никаких claims на post-build файлы до этого.

## 7. Ограничения

- ASan не покрывает rocksdb/database; `Test_NrdPreCommit`/`Test_NrdFalseBan`/
  `Test_NrdBatchSafety`/`Test_KernelIndex` ASan-BLOCKED (LNK2019) — покрыты
  normal-suite C8C;
- coverage-guided fuzzing отсутствует; fuzz smoke — bounded, не safety proof;
- snapshot activation/NRD transitions — unit-симуляции (см. FAULT_INJECTION.md);
- live G-09 NOT VERIFIED; crafted-zip end-to-end NOT VERIFIED; исторический
  0xC0000409 root cause NOT VERIFIED (teardown-UAF — отдельная находка).
