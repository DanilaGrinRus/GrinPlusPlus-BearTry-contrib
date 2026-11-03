# Gplus: статус трёх линий (оперативная доска)

Дата: 2026-11-03. Кодовый integration commit: `793b7db`; текущий финальный
кандидат — **build-10** (`Tests C17C3BC0…`, `GrinNode 60D7F7A6…`).

Единственный оперативный источник статуса — этот файл. Технические досье линий
содержат детали и историю; исторические срезы не считать текущими без сверки.

## Артефакты

### Build-10 — final candidate

- Tests `C17C3BC04BE04C501DB425E3617A71E61985E9257EF41D879E130E28F8B621A4`,
  9 410 560 B; preserved `C:\DHW\.tmp\build-10\Tests.exe`.
- GrinNode `60D7F7A6B3FFCC702B260E80036B7143A06DB9B036AD3DF3754B3C7B9438CB23`,
  7 809 024 B; preserved `C:\DHW\.tmp\build-10\GrinNode.exe`.
- Clean full: **254 cases / 253 passed / 1 failed**; единственный fail —
  test-only `Test_NrdBodylessRestart`; production/agreed suites 0 fails.
- PASS: archive 8/187, Nrd 30/428, KernelIndex 5/74, GetKernelHandler 2/49,
  schema-v2 2/26, duplicate-range 1/10, advert 2/15; ASan refresh pending/see L3.
- Final-binary persisted-state restart: no_sync tip 3318655 / `2945348e…`,
  C++ FULLY_SYNCED, 0 PIBD re-downloads; kernel index published (gen1, 3,354,613).
- Bodyless-NRD и kill-recovery — **NOT VERIFIED**.

### Build-09 — historical (7B/restart PASS scenario)

`Tests 0A3B7D66…` / `GrinNode F90EEF38…`; ordinary 7B PASS + restart PASS;
full 249/248/1 (bodyless). ASan build-09 source-bound `CF78C188…`/`12C3D16C…`.

### Build-08 — historical, non-green

`Tests 7994F191…` / `GrinNode 33258AE7…`; full suite имел 12 фейлов
(11 env wallet-port + bodyless test-only), не authoritative.

### Build-07 — historical accepted snapshot

`Tests C8C386BE…` / `GrinNode F9EF06B4…`; full 245/4013, archive 8/187,
advert 2/15, Nrd 30/428, KernelIndex 3/48, paired NRD 11/11/0.


## L1 — архивный serving / G-09

| ID | Статус | Доказательство / граница |
|---|---|---|
| L1-01 контракт | PASS (review) | `G09_ARCHIVE_SERVING.md` |
| L1-02 opt-in/archive server | PARTIAL | build-09 compiled; default-off; archive H=3315600; capabilities dynamic |
| L1-03 advertised tip | PASS (code/unit) | atomic snapshot + Pong header-head fix; advert 2/15 PASS; restart peer-wait root cause evidenced |
| L1-04 wrong-H refusal | PASS (bounded) | build-09: Rust H=3317040 vs C++ archive H=3315600; exact-hash refusals, no segments/wrong data/crash; fallback NOT VERIFIED |
| L1-05 full PIBD/no_sync | PASS (scenario) | 7B: headers from zero → H=3315600 → PIBD 10,203,442 → validation → body sync → no_sync 3318655/2945348e…; fixture/loopback caveats |
| L1-06 restart | PASS (scenario) | persisted profile: awaiting_peers → no_sync same tip/hash; 0 PIBD re-downloads |
| L1-06b reorg/horizon/negative/resources | BLOCKED/NOT VERIFIED | no +720 source; negative wire/resource not executed |
| L1-09 old-archive recovery | **FAIL / root cause NOT VERIFIED** | H=3313440 recovery fails at block 3317930: 1 input / log says 0 spent positions, but canonical DB has a 1-entry record (hash 30c524…, leaf 6856589); discrepancy unresolved; H=3315600 serving unaffected; [ARCHIVE_RECOVERY_2026-11-03.md](ARCHIVE_RECOVERY_2026-11-03.md) |
| L1-10 atomic archive publication | OPEN | direct final-path copy, no fsync/atomic rename; readiness via exists; fix in progress |
| L1-07 Output Commitment | NOT VERIFIED | 0 occurrences in 7B/restart; static mechanism, controlled repro BLOCKED |
| L1-08 ordinary live G-09 | PASS only for 7B scenario | [G09_ORDINARY_LIVE_PASS_2026-11-03.md](G09_ORDINARY_LIVE_PASS_2026-11-03.md); not full G-09/Mainnet |

