# AUDIT_DELTA (EN) — Grin++ 1.2.9 → Grin v5.5.2 compatibility

Baseline: Grin++ 1.2.9 (`5f511e6`). Delta updated: 2026-10-31, HEAD `256cbe4`.
Companion documents: `AUDIT_PACKAGE.md` (package rules), `GAP_MATRIX.md`
(status of every gap), `TEST_RESULTS.md` (E1–E47 evidence), `DB_CONSISTENCY.md`
(chain-store design).

Scope: one card per change where code differs from the baseline, or per
inseparable group of changes. Each card states the commit(s), the touched
files, the upstream Rust reference, a before/after fragment, the evidence and
the risk plus rollback. Planned items without code are excluded (see the end);
documentation-only commits are tracked in the evidence map, not as cards.

## Cards

### G-24 — Reproducible build (offline sysroot)
- Status: DONE
- Commits: `89eb22c`, `d034a2f`
- Files: `tools/deps/build_sysroot.py`, `tools/build/build.ps1`, `CMakeLists.txt`, `src/CMakeLists.txt`
- Rust reference: n/a (build infrastructure)
- Before: configure resolved dependencies from the network/system vcpkg and used whichever MSVC/Ninja was on `PATH`; builds were not reproducible offline.
- After: `.toolchain/sysroot` provides headers and libraries offline; the generator/toolset are pinned; `build.ps1 -Target GrinNode|Tests` is the single entry point; warnings build with `/W4 /WX`.
- Diff: `tools/build/build.ps1`, commit `89eb22c` — offline sysroot/`/W4 /WX` entry point.
- Evidence: `BUILD.md`, E6–E9 (clean build, no warnings disabled), binary SHA-256 in `BUILD.md`.
- Risk, rollback: risk is a stale or incomplete sysroot; roll back by reverting the build scripts/CMake changes and using system dependencies.

### G-19 — Bounds-safe deserialization
- Status: DONE
- Commits: `fc9e988`
- Files: `include/Core/Serialization/ByteBuffer.h`, `tests/src/Core/Serialization/Test_ByteBuffer.cpp`
- Rust reference: `core/src/ser.rs` (reader bounds checks)
- Before: varint/vector/string readers could consume bytes beyond the payload and size arithmetic was not checked against the remaining buffer.
- After: every read is bounds-checked against the buffer end (protocol-version aware); truncated and oversized inputs fail cleanly; unit tests cover both.
- Diff: `include/Core/Serialization/ByteBuffer.h`, commit `fc9e988`
  ```diff
  --- a/include/Core/Serialization/ByteBuffer.h
  +++ b/include/Core/Serialization/ByteBuffer.h
  -        if (m_index + sizeof(T) > m_bytes.size()) {
  -            throw DESERIALIZATION_EXCEPTION("Attempted to read past end of ByteBuffer.");
  -        }
  +        RequireRemaining(sizeof(T));
  ```
- Evidence: E2 (reproduced before/after), `Test_ByteBuffer`.
- Risk, rollback: strict checks can reject edge-valid messages; the protocol-version gate limits this to formats that require it; roll back by reverting `ByteBuffer`.

### G-08 / G-10 / G-11 — Message table, capabilities, unknown types
- Status: DONE
- Commits: `a5c88bf`
- Files: `include/P2P/Capabilities.h`, `src/P2P/Messages/MessageTypes.h`, `src/P2P/Messages/MessageHeader.h`, `tests/src/Net/Test_P2PCompat.cpp`
- Rust reference: `p2p/src/msg.rs:67-97`, `p2p/src/types.rs:381-399`
- Before: the message table and capability bits did not match grin v5.5.2; PIBD segment types were absent and unknown types were not rejected explicitly.
- After: full `EMessageType` table including segment requests/responses 21–28, capability values aligned with Rust, unknown types rejected before dispatch; compatibility tests pin both.
- Evidence: E3, `Test_P2PCompat`.
- Risk, rollback: an enum mismatch breaks the handshake/message dispatch; tests pin the wire values; roll back the table and capability headers together.

### G-12 / G-14 / G-15 — Handshake genesis, archive send/receive
- Status: DONE (code); live archive transfer NOT VERIFIED
- Commits: `e5287f2`
- Files: `src/P2P/Seed/HandShake.cpp`, `src/P2P/MessageProcessor.cpp`, `src/P2P/Pipeline/TxHashSetPipe.cpp`
- Rust reference: `p2p/src/handshake.rs`, `p2p/src/msg.rs:735-789`, `p2p/src/protocol.rs:254-265`
- Before: the handshake did not verify the genesis hash, and the archive sender looped on `istream::read`, silently dropping the final partial chunk; the receiver had no size/cleanup guard.
- After: genesis mismatch rejects the peer; archive framing uses `gcount()` and sends every byte, with a size check and removal of partial files on failure.
- Evidence: E4/E5 (code-level); live legacy archive transfer still NOT VERIFIED (G-52).
- Risk, rollback: rejecting a wrong-genesis peer is intended; archive framing changes are limited to `TxHashSetPipe`; roll back both files.

