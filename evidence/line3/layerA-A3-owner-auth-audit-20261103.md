# Layer A A3 — owner API unauthenticated read: build-15 finding / build-16 static fix

Status: **BUILD-15 A3 CONFIRMED; BUILD-16 STATIC FIX PRESENT; BUILD16
RUNTIME NOT VERIFIED.** No source/build/live/profile access.

## 1. Confirmed build-15 evidence

Line1 public Floonet against build15 (`task13-public-floonet/evidence`):

| Request | Result |
|---|---|
| `/v2/owner/get_status` no secret | **HTTP 200** full status JSON |
| `/v2/owner/get_config` no secret | `Unauthorized: valid API secret required` |
| `/v2/owner/update_config` no secret | `Unauthorized: valid API secret required` |

Copied evidence: `evidence/line3/layerA-A3-20261103/`
`api-owner-get_status-noauth.txt` `4FB396AC…`, get_config `EDE91B8C…`,
update_config `A060AFC3…`.

Cause: `get_status`, `get_peers`, `get_connected_peers` inherited the
`RPCMethod` default PUBLIC; `get_config` is PRIVATE and `update_config` ADMIN.
`NodeRestServer.cpp:32` uses `Server::Create(EServerType::LOCAL, …)`;
`Server.cpp:11` maps LOCAL to `127.0.0.1`. **Not remotely exposed**, but local
unauthenticated owner reads were possible.

## 2. Build-16 static fix (present, uncommitted)

Diff adds exactly:

```cpp
ApiAccess::Level GetAccessLevel() const noexcept final { return ApiAccess::Level::PRIVATE; }
```

to:

- `src/API/Node/Handlers/GetStatusOwnerHandler.h:63`
- `src/API/Node/Handlers/GetPeersOwnerHandler.h:52`
- `src/API/Node/Handlers/GetConnectedPeersOwnerHandler.h:34`

Test `tests/src/API/Test_OwnerHandlerAccess.cpp`
(SHA `F258176D2C08D205DA42BC518834965CFCBAC778FC26CB320A531FC2D0DA518C`)
is added to `tests/src/API/CMakeLists.txt` and asserts all three owner
handlers are PRIVATE, get_config PRIVATE, update_config ADMIN, generic probe
PUBLIC.

`get_connected_peers` is covered too. Foreign public methods remain PUBLIC by
code inspection. Source is dirty (3 handlers, CMake, untracked test); no
build16 binary/log/freeze; latest Tests is still build15 `920F000F…` from
13:34; line1 public Floonet is still running build15 `GrinNode-A7D28BCC…`.

## 3. Test gap / runtime requirements

The new test asserts only direct `GetAccessLevel()` values. It does **not**
send HTTP no-secret/bad-secret requests or a valid-secret 200 request; the
central gate is covered elsewhere but A3's actual wire behavior needs a
runtime check.

Before build16 can be A3 PASS:

1. Build from frozen source (3 handlers + test + CMake); record Tests/GrinNode
   SHA, build log, freeze.
2. Run `Test_OwnerHandlerAccess` and full normal suite.
3. API smoke on isolated copy: no-secret and bad-secret
   `get_status`/`get_peers`/`get_connected_peers` → Unauthorized; valid secret
   → 200; `get_config` still Unauthorized, `update_config` still ADMIN; public
   foreign route still 200; no secret leakage; loopback bind unchanged.
4. Restart line1 public Floonet on build16 and re-capture A3 evidence; current
   build15 run remains **A3 FAIL** and must not be marked live-ready.
5. ASan/build16 provenance audit as usual.

## 4. Classification

- Build15 A3: **CONFIRMED FAIL** (local owner reads; loopback only).
- Build16 static fix: **PASS by inspection** (all three owner reads PRIVATE,
  test included in CMake).
- Build16 runtime: **NOT VERIFIED** (no binary/full test; line1 still build15).
- A3 closure: **NOT YET**.
- No source/build/live/profile/ports edits by line3.

## 5. Evidence

- `layerA-A3-20261103/api-owner-*-noauth.txt`
- `layerA-A3-owner-auth-audit-20261103.md` / `.json`
- `layerA-public-floonet-preauth-20261103.md` / `.json`
- Diff above is untracked worktree state at HEAD `921bcfd…`.

## 6. Update: integration test present (still build16 runtime pending)

Line2 also added `tests/src/Net/Test_RpcAuth.cpp`
(`6CDE5399…`, included in `tests/src/Net/CMakeLists.txt`). It starts a LOCAL
RPC server and verifies: private probe with no token/wrong 64-hex token →
`-32001`; valid token → `private_ok`; public probe without token →
`public_ok`. Together with `Test_OwnerHandlerAccess` (owner handlers PRIVATE,
config levels unchanged), the static A3 regression coverage is materially
complete.

Residual: the HTTP integration uses generic probes, not the concrete owner
route bodies; this is acceptable only because `Test_OwnerHandlerAccess` asserts
the actual owner handlers are PRIVATE and all use the central RPCServer gate.
Build16 runtime is still **NOT VERIFIED**: no build16 binary/log/freeze; latest
Tests is still build15 `920F000F…`. Run full build16, `Owner handlers require
PRIVATE access…`, `Owner RPC methods reject missing/wrong token…`, then API smoke
on line1 restart before A3 closure.

## 7. Build-16 artifacts/tests audited; live restart not observed

Build-16 Tests `1592F3DF…44CB6`, GrinNode `7E3AA29E…CA18`; build log
`95E621F0…` BUILD OK; source-bound objects/tests (OwnerHandlerAccess obj
14:56:46, RpcAuth obj 14:58:11; Tests 15:00:15). Independent line3 runs:
`Owner handlers require PRIVATE access…` PASS 1/6; `Owner RPC methods reject
missing/wrong token…` PASS 1/8; full **262/261/1**, 4281/4280, sole bodyless.

Live owner API is **NOT VERIFIED** because line1 remained on build15
`GrinNode-A7D28BCC…` (PID 13284) through the audit cutoff 15:08:48; no build16
process/restart was observed. A3 closure requires line1 restart on build16, then
no-auth/wrong/valid and public foreign probes.

## 8. Build-16 live A3 audit — PASS

Line1 restarted on build16 at **15:12:12** (`GrinNode-7E3AA29E…`, PID 33856).
Independent line3 probes on `127.0.0.1:25450`:

- owner `get_status`, `get_peers`, `get_connected_peers`: no-token and wrong
  64-hex token → **-32001 Unauthorized**; valid Bearer → **OK**.
- `get_config`: no-token → -32001; valid → **OK**.
- `update_config`: no-token/wrong → -32001; valid not sent (ADMIN; no mutation).
- foreign `get_tip`/`get_version`: **200 without token**, unchanged.
- Listener exactly `127.0.0.1:25450 LISTENING`; no wildcard bind.
- Config `PIBD_ENABLED=false`, `PIHD_MODE=off`; startup log PIBD archive
  serving disabled (default).
- Headers resumed from **2,503,680** to 2,542,976 and independently sampled
  2,613,120→2,704,768; network 3,321,049; **not genesis**, header sync ongoing.

A3 is **closed** for build16: no-auth/wrong rejected, valid accepted, config
levels unchanged, public foreign unchanged, loopback bind. Public full sync is
still in progress; this is not a full-sync/7B live claim.

Evidence: `layerA-A3-live-20261103.md` / `.json`;
`layerA-A3-live-20261103/`; `layerA-public-sync-20261103.md`.
