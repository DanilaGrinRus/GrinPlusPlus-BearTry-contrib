# FAULT_INJECTION — приоритетные сценарии и текущее покрытие (линия 3)

Цель: явно отделить **симуляции отказов в unit-тестах** от **настоящего
fault-injection под ASan** и зафиксировать, что именно должны передать линии 1/2.

## Матрица приоритетных сценариев

| # | Сценарий | Текущее покрытие | Чего не хватает | Нужно от линии |
|---|---|---|---|---|
| F1 | Ошибка чтения при NRD rebuild (IBlockDB/header read error) | `Test_NrdIndexRebuild` (`NrdRebuildNeverPublishesPartialIndex`, `NrdRebuildPibdWithoutBodies...`), `Test_KernelIndex` (`KernelIndex ignores an interrupted unpublished rebuild`) — симуляции на fake DB | Инъекция ошибки чтения в конкретный вызов/итерацию + проверка fail-closed и отсутствия публикации partial index под ASan | line2: точки инъекции в `RebuildRecentKernelIndex`/`KernelIndex::Ensure`, ожидаемый статус |
| F2 | Прерывание до commit (staged state, crash до публикации) | `NrdPreCommitRefusalKeepsPreviousWorkingStateUntouched` (byte-identical previous state), `NrdRebuildNeverPublishesPartialIndex` | Crash/kill на границе staging→commit + повторный старт; ASan-run | line2: контракт staged/committed фаз и crash-safe cleanup |
| F3 | Ошибки snapshot activation (staging/rename/commit, disk-full) | `PibdArchiveServer::ValidateExtractedState`, `StagingCleanup`, `DeferredGarbageDrain`, `PibdSnapshot::*`, `PibdStateWriter::RejectsSnapshotWithoutPrunedHashes` | Инъекция: частичный файл, ошибка rename/commit, disk-full, crash между этапами; проверка отсутствия «полуопубликованного» снапшота | line1: точки активации и ожидаемые состояния/уборка |
| F4 | Rollback NRD после reorg | `NrdRollbackRestoresIndexSoReappliedBlockIsAccepted`, `NrdReorgBackup restores last committed index after failed alternative branch`, `NrdRebuildException is local-unavailable and never escapes` | Reorg + рестарт + read-error в середине rollback; ASan fault-injection | line2: последовательности reorg/rollback и допустимые состояния |

## Что уже включено в common CI harness

`tools/build/ci-regressions.ps1` (SHA-bound, isolated HOME/TMP, parallel для
непортовых групп):

- `advertised-tip` — `P2P::AdvertisedTipUsesHeaderHead`, `...ReadsOneSnapshot`;
- `false-ban` — `NrdFalseBan*` (локальное исключение без peer-ban);
- `nrd-rollback-reorg` — `NrdRollback*`, `NrdReorgBackup*`;
- `rebuild-interruption` — `KernelIndex*`, `NrdRebuild*`.

Это **unit-симуляции**, а не fault-injection. Полноценные F1–F3 требуют
инъекции ошибок на уровне IBlockDB/FS и ASan-сборки соответствующих TU.

## Технические ограничения ASan

- `KernelMMR/KernelIndex/TxHashSetProcessor` под ASan линкуются только в
  copied-tree ASan-профиле (см. `SANITIZERS.md`); `BlockChain→Database→rocksdb`
  остаётся uninstrumented, поэтому Database read-error сценарии в ASan не
  покрываются до ASan-пересборки rocksdb/C++ sysroot.
- `Test_NrdPreCommit` и `Test_KernelIndex` в ASan TestsMini **BLOCKED** по
  LNK2019 (точные ошибки: `evidence/line3/asan-d2-blocked-kernidx/linker_errors.txt`);
  они покрыты только normal-binary suite.
- Инъекция FS-ошибок для snapshot activation возможна без rocksdb (архивные
  PMMR/minizip), но требует отдельного harness и координации с line1.

## План внедрения (после передачи точек инъекции)

1. Обёртки `FailingBlockDB`/`FailingBlockStore` (инъекция по счётчику вызовов)
   для F1/F2 в copied-tree ASan-сборке.
2. FS-fault harness для F3: read-only/занятый каталог, обрыв rename, имитация
   disk-full через лимит записи; проверка инвариантов registry/lease.
3. Reorg/restart последовательности для F4 под ASan + повтор после рестарта.
4. Каждый сценарий — отдельный CI-фильтр с SHA бинарника и исходников; падение
   не подавлять; ASan-отчёт сохранять в evidence.
