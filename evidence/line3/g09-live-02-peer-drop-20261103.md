# G-09 live-02 peer drop after PIBD transfer — 2026-11-03

Status: OBSERVED / TEST-ENVIRONMENT. PIBD segments were transferred and Rust
continued local rangeproof validation; the P2P connection was then lost.
Body sync is NOT VERIFIED, but this is not treated as a production blocker.

Disposition: after review, this is considered a test-environment artifact
(long CPU-bound validation phase on a controlled fixture). Normal peers send
P2P traffic, so the fixed 60s idle timeout is unlikely to matter in production.
No C++/Rust code change is planned for this observation. Expected behaviour is
that Rust reconnects after validation completes.

## Run

- root: `C:\DHW\.tmp\g09-live-02`
- C++: PID 4724, `GrinNode-F90EEF389D1B.exe --floonet --no-wallet --headless
  --config-path ...\servers\server_config.json`; REST 25430, P2P listen 25536,
  preferred/allowed peer 127.0.0.1.
- Rust: PID 31792, `G09RustLive.exe --testnet server -c ...\rust\grin.toml run`;
  REST 25431, P2P listen 25537, `peers_allow = ["127.0.0.1:25536"]`,
  `seeds = []`, `peer_max_outbound_count = 1`.
- No secret values are included here.

## Observed state (as of 2026-11-03T08:09+07:00)

- Both processes alive; C++ P2P 25536 and Rust P2P 25537 are only listeners.
- No established TCP pair 25536 <-> 60156 remains.
- C++ `get_status`: `connections=0`, `sync_status=awaiting_peers`, tip 3318655.
- Rust `get_status`: `connections=1` (stale), `txhashset_rangeproofs_validation`,
  tip 0, sync_info `rproofs=1,137,000 / rproofs_total=2,943,034` (08:09:04).
- C++ `get_connected_peers`: `[]`; C++ `get_peers` still lists 127.0.0.1 as
  `Healthy` (last connected), i.e. a known peer record, not a live socket.
- Rust `get_connected_peers` still returns inbound 127.0.0.1:25536, which is
  inconsistent with the closed socket; treat the Rust count as stale.

## Timeline (Asia/Bangkok, 2026-11-03)

- 07:55:02 — Rust still requesting/applying PIBD segments from 127.0.0.1:25536.
- 08:00:07 — C++ `[SEED] IsConnectionActive`: "127.0.0.1 has not received a
  message in more than a minute"; disconnects socket 127.0.0.1:60156.
- 08:00:21 — C++ reconnects; Rust accepts handshake from 127.0.0.1:61055 and
  adds peer 127.0.0.1:25536; C++ sends capabilities.
- 08:00:52 — Rust: `validated total kernel sums, took 107s`; enters
  `TxHashsetRangeProofsValidation`.
- 08:01:21 — C++ idle-check again: no message for >1 minute; disconnects
  socket 127.0.0.1:25537. `connected=0` from then on.
- 08:05:46 — C++ retry: `PerformOutboundHandshake: Shake message not received`;
  Rust log has no matching `accept: handshaking` line for this attempt.
- 08:09:04 — Rust validation still progressing (~38.6%); no P2P reconnect.

## Cause

C++ applies a hard-coded 60-second receive-idle timeout in
`src/P2P/Connection.cpp:40`:

```cpp
if ((m_lastReceived + std::chrono::seconds(60)) < system_clock::now()) {
    LOG_DEBUG_F("{} has not received a message in more than a minute", GetPeer());
    return false;
}
```

After the PIBD segment transfer, Rust entered CPU-bound rangeproof validation and
sent no further P2P traffic; C++ therefore reaped the connection. Reconnect
attempts did not complete the handshake: C++ reports "Shake message not
received"; Rust shows no accept/handshake processing for the 08:05:46 attempt.
This is not an allow-list/capability rejection: the same pair had already
connected and served PIBD, and Rust's allow-list explicitly accepted
`127.0.0.1:25536` at 08:00:21.

The exact reason Rust did not complete the later Shake is not proven; likely
candidates are P2P handshake starvation during rangeproof validation or stale
single-peer state (Rust still reports the dead inbound peer as connected).
Recorded as a hypothesis, not a confirmed Rust defect.

## What is verified vs not

- PIBD segments were served by C++ and consumed/applied by Rust before the drop.
- Rust continued local rangeproof validation after the drop.
- P2P liveness across a >60s silent validation phase: FAIL (connection reaped).
- Body sync / post-validation reconnect: NOT VERIFIED.
- No C++/Rust code changes were made for this observation.

## Optional follow-up (not required)

1. Do not restart Rust during validation; it would discard progress.
2. Watch whether Rust reconnects after validation completes and whether C++
   completes the handshake. C++ seeder retries are 300s apart.
3. Only if the connection does **not** recover after validation should this be
   revisited as a test-environment liveness issue. A production fix is not
   proposed or scheduled.
4. Re-run the G-09 live test only if a reconnect/body-sync check is needed; do
   not bind the current run as PASS.

## Reproduction commands (read-only)

```powershell
netstat -ano | findstr "25536 25537"
Select-String C:\DHW\.tmp\g09-live-02\cpp-profile\LOGS\Node.log `
  -Pattern 'has not received a message|Shake message not received'
Select-String C:\DHW\.tmp\g09-live-02\rust\grin.log `
  -Pattern 'accept: handshaking|Adding newly connected peer|TxHashsetRangeProofsValidation'
```

## Update — 2026-11-03T08:31+07:00

The pair reconnected by itself; the earlier drop was transient, as expected for
this test environment. Current live state:

- TCP established: C++ `127.0.0.1:25536` <-> Rust `127.0.0.1:55667`.
- C++: `connections=1`, `sync_status=no_sync`, tip 3318655.
- Rust: `connections=1`, `sync_status=body_sync`, tip 3318593,
  `sync_info.current_height=3318589`, `highest_height=3318655` (~96.5%).

G-09 body sync is still in progress; this is not yet a final PASS. Re-check
until Rust reaches `no_sync` / tip 3318655, then bind the result with evidence.

## Final outcome — PASS (scoped)

Final evidence at 2026-11-03T08:35:19+07:00 / 01:35:19Z:

- Rust `no_sync`, tip 3318655,
  hash `2945348ef765160a160008cfd7c2b41aa44ed3cea77d6bcfe2083acf0fc4bbf1`,
  conn=1;
- C++ `FULLY_SYNCED`, chain/header 3318655, same hash, conn=1, in/out=1/0.

The pair reconnected and the 7B correct-H full PIBD → validation → activation →
body sync → no_sync scenario completed. This liveness drop is therefore a
transient test-environment finding, not a production blocker; no liveness fix
is proposed. See
`evidence/line3/g09-live-02-7b-success-20261103.md` and its `MANIFEST.json`.

The nodes were stopped after evidence capture; they were not restarted. G-09 is
still only scoped-PASS for 7B; restart/reorg/horizon/negative/resources and
Output Commitment, plus build-10 normal/ASan, remain pending.
