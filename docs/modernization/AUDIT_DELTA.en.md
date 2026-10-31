# AUDIT_DELTA (EN) — Grin++ 1.2.9 → Grin v5.5.2 compatibility

Status: **DRAFT skeleton**, created 2026-10-31. Not a final audit artifact yet.
Baseline: `5f511e6` (upstream Grin++ 1.2.9 snapshot). Range: `5f511e6..HEAD`.

Scope: every commit that touched code. Documentation-only commits are not cards;
they are referenced from the evidence map. `Before`/`After`/`Risk` are deliberately
left as `TODO` — they must be filled from the real diff before handover.

## How to fill a card

1. Replace every `TODO` with real content (no paraphrases: paste diff fragments
   with file and line numbers).
2. `Rust reference` — exact `file:line` in grin v5.5.2.
3. `Evidence` — test name, `E`-number, or live artifact; `NOT VERIFIED` if none.
4. `Risk, rollback` — what can break and how to revert.

Rules: English is canonical; German (`AUDIT_DELTA.de.md`) and Russian
(`AUDIT_DELTA.ru.md`) are translations and must keep the same card IDs.
Code comments are English-only.

## Cards

### G-24 — Reproducible build (offline sysroot)
- Status: DONE
- Commits: 89eb22c, d034a2f
- Files: `tools/deps/build_sysroot.py`, `tools/build/build.ps1`, `CMakeLists.txt`, `src/CMakeLists.txt`
- Rust reference: n/a (build infrastructure)
- Before: TODO
- After: TODO
- Evidence: `BUILD.md`, E6–E9 (clean build, `/W4 /WX`)
- Risk, rollback: TODO

### G-19 — Bounds-safe deserialization
- Status: DONE
- Commits: fc9e988
- Files: `include/Core/Serialization/ByteBuffer.h`, `tests/src/Core/Serialization/Test_ByteBuffer.cpp`
- Rust reference: `core/src/ser.rs` (reader bounds checks)
- Before: TODO
- After: TODO
- Evidence: E2 (reproduced before/after), `Test_ByteBuffer`
- Risk, rollback: TODO

### G-08 / G-10 / G-11 — Message table, capabilities, unknown types
- Status: DONE
- Commits: a5c88bf
- Files: `include/P2P/Capabilities.h`, `src/P2P/Messages/MessageTypes.h`, `src/P2P/Messages/MessageHeader.h`, `tests/src/Net/Test_P2PCompat.cpp`
- Rust reference: `p2p/src/msg.rs:67-97`, `p2p/src/types.rs:381-399`
- Before: TODO
- After: TODO
- Evidence: E3, `Test_P2PCompat`
- Risk, rollback: TODO

### G-12 / G-14 / G-15 — Handshake genesis, archive send/receive
- Status: DONE
- Commits: e5287f2
- Files: `src/P2P/Seed/HandShake.cpp`, `src/P2P/MessageProcessor.cpp`, `src/P2P/Pipeline/TxHashSetPipe.cpp`
- Rust reference: `p2p/src/handshake.rs`, `p2p/src/msg.rs:735-789`, `p2p/src/protocol.rs:254-265`
- Before: TODO
- After: TODO
- Evidence: E4/E5 (code-level), `NOT VERIFIED` live
- Risk, rollback: TODO

### G-25 — Version 1.2.9
- Status: DONE
- Commits: 348d87e
- Files: `include/GrinVersion.h`, `include/P2P/Common.h`
- Rust reference: n/a
- Before: TODO
- After: TODO
- Evidence: `GrinVersion::MatchesRelease` test, `/v1/status` shows `Grin++ 1.2.9`
- Risk, rollback: TODO

### G-29 — Accept loop survives rejected peers
- Status: DONE
- Commits: bfc31a0
- Files: `src/P2P/Seed/Seeder.cpp`
- Rust reference: `p2p/src/serv.rs` accept loop
- Before: TODO
- After: TODO
- Evidence: E29 (`0.0.0.0:13414 LISTENING`), live handshake
- Risk, rollback: TODO

### G-30 — Advertised capabilities in handshake + configurable P2P port
- Status: DONE
- Commits: 72c94f2
- Files: `src/P2P/Seed/HandShake.cpp`, `include/Core/Config.h`, `src/Core/Config.cpp`, `src/Core/Config/ConfigProps.h`, `src/Core/Config/P2PConfig.h`
- Rust reference: `p2p/src/msg.rs` Hand/Shake fields
- Before: TODO
- After: TODO
- Evidence: E30 (Rust peer sees `PIBD_HIST | PIBD_HIST_1`)
- Risk, rollback: TODO

