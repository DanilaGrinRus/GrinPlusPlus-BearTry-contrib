# Public Floonet legacy fallback — attempted end-to-end (2026-11-03)

Status: **BLOCKED / PARTIAL; long run left active.** No full public-to-public
legacy txhashset download completed at the time of this report; no PASS claim.

## What was run

- Binary: build-33 `GrinNode 6E2303BA…` (current production fixes; the earlier
  build-30 run is historical evidence).
- Isolated profile `C:\DHW\.tmp\task2-legacy-public`; public Floonet seeds only;
  P2P listen 13614, outbound peer port 13414, REST 127.0.0.1:25650; no loopback
  allowlist; no fixture; no wallet; `PIBD_ENABLED=false`, `PIHD_MODE=off`,
  archive serving disabled.
- **Pre-reboot run:** headers reached **3,321,636** (network ~3,321,443); a
  `txhashset_*.zip` reached **369,360,896 B** before the machine rebooted. The
  buffered Node.log still showed the early `download still not started` flood;
  the file growth proves a transfer had begun, but no activation was observed.
- **Post-reboot resume:** PID **3680** is left running on the same profile with
  logs `run3-stdout.log`, `run3-stderr.log`, `cpp-profile\LOGS\Node.log`; headers
  3,321,656, status `DOWNLOADING_TXHASHSET`, 4 public peers. The resumed temp zip
  was recreated at 0 B, then peer rotation occurred (80.228.193.72 →
  66.154.127.84; peer disconnected) — no completed transfer at report time.
- A separate earlier fresh run stopped at headers 27,648; that value belongs to
  the earlier bounded run, not to the pre-reboot 3.32M run.

## Why it did not reach the legacy path

The public header sync is far from the tip. The previous public legacy attempt
started the txhashset download only after headers; its legacy transfer was
observed at ~0.46 MB/s and abandoned at ~501 MB. At that rate, the ~3.1 GB
archive alone is several hours, before extraction/validation/body sync. The long
run is **not** being stopped for this report; no PASS is claimed until the whole
path completes.

## What remains unverified

- Receipt of a complete public `txhashset.zip` from a real external peer.
- Full decompression, roots/proof validation and atomic activation.
- Body catch-up to the live tip and `FULLY_SYNCED`.
- Restart/resume from the legacy-activated state with no full re-download.
- Peer switching/bans/errors over the complete transfer.

The PIBD client path did complete on build-16 (Layer A: FULLY_SYNCED 3,321,243,
restart resume/catch-up 3,321,250); that is **not** a legacy fallback PASS.

## Next practical step

Use a public-peer archive transfer with a multi-hour budget (or a controlled
public endpoint that exposes the legacy archive) and monitor full path plus
restart. Record bytes, rate, peer changes, bans, extraction/validation and final
tip/hash. Until then: **legacy public fallback NOT VERIFIED.**

## Evidence

- `C:\DHW\.tmp\task2-legacy-public\servers.json`
- `C:\DHW\.tmp\task2-legacy-public\stdout.log`, `stderr.log`
- `C:\DHW\.tmp\task2-legacy-public\cpp-profile\LOGS\Node.log`
- Final sample before stop: header 27,648, chain 0, `SYNCING_HEADERS`, 4 peers.
- Prior partial public legacy evidence: Layer A report, ~501 MB at ~0.46 MB/s.