### G-25 — Version 1.2.9
- Status: DONE
- Commits: `348d87e`
- Files: `include/GrinVersion.h`, `include/P2P/Common.h`
- Rust reference: n/a
- Before: `GRINPP_VERSION`/user agent reported 1.2.8.
- After: version and UA report 1.2.9; `GrinVersion::MatchesRelease("Grin++ 1.2.9")` pins it and `/v1/status` exposes the same string.
- Evidence: unit test, live `/v1/status` (E30).
- Risk, rollback: version changes influence peer banners; rollback is a one-line revert.

### G-29 — Accept loop survives rejected peers
- Status: DONE
- Commits: `bfc31a0`
- Files: `src/P2P/Seed/Seeder.cpp`
- Rust reference: `p2p/src/serv.rs` (accept loop)
- Before: `Seeder::Accept` returned after rejecting a peer without re-arming `async_accept`, so the listener stopped after the first rejected connection.
- After: the accept handler always re-arms the listener; a rejected connection no longer kills inbound service.
- Evidence: E29 (`0.0.0.0:13414 LISTENING` after rejections), live handshakes.
- Risk, rollback: a persistent accept error could spin; the handler keeps the error path guarded; rollback reverts the re-arm.

### G-30 — Advertised capabilities in handshake + configurable P2P port
- Status: DONE
- Commits: `72c94f2`
- Files: `src/P2P/Seed/HandShake.cpp`, `include/Core/Config.h`, `src/Core/Config.cpp`, `src/Core/Config/ConfigProps.h`, `src/Core/Config/P2PConfig.h`
- Rust reference: `p2p/src/msg.rs` (Hand/Shake fields)
- Before: the handshake advertised a hardcoded `FAST_SYNC_NODE`, hiding the PIBD bits, and the listen port was also used to dial the network.
- After: the handshake advertises `ADVERTISED` (`FAST_SYNC_NODE | PIBD_HIST | PIBD_HIST_1`); `P2P_PEER_PORT` selects the dialling port; both are configuration-backed and tested.
- Diff: `src/P2P/Seed/HandShake.cpp`, commit `72c94f2`
  ```diff
  --- a/src/P2P/Seed/HandShake.cpp
  +++ b/src/P2P/Seed/HandShake.cpp
  -        Capabilities::FAST_SYNC_NODE,
  +        Capabilities::ADVERTISED,
  -        Capabilities::FAST_SYNC_NODE,
  +        Capabilities::ADVERTISED,
  ```
- Evidence: E30 (a live Rust peer sees `TXHASHSET_HIST | PEER_LIST | PIBD_HIST | PIBD_HIST_1`).
- Risk, rollback: peers may treat extra bits differently; PIBD bits can be disabled by policy; rollback restores the previous advertised value.

### G-09a — PIBD wire layer, segment verification, generator
- Status: DONE
- Commits: `62a146c`, `8e2f493`, `de3f1a5`, `c390ee9`
- Files: `src/PMMR/Models/Segment*.h`, `src/PMMR/Models/Bitmap*.h`, `src/P2P/Messages/Segment*Message.h`, `tests/src/PMMR/Test_Segment.cpp`, `tests/src/PMMR/Test_SegmentGenerator.cpp`
- Rust reference: `core/src/core/pmmr/segment.rs`, `chain/src/txhashset/*`
- Before: there were no PIBD wire messages or segment models; only the legacy zip path existed.
- After: segment request/response messages 21–28, bitmap chunk/block and segment models with verification, and a deterministic segment generator; unit tests compare roots and wire bytes.
- Evidence: E26, unit suites `Test_Segment`, `Test_SegmentGenerator`.
- Risk, rollback: a wire mismatch makes PIBD unusable but leaves legacy intact; the feature flag `P2P.PIBD_ENABLED=false` disables the path; rollback removes the messages/models.

### G-09b — Bitmap assembly, download orchestration, session/scheduler, snapshot
- Status: DONE
- Commits: `621efad`, `ec006a9`, `21df0b9`, `df81cc3`, `e167e8a`
- Files: `src/P2P/PIBD/BitmapSegmentAssembler.h`, `PibdDownload.h`, `PibdSession.h`, `PibdScheduler.h`, `PibdSnapshot.h`, `PibdTreeAssembler.h`, `PibdTreeRebuilder.h`
- Rust reference: `chain/src/txhashset/desegmenter.rs`, `segmenter.rs`, `p2p/src/protocol.rs:294+`
- Before: no bitmap assembly, no download orchestration, no request scheduler; PIBD could not be driven.
- After: bitmap segments are assembled and validated, downloads are scheduled with in-flight limits/timeouts/retries/fallback, and trees are assembled/rebuilt from segments with a snapshot API.
- Evidence: E24/E25/E26, unit tests per module.
- Risk, rollback: memory/CPU growth and retry storms are bounded by the scheduler limits; disable via `PIBD_ENABLED` or revert the module.