### G-09a — PIBD wire layer, segment verification, generator
- Status: DONE
- Commits: 62a146c, 8e2f493, de3f1a5, c390ee9
- Files: `src/PMMR/Models/Segment*.h`, `src/PMMR/Models/Bitmap*.h`, `src/P2P/Messages/Segment*Message.h`, `tests/src/PMMR/Test_Segment.cpp`, `Test_SegmentGenerator.cpp`
- Rust reference: `core/src/core/pmmr/segment.rs`, `chain/src/txhashset/*`
- Before: TODO
- After: TODO
- Evidence: unit suites, E26 (root model)
- Risk, rollback: TODO

### G-09b — Bitmap assembly, download orchestration, session/scheduler, snapshot
- Status: DONE
- Commits: 621efad, ec006a9, 21df0b9, df81cc3, e167e8a
- Files: `src/P2P/PIBD/BitmapSegmentAssembler.h`, `PibdDownload.h`, `PibdSession.h`, `PibdScheduler.h`, `PibdSnapshot.h`, `PibdTreeAssembler.h`, `PibdTreeRebuilder.h`
- Rust reference: `chain/src/txhashset/desegmenter.rs`, `segmenter.rs`, `p2p/src/protocol.rs:294+`
- Before: TODO
- After: TODO
- Evidence: unit tests for each module (E24/E25/E26)
- Risk, rollback: TODO

### G-09c — Staging PMMR write and atomic activation
- Status: DONE
- Commits: b681c2e, fcea044
- Files: `src/P2P/PIBD/PibdStateWriter.h`, `src/P2P/PIBD/PibdActivation.h`
- Rust reference: `chain/src/txhashset/txhashset.rs` (snapshot), activation semantics
- Before: TODO
- After: TODO
- Evidence: `Test_PibdStateWriter`, `Test_PibdActivation` (crash in both directions)
- Risk, rollback: TODO

### G-09d — PIBD serving from node PMMR (incl. pruned)
- Status: DONE
- Commits: 49942ab, 30012d6, 4f9a569, 177c97a
- Files: `src/P2P/PIBD/PibdSegmentService.h`, `src/P2P/MessageProcessor.cpp`, `include/BlockChain/BlockChain.h`, `src/BlockChain/BlockChainImpl.cpp`
- Rust reference: `chain/src/txhashset/segmenter.rs`, `p2p/src/protocol.rs` (21/23/25/27)
- Before: TODO
- After: TODO
- Evidence: `Test_PibdServing` (pruned bitmap), live serving `NOT VERIFIED`
- Risk, rollback: TODO

### G-09e — Node wiring: StateSyncer, P2P server, TxHashSet processing
- Status: DONE
- Commits: 7ed7ffb, edd6151, 921ca42
- Files: `src/P2P/Sync/StateSyncer.cpp`, `src/P2P/P2PServerImpl.cpp`, `src/BlockChain/Processors/TxHashSetProcessor.cpp`, `src/BlockChain/BlockChainImpl.cpp`
- Rust reference: `chain/src/chain.rs` (txhashset apply), `p2p/src/protocol.rs`
- Before: TODO
- After: TODO
- Evidence: E32–E34 (live activation, validation, `FULLY_SYNCED`)
- Risk, rollback: TODO

### G-31 — Deterministic archive horizon
- Status: DONE
- Commits: 39b77a9, 9027ebf
- Files: `src/P2P/Sync/StateSyncer.cpp`, `include/Consensus.h`
- Rust reference: `chain/src/chain.rs:985` (`txhashset_archive_header`)
- Before: TODO
- After: TODO
- Evidence: E31 (live segment matches `header.output_root`), E42
- Risk, rollback: TODO

### G-32 / G-35 — Bitmap block size (64 chunks) and MSB-first bit order
- Status: DONE
- Commits: 39b77a9, c6ab7ff
- Files: `src/PMMR/Models/BitmapBlock.h`, `src/PMMR/Models/BitmapChunk.h`, `src/P2P/PIBD/BitmapSegmentAssembler.h`, `tests/src/PMMR/Test_Segment.cpp`
- Rust reference: `chain/src/txhashset/bitmap_accumulator.rs:470,213`
- Before: TODO
- After: TODO
- Evidence: live bitmap segment validates, oracle 7/7 roots (G-35), E32
- Risk, rollback: TODO

