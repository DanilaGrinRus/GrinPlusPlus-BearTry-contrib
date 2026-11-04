# Node status API: Rust-compatible schema (contract)

Дата: 2026-11-04. Статус: **реализовано и проверено на патченном build-59
(`GrinNode ECB1B55E…`) локально; публичный live API отдельно NOT VERIFIED.**
Проверка: `/v1/status.rust`, owner `get_status` с Bearer/X-API-Secret,
no-auth → ожидаемый `-32001`, foreign `get_version`/`get_tip`; full suite
**280/280, 4403 assertions PASS**. Evidence:
`evidence/legacy-build59-20261104/integration-20261104/API_CHECK_20261104.md`.

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

## Проверка (2026-11-04)

Проверено на патченном `GrinNode.exe` SHA-256
`ECB1B55E31CEFCB03DE517DDC5EB9FFF86203E4981448B9A9D8BB9709DE28B3D`
(локальный Floonet, header_sync, REST 25950):

1. Сборка `Tests;GrinNode` с `/W4 /WX` — PASS; full suite
   **280/280, 4403 assertions**.
2. `GET /v1/status` — HTTP 200, legacy-поля сохранены, объект `rust` и алиасы
   присутствуют; `rust.chain` — строка, `rust.tip` — объект, `connections` =
   inbound+outbound.
3. `POST /v2/owner get_status` с `Authorization: Bearer <secret>` и с
   `X-API-Secret` — HTTP 200, `result.Ok`; без секрета — ожидаемый JSON-RPC
   `-32001 Unauthorized`.
4. `POST /v2/foreign get_version` / `get_tip` — HTTP 200.
5. Сравнение `/v1/status.rust` и owner `Ok` на одном узле: совпали `chain`,
   `tip.height/hash`, `sync_status`, `connections`, `protocol_version`,
   `user_agent`; `sync_info.current_height` отличался только из-за активной
   header-синхронизации между запросами.
6. Restart на полном активированном профиле: `FULLY_SYNCED`, без повторной
   загрузки state; `rust`-объект не теряет tip.

Остаток: публичный Floonet API-прогон и fresh single-SHA download→activation
цикл — **NOT VERIFIED**; DACL-предупреждение скопированного `.api_secret`
безопасно документировано и не блокирует Bearer/X-API-Secret.