### G-09c — Staging PMMR write and atomic activation
- Status: DONE
- Commits: `b681c2e`, `fcea044`
- Files: `src/P2P/PIBD/PibdStateWriter.h`, `src/P2P/PIBD/PibdActivation.h`
- Rust reference: `chain/src/txhashset/txhashset.rs` (snapshot), activation semantics
- Before: a downloaded PIBD state had no crash-safe staging/activation path.
- After: `PibdStateWriter` writes staging PMMR files; activation uses a temporary directory/marker and switches after validation; tests inject crashes in both directions and verify recovery.
- Diff: `src/P2P/PIBD/PibdActivation.h`, commit `fcea044`
  ```diff
  --- /dev/null
  +++ b/src/P2P/PIBD/PibdActivation.h
  +// Atomic activation of a downloaded PIBD state.
  +//
  +// The writer produces a complete state in a staging directory. Activation swaps
  +// it in with two renames and a marker file:
  +//
  +//   1. write <live parent>/pibd_activation.pending with (staging, backup) paths
  +//   2. rename live -> backup            (if a live state exists)
  +//   3. rename staging -> live
  ```
- Evidence: `Test_PibdStateWriter`, `Test_PibdActivation`.
- Risk, rollback: a partial staging directory must never be installed; the activation marker and tests enforce it; rollback deletes staging and keeps the live state.

### G-09d — PIBD serving from node PMMR (incl. pruned)
- Status: DONE (unit); live serving NOT VERIFIED
- Commits: `49942ab`, `30012d6`, `4f9a569`, `177c97a`
- Files: `src/P2P/PIBD/PibdSegmentService.h`, `src/P2P/MessageProcessor.cpp`, `include/BlockChain/BlockChain.h`, `src/BlockChain/BlockChainImpl.cpp`
- Rust reference: `chain/src/txhashset/segmenter.rs`, `p2p/src/protocol.rs` (21/23/25/27)
- Before: the node could not answer `GetSegment`; segment serving did not exist.
- After: `PibdSegmentService` serves bitmap/output/rangeproof/kernel segments from the node PMMR, including pruned paths; MessageProcessor routes requests and BlockChain exposes the segmenter; a pruned-bitmap test covers the hard case.
- Evidence: `Test_PibdServing`; live serving to a Rust client NOT VERIFIED.
- Risk, rollback: serving a wrong segment or allowing a DoS is possible if horizon checks regress; unit tests pin the paths; rollback removes routing and service.

### G-09e — Node wiring: StateSyncer, P2P server, TxHashSet processing
- Status: DONE
- Commits: `7ed7ffb`, `edd6151`, `921ca42`
- Files: `src/P2P/Sync/StateSyncer.cpp`, `src/P2P/P2PServerImpl.cpp`, `src/BlockChain/Processors/TxHashSetProcessor.cpp`, `src/BlockChain/BlockChainImpl.cpp`
- Rust reference: `chain/src/chain.rs` (txhashset apply), `p2p/src/protocol.rs`
- Before: StateSyncer only used `txhashset.zip`; the P2P server had no PIBD resources; PIBD state installation/validation was missing.
- After: `TryPibd` drives the PIBD session before legacy fallback, the server wires resume/serving paths, and `ProcessExistingTxHashSet` validates and installs the downloaded state.
- Evidence: E32–E34, E42 (live activation, validation, `FULLY_SYNCED`).
- Risk, rollback: a broken install can corrupt the state; validation runs before activation; `PIBD_ENABLED=false` is the safe rollback.

### G-31 — Deterministic archive horizon
- Status: DONE
- Commits: `39b77a9`, `9027ebf`
- Files: `src/P2P/Sync/StateSyncer.cpp`, `include/Consensus.h`
- Rust reference: `chain/src/chain.rs:985` (`txhashset_archive_header`)
- Before: PIBD requested the candidate tip hash, so peers answered `SegmenterHeaderMismatch`.
- After: the horizon is `archive_height = (head − STATE_SYNC_THRESHOLD)` rounded down to `TXHASHSET_ARCHIVE_INTERVAL = 720`; PIBD and legacy archive requests use that height.
- Diff: `src/P2P/Sync/StateSyncer.cpp`, commit `39b77a9`
  ```diff
  --- a/src/P2P/Sync/StateSyncer.cpp
  +++ b/src/P2P/Sync/StateSyncer.cpp
  +        // archive_height = (header_head - STATE_SYNC_THRESHOLD) rounded down to a
  +        // multiple of TXHASHSET_ARCHIVE_INTERVAL (grin txhashset_archive_header_header_only).
  +        uint64_t archiveHeight = pTip->GetHeight();
  +        archiveHeight = archiveHeight > Consensus::STATE_SYNC_THRESHOLD
  +            ? archiveHeight - Consensus::STATE_SYNC_THRESHOLD
  +            : 0;
  +        archiveHeight -= archiveHeight % Consensus::TXHASHSET_ARCHIVE_INTERVAL;
  ```
