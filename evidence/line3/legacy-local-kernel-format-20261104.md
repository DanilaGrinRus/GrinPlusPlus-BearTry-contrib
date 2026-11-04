# Legacy Rust 5.5.2 → C++ 1.2.9 TxHashSet: локальный baseline формата kernel PMMR

Дата: 2026-11-04 (Asia/Bangkok). Статус: **формат установлен; исправление в build-59;
полный live-цикл ещё NOT VERIFIED.**

## Стенд

- Rust: обычный `grin 5.5.2`, бинарник
  `C:\Users\nurli\AppData\Local\Temp\grin55-target\release\grin.exe`.
- Rust state: копия `C:\DHW\.tmp\task12-build13-7b\rust\chain_data` →
  `C:\DHW\.tmp\legacy-local-01\rust\chain_data` (5.78 GB).
- Rust config: копия, loopback P2P 25837, API 25831, `peers_allow =
  ["127.0.0.1:25836"]`, публичные seeds не используются.
- C++: build-58/59, Floonet, PIBD disabled, PIHD off, loopback 25836,
  REST 25850, профиль-копия `legacy-local-01/cpp*`; live legacy PID 3680/26880
  не затрагивались.
- Архив сохранён до удаления:
  `C:\DHW\.tmp\legacy-local-01\cpp\tmp\txhashset_failed.zip`,
  размер **3 067 445 011 B**,
  SHA-256 **D73EBD1811A40AFC876D4AA5B006D2D2E11B1BE5EDD0E102E8DF7D7FB2036C6C**.
- Заголовок: `3615bb625917582f7d028fe4ff738d82cb939b71ac768e885519b8798544ec9f`,
  height **3 315 600**, kernels **3 351 542**, outputs **3 425 950**.
  Rust state tip в том же прогоне: **3 318 655**.

## Что в ZIP

| entry | bytes |
|---|---:|
| `kernel/pmmr_data.bin` | 329 241 204 |
| `kernel/pmmr_hash.bin` | 214 694 752 |
| `output/pmmr_data.bin` | 101 298 546 |
| `output/pmmr_hash.bin` | 192 741 152 |
| `output/pmmr_leaf.bin.3615bb625917` | 849 328 |
| `output/pmmr_prun.bin` | 59 628 |
| `rangeproof/pmmr_data.bin` | 2 034 909 027 |
| `rangeproof/pmmr_hash.bin` | 192 741 152 |
| `rangeproof/pmmr_leaf.bin.3615bb625917` | 849 328 |
| `rangeproof/pmmr_prun.bin` | 59 628 |

`kernel/pmmr_size.bin` в ZIP **отсутствует**.

## Baseline C++ (build-58, до variable loader)

```text
Kernel MMR rewind failed: target kernels=3351542, actual MMR size=6709211
(Rewind: Failed to rewind to size 3351542)
```

- C++ `KernelMMR`/`DataFile<114>` ожидал `3 351 542 × 114 = 382 405 932 B`.
- Фактический Rust `kernel/pmmr_data.bin` = **329 241 204 B**, не фиксированный
  размер.
- Rust `pmmr_size.bin` (в state, не в ZIP) = **33 546 130 B = 3 354 613
  записей по 10 B**; формат записей big-endian `offset u64 + size u16`.
- Фактические размеры в state: `98` — 3 318 658, `114` — 25 186,
  `106` — 10 768, `108` — 1. Первая запись: offset 0, size 98, feature byte
  `0x01` (coinbase).

**Вывод:** число `3 354 438` из публичной ошибки соответствует kernel MMR
horizon, но точная причина публичного отказа остаётся **NOT VERIFIED**, пока
нет публичного ZIP. Локальный ZIP доказал несовместимость C++ fixed-114 с
Rust variable-size kernel data.

## Исправление

`src/PMMR/KernelMMR.h/.cpp`:

- `KernelMMR` хранит raw variable-size data + entries `{offset,size}`;
- при загрузке ZIP без size-файла размеры определяются по feature byte:
  98/106/108/114;
- `GetKernelAt` использует `EProtocolVersion::V2` для variable data;
- `ApplyKernel`:
  - storage bytes — V2 для variable (V1 для старого fixed layout);
  - **MMR leaf hash — всегда V1**, потому что Rust `KernelFeatures::write`
    в hash mode использует original v1 serialization;
- `Rollback` восстанавливает committed entry count; `Rewind` обрезает data
  по offset/size.

Build-59 Tests SHA-256:
`78218F3C56428CB7FDC483478277D33BC55D23DD2724B57B143ACA16876B8C68`.
Full suite: **276/276, 4387 assertions PASS**.
GrinNode SHA-256:
`60D8DFD05C87C8A516696D76ABDDCBF075D2D0CA28B62190181F430254766148`.

## Локальный прогон build-58 (до v1-hash fix)

- variable loader: **3 354 613 kernels** загружены из 329 241 204 B;
- TxHashSet validation на H=3 315 600 прошла kernel history/kernel sums/
  range proofs/kernel signatures; активация достигла confirmed height
  **3 315 600**;
- затем body sync блока H+1 упал: `Kernel root not matching` — build-58
  хешировал v2-сериализацию вместо Rust v1 hash mode; peer был забанен
  старым block-pipe exception.
- build-59 содержит v1-hash fix, но **body sync после него не запускался**
  по указанию пользователя ждать G-49.

## Ограничения

- Полный цикл ordinary Rust → C++: activation → body sync → FULLY_SYNCED →
  restart после build-59 **NOT VERIFIED**.
- Public Floonet повтор **NOT VERIFIED**.
- Static concerns legacy (code inspection, не воспроизведено):
  1. activation marker/backup удаляются до `pChainStateBatch->Commit()`;
     crash в окне оставляет новый live TxHashSet при старом chain;
  2. `Thread_SendTxHashSet` early returns не снимают `DisableSends(true)`;
  3. фиксированный `txhashset_failed.zip` может столкнуться при параллельных
     процессах/запросах.
  Это кандидаты, а не доказанные live-дефекты.
