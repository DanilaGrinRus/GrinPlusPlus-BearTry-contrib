# Gplus: статус трёх линий (оперативная доска)

Дата: 2026-11-03. Кодовая база: commit `793b7db`, docs `cb51030`; текущий
незакоммиченный integration candidate — **build-09**.

Единственный оперативный источник статуса — этот файл. Технические досье линий
содержат детали и историю; исторические срезы не считать текущими без сверки.

## Артефакты

### Build-09 — final candidate, независимый аудит ещё идёт

- Tests `0A3B7D66E78F5E70A6D95616F0138D994AEB820796C0C77E98AEC7EAC4F2735B`,
  9 335 808 B; preserved `C:\DHW\.tmp\build-09\Tests.exe`.
- GrinNode `F90EEF389D1B35B7A17BD8E3C35F06AECFD9C39410C4F755713E1CD1F448A2F8`,
  7 805 440 B; preserved `C:\DHW\.tmp\build-09\GrinNode.exe`.
- Содержит line1 Pong snapshot fix, line2 task-8 tests, `_dupenv_s` compile fix.
- Аудит build-09: full **249 cases / 248 passed / 1 failed**; единственный fail —
  test-only `Test_NrdBodylessRestart` (short-chain header-version constraint).
- PASS: BlockPipe* 3/16, Nrd* 30/428, KernelIndex* 3/48, archive 8/187,
  advert 2/15; ASan source-bound PASS, 0 reports; pipeline FAIL лишь из-за
  test-only full-suite fail.
- Bodyless-NRD и kill-recovery — **NOT VERIFIED** (ограничения тестового harness,
  не production; см. L2). Это не делает build-09 полностью зелёным.

### Build-08 — historical, non-green

`Tests 7994F191…` / `GrinNode 33258AE7…`; full suite имел 12 фейлов (11 env
wallet-port + bodyless test-only). Не является authoritative и не привязывается
к финальным PASS.

### Build-07 — historical accepted snapshot

`Tests C8C386BE…` / `GrinNode F9EF06B4…`; full 245/4013, archive 8/187,
advert 2/15, Nrd 30/428, KernelIndex 3/48, paired NRD 11/11/0. Только для своего
снимка; ASan для него source-bound и не относится к build-09.

## L1 — архивный serving / G-09

| ID | Статус | Доказательство / граница |
|---|---|---|
| L1-01 контракт | PASS (review) | `G09_ARCHIVE_SERVING.md` |
| L1-02 opt-in/archive server | PARTIAL | build-09 compiled; default-off; archive H=3315600; capabilities dynamic |
| L1-03 advertised tip | PASS (code/unit) | atomic snapshot + Pong header-head fix; advert 2/15 PASS; restart peer-wait root cause evidenced |
| L1-04 fixture/horizon | BLOCKED/PARTIAL | header 3320005 ⇒ Rust H=3317040 vs archive 3315600; 7C wrong-H refusal in progress |
| L1-05 full PIBD/no_sync | IN PROGRESS | 7B trimmed fixture: Rust header_sync from zero (bounded run); no PASS until activation/body sync/no_sync |
| L1-06 restart/reorg/negative/resources | NOT VERIFIED | после первого полного PIBD |
| L1-07 Output Commitment | NOT VERIFIED | нужен repro/диагноз |
| L1-08 ordinary live G-09 | NOT VERIFIED | fixed-H PASS — только историческое evidence |

## L2 — Crypto/NRD

| ID | Статус | Доказательство |
|---|---|---|
| Rust oracle/differential | PASS (computed subset) | oracle v5: 11 paired/0 mismatches; 2 computed_unpaired; 1 not_computed |
| NRD rebuild/reorg/restart | PASS (unit/synthetic) | Nrd* 30/428; batch-safety/rollback/exception tests |
| False-ban classification | PASS (fault-injected) | BlockPipe* 3/16; live organic peer-ban NOT VERIFIED |
| Real bodyless NRD state | NOT VERIFIED | short-chain HF3 header-version constraint; no production hook |
| Rebuild interruption/kill | NOT VERIFIED | child kill works; test-side restore fragile |
| D2 mainnet NRD | PARTIAL | block path wired; txpool/index policy OPEN |

## L3 — ASan/fuzz/CI

| ID | Статус | Доказательство |
|---|---|---|
| Isolation/fast-fail | PASS | isolated HOME/TMP; profile unchanged; 0xC0000409 root cause NOT VERIFIED |
| ASan | audit pending build-09 | build-08 ASan source-bound only; rocksdb/DB gap |
| Fuzz | PASS smoke | bounded fuzz; coverage-guided fuzzing absent |
| CI | PARTIAL | pipeline PASS_WITH_UNCOVERED; hosted deployment NOT VERIFIED |
| TestMain UAF | FIXED (test) | production Global lifetime hazard NOT VERIFIED |

## G-09 verdict

- **Fixed-H экспериментальный ordinary Rust PASS** — историческое доказательство,
  не готовность обычного binary.
- **Build-09 ordinary candidate**: unit/archive evidence PASS, но live
  PIBD/activation/no_sync пока **NOT VERIFIED**; 7C refusal и 7B full PIBD
  выполняются после Pong-фикса.
- Horizon/reorg/restart/negative/resources и Output Commitment — отдельные
  **NOT VERIFIED/BLOCKED**; full G-09 PASS не заявлять до их прохождения.

## Следующие ворота

1. 7C wrong-H safe refusal на build-09; 7B trimmed fixture → full PIBD/no_sync.
2. Restart/reorg/horizon/negative/resource checks; Output Commitment repro.
3. D2 txpool/index policy; real bodyless/kill harness — backlog с явными рисками.
4. ASan refresh build-09 source; затем финальный commit/SHA и обновление пакета.