- Evidence: E31 (live segment matches `header.output_root`), E42.
- Risk, rollback: an off-by-one interval breaks segment validation; oracle/live tests pin it; rollback restores the tip request (known broken).

### G-32 / G-35 — Bitmap block size (64 chunks) and MSB-first bit order
- Status: DONE
- Commits: `39b77a9`, `c6ab7ff`
- Files: `src/PMMR/Models/BitmapBlock.h`, `src/PMMR/Models/BitmapChunk.h`, `src/P2P/PIBD/BitmapSegmentAssembler.h`, `tests/src/PMMR/Test_Segment.cpp`
- Rust reference: `chain/src/txhashset/bitmap_accumulator.rs:470,213`
- Before: a `BitmapBlock` held 16 chunks and indexed bits LSB-first; sparse (positive/negative) segments were misread and real segments did not deserialize.
- After: 2^16 bits = 64 chunks × 1024; bit `n` lives in byte `n/8`, bit `7−(n%8)` (MSB-first, as `bitvec::to_bytes`); raw chunks serialize the 1024 bytes without a length prefix.
- Diff: `src/PMMR/Models/BitmapChunk.h`, commit `c6ab7ff`
  ```diff
  --- a/src/PMMR/Models/BitmapChunk.h
  +++ b/src/PMMR/Models/BitmapChunk.h
  -        return (m_bytes[idx / 8] & (uint8_t)(1u << (idx % 8))) != 0;
  +        return (m_bytes[idx / 8] & (uint8_t)(1u << (7 - (idx % 8)))) != 0;
  -        const uint8_t mask = (uint8_t)(1u << (idx % 8));
  +        const uint8_t mask = (uint8_t)(1u << (7 - (idx % 8)));
  ```
- Evidence: E32/E35, oracle vectors (`grin_chain 5.5.2`): 7/7 segment roots and the whole bitmap root match.
- Risk, rollback: wrong bit order silently corrupts roots; oracle and unit vectors catch it; rollback restores the previous layout only with PIBD disabled.

### G-09f — PIBD resume journal
- Status: DONE (unit + live replay)
- Commits: `f73c7e8`
- Files: `src/P2P/PIBD/PibdResume.h`, `src/P2P/PIBD/PibdSync.h`
- Rust reference: `chain/src/txhashset/desegmenter.rs` (resume position)
- Before: a restart threw away all downloaded segments and PIBD started from zero.
- After: per-kind append-only journals (`bitmap.log`, `output.log`, `rangeproof.log`, `kernel.log`, `manifest`) with u32 BE length frames are replayed before new requests; the manifest binds the journal to the horizon and is cleared only after activation.
- Evidence: E41/E42 (after restart, zero repeated bitmap requests; journal grew and download completed).
- Risk, rollback: a stale/corrupt journal could feed wrong segments; horizon/deserialization checks discard it; rollback deletes `NODE/pibd_resume`.

### G-43 — Peer rotation, retry backoff, rate threshold, journal retention
- Status: DONE
- Commits: `47b6bf1`, `5de693d`, `a5a976a`, `b377220`
- Files: `src/P2P/PIBD/PibdScheduler.h`, `src/P2P/Sync/StateSyncer.cpp`
- Rust reference: `p2p/src/peers.rs` (peer selection), timer semantics of `pibd.rs`
- Before: the first stalled segment aborted PIBD and disabled it for 120 minutes; a slow-but-alive peer was never rotated; after a horizon reset the peer pointer could be null (crash).
- After: a segment retries until 660 s without any accepted response; `RotatePibdPeer` tries other peers, then up to three rounds over the peer set with 30/60/120 s backoff; `RateWindow` rotates peers that accept fewer than 10 segments per 180 s; `Reset()` keeps the journal; the peer is re-picked after a horizon change.
- Evidence: E40 (null-peer regression fixed), E41/E42 (live rotation 204→66, full cycle).
- Risk, rollback: rotation churn on a slow network; thresholds are constants; rollback reverts the scheduler/rotation and keeps legacy fallback.

