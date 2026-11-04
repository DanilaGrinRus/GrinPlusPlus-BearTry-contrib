# Legacy live: ordinary Rust 5.5.2 → C++ build-59, локальный loopback

Дата: 2026-11-04 07:30–07:49 (Asia/Bangkok). Статус: **локальный body sync +
FULLY_SYNCED + restart PASS для build-59; полный один-бинарь цикл скачивания
архива/активации build-59 не повторён.**

## Стенд

- Rust: обычный `grin 5.5.2`,
  `C:\Users\nurli\AppData\Local\Temp\grin55-target\release\grin.exe`,
  state-копия `C:\DHW\.tmp\legacy-local-01\rust\chain_data`, loopback P2P 25837,
  API 25831; публичные peers не подключались.
- C++: build-59 `GrinNode.exe`
  SHA-256 `60D8DFD05C87C8A516696D76ABDDCBF075D2D0CA28B62190181F430254766148`,
  Floonet, PIBD disabled, PIHD off, loopback 25836, REST 25850,
  профиль `C:\DHW\.tmp\legacy-local-01\cpp2`.
- Тесты build-59 `Tests.exe`
  SHA-256 `78218F3C56428CB7FDC483478277D33BC55D23DD2724B57B143ACA16876B8C68`;
  full suite **276/276, 4387 assertions PASS**.
- Live legacy PID 3680/26880, публичные порты и state не затрагивались.

## Что проверено

1. Архив `3615bb…`, H=3 315 600 был скачан у локального Rust и активирован
   build-58 (variable-size kernel data). Архив сохранён:
   SHA-256 `D73EBD1811A40AFC876D4AA5B006D2D2E11B1BE5EDD0E102E8DF7D7FB2036C6C`.
2. build-58 после активации сделал body sync до блока H+1, но получил
   `Kernel root not matching` и забанed loopback peer. Причина: C++ хешировал
   kernels в v2, тогда как Rust kernel MMR использует v1 hash mode при
   variable-size v2 storage.
3. В build-59 добавлено: storage V2, MMR leaf hash **V1**; `KernelMMR`
   variable-size loader/size/rollback.
4. build-59 загрузил активированное состояние, body sync до
   **height 3 318 655**, `FULLY_SYNCED`; kernel root ошибок после исправления
   нет.
5. Restart build-59 на том же профиле: снова `FULLY_SYNCED`, height 3 318 655,
   **нового `Requesting TxHashSet`/полной загрузки state нет**; kernel index
   пересобран из локального state (generation 3, 3 354 613 kernels).

## Итоговый статус (07:49:29)

```json
{
  "chain": {
    "hash": "2945348ef765160a160008cfd7c2b41aa44ed3cea77d6bcfe2083acf0fc4bbf1",
    "height": 3318655,
    "total_difficulty": 1207035333622
  },
  "header_height": 3318655,
  "sync_status": "FULLY_SYNCED",
  "network": {
    "height": 3318655,
    "num_inbound": 0,
    "num_outbound": 1,
    "total_difficulty": 1207035333622
  }
}
```

Лог рестарта: `C:\DHW\.tmp\legacy-local-01\cpp2\profile\LOGS\Node.log`,
`07:44:08 Starting Grin++`, `07:44:59 Kernel MMR loaded 3 354 613`,
`07:46:29 FULLY_SYNCED`, без `Requesting TxHashSet`.

## Честные ограничения

- Скачивание/проверка/активация архива сделаны build-58; body sync/restart —
  build-59. **Единый build-59 цикл download → verify → activation не повторён**
  (полная range-proof проверка ~22–28 мин на одном ядре).
- G-49 experimental parallel worker не интегрирован в build-59; его результаты
  отдельные и требуют review/интеграции.
- Публичный Floonet после локального успеха **NOT VERIFIED**.
- Mainnet не затрагивался.

## Статические замечания legacy (по коду, НЕ воспроизведены)

1. **Атомарность активации/chain commit:** `TxHashSetProcessor.cpp:214–227`
   удаляет marker и backup до `pChainStateBatch->Commit()` на строке 267.
   Crash в этом окне оставляет новый live TXHASHSET при старом chain/index;
   `TxHashSetStaging::Recover` при существующем live не восстанавливает backup.
   Это code-level окно, live-падение не воспроизведено.
2. **Освобождение send state:** `TxHashSetPipe.cpp:262–270,315–321` ранние
   `return` после `DisableSends(true)` (нет header / не открылся файл) не
   вызывают `DisableSends(false)`. Code-level, не воспроизведено.
3. **Temp collision:** имя `txhashset_<shorthash>_attempt_<n>.zip` уникально в
   одном `TxHashSetPipe`, но фиксированный `txhashset_failed.zip` и
   `txhashset_activation.pending` могут столкнуться между процессами/нодами.
   Code-level риск, не воспроизведён.