### G-09f — PIBD resume journal
- Status: DONE (unit), live replay PASS
- Commits: f73c7e8
- Files: `src/P2P/PIBD/PibdResume.h`, `src/P2P/PIBD/PibdSync.h`
- Rust reference: `chain/src/txhashset/desegmenter.rs` (resume position)
- Before: TODO
- After: TODO
- Evidence: E41/E42 (journal replay, no repeated bitmap requests)
- Risk, rollback: TODO

### G-43 — Peer rotation, retry backoff, rate threshold, journal retention
- Status: DONE
- Commits: 47b6bf1, 5de693d, a5a976a, b377220
- Files: `src/P2P/PIBD/PibdScheduler.h`, `src/P2P/Sync/StateSyncer.cpp`
- Rust reference: `p2p/src/peers.rs` (peer selection), timer semantics of `pibd.rs`
- Before: TODO
- After: TODO
- Evidence: E40 (null-peer regression fixed), E41, E42 (rotation 204→66, full cycle)
- Risk, rollback: TODO

### G-38 — Block sync policy: no timeout bans
- Status: DONE
- Commits: 4dbae1b, c06888b
- Files: `src/P2P/Sync/BlockSyncPolicy.h`, `src/P2P/Sync/BlockSyncer.cpp`, `src/P2P/Sync/HeaderSyncer.cpp`, `src/P2P/Seed/Seeder.cpp`, `include/P2P/SyncStatus.h`, `src/P2P/MessageProcessor.cpp`
- Rust reference: `servers/src/grin/sync/body_sync.rs:149,191-204`, `header_sync.rs:135-141`
- Before: TODO
- After: TODO
- Evidence: E37 (32-min window), E42 (4 222 reassignments, 0 fraud bans)
- Risk, rollback: TODO

### G-42 — Legacy state sync peer rotation instead of ban
- Status: DONE (code), live rotation PASS
- Commits: bbf9a56
- Files: `src/P2P/Sync/StateSyncer.cpp`
- Rust reference: `servers/src/grin/sync/syncer.rs` / `state_sync.rs`
- Before: TODO
- After: TODO
- Evidence: E41/E42 (`Rotating txhashset sync peer`)
- Risk, rollback: TODO

### G-46 / G-47 — Chain store consistency repair and Ping null guard
- Status: DONE (mitigation)
- Commits: 0331ea1
- Files: `src/BlockChain/ChainConsistency.h`, `src/BlockChain/BlockChainImpl.cpp`, `src/P2P/MessageProcessor.cpp`, `tests/src/BlockChain/Test_ChainConsistency.cpp`
- Rust reference: n/a (load-time invariants)
- Before: TODO
- After: TODO
- Evidence: E39 (unit + live injection both directions)
- Risk, rollback: TODO

### G-48 — Atomic checkpoint for chain stores
- Status: OPEN (implementation in progress; `DbCheckpoint` uncommitted at draft time)
- Commits: TODO (fill when merged)
- Files: `src/BlockChain/DbCheckpoint.h`, `src/BlockChain/ChainState.*`, `src/BlockChain/BlockChainImpl.cpp`
- Rust reference: n/a (crash-consistency design; see `DB_CONSISTENCY.md` §7)
- Before: TODO
- After: TODO
- Evidence: pending crash-injection tests
- Risk, rollback: TODO

### G-50 — Repeated DNS peer discovery
- Status: DONE (code)
- Commits: b37caa0
- Files: `src/P2P/Seed/Seeder.cpp`
- Rust reference: `p2p/src/peers.rs` (seeding)
- Before: TODO
- After: TODO
- Evidence: live run, two DNS queries, peers added
- Risk, rollback: TODO

### G-27 — PIHD detection only (no protocol)
- Status: N/A for v5.5.2 (bit detected, not advertised)
- Commits: a5c88bf (bit), f73c7e8, 348d87e
- Files: `include/P2P/Capabilities.h`, `tests/src/Net/Test_P2PCompat.cpp`
- Rust reference: `p2p/src/msg.rs` (v5.5.2 ends at `KernelSegment = 28`)
- Before: TODO
- After: TODO
- Evidence: E33, `Test_P2PCompat`
- Risk, rollback: TODO

## Excluded from the delta

- G-44 (PIHD dormant scaffold), G-45 (mainnet dry-run), G-49 (rangeproof
  performance), G-51 (this package): planned, no code changes yet.
- Documentation-only commits (`docs: …`): listed in the evidence map, not as cards.