### G-38 — Block sync policy: no timeout bans
- Status: DONE
- Commits: `4dbae1b`, `c06888b`, `7997ca5`
- Files: `src/P2P/Sync/BlockSyncPolicy.h`, `src/P2P/Sync/BlockSyncer.cpp`, `src/P2P/Sync/HeaderSyncer.cpp`, `src/P2P/Seed/Seeder.cpp`, `include/P2P/SyncStatus.h`, `src/P2P/MessageProcessor.cpp`, `tests/src/Net/Test_BlockSyncPolicy.cpp`
- Rust reference: `servers/src/grin/sync/body_sync.rs:149,191-204`, `header_sync.rs:135-141`
- Before: `BlockSyncer` banned a peer for `FraudHeight` after two timeouts, `m_slowPeers` never expired, the request window ignored peer heights, and seeder/syncer stayed silent after the bans; the header fraud rule used "height OR difficulty".
- After: a timeout is counted and the block is reassigned; requests are capped by `min(candidate tip, network height, per-connection height)` and batch ≤100; retry 6 s/1 s + backoff; slow-peer cooldown 60 s; `FraudHeight` now mirrors Rust exactly — strictly greater TOTAL DIFFICULTY plus 120 s without header progress (a live server produced three false bans with the earlier height-OR-difficulty rule; fixed and regression-tested, E48); seeder logs the reason; `/v1/status` exposes waiting reason and counters.
- Diff: `src/P2P/Sync/BlockSyncPolicy.h`, commit `4dbae1b`
  ```diff
  --- /dev/null
  +++ b/src/P2P/Sync/BlockSyncPolicy.h
  +// Behaviour mirrored from grin v5.5.2 body_sync.rs / header_sync.rs:
  +//   * a timeout is never fraud: reassign the block and count it;
  +//   * FraudHeight requires a peer that claims more work than we have and has
  +//     made no progress for a long time;
  ```
- Evidence: E37 (32-min live window), E42 (4 222 reassignments, zero fraud bans), E48 (TD-only fix after live false bans).
- Risk, rollback: without bans an unresponsive peer can slow catch-up; cooldown/rotation bound it; rollback restores the previous ban path.

### G-42 — Legacy state sync peer rotation instead of ban
- Status: DONE
- Commits: `bbf9a56`
- Files: `src/P2P/Sync/StateSyncer.cpp`
- Rust reference: `servers/src/grin/sync/syncer.rs`, `state_sync.rs`
- Before: `RequestState` banned the peer with `BadTxHashSet` whenever the zip made no progress for 10 s or reached the 120-minute timeout, which could leave the node peerless.
- After: a slow peer is rotated (`Rotating txhashset sync peer`); a genuinely invalid archive is still banned by the txhashset pipeline after validation fails.
- Diff: `src/P2P/Sync/StateSyncer.cpp`, commit `bbf9a56`
  ```diff
  -        LOG_WARNING_F("Banning peer: {}", m_pPeer);
  -        m_pPeer->Ban(EBanReason::BadTxHashSet);
  +        LOG_INFO_F("Rotating txhashset sync peer: {}", m_pPeer);
  ```
- Evidence: E41/E42.
- Risk, rollback: a broken peer may be retried; rotation is logged; rollback restores the ban.
### G-46 / G-47 — Chain store consistency repair and Ping null guard
- Status: DONE (load-time mitigation)
- Commits: `0331ea1`
- Files: `src/BlockChain/ChainConsistency.h`, `src/BlockChain/BlockChainImpl.cpp`, `src/P2P/MessageProcessor.cpp`, `tests/src/BlockChain/Test_ChainConsistency.cpp`
- Rust reference: n/a (load-time invariants)
- Before: after an unclean shutdown the header MMR and candidate/confirmed chain could diverge; loading such a state read past the MMR (0xC0000005). The Ping handler dereferenced a missing confirmed tip.
- After: `ComputeChainRepair` compares leaf/entry counts and truncates the stores to a consistent target before `ChainState` opens them; the Ping handler checks the tip and answers or logs a warning instead of crashing.
- Diff: `src/BlockChain/ChainConsistency.h`, commit `0331ea1` — load-time counts repair and Ping null guard.
- Evidence: E39 (unit + live injection in both directions).
- Risk, rollback: repair can truncate uncommitted headers; recovery copies and the checkpoint bound the loss; rollback restores `NODE/recovery-*`.

### G-48 — Atomic checkpoint for chain stores
- Status: DONE (checkpoint and no-loss repair; see G-23 for hash-level validation)
- Commits: `fdef6e6`, `1c4d020`, `7997ca5`
- Files: `src/BlockChain/DbCheckpoint.h`, `src/BlockChain/ChainState.*`, `src/BlockChain/BlockChainImpl.cpp`, `include/BlockChain/DbRecovery.h`
- Rust reference: n/a (crash-consistency design; `DB_CONSISTENCY.md` §4)
- Before: `header_mmr.bin`, `candidate.chain` and `confirmed.chain` were committed independently; a crash between commits left different heights, and early repair logic truncated the chain down to a lagging MMR, losing headers.
- After: `ChainState::Commit` writes `chain.checkpoint` (52 bytes, FNV-1a) via temp+rename after all store commits; loading validates invariants, rebuilds a lagging MMR from the block DB, truncates only extra leaves, restores a confirmed chain tail when possible, requires a verified recovery copy, and aborts if the chain index is behind the checkpoint instead of truncating; `db_repaired`/`db_last_repair` are exposed. Live: hard kill seq 26→27; MMR −2000 leaves rebuilt with headers preserved (E43/E44/E48).
- Diff: `src/BlockChain/DbCheckpoint.h`, commit `fdef6e6`
  ```diff
  --- /dev/null
  +++ b/src/BlockChain/DbCheckpoint.h
  +struct DbCheckpointData
  +{
  +    uint64_t Sequence = 0;
  +    uint64_t MmrLeaves = 0;
  +    uint64_t CandidateEntries = 0;
  +    uint64_t ConfirmedEntries = 0;
  +};
  ```