## L2 — Crypto/NRD

| ID | Статус | Доказательство |
|---|---|---|
| Rust oracle/differential | PASS (computed subset) | oracle v5: 11 paired/0 mismatches; 2 computed_unpaired; 1 not_computed |
| NRD rebuild/reorg/restart | PASS (unit/synthetic) | Nrd* 30/428; batch-safety/rollback/exception tests |
| False-ban classification | PASS (fault-injected) | BlockPipe* 3/16; live organic peer-ban NOT VERIFIED |
| Real bodyless NRD state | NOT VERIFIED | short-chain HF3 header-version constraint; no production hook |
| Rebuild interruption/kill | NOT VERIFIED | child kill works; test-side restore fragile |
| D2 mainnet NRD | PARTIAL | block path wired; txpool/index policy OPEN |
| API `get_kernel` direct scan | CONFIRMED UB, fix deferred | local optional dangles after fallback if-block; indexed path safe; task-9; not G-09/consensus blocker |
| Kernel-index duplicate excess | CONFIRMED, fix deferred | fixture: 3,354,613 leaves / 3,354,326 unique; 281×2 + 3×3 = 287 extras; RocksDB Put overwrites excess keys, verification impossible; fallback direct scan; task-9 design A/B; not consensus/PIBD defect claim |

## L3 — ASan/fuzz/CI

| ID | Статус | Доказательство |
|---|---|---|
| Isolation/fast-fail | PASS | isolated HOME/TMP; profile unchanged; 0xC0000409 root cause NOT VERIFIED |
| ASan | PASS subset (build-10) | source `776B0DB3…` (1292 files), TestsMini `497353EC…`; Nrd 21/365, archive 8/187, Segment 16/68, P2P 7/61, teardown PASS, 0 reports; rocksdb/DB gap |
| Fuzz | PASS smoke | bounded fuzz; coverage-guided fuzzing absent |
| CI | PARTIAL | pipeline PASS_WITH_UNCOVERED; hosted deployment NOT VERIFIED |
| TestMain UAF | FIXED (test) | production Global lifetime hazard NOT VERIFIED |

## G-09 verdict

- **Build-09 ordinary scenario: 7B PASS + restart PASS.**
  Rust с чистыми headers/state → H=3315600 → PIBD 10 203 442 → validation →
  activation → body sync → `no_sync` tip 3318655 / hash `2945348e…`; restart без
  повторного PIBD. Это **scenario PASS**, не полный G-09/Mainnet.
- Ограничения: C++ fixture-based/loopback, no public peers; reorg/horizon BLOCKED;
  negative wire/resources NOT VERIFIED; Output Commitment NOT REPRODUCED;
  liveness idle-timeout и archive-regeneration warning открыты; build-10 final
  restart ещё не выполнен.

## Следующие ворота

1. Build-10 full/ASan audit + final-binary persisted restart; bind final SHA.
2. Reorg/horizon (source needed), negative proof/root/ID, resource limits; Output Commitment repro.
3. D2 txpool/index policy; bodyless/kill harness — backlog с явными рисками.
4. ASan refresh build-09 source; затем финальный commit/SHA и обновление пакета.
