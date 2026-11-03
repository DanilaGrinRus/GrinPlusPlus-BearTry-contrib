# Build-13 full 7B live — independent audit

Status: **PASS for the full build-13 7B live scenario** (test/floonet; no product
functional failure). This is build-13 final live binding. No build/commit/source
edit by line3.

## Identity

- Tests `001DF4BC31F194DCDDE83FE736B06EB6FD942666F2BEFF62FF1FB5F1BB8DB3A8`
  (9,465,856 B, PE `0x6AE957C5`)
- GrinNode `F39BDFE5A8DAB22BE969406874EDCEFD023770BE1A1BA6330D7506B2C2571201`
  (7,846,912 B, PE `0x6AE956CB`)
- Build log `8C54DD17…` BUILD OK; freeze `4FC8DCE1…`
- Evidence copied to `evidence/line3/build13-7b-full-20261103/`
  (35 files, 21,645,390 B).

## Verified counts / final state

| Item | Value | Evidence |
|---|---|---|
| C++/Rust final tip | 3318655 `2945348ef765160a160008cfd7c2b41aa44ed3cea77d6bcfe2083acf0fc4bbf1` | `7b-full-final-status.json`, C++ status |
| C++ sync status | `FULLY_SYNCED`; network height 3318655; 1 inbound/1 outbound | `7b-full-cpp-status.json` |
| Rust sync status | `no_sync`; connections 1; total difficulty 1207035333622 | final status |
| PIBD leaves | **10,203,442 / 10,203,442**, completed height **3315600** | `7b-full-rust.log:8216` `TxHashsetPibd` |
| Range proofs | **2,943,034** (`rproofs_total`) | `7b-full-rust.log:8270` |
| Kernels | **3,351,542** (`kernels_total`, 3,351,542 signatures verified; pmmr size 6,703,071) | `7b-full-rust.log:15499-15501` |
| C++/Rust roots | kernel `17d1cc11…`, output `8a9be5f8…`, rangeproof `971c4ebd…`; pre == post | roots JSON |
| PIBD archive serving | H=3315600 `TxHashSet.3315600.3615bb625917.zip`, 3,118,756,420 B SHA `DA23A1C7…`; ready at 11:04:41 | evidence log |
| Duration | C++ start 11:03:22 → final status 13:02:21 = **1h58m59s**; status samples 11:04:59→13:02:08 = 1h57m09s | timestamps |

## Errors / discrepancies

1. **Non-blocking environment error** (`7b-full-cpp.log:7`): private/admin API
   refused because secret file owner/DACL is not owner-only. Test harness setup;
   no chain/sync impact.
2. **Transient peer connect** (`7b-full-cpp.log:15`): `Failed to connect to
   127.0.0.1: No response` at startup; later 1 connection and full sync. Not
   product failure.
3. **Stale status text**: `7b-full-cpp-status.json` says `waiting_reason:
   waiting for peers: 0 active connection(s), 1 required` while
   `sync_status=FULLY_SYNCED`, network height=3318655, inbound/outbound=1.
   Cosmetic/stale field; no effect on binding.
4. Rust **0** `ERROR`/panic. `WARN` lines are dominated by loopback
   `no suitable outbound peer`/header-sync messages; expected for this testnet
   topology, counted separately.
5. `10,203,442` is **PIBD completed leaves at H=3315600**, not total outputs or
   archive bytes. `2,943,034` is range proofs / UTXO count context; `3,351,542`
   kernels. No count mismatch with Lead report.
6. Test is floonet/test (`C++ --floonet`, Rust chain `test`), not Mainnet.
   Existing full G-09/Mainnet readiness is not claimed.

## Crash/publication/resource checklist

Prepared in `build13-7b-live-audit-plan-20261103.md`:
- PUB-01..PUB-10: kill after staged zip, during Extract/LoadAndValidate, after
  validation before PublishStaged, during rename; truncated/corrupt/wrong-root/
  height zip; missing entry/leaf; stale staging; idempotent valid final;
  bounded same-target restart.
- RES-01..RES-08: ENOSPC staged copy/extract and rename/fsync; ~3.1 GB zip64
  boundary; CPU/memory during validation; concurrent serving+recovery; file
  lock/read-only archive dir; crash/restart cleanup; repeated recovery/restart.
- All crash/resource cases must use fresh copies; original untouched; no source
  edits/build. `task12-crash-evidence` currently contains only an initial
  crash/retry run scaffold; it is not yet an audited PASS.

## Evidence hashes

- `7b-full-cpp.log` `B10388E52BBB1EF4665EE22ED396932896AB9D76211A51A047078231051C74FF`
- `7b-full-rust.log` `6098543511778855578B8C4057995559EE644F982F8A969D879A76942917256B`
- `7b-full-final-status.json` `84525B85BC8E06D5CBE313D3B281A6838DA6F6E18A6688B8A3650500139E14E9`
- `7b-full-cpp-status.json` `247AED786CB45FAAC43319F7B571F38A149E580352838B15E6EF02123E9B0589`
- `7b-full-cpp-roots.json` `027F9D4525549937AD15436CE172FC526D110E811C6CAB819F8CFF75D29ADA4D`
- `7b-full-status.jsonl` `3E417A2AA0566BE29B7A6382585934EF2D85FBB4FD4BFFB342B4230009CE681E`
- `7b-full-status-cont.jsonl` `6D6B9B5C8C1EC7E1614C7536D30C063C62FAF4CDCB35C6AA38683C669AC53114`
- Archive H3315600 SHA `DA23A1C721BE96BA215A70022B3C26DC262E3122E5BE41CDBDA560631F0E78F5`
- Build-13 binaries/freeze as above.

No source edits/build/commit; original `g09-live-02` untouched. This is a
test/floonet 7B binding, not a full G-09/Mainnet PASS.