- Evidence: E43, E44, E48.
- Risk, rollback: an impossible repair aborts startup (safe, no silent loss); recovery copies remain; rollback removes `chain.checkpoint` and falls back to the counts heuristic.

### G-50 — Repeated DNS peer discovery
- Status: DONE
- Commits: `b37caa0`
- Files: `src/P2P/Seed/Seeder.cpp`
- Rust reference: `p2p/src/peers.rs` (seeding)
- Before: DNS was queried once (`m_usedDNS`); if the seeds were stale the node stayed peerless forever.
- After: DNS is queried again every five minutes while no candidate exists; the number of returned peers is logged and added, with the existing seeder backoff and reason log.
- Diff: `src/P2P/Seed/Seeder.cpp`, commit `b37caa0`
  ```diff
  -    } else if (!m_usedDNS.exchange(true)) {
  -        std::vector<SocketAddress> peerAddresses = DNSSeeder::GetPeersFromDNS();
  +    } else if (m_lastDnsAttempt + std::chrono::minutes(5) <= now) {
  +        m_lastDnsAttempt = now;
  +        std::vector<SocketAddress> peerAddresses = DNSSeeder::GetPeersFromDNS();
  ```
- Evidence: live run (two DNS rounds, addresses added).
- Risk, rollback: repeated DNS lookups; the five-minute throttle bounds them; rollback restores the single query.
### G-27 — PIHD detection only (no protocol)
- Status: N/A for grin v5.5.2 (bit detected, never advertised)
- Commits: `a5c88bf`, `f73c7e8`, `348d87e`
- Files: `include/P2P/Capabilities.h`, `tests/src/Net/Test_P2PCompat.cpp`
- Rust reference: `p2p/src/msg.rs` (v5.5.2 ends at `KernelSegment = 28`)
- Before: PIHD was treated as an implementable header-sync mode even though the reference has no PIHD messages.
- After: the `PIHD_HIST` bit (`0x80`) is detected but not advertised and PIHD is never negotiated; header sync always uses the legacy path; a test pins the behaviour.
- Evidence: E33, `Test_P2PCompat`.
- Risk, rollback: none for compatibility (the reference cannot speak PIHD); keep detection-only.

### G-16 — TxHashSet archive size limit
- Status: DONE
- Commits: `1c4d020`
- Files: `include/P2P/Pipeline/TxHashSetLimits.h`, `src/P2P/Pipeline/TxHashSetPipe.cpp`, `tests/src/Net/Test_TxHashSetLimits.cpp`
- Rust reference: n/a (defensive bound; the reference validates the archive itself)
- Before: the `zipped_size` from `TxHashSetArchiveMessage` was used directly to create and write the archive file; zero or arbitrarily large sizes could fill the disk.
- After: `IsPlausibleSize` requires `0 < size ≤ 8 GiB`; the pipeline rejects an implausible archive before creating the file, marks `TXHASHSET_SYNC_FAILED` and bans `BadTxHashSet`.
- Diff: `include/P2P/Pipeline/TxHashSetLimits.h`, commit `1c4d020` — 8 GiB archive bound before writing.
- Evidence: E44, unit `TxHashSetLimits`.
- Risk, rollback: a legitimate archive above 8 GiB would be rejected (mainnet archives are far below); the constant is in one header; rollback removes the check.

### G-04 — PoW edge-bit validation
- Status: DONE
- Commits: `9d9f1ae`
- Files: `include/PoW/PoWValidator.h`, `src/PoW/PoWValidator.cpp`, `tests/src/Consensus/Test_PoWEdgeBits.cpp`
- Rust reference: `chain/src/pipe.rs` (`validate_pow_only`, `is_primary`/`is_secondary`), `core/src/pow/cuckatoo.rs` (`verify_size`)
- Before: only `edge_bits == 29` selected a Cuckoo variant; any other value (0, 24, 30, 64, 255) reached `Cuckatoo::Validate`, where a 64-bit shift by `edge_bits` is undefined.
- After: `PoWValidator::IsSupportedEdgeBits` accepts secondary 29 and primary 31–63; `IsPoWValid` rejects every other size before dispatch; the boundary table is unit-tested.
- Diff: `src/PoW/PoWValidator.cpp`, commit `9d9f1ae`
  ```diff
  +    if (!IsSupportedEdgeBits(header.GetEdgeBits())) {
  +        LOG_WARNING_F("Unsupported PoW edge bits {} for block {}", header.GetEdgeBits(), header);
  +        return false;
  +    }
  ```
