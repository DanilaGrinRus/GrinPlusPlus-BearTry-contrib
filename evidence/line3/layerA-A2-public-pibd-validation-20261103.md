# Layer A A2 public PIBD/validation evidence — separate status

Status: **NOT VERIFIED / IN PROGRESS.** This is separate from build18 normal
B1/B2 results; do not combine them into an A2 pass.

## Current public build16 run

- Process `GrinNode-7E3AA29E1160.exe`, PID 12312, restart 15:41:33.
- Latest status: header **3,321,092**, network **3,321,090**, chain height
  **0**, sync **DOWNLOADING_TXHASHSET**, outbound 1 — no full sync.
- `pibd-postactivation-status` / `pibd-progress-status` show
  `DOWNLOADING_TXHASHSET`, `processing_status` 16/45, no completed validation;
  `public-pibd-sync.jsonl` remains header/txhashset counts, no full-sync event.
- `PIBD_ENABLED=false` / `PIHD_MODE=off` in the active public config; no public
  PIBD-serving or PIHD claim. Evidence names containing `pibd-*` are not proof
  of public PIBD serving/validation.
- No post-full-sync restart persistence verified. Full/validation A2 is
  **pending**.

## Separation

- Build18 B1/B2/F14/owner results are unit/guard evidence only.
- Build16 public A2 requires full sync + validation + restart on persisted state;
  it is not implied by build18.
- Build18 later restart on persisted state is a separate event; record only
  when observed with real evidence.

## Evidence

- `C:\DHW\Gplus\work\runs\task13-public-floonet\evidence\ (pibd-*.json)`,
  `public-pibd-sync.jsonl` (read-only copies/hashes pending final)
- `layerA-public-sync-20261103.md`,
  `layerA-A3-live-20261103.md`,
  `layerA-A1-A3-A4-audit-20261103.md`
- line1 process/API status sampled 16:09-16:16.
