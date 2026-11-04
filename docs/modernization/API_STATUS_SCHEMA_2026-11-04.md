# Node status API: Rust-compatible schema (contract)

Дата: 2026-11-04. Статус: **контракт зафиксирован; исполняемая проверка build-60
ещё не выполнена (код не собран, ждём патчи).**

## Зачем

C++ `/v1/status` исторически отдавал legacy-схему Grin++ (`chain` — объект,
`network`, `state`, `sync_status` в верхнем регистре). Rust v5.5.2 owner
`get_status` отдаёт `api::types::Status`: `chain` — короткое имя сети,
`tip` — отдельный объект, `sync_status` — snake-case имя Rust. Разные схемы
заставляли UI/клиентов иметь две ветки разбора.

Выбран **совместимый** вариант: legacy-поля `/v1/status` не удаляются; к ним
добавляется точный Rust-объект `rust` из одного живого снимка chain/P2P.
`/v2/owner get_status` использует тот же builder. Пасхалок/зашитых статусов нет:
все значения берутся из текущего tip, sync snapshot и соединений.

## Контракт `GET /v1/status`

Legacy-поля сохраняются без изменения:

```json
{
  "protocol_version": 1000,
  "user_agent": "Grin++ 1.2.9",
  "sync_status": "FULLY_SYNCED",
  "waiting_reason": "",
  "block_sync": {...},
  "state": {"downloaded": 0, "download_size": 0, "processing_status": 100},
  "network": {"height": 3318655, "num_inbound": 0, "num_outbound": 1},
  "chain": {
    "height": 3318655,
    "hash": "2945348e...",
    "previous_hash": "66d4e7a0...",
    "total_difficulty": 1207035333622
  },
  "header_height": 3318655
}
```

Добавляются Rust-совместимые поля:

```json
{
  "rust": {
    "chain": "floonet",
    "protocol_version": 1000,
    "user_agent": "Grin++ 1.2.9",
    "connections": 1,
    "tip": {
      "height": 3318655,
      "last_block_pushed": "2945348e...",
      "prev_block_to_last": "66d4e7a0...",
      "total_difficulty": 1207035333622
    },
    "sync_status": "no_sync",
    "sync_info": {...}
  },
  "tip": {...},
  "connections": 1,
  "chain_type": "floonet",
  "sync_status_rust": "no_sync"
}
```

`rust` — точная схема Rust `Status`; верхнеуровневые `tip`, `connections`,
`chain_type`, `sync_status_rust` — удобные алиасы без коллизии с legacy
`chain`-объектом. `sync_info` присутствует только когда есть реальные данные
фазы; отсутствие поля означает отсутствие данных, а не подставленное значение.

## `POST /v2/owner get_status`

Возвращает тот же `rust`-объект в JSON-RPC:

```json
{"jsonrpc":"2.0","result":{"Ok": { ...Status... }},"id":1}
```

Owner-доступ — `PRIVATE`: `.api_secret` должен иметь owner-only файл/DACL.
Скопированный профиль с унаследованным DACL может закономерно получить **401
`secret file owner/DACL is not owner-only`**. Это проверка доступа, а не разница
схемы; для локального стенда DACL нужно исправить, а не ослаблять проверку.

## Соответствие `sync_status`

| C++ `ESyncStatus` | Rust-имя в `rust.sync_status` | `sync_info` |
|---|---|---|
| `SYNCING_HEADERS` | `header_sync` | `current_height`, `highest_height` |
| `SYNCING_TXHASHSET` | `txhashset_download` | `downloaded_size`, `total_size` |
| `PROCESSING_TXHASHSET` | `txhashset_rangeproofs_validation` | `processing_status` (реальный C++ процент) |
| `TXHASHSET_SYNC_FAILED` | `txhashset_sync_failed` | `failed: true` |
| `SYNCING_BLOCKS` | `body_sync` | `current_height`, `highest_height` |
| `WAITING_FOR_PEERS` | `awaiting_peers` | — |
| `NOT_SYNCING` | `no_sync` | — |

Ограничение: C++ имеет одну фазу `PROCESSING_TXHASHSET`, Rust разделяет
setup/range-proof/kernel phases. Имя `txhashset_rangeproofs_validation`
отражает фактический прогресс C++ и не подменяет отсутствующие счётчики
`rproofs`/`kernels`; в `sync_info` отдаётся `processing_status`.

## Совместимость

- Старые клиенты, читающие `chain.height`, `network`, `state`,
  `sync_status`/`FULLY_SYNCED`, продолжают работать.
- Новые клиенты читают `rust` (точная Rust-схема) или owner `get_status`.
- Удаление legacy-полей и переименование `chain` в строку — отдельное
  breaking-решение; в этом контракте не выполняется.
- `/v1/status` остаётся HTTP JSON без JSON-RPC; owner-ответ — JSON-RPC.

## Проверка (после build-60)

1. Собрать `Tests;GrinNode` с `/W4 /WX`.
2. Unit/contract-тест: `/v1/status` содержит legacy-поля и `rust`;
   `rust.chain` — строка, `rust.tip.height` = confirmed tip, `connections` =
   inbound+outbound; `sync_status` соответствует таблице.
3. Owner `get_status` возвращает тот же `Ok`-объект при корректном DACL.
4. Live: C++ build-60 на loopback, `FULLY_SYNCED`, сравнить с Rust
   `get_status` на одном горизонте; расхождения только документированные
   (C++ protocol/user_agent/phase granularity).
5. Restart: статус не становится 0/заглушкой; `rust` не теряет tip.

До выполнения пунктов 1–5 статус кода: **NOT VERIFIED**, не PASS.