- Evidence: E50, `Test_PoWEdgeBits` (11 assertions).
- Risk, rollback: future primary sizes above 63 would need a consensus change; the accepted set is one constant; rollback removes the guard.
### G-20 — Parser size limits
- Status: DONE (PIBD/segment parsers; fuzz outside the matrix)
- Commits: `9d9f1ae`
- Files: `src/PMMR/Models/Segment.h`, `tests/src/PMMR/Test_Segment.cpp`, `src/PMMR/Models/BitmapSegment.h`, `src/PMMR/Models/SegmentProof.h`
- Rust reference: `core/src/ser.rs` (bounds checks), `core/src/core/pmmr/segment.rs` (`MAX_SEGMENT_READ_ITEMS`)
- Before: segment counts were bounded by `MAX_SEGMENT_READ_ITEMS`, checked arithmetic and message caps, but the leaf vector could reserve up to that limit before verifying that the leaf list fits the payload.
- After: `numLeaves > remaining` is rejected before reserving leaf storage; `SegmentProof`/`BitmapSegment`/`BitmapBlock` limits, checked arithmetic and the message-size cap remain; tests cover huge counts, non-increasing positions and over-height segments.
- Diff: `src/PMMR/Models/Segment.h`, commit `9d9f1ae`
  ```diff
  +        if (numLeaves > byteBuffer.GetRemainingSize()) {
  +            throw DESERIALIZATION_EXCEPTION("Segment leaf list does not fit in the payload");
  +        }
  ```
- Evidence: E50, `Test_Segment` (21 cases, including huge counts).
- Risk, rollback: malformed payloads are rejected earlier; valid segments are unaffected; rollback removes the leaf-fit guard.
### G-21 — Atomic legacy txhashset activation
- Status: DONE
- Commits: `2122e32`
- Files: `src/PMMR/Zip/TxHashSetZip.*`, `include/PMMR/TxHashSetManager.h`, `src/PMMR/TxHashSetManager.cpp`, `src/BlockChain/Processors/TxHashSetProcessor.cpp`, `include/PMMR/TxHashSetStaging.h`, `tests/src/PMMR/Test_TxHashSetStaging.cpp`
- Rust reference: `chain/src/txhashset/txhashset.rs` (state installation), `servers/src/grin/sync/state_sync.rs`
- Before: `TxHashSetZip::Extract` removed and rewrote the live `NODE/TXHASHSET` before validation; a failed or interrupted legacy sync left a partial state.
- After: the zip is extracted to `TXHASHSET.staging` and validated there while the live state is untouched; failure removes staging and reopens the live state; success performs `live → backup` / `staging → live` under `txhashset_activation.pending`, and startup `TxHashSetStaging::Recover` restores the backup or drops stale staging.
- Diff: `src/BlockChain/Processors/TxHashSetProcessor.cpp`, commit `2122e32`
  ```diff
  -        ITxHashSetPtr pTxHashSet = TxHashSetManager::LoadFromZip(m_config, path, pHeader);
  +        const fs::path stagingPath = TxHashSetStaging::Staging(livePath);
  +        ITxHashSetPtr pStaged = TxHashSetManager::LoadFromZip(m_config, path, pHeader, stagingPath);
  ```
- Evidence: E49, `Test_TxHashSetStaging` (3 cases).
- Risk, rollback: a rename failure restores the backup; a crash between renames is recovered by the marker; rollback removes the staging path and returns to direct extraction.
### G-22 — Database backup before migration
- Status: DONE
- Commits: `9c6bb34`
- Files: `include/Database/DbBackup.h`, `src/BlockChain/BlockChainImpl.cpp`, `tests/src/Database/Test_DbBackup.cpp`
- Rust reference: n/a (operational safety)
- Before: the v1→v2 and v2→v3 migrations rewrote the DB in place with no previous copy.
- After: `DbBackup` copies the open RocksDB directory recursively with stream copies (skipping `LOCK`) to `DB.backup-v<version>-<timestamp>` before migration; a failed copy aborts the migration; a fresh database (version 0) is not copied.
- Diff: `include/Database/DbBackup.h`, commit `9c6bb34` — recursive stream backup before migration.
- Evidence: E47, `Test_DbBackup`.
- Risk, rollback: extra disk/time during migrations; only real migrations copy; rollback removes the backup and the call.

