# G-09 ordinary-node live PASS (scenario-scoped)

Date: 2026-11-03. Status: **PASS for the exact scenario below**, not full G-09
acceptance. Binary: build-09 `GrinNode F90EEF38…`, `Tests 0A3B7D66…`.

## Scenario

- C++ Grin++ Floonet node on a checkpoint-trimmed fixture:
  candidate==body==3318655, archive horizon **H=3315600**,
  archive SHA-256 `13F9781E207F787F5F58367F9C98947D4A645D660E1CA23186AD55D9D064C530`.
- Archive serving opt-in (`GRINPP_PIBD_ARCHIVE_SERVING=1`), default-off preserved.
- Rust Grin 5.5.2 (`G09RustLive.exe` `E5219C91…`) with **clean headers/state**,
  isolated loopback profile; only C++ peer allowed.
- C++ data is fixture-based; no public Floonet sync/peers.

## Result

- Rust headers from zero selected **H=3315600** at 07:21:00.
- PIBD: **10,203,442 / 10,203,442 leaves** complete 07:58:41.
- Validation: **2,943,034 rangeproofs** (08:01:01–08:21:58), kernels, txhashset_save
  (08:24:58), activation/transient no_sync at H=3315600 (08:25:38).
- Body sync **3315600 → 3318655** (08:26:18–08:35:19).
- Final Rust: **`no_sync`**, tip **3318655**, hash
  `2945348ef765160a160008cfd7c2b41aa44ed3cea77d6bcfe2083acf0fc4bbf1`, conn=1.
- Final C++: `FULLY_SYNCED`, chain/header 3318655, same hash,
  TD `1207035333622`, inbound/outbound 1/0.
- Roots: kernel `17d1cc11…`, output `8a9be5f8…`, rangeproof `971c4ebd…`.

## Restart PASS

C++ restarted first, then Rust on the persisted profile:
`awaiting_peers` → `no_sync` at 08:44:56, tip 3318655, same hash; C++ inbound=1,
`FULLY_SYNCED`. New PIBD segment requests: **0**; Rust PIBD markers: **0**;
no crash/wrong H.

## Build-10 final-binary persisted restart

Build-10 `GrinNode 60D7F7A6…` / `Tests C17C3BC0…` restarted on the same persisted
`g09-live-02` profile at 08:59: Rust `no_sync` at tip 3318655 / hash
`2945348e…bbf1`, C++ `FULLY_SYNCED` same tip/hash inbound=1; **0** new PIBD
segment requests, **0** Rust PIBD markers; no crash/wrong H. Kernel index is now
published (generation 1, 3,354,613 kernels), unlike build-09 direct-scan fallback.
This is a final-binary restart PASS, not a second full PIBD. Evidence:
`C:\DHW\.tmp\task7-B\evidence\build10\` (`build10-restart-final.txt` `405DB2EB…`).
Open warning remains: `ArchivePibdState` spent-output rewind warning for
`Commitment{0989ac92…}` after activation.

## Wrong-H refusal (7C)

Rust selected H=3317040 against archive H=3315600; C++ logged 7 exact-hash
refusals, served no segment/wrong data, no crash; fallback NOT VERIFIED (bounded run).

## Evidence

- `C:\DHW\.tmp\task7-B\evidence\7B-pass.txt`, `7B-freeze.txt`, `7B-liveness.txt`,
  `7B-restart-final.txt`, logs/status JSONL/config copies.
- Repo: `evidence/line3/g09-live-02-7b-success-20261103.md` + MANIFEST;
  `evidence/line3/g09-live-02-peer-drop-20261103.md`.
- Logs: C++ `77518AEA…C99D` (3,075,445 B); Rust `B904A7A7…B518` (2,766,191 B);
  status JSONL `5E7BD611…2363`.
- Configs: C++ `355B15F7…7C7C`; Rust `AEE5E753…8B2B`.

## Findings / limits

- C++ fixed **60 s receive-idle** disconnected Rust during CPU-bound rangeproof
  validation; Rust continued offline, C++ reconnected ~08:26:18 and body sync
  completed. Needs heartbeat or bounded validation grace; test-env artifact,
  not patched.
- `ArchivePibdState` warned once after activation:
  `Failed to archive the activated TxHashSet: Rewind: Spent output not found for
  Commitment{0989ac92…}`; restart no_sync unaffected; archive regeneration needs review.
- Kernel index fail-closed direct scan remains (duplicate-excess API/index fix in build-10).
- Output Commitment not found: **0 live occurrences**; static mechanism only,
  controlled repro BLOCKED.
- Reorg/horizon BLOCKED (no +720 block source); negative wire/resource scenarios
  NOT VERIFIED.
- This PASS does not certify public-network PIBD, Mainnet, or final build-10
  binary; build-10 API/index changes are outside the P2P path and require their
  own restart evidence.
## Build-13 clean full 7B from zero — PASS

Final build-13 (Tests `001DF4BC…B3A8`, GrinNode `F39BDFE5…1201`) was run with a
fully clean Rust `chain_data` (tip 0/genesis), C++ archive H=3315600
(`3,118,756,420 B`, SHA `DA23A1C7…78F5`), loopback.

- headers from zero: measured 1,176.94 headers/s;
- selected required_height **3315600**;
- PIBD **10,203,442 / 10,203,442** leaves, completed_to_height 3315600;
- rangeproofs **2,943,034**, kernels **3,351,542**, setup/save;
- body sync **3315600 → 3318655**;
- final Rust **`no_sync`**, tip **3318655**, hash
  `2945348ef765160a160008cfd7c2b41aa44ed3cea77d6bcfe2083acf0fc4bbf1`, conn=1;
- C++ `FULLY_SYNCED`, inbound=1/outbound=1, archive ready H=3315600;
- total ~1 h 58 min; 5,062 PIBD segment requests.
- Evidence: `C:\DHW\Gplus\work\runs\task12-build13-7b\evidence` (rust/cpp logs, status JSONL,
  roots/status hashes); nodes stopped.
- Caveats unchanged: fixture-based C++/loopback, no public peers; crash/resource,
  reorg/horizon, negative tests still separate.
