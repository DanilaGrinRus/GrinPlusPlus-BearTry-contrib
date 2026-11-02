# Ответ на независимый аудит 02.11.2026

Исходный аудит: `Gplus-аудит-и-очередность-работ-2026-11-02.md` (HEAD `820eedf`).
Ниже — что изменено после него, с явными границами PASS. Это не переписывание
исторических evidence.

## P0: Bulletproofs cache и batch

**Статус: PARTIAL, исправление кода + unit-тесты.**

- `BulletProofsCache` теперь ключуется по **commitment + точным байтам proof**,
  а не только по commitment. Подмена proof для того же commitment не попадает
  в кэш.
- `VerifyBulletproofs`:
  - пустой batch возвращает `true`, но пустой/слишком длинный отдельный proof
    отвергается до secp;
  - batch группируется по длине proof вместо применения длины первого элемента
    ко всем;
  - запись в кэш выполняется только после успешной проверки всей группы;
  - проверяется отказ scratch-space и конвертации commitments.
- Тесты: `Crypto::BulletproofsCacheBindsProofBytes`,
  `Crypto::BulletproofsRejectsEmptyBatchItem`,
  `Crypto::BulletproofsWarmCacheRejectsMutatedProof` (реальный proof, warm cache,
  изменённый байт отклонён).
- **Не сделано:** дифференциальный прогон против Rust, ASan/fuzz, проверка
  всех сетевых callers. Поэтому это не финальный PASS по P0.

## P0: NRD fail-closed

**Статус: PARTIAL, fail-closed реализован.**

- `RebuildRecentKernelIndex` больше не оставляет «пустой, но выглядящий готовым»
  индекс: при отсутствии блока/header до cutoff, неполном диапазоне или
  underflow числа kernels индекс помечается неполным.
- `ApplyBlock` отвергает NRD-ядро, если `m_recentNrdIndexComplete == false`;
  молчаливого принятия NRD-блока при неполной истории больше нет.
- Индекс публикуется только после успешного построения полного диапазона.
- **Не сделано:** end-to-end repro принятия NRD-нарушения, тесты
  `relative_height-1/равно/+1`, PIBD activation без тел, reorg/fault injection,
  сверка с Rust `MultiIndex`. Это остаётся P0-проверкой, а не закрытым пунктом.

## PIBD snapshot, cache и rate limit

**Статус: PARTIAL, защита реализована; конкурентный тест не полный.**

- `GetApiStateSnapshot()` теперь возвращает **read lease**: mutation lock
  (`m_stateWriteMutex`) удерживается, пока snapshot жив. PMMR/txhashset не могут
  измениться во время построения ответа. Lease освобождается до `SendAsync`;
  сетевой отправкой блокировка не удерживается.
- `CloseTxHashSet`/`OpenTxHashSet` также взяты под `StateWriteGuard`.
- Bitmap/root кэшируются по **horizon hash + MMR size + output root**; после
  reorg/смены H ключ не совпадает.
- PIBD rate limit: **500 запросов/мин на connection**, окно на
  `steady_clock`; сверх квоты запрос отбрасывается, а не обслуживается.
- Живые проверки:
  - quiescent H=3 315 600: identity-checked Rust PASS 4990/4990;
  - active `SYNCING_BLOCKS`: kernel-only 1637/1637 PASS, без deadlock.
- **Не сделано:** контролируемая остановка посреди сборки с apply/reorg,
  single-flight построения bitmap, глобальный rate/backpressure, p50/p95/p99,
  поведение при скачке времени, fairness/retry. Лимит остаётся per-connection,
  не глобальным.

## G-09: что по-прежнему не доказано

- H и roots взяты из C++ REST, не из независимо синхронизированной Rust-цепочки.
- Обычная Rust 5.5.2-нода: выбор H → PIBD → десегментация → активация →
  рестарт → догон — **NOT VERIFIED**.
- Негативный тест изменения proof/root/ID не выполнен.
- Прошлые timeout/reset и `Output Commitment not found` не объяснены.

## Не начиналось в этом срезе

- block sync после PIBD и `TransactionInput`/CoinView;
- API/wallet G-53;
- fuzz/sanitizers/CI и воспроизводимая сборка;
- G-49, G-44 staging-совместимость, G-45 mainnet dry-run;
- согласование старых `REMAINING*` и архивного manifest.

## Актуальные артефакты

- `GrinNode.exe`: SHA-256 `9DC493E201481362FE371F2B6A9AC59A542373EB56502FE95C4C4DAD3D5F1EA7`,
  **7 741 952 B = 7 560.5 KiB**.
- `Tests.exe`: SHA-256 `9E9014D4E827151A7E4BCB21D99BB8784866390EB6FC8983D9B987A2F53EA471`.
- Suite: **201 / 2309 PASS** (`C:\DHW\.tmp\tests-proof-mutation.log`).
- Wire PASS: `C:\DHW\.tmp\pibd-wire-lease.log`.
- Active-sync kernel PASS: `C:\DHW\.tmp\pibd-kernel-lease-active.log`.
- Исходники относительно 1.2.9: **+17 010 / −594** строк; 186 tracked + 1 новый
  файл; diff/new C++ ≈ **717.7 KiB**.

## Приоритет дальше

1. Закрыть P0 crypto/NRD реальными negative/differential тестами.
2. Конкурентный snapshot/reorg тест и глобальный backpressure.
3. Независимый Rust H + полный обычный Rust PIBD/legacy/resume.
4. Block sync, API/wallet, затем CI/fuzz и G-49.