### G-23 — Header MMR structural validation and rebuild on load
- Status: PARTIAL (structural check and rebuild DONE; hash/root revalidation open)
- Commits: `1c4d020`, `0331ea1`, `7997ca5`
- Files: `include/PMMR/Common/MmrStructure.h`, `include/PMMR/HeaderMMR.h`, `src/PMMR/HeaderMMRImpl.h`, `src/BlockChain/BlockChainImpl.cpp`, `include/BlockChain/DbRecovery.h`, `tests/src/PMMR/Test_MmrStructure.cpp`
- Rust reference: n/a (MMR invariant `N = 2L − popcount(L)`; see `DB_CONSISTENCY.md`)
- Before: the raw node count in `header_mmr.bin` was trusted; a truncated file could be loaded as a valid MMR, and the first load-time repair truncated the candidate chain when the MMR lagged (live header loss).
- After: `IsValidNodeCount` verifies `N = 2L − popcount(L)`; a lagging MMR is rebuilt from the block DB (heights from the current leaf count to the candidate tip), only extra MMR leaves are dropped, and every modification requires a size-verified recovery copy; a chain index behind the checkpoint aborts startup instead of truncating. Live injection: MMR −2000 leaves → `Rebuilding header MMR … heights 3316116-3317117`, header preserved, candidate file unchanged (E48).
- Diff: `include/PMMR/Common/MmrStructure.h`, commit `1c4d020` — `N = 2L − popcount(L)`; rebuild lagging MMR from block DB (E48).
- Evidence: E44 (structural regression/guard), E48 (no-loss rebuild; abort when the block DB lacks a header).
- Risk, rollback: a missing DB header aborts startup (safe failure, no truncation); hash-level validation is still absent; rollback disables the rebuild and keeps counts/checkpoint.### G-52 — Historical TxHashSet archive: persistence/serving (OPEN, partially implemented)
### G-52 — Historical TxHashSet archive: persistence/serving (OPEN, partially implemented)
- Status: OPEN (archive persistence/serving added; live transfer NOT VERIFIED)
- Commits: `7973d46`, `7997ca5`
- Files: `src/PMMR/TxHashSetManager.cpp`, `src/PMMR/TxHashSetImpl.cpp`, `src/P2P/Pipeline/TxHashSetPipe.cpp`, `include/P2P/TxHashSetArchive.h`, `src/P2P/Sync/StateSyncer.cpp`
- Rust reference: `chain/src/txhashset/txhashset.rs` (archive snapshot)
- Before: a failed historical snapshot was swallowed (`catch (...) { return; }`), and serving rebuilt historical state on every request (failing on the spent-position gap at block 3 316 349).
- After: the activation path saves the horizon state to `NODE/TXHASHSET/archive/TxHashSet.<height>.<hash>.zip` (`ArchivePibdState`), and `TxHashSetPipe` serves that archive by hash without any rewind; snapshot failures log the flushed/requested heights and the failing block/input/spent counts. The underlying spent-position gap remains for arbitrary historical snapshots, and the live archive/legacy transfer is still NOT VERIFIED.
- Diff: `src/PMMR/TxHashSetManager.cpp`, commit `7973d46`
  ```diff
  --- a/src/PMMR/TxHashSetManager.cpp
  +++ b/src/PMMR/TxHashSetManager.cpp
  +            LOG_INFO_F(
  +                "TxHashSet snapshot: flushed height {}, requested height {}.",
  +                pFlushedHeader != nullptr ? pFlushedHeader->GetHeight() : 0,
  +                pHeader->GetHeight());
  ```
- Evidence: E45/E46 (diagnosis), E48 (archive persistence code; live pending).
- Risk, rollback: archive files consume disk; only horizon activations create them; rollback removes the archive directory and the save/serve hooks.## Known open items (not cards)

- **G-52 (OPEN)**: historical `TxHashSet` snapshot/serving fails on a spent-position
  history gap (`Rewind: Spent output not found … block 3 316 349`, zero records for
  one input). Legacy serving and G-41 remain NOT VERIFIED; options are recorded in
  `TEST_RESULTS.md` E46 and `REMAINING.md`.
- **G-41 (OPEN)**: live legacy `txhashset.zip` fallback not completed (peers throttle
  the handshake; server side blocked by G-52). The client now requests the archive
  at the rounded 720-block height.
- **Live PIBD serving** to a Rust client: unit-tested only.
- **G-23 hash-level** MMR revalidation; **G-21** atomic legacy activation;
  **G-20** remaining parser limits.
- **G-04** PoW edge bits, **G-07** NRD, **G-49** rangeproof validation performance,
  differential/fuzz suites, mainnet (out of scope for this audit).
- **PIHD**: N/A for grin v5.5.2; detection-only by design.

## Excluded from the delta

- G-44 (PIHD dormant scaffold), G-45 (mainnet dry-run), G-49 (rangeproof
  performance), G-51 (this package): closed (audit package; owner acceptance 01.11.2026).
- Documentation-only commits (`docs: …`): listed in the evidence map, not as cards.
