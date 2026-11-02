# G-07: NRD (NoRecentDuplicate) kernels

> **Исторический срез.** Дальнейшее описание может не соответствовать текущему коду. Актуальные результаты и ограничения: [NRD_DIFFERENTIAL.md](NRD_DIFFERENTIAL.md); общий статус: [THREE_LINE_STATUS.md](THREE_LINE_STATUS.md).
>
> **Текущие факты (2026-11-03):** NRD запрещён до header version **4** (HF3);
> mainnet NRD block-gate подключён (`Config::IsNrdKernelEnabled`), txpool/index
> policy остаётся **OPEN/UNWIRED**. Pre-commit rebuild, rollback и false-ban
> классификация покрыты тестами; реальный PIBD-state/live peer ban — NOT VERIFIED.
> Не используйте раздел «Status» ниже как текущую инструкцию.

## Implemented

- `TxHashSet` keeps a recent NRD kernel index:
  `excess -> [(kernel PMMR position, block height)]`.
- On `ApplyBlock`, every NRD kernel is checked against the previous occurrence:
  - duplicate NRD kernels in the same block are rejected;
  - `current_height - previous_height < relative_height` is rejected;
  - entries older than `2 * WEEK_HEIGHT` are ignored/pruned.
- On `Rewind`, NRD entries introduced by the rewound block are removed using
  the previous header's kernel MMR size.
- Startup and PIBD activation rebuild the recent index from block history
  (last two weeks).
- `BlockValidator` rejects NRD kernels before header version 4 (HF3).

The pure relative-height/duplicate rules are in `include/PMMR/NrdRules.h` and
covered by unit tests for `relative_height` 1/2 and the recent cutoff.

## Status (historical snapshot)

**PARTIAL at the time of this snapshot.** The enforcement path was implemented and
the suite passed, but:

- the index was in-memory and rebuilt on startup, not persisted incrementally in
  RocksDB like Rust's `MultiIndex`;
- no full block-processing/reorg NRD test and no live NRD peer test had been run.

Current status supersedes this section: see `NRD_DIFFERENTIAL.md` and
`THREE_LINE_STATUS.md`. Txpool/index-policy parity remains open.

## Artifacts (historical)

`GrinNode E16A014E…`, `Tests CC6472B1…`; full suite 194 cases / 2227 assertions.
These artifacts are not current; current normal artifacts are recorded in
`THREE_LINE_STATUS.md`.
