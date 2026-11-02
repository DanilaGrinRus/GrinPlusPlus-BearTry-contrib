# Build-08 historical audit (7994F191) — NOT GREEN, superseded by build-09

Lead-ordered frozen build-08 (line1 Ping/Pong fix + line2 task-8 tests). Audit by
line3; build-08 is **historical only**. Two task-8 test-only defects were found;
Lead approved test-only fixes and ordered build-09. ASan refresh for build-08 was
**cancelled/paused** because it would be immediately stale.

## 1. Identity (verified; preserved copies)

| Артефакт | SHA-256 | Size | PE ts |
|---|---|---|---|
| Tests.exe | `7994F1915F3FC770A5658034174808C1AC4CC8E1CB1410B540259D762ECFE45F` | 9 316 864 B | `0x6AE9185D` |
| GrinNode.exe | `33258AE746DAA733CCF12529DF591D5C2D340B68A694A90B41C00A2CDC9BE974` | 7 805 440 B | `0x6AE917B2` |

repo == `C:\DHW\.tmp\line1-build-08\Tests-retry.exe` / `GrinNode-retry.exe`
(SHA совпал) == line2 preserved `Tests-7994F1915F3F.exe` (+`.sha256.txt`);
`build-console-retry.log` → BUILD OK. Attempt-1 invalid pair: GrinNode-only
rebuild + stale Tests copy (C8C386BE) — не evidence.

Frozen source: 1289 files, aggregate
`EAF90A6F0C9818140A52BB4C186CF54A3AFDB628F37589949A5848FABAA84277`; **0**
compiled-source файлов изменено после старта сборки (06:03:03); production
`src/P2P/MessageProcessor.cpp` = `964FE6D7…`. Snapshot:
`evidence/line3/build-08-freeze-snapshot-final.json`.

## 2. Line3 independent normal audit — full suite FAIL

Preserved Tests copy, isolated HOME/TMP, профиль не изменён; `-KeepBinary` для
ключевых фильтров; port 3421 preflight: свободен.

| filter | результат |
|---|---|
| full suite (2 прогона) | **FAIL: 249 cases — 237 passed / 12 failed; assertions 4001→3989** (2-й прогон 4017→4005) |
| `PibdArchiveServer::*` | **8/187 PASS** |
| `Nrd*` | **30/428 PASS** |
| `KernelIndex*` | **3/48 PASS** |
| `P2P::AdvertisedTip*` | **2 cases/15 assertions PASS** |
| `BlockPipe*` | **3 cases/16 assertions PASS** |

### Провалы (12)

- **11× environment/wallet-port:** 8 API wallet tests + 2 `Test_InitializeNewWallet`
  + 1 `Test_TorConnection` — все `Create - mg_start failed 127.0.0.1:3421`
  (8 API: CreateWallet×2, EstimateFee, Login×3, Send, Receive; плюс Wallet
  init ×2, Tor ×1). После моего port-preflight конфликт возник уже во время
  прогона (line1/line2 активность); это не NRD/archive regression.
- **1× test-only task-8 defect:** `tests/src/BlockChain/Test_NrdBodylessRestart.cpp(103)`
  `REQUIRE(pChain->AddBlock(block30) == EBlockChainStatus::SUCCESS)` →
  `VerifyKernelFeatures: NRD kernel ... in block 30 is not valid before header
  version 4` — TestMiner выдаёт header version 1, NRD требует 4. Production не
  виноват; line2 подтвердил test-only fix.
- `Test_NrdKillHarness.cpp` имеет `TEST_CASE(..., "[.]")` (hidden) и **не указан
  в `tests/CMakeLists.txt`** → в build-08 Tests он не компилировался/не
  запускался; line2 гонял его отдельно (`fs::copy` restore error) — также
  test-only, fix запрошен.

## 3. Line2 evidence на build-08 — проверено

- preserved `Tests-7994F1915F3F.exe` SHA = 7994F191…; dirs keep own copies.
- `blockpipe-b08-…`: **3/16 PASS**; `nrd-b08-…`: **30/428 PASS**;
  `kernidx-b08-…`: **3/48 PASS**; `bodyless-b08-…`: **FAIL** (4 assertions,
  3 passed/1 failed; тот же test-only header-version дефект).
- `kill-b08-…` — отдельный harness-run (hidden test, не в CMake/Tests);
  `fs::copy` restore error, backup tree intact — test-only fix запрошен.

## 4. Вывод и статус

- Build-08 **НЕ зелёный** и **не финальный**: 12 full-suite провалов = 11 env
  (wallet port 3421) + 1 task-8 test-only; production-кода в провалах нет.
- Lead: test-only fixes approved; build-09 follows. Build-08 evidence —
  historical only; к build-09 не привязывается.
- **ASan refresh для build-08 отменён/приостановлен** по Lead (стал бы сразу
  stale из-за build-09). Один ASan refresh — после build-09 SHA на frozen source.
- C8C386BE/F9EF06B4 остаются предыдущим accepted snapshot; build-08 —
  historical; build-09 — ожидаемый normal authoritative.
