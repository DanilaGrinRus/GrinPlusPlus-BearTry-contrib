# Gplus: статус трёх линий (интеграционный срез)

Дата: 2026-11-03. База: `e0a5d2d` + незакоммиченные изменения линий 1–3.
Финальный интеграционный кандидат: **Tests `C8C386BE…` / GrinNode `F9EF06B4…`**.

## Артефакты

- `Tests.exe` SHA-256 `C8C386BE576A62604404E0FA02E56EE9E8F06B50EA0B15AD109F36F5AFF5B7FE`,
  9 241 600 B, PE `0x6AE90EE9`; preserved `C:\DHW\.tmp\line1-build-07\Tests-C8C386BE576A.exe`.
- `GrinNode.exe` SHA-256 `F9EF06B4CB0962B50E6DDBD186E1C61F5872FAFF9E41A779EE7C317A2ED25BDA`,
  7 805 440 B; preserved `C:\DHW\.tmp\line1-build-07\GrinNode-F9EF06B4CB09.exe`.
- Full suite: **245 cases / 4013 assertions PASS**, независимо 3× line3 + line1.
- Archive subset **8/187**, advert-tip **2/15**, `Nrd*` **30/428**, `KernelIndex*` **3/48**,
  paired NRD **11 computed / 3 not_computed / 11 paired / 0 mismatches**.
- ASan: **source-bound, STALE для C8C**, не привязан к нормальному SHA.
- Тестовый HOME/TMP изолирован; пользовательский профиль не изменён.
- Старые 205/3700, 236/3952, 242/3980 относятся к другим SHA и не являются текущим PASS.

## L1 — архивный serving / G-09

| ID | Статус | Доказательство / граница |
|---|---|---|
| L1-01 контракт | PASS (review) | `G09_ARCHIVE_SERVING.md` |
| L1-02 opt-in/archive server | PARTIAL | build-07 compiled; default-off; archive ready H=3315600; capability gating verified |
| L1-03 advertised tip | PARTIAL | candidate height/TD advertise fixed; atomic snapshot test PASS; L1 rerun reached header head 3320000/3320005, but Rust stayed `header_sync`/`awaiting_peers` |
| L1-04 fixture/horizon | BLOCKED | header head 3320005 => Rust H=3317040, archive H=3315600; safe refusal not reached, full PIBD not exercised |
| L1-05 restart/horizon/reorg/negative | NOT VERIFIED | no consistent fixture/live path |
| L1-06 Output Commitment | NOT VERIFIED | static root-cause candidate only |
| L1-07 ordinary live G-09 | NOT VERIFIED | fixed-H experimental PASS remains historical only |

## L2 — Crypto/NRD

| ID | Статус | Доказательство |
|---|---|---|
| Rust oracle/differential | PASS | oracle v3, 11 computed paired 11/11, 0 mismatches; 3 not_computed explicitly listed |
| NRD rebuild/reorg/restart | PASS (synthetic) | `Nrd*` 30/428; batch-safety/rollback/exception tests |
| False-ban path | PASS (unit/static) | `NrdFalseBan*` 5/17; live peer-ban repro NOT VERIFIED |
| D2 mainnet NRD | PARTIAL | block path wired; txpool/index policy unwired |
| crafted zip / real PIBD state | NOT VERIFIED | documented blockers |

## L3 — ASan/fuzz/CI

| ID | Статус | Доказательство |
|---|---|---|
| Isolation/fast-fail | PASS | isolated runner; profile unchanged; 0xC0000409 root cause NOT VERIFIED |
| ASan | PASS subset / STALE для C8C | source manifest `5A769673…`, TestsMini `4AB0CDDD…`; rocksdb/DB gap |
| Fuzz | PASS smoke | bounded fuzz targets; coverage-guided fuzzing absent |
| CI | PARTIAL | `ci-c8c-01` PASS_WITH_UNCOVERED; hosted pipeline NOT DEPLOYED |
| TestMain UAF | FIXED (test) | production Global lifetime hazard NOT VERIFIED |

## G-09 verdict

- **Fixed-H экспериментальный ordinary Rust PASS**: сохраняется как историческое
  доказательство, но не относится к обычному repo-бинарнику.
- **Обычный repo GrinNode**: build/unit/archive/ASan-subset зелёные, но live
  PIBD/activation/no_sync **NOT VERIFIED**; L1 остановлен на
  `header_sync`/`awaiting_peers`, fixture H mismatch.
- Смена горизонта, reorg, restart, negative proof/root/ID и ресурсные лимиты —
  отдельные **NOT VERIFIED/BLOCKED** статусы.

## Следующие ворота

1. Согласованный fixture (candidate==body==3318655 или архив H=3317040); повторить L1 с нуля.
2. L1 archive refusal при H mismatch и L3 restart/reorg/negative.
3. Output Commitment repro; полный body sync.
4. D2 txpool gate; crafted-zip/real PIBD-state NRD.
5. ASan refresh на финальном source; CI deployment.
