# Verification P0 — node API routes and access control

## Implemented

- Ported `Net/Servers/RPC/ApiAccess` from the API integration kit, adapted to
  this tree.
- `RPCMethod::GetAccessLevel()` and central enforcement in `RPCServer`.
- `RPC::ErrorCode::UNAUTHORIZED` (-32001) / `FORBIDDEN` (-32002).
- `NodeRestServer` initializes the API secret and requires ADMIN access for
  `/v1/shutdown`.
- `UpdateConfigHandler` is ADMIN; `GetConfigHandler` is PRIVATE.
- Routes corrected:
  - `update_config` removed from `/v2/foreign`;
  - `get_config` and `update_config` added to `/v2/owner`.
- `Net` CMake links `advapi32` for the Windows secret-file DACL checks.

## Live verification

Node started on the test HOME. `.api_secret` was generated (64 hex chars; not
logged).

| Request | Result |
|---|---|
| `POST /v2/owner` `get_config` without auth | `-32001 Unauthorized: valid API secret required` |
| same with `Authorization: Bearer <secret>` | result with config fields |
| `POST /v2/foreign` `update_config` with auth | `-32601 Method not supported` |
| `GET /v1/shutdown` without auth | HTTP 401 |
| `GET /v1/shutdown` with auth | node shuts down (connection closes) |

## Tests

Added `tests/src/Net/Test_ApiAccess.cpp`:

- constant-time token validation (case-insensitive, length, wrong token);
- loopback Host allow/deny (ports, IPv6, evil host, credentials);
- Origin allow/deny.

Focused run: **3 cases / 23 assertions PASS**.

Full suite: **187 cases / 2069 assertions PASS**.

## Artifacts

- `GrinNode.exe 102F8702EF5A3D116AE9AA2B5A9F4EC1EE7A140B9F9094C44BEFCE2AE3C79B3F`
- `Tests.exe FBEECC3408453A492C3A6FB32DDAABF3A61D13ADC3CCDC029641FFA3791BAB55`
