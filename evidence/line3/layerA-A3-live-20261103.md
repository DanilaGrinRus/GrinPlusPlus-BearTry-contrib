# Layer A A3 live audit — build-16 public restart

Status: **PASS (live owner auth + public foreign) / PUBLIC SYNC IN PROGRESS.**
Read-only live audit; no source/build/profile edits.

## Process / bind

- `GrinNode-7E3AA29E1160.exe`, PID **33856**, path
  `C:\DHW\.tmp\line1-build-16\GrinNode-7E3AA29E1160.exe`, start
  **15:12:12**; SHA-256 `7E3AA29E…CA18`.
- Listener: **`127.0.0.1:25450` LISTENING**; no `0.0.0.0`/wildcard listener.
  API remains loopback-only.
- Startup log 15:12:12; `PIBD archive serving: DISABLED (default)`; config
  `PIBD_ENABLED=false`, `PIHD_MODE=off`.

## Independent probes (token never logged)

| Probe | none | wrong 64-hex | valid |
|---|---|---|---|
| `get_status` | -32001 | -32001 | OK |
| `get_peers` | -32001 | -32001 | OK |
| `get_connected_peers` | -32001 | -32001 | OK |
| `get_config` | -32001 | — | OK |
| `update_config` | -32001 | -32001 | not called (ADMIN; no mutation) |

- `get_config` valid returned normal config keys; no secret in response.
- `get_tip` / `get_version` on `/v2/foreign` **200 without token**; public
  foreign behavior unchanged.
- Line1 copied evidence (`b16-owner-*`) matches: no/wrong → -32001, valid
  bearer success; foreign public 200.

## Restart / resume

- `build16-pre-status.json`: header **2,503,680** (pre-restart run).
- `build16-sync-status.json`: header **2,542,976** shortly after restart.
- Independent samples: **2,613,120 → 2,639,232 → 2,664,832 → 2,691,456**, then
  **2,704,768** at 15:15:24; network height **3,321,049**; sync
  `SYNCING_HEADERS`, outbound 4.
- Headers resumed above 2.5M, **not genesis**; block chain height still 0
  because header sync is ongoing. No full-sync claim.

## Classification

- A3 owner auth (build15): confirmed fail.
- A3 build16 live: **PASS** — no/wrong token unauthorized, valid token success,
  config levels, public foreign unchanged, loopback bind.
- Public Floonet sync: **IN PROGRESS**; full sync/restart persistence not yet.
- No source/build edits; line1 profile/ports not modified.

## Evidence

- `evidence/line3/layerA-A3-live-20261103/independent-probes.txt`
  `C1230924…`
- copied line1 `b16-owner-*`, `b16-foreign-*`, `build16-pre-status.json`,
  `build16-sync-status.json`
- `evidence/line3/layerA-public-sync-20261103/independent-sync-samples.jsonl`
  `851E79C4…`
- `normal-binary-build16-audit-20261103.md`,
  `layerA-A3-owner-auth-audit-20261103.md`,
  `build-16-freeze-snapshot-final.json`
