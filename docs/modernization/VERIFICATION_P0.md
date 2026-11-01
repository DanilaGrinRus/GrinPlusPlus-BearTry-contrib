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

Full suite: **187 cases / 2100 assertions PASS**.

## Artifacts

- `GrinNode.exe 3D05CE6F5E217DA9FFC3B10C83D5B9DF62AAF8F0280E24D309BA62EC3899F33A`
- `Tests.exe 0AAB7BCAD18D75FA12D4C5EAFE5BC2216D70008BD13BF4395D9051452E116B44`
