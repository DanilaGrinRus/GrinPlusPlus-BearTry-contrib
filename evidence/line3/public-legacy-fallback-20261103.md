# Public Floonet legacy fallback — attempted end-to-end (2026-11-03)

Status: **BLOCKED / PARTIAL.** No full public-to-public legacy txhashset
download was completed; no PASS claim.

## What was run

- Binary: build-30 `GrinNode 0EF111EF…` (source includes the IsCoinbase fix).
- Fresh isolated profile `C:\DHW\.tmp\task2-legacy-public`.
- Public Floonet seeds only; P2P listen 13614, outbound peer port 13414, REST
  127.0.0.1:25650; no loopback allowlist; no fixture; no wallet.
- Config explicitly `PIBD_ENABLED=false`, `PIHD_MODE=off`, archive serving
  disabled.
- Run stopped deliberately after ~1 minute at header height **27,648** (network
  tip 3,321,443); 4 public peers, status `SYNCING_HEADERS`.

## Why it did not reach the legacy path

The public header sync is far from the tip. The previous public legacy attempt
started the txhashset download only after headers; its legacy transfer was
observed at ~0.46 MB/s and abandoned at ~501 MB. At that rate, the ~3.1 GB
archive alone is several hours, before extraction/validation/body sync. This run
was stopped to respect the bounded test budget; it is **not** evidence that
legacy works end-to-end.

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
