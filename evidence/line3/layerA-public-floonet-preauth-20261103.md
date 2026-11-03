# Layer A public Floonet — candidate provenance pre-audit (2026-11-03)

Status: **PRE-AUDIT / PUBLIC FLOONET STARTING (line1/task-13) / NO
BUILD/SOURCE EDIT / LINE1 PROFILE & PORTS UNTOUCHED.**

Build-15 is the source-bound fail-closed/D2 candidate. It is **not** a
replacement live claim for build-13's existing full 7B binding; this Layer A
public Floonet run is a separate candidate/evidence scope.

## 1. Provenance

| Item | Value |
|---|---|
| Current HEAD | `921bcfd6f9965307b14973451d3175c82beef712` (`docs: final build-15 integration HEAD`) |
| Build source HEAD | `75a09d63…` (build-15 source, before docs-only commit) |
| Build15 Tests | `920F000F38958DA545ED3BC57DB796A0062CB5EF6E304E7DB354DAD065E6227D` (9,495,552 B) |
| Build15 GrinNode | `A7D28BCC2F4A1B8FC5D57083E0F5F43F75A81BDF1771C946BE041A4CADA7246C` (7,850,496 B) |
| Build log | `build-console-f13fix-20261103-132502.log` `5C70A1FB…` BUILD OK |
| Freeze | `build-15-freeze-snapshot-final.json` `E59B65FF…`, 1005 entries, aggregate `D6A66439…` |
| Freeze vs worktree | **1005/1005 match, 0 missing** |
| Dirty docs | **21 modified docs** + 1 untracked evidence checklist; **no dirty source/test files** |
| Source binding | Test source 13:24:59 → object 13:33:18 → Tests.exe 13:34:19; valid |
| Compiler flags | Project C++ `/W4 /WX`; main `GrinNode`/`Tests` Level4 +
  TreatWarningAsError; **`/EHsc` (Sync exceptions)**, not `/EHa`; C deps `/W3` |
| Full exceptions | Build15 full: only known test-only bodyless; ASan F13/archive
  link-boundary; build14 stale artifact historical |

Build-15 normal pass remains **260/259/1 bodyless-only**; F11/F13/D2/archive/
preflight PASS; ASan partial PASS with F13/archive link boundary. Build-13 full
7B live binding remains final and separate.

## 2. Defaults audit (repo code; no line1 profile access)

### API

- Access levels: **PUBLIC** read-only methods need no secret; **PRIVATE/ADMIN**
  require `.api_secret` and ADMIN also loopback Host/Origin. Invalid/missing
  secret is fail-closed for private/admin.
- REST port default: **3413 Mainnet, 13413 non-Mainnet**; overridden by
  `server.REST_API_PORT`. No code-level "API disabled" default found; public
  routes are the unauthenticated surface. A public Floonet candidate must be
  checked for accidental private/admin exposure and API-secret file DACL.
- The earlier test harness had a DACL refusal for private/admin API; that was a
  test-profile setup issue, not a public API gate.

### P2P / PIBD / PIHD

- PIBD enabled default **true** (`P2PConfig::m_pibdEnabled = true`).
- PIBD archive serving default **disabled**; opt-in only via
  `GRINPP_PIBD_ARCHIVE_SERVING=1`, and explicitly refused on Mainnet.
- PIHD default **OFF**; config `client`/`full` maps to CLIENT only (no server
  implementation; must not advertise PIHD_HIST).
- Public Floonet evidence must show actual line1 config/log values, not assume
  repo defaults.

## 3. Public sync/restart audit checklist (after line1 reports)

No direct profile/port access; use line1 evidence copies/status/logs only.

1. Candidate identity: binary SHA/PE/freeze; actual config/env hash; public
   chain/network identifier; no Mainnet data.
2. Public p2p: outbound discovery/connection, inbound capability, peer
   diversity; no loopback-only pass claimed as public.
3. Sync: headers/body from actual peers to network tip; tip
   height/hash/difficulty; PIBD enabled/archive serving state; PIHD state must
   remain OFF; no hidden local-only peer.
4. API: public endpoints reachable without secret; private/admin rejected
   without secret and with bad DACL; loopback enforcement; no secret leakage.
5. Archive readiness: only validated archives advertised; no partial/staging
   final; recovery markers do not suppress regeneration; PIBD archive serving
   remains default-off unless explicitly observed.
6. Restart: same public profile, persistent tip/hash; no full re-download from
   genesis; no stale recovery/partial marker loop; API/PIBD state resumes;
   resource/disk usage bounded.
7. Process clean shutdown/restart; no orphan port/process; no source/build
   changes; evidence hashes and line1 report reconciled.
8. Classify PASS/FAIL/NOT VERIFIED; public Floonet evidence does not replace
   build-13's 7B binding or claim Mainnet.

## 4. Current blockers

- Public Floonet just starting; no sync/restart evidence yet.
- Line1 profile/ports deliberately untouched; evidence must be supplied by
  line1.
- Build15 ASan F13/archive link boundary remains; normal F13/archive pass only.
- Crash/publication/resource matrix prepared, not run.

## 5. Evidence

- Build15 freeze/audit: `build-15-freeze-snapshot-final.json`,
  `normal-binary-build15-audit-20261103.md`,
  `build15-asan-refresh-20261103.md`.
- Build13 live binding: `build13-7b-full-audit-20261103.md`.
- This preaudit: `layerA-public-floonet-preauth-20261103.md` / `.json`.

## 5. A3 owner API security gap (build15 confirmed; build16 static fix pending runtime)

Line1 evidence: `GET /v2/owner/get_status` returned **HTTP 200 without a
secret** (chain/tip/connections), while `get_config` and `update_config`
returned `Unauthorized: valid API secret required`. Cause: three owner read
handlers inherited the default PUBLIC access level. The API is bound to
`127.0.0.1` (`EServerType::LOCAL`), so this is a local same-host read exposure,
not remote.

**Build16 static fix now present** as an uncommitted worktree change:
`GetAccessLevel()` = PRIVATE added to `GetStatusOwnerHandler`,
`GetPeersOwnerHandler`, `GetConnectedPeersOwnerHandler`; new
`tests/src/API/Test_OwnerHandlerAccess.cpp` included in `tests/src/API/CMakeLists.txt`.
`get_config` remains PRIVATE, `update_config` ADMIN, foreign public handlers
PUBLIC.

**Build16 runtime NOT VERIFIED**: no build16 binary/log/freeze; latest Tests is
still build15 `920F000F…`; line1 public Floonet is still running build15. The
new test checks direct handler levels, not HTTP no-secret/bad-secret/valid-secret
behavior. Before A3 closure: build16 freeze/SHA, run the new test + full suite,
API smoke all three owner reads no/bad secret → Unauthorized and valid secret →
200, config levels/public foreign unchanged, no secret leak, then restart line1
and recapture evidence. Build15 Layer A remains **A3 FAIL** until that happens.

Evidence: `layerA-A3-owner-auth-audit-20261103.md` / `.json`;
`layerA-A3-20261103/api-owner-*-noauth.txt`.
