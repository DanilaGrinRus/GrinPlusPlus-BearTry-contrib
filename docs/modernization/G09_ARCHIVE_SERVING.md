# G-09 archive serving: snapshot/capability contract, gap analysis and DRAFT review

> **Текущий статус (2026-11-03):** это техническое досье, а не текущая доска.
> Актуальный срез: [THREE_LINE_STATUS.md](THREE_LINE_STATUS.md); матрица рисков:
> [GAP_MATRIX.md](GAP_MATRIX.md). Build-07 (`Tests C8C386BE…`,
> `GrinNode F9EF06B4…`) — принятое evidence только для своего снимка.
> Финальный кандидат — build-09 (`Tests 0A3B7D66…`, `GrinNode F90EEF38…`);
> независимый full/ASan аудит ещё идёт. Build-08 (`7994F191…`) — historical,
> non-green (wallet-port env + test-only bodyless). Archive serving —
> **default-off, opt-in**. Ordinary live G-09 остаётся **NOT VERIFIED/BLOCKED**:
> Pong snapshot fix и 7C/7B live-проверки выполняются; fixed-H PASS ниже —
> историческое доказательство, не готовность обычного бинарника.
>
> Разделы ниже — хронология review/реализации/прогонов. Начальные фразы
> «review candidate / no code implemented» относятся к раннему снимку и не
> описывают текущее состояние.

Status of the initial review snapshot (historical): **review candidate**. No code
from that draft was implemented at review time. Line 1, 2026-11-03.

- Repository: `C:\DHW\Gplus\src\grinplusplus`
- Code integration commit: `0031267`; docs commit `cb51030`.
- Initial review HEAD: `e0a5d2d68b990234e7bf1fb1b651f868dc30e9b6`.
- At initial review time, auditor-owned uncommitted files existed; they were
  preserved and later included in the integration commit, not overwritten.
- Rust reference on disk: `C:\DHW\Gplus\reference\grin-5.5.2` — **source snapshot,
  no `.git` of its own** (`Test-Path ...\.git` = False). `Cargo.toml` says
  `name = "grin"`, `version = "5.5.2"`; deterministic file manifest (391 files,
  sorted `relpath<TAB>sha256<TAB>size`, SHA-256 of the UTF-8 manifest, excluding
  `.git/` and `target/`) is
  `8176D489733D3BC2BEA303A87A65D6718D72D328E2938820FB193071240DE998`
  (manifest size 38 862 bytes). The task names `dbaff34` as the authoritative
  Grin commit; that SHA **cannot be verified from this snapshot** — a `git`
  command there resolves the parent Flint repository (`092859b…`), which is not
  Grin. Git-SHA provenance: **NOT VERIFIED**; use the manifest hash + Cargo
  version as the reference identity in this document. No git SHA of the Rust
  reference is used anywhere below.
- This increment performed static review only. No `tools/build/build.ps1`, no
  `Tests.exe`, no live node run (shared build directory is owned by other lines).

Bottom line:

1. The ordinary repo node **can create and serve a TxHashSet archive over the legacy
   `TxHashSetRequest` path, but it cannot serve PIBD segments from that archive**.
   The PIBD serving path only answers requests whose hash equals the currently open
   flushed state. PIBD bits are deliberately off (`807b5b1`). The G-09 live PASS used
   a different, experimental binary; it is **not** evidence for the ordinary node.
2. `outputs\G09-DRAFT-archive-serving.patch` is directionally correct but **not safe
   as-is**: no capability gating, accepts arbitrary canonical horizons instead of the
   exact archive hash Rust serves, no archive size/format precheck, staging in the OS
   temp directory, non-atomic publication of the archive itself, and async lifetime
   problems (§5).
3. The `Output Commitment not found` report is most likely
   `CoinView::GetOutputType` throwing during **V3 input deserialization**
   (`TransactionInput::Deserialize`), i.e. before the block reaches the apply stage,
   whenever block N+1 is deserialized before block N has been applied. This is a
   static root-cause candidate, **not yet reproduced** (§7).

---

## 1. Verified current behaviour of the ordinary repo node

All of this is committed HEAD state, verified by reading the tree.

### 1.1 Archive creation (exists, but only at PIBD activation)

- `StateSyncer::TryPibd` (client) computes the horizon as
  `(header_head - STATE_SYNC_THRESHOLD)` rounded down to
  `TXHASHSET_ARCHIVE_INTERVAL` (`src/P2P/Sync/StateSyncer.cpp:199-206`), using
  `EChainType::CANDIDATE` (header head).
- After a successful activation it calls `ArchivePibdState(horizonHash)`
  (`StateSyncer.cpp:295-297`), which:
  - calls `m_pBlockChain->SnapshotTxHashSet(pHeader)` (`StateSyncer.cpp:371`),
  - copies the returned zip to `TXHASHSET/archive/TxHashSet.<height>.<shorthash>.zip`
    (`StateSyncer.cpp:363-380`, naming in `include/P2P/TxHashSetArchive.h:11-27`).
- `TryRecoverArchive` on start recreates the archive if a `pibd_state.complete`
  marker exists and the archive is missing (`StateSyncer.cpp:316-353`).
- `BlockChain::SnapshotTxHashSet` (`src/BlockChain/BlockChainImpl.cpp:454-466`)
  takes a read-only ChainState batch, rejects a header older than
  `GetHorizonHeight(CONFIRMED height)` (= one week), then calls
  `TxHashSetManager::SaveSnapshot`.
- `TxHashSetManager::SaveSnapshot` (`src/PMMR/TxHashSetManager.cpp:98-193`):
  copies the **live** txhashset directory to `%TEMP%/Snapshots/<shorthash>`
  (line 118), re-loads it, `TxHashSet::Rewind(pBlockDB, *pHeader)` (line 135),
  commits, materialises the legacy roaring `pmmr_leaf.bin`, renames leaf files,
  and creates the zip in `%TEMP%/Snapshots/TxHashSet.<shorthash>.zip` (line 184).
  `Rewind` needs block bodies and spent positions for every block between the
  live header and H (`src/PMMR/TxHashSetImpl.cpp:560-609`).
- There is **no periodic re-archive and no retention/pruning**. The archive is
  created only on a PIBD activation (or its recovery). A node that never runs a
  PIBD client never writes one, and a node that synced forward keeps the single
  old file forever.

### 1.2 Legacy archive serving (exists)

- `TxHashSetPipe::Thread_SendTxHashSet` (`src/P2P/Pipeline/TxHashSetPipe.cpp:198-300`)
  serves `TXHASHSET/archive/...` when present (lines 211-221); otherwise it calls
  `SnapshotTxHashSet` on demand (line 225) and removes the temporary zip.
- Peer abuse window: one request per peer per two hours (`TxHashSetPipe.cpp:176-183`).

### 1.3 PIBD segment serving (only the open flushed state)

- `MessageProcessor::ProcessMessageInternal` handles `GetOutputBitmapSegment`,
  `GetOutputSegment`, `GetRangeProofSegment`, `GetKernelSegment`
  (`src/P2P/MessageProcessor.cpp:211-330`).
- It takes `GetApiStateSnapshot()` (read lease over the chain mutation lock),
  then requires `pFlushedHeader->GetHash() == requested blockHash`
  (`MessageProcessor.cpp:259-267`). Any other hash is ignored: no response, no
  ban. This is the blocker for an archive horizon.
- Request bounds: `identifier.GetHeight() > 13` rejected (`:229`); per-connection
  rate limit 500/min (`:234`, `src/P2P/Connection.cpp:75-90`,
  `include/P2P/Common.h:28`); up to 3 retries if the state sequence changed
  (`:243-315`). Response is serialised under the lease, lease released before
  `SendAsync` (`:302-315`).
- `PibdSegmentService` derives the UTXO bitmap from the output PMMR leaf set
  (`src/P2P/PIBD/PibdSegmentService.h:83-113`) and caches it in a process-wide
  static keyed by `horizonHash + MMR size + output root` (`:243-255`). The bitmap
  root is rebuilt from 1024-bit chunks (`:116-132`). Bitmap build walks every
  leaf (10.2M at the live horizon) per call.

### 1.4 Capabilities (statically off)

- `Capabilities::ADVERTISED = HEADER_HIST | FAST_SYNC_NODE`; PIBD bits are
  commented off (`include/P2P/Capabilities.h:49-54`, commit `807b5b1`).
- Handshake sends the compile-time constant in both directions
  (`src/P2P/Seed/HandShake.cpp:110,128`). `Connection` constructs `HandShake`
  with only `ConnectionManager` and `SyncStatus`
  (`src/P2P/Connection.cpp:104`), so the handshake has no serving-readiness
  input at all.
- Regression test pins the current behaviour:
  `tests/src/Net/Test_P2PCompat.cpp:31-37`.

### 1.5 What the G-09 live PASS actually proved

- `docs/modernization/G09_LIVE_PASS_2026-11-03.md` and
  `.../outputs/G09-acceptance-2026-11-03.md`: the passing C++ binary was
  `8EA4C77A...` (experimental archive cache), not the project GrinNode
  `92A8B53B...`; headers were pre-seeded; PIBD was served at a **fixed**
  H=3 315 600; automatic later horizons, disk quota/recovery, reorg,
  restart-during-serving and negative proofs remain open
  (`G09_LIVE_PASS_2026-11-03.md:58-76`).
- Therefore: **G-09 for the ordinary repo binary is NOT VERIFIED**. Nothing in
  this document changes that.

---

## 2. Rust 5.5.2 reference semantics

### 2.1 Horizon selection

Two functions in `chain/src/chain.rs`:

| function | head used | used by |
|---|---|---|
| `txhashset_archive_header()` (`chain.rs:985-999`) | **body head** (`self.head()`) | server-side segmenter, legacy TxHashSet serving |
| `txhashset_archive_header_header_only()` (`chain.rs:1002-1009`) | **header head** (`self.header_head()`) | PIBD client (state sync, desegmenter) |

Both do, in this order:

```
height = head.height.saturating_sub(state_sync_threshold())
height = height.saturating_sub(height % txhashset_archive_interval())
header = get_header_by_height(height)
```

Constants (`core/src/consensus.rs:99`, `core/src/global.rs:74,106,109`):

| chain type | threshold | interval | product |
|---|---|---|---|
| Mainnet, **Testnet** (`_ =>`) | `STATE_SYNC_THRESHOLD = 2*DAY_HEIGHT = 2880` | `TXHASHSET_ARCHIVE_INTERVAL = 720` | C++ `Consensus.h:166,169` matches |
| AutomatedTesting, UserTesting | `TESTING_STATE_SYNC_THRESHOLD = 20` | `TESTING_TXHASHSET_ARCHIVE_INTERVAL = 10` | C++ has no test-chain variant |

The live Rust client config was `chain_type = "Testnet"`
(`...\work\g09-rust-client-real\grin.toml`), so it used 2880/720; for the live tip
3 318 655 this gives (3318655-2880=3315775; 3315775%720=175) → **H=3 315 600**,
exactly the fixed H of the live run.

Consequence for C++: the hardcoded 2880/720 is correct for Floonet/Testnet and
Mainnet, but **not** for Rust `AutomatedTesting`/`UserTesting` peers. The contract
must either declare those chain types out of scope for PIBD serving, or make
`Consensus` horizon constants environment-aware.

### 2.2 Server-side segment serving rules

`servers/src/common/adapters.rs:493-545`:

1. Validate the requested segment height against a **per-tree** range
   (`adapters.rs:50-53`):
   - bitmap 9..14 (heights 9-13), kernel 9..14,
   - output 11..16 (11-15), rangeproof 7..12 (7-11).
2. Call `chain().segmenter()` and require
   `segmenter.header().hash() == requested_hash`, else
   `Error::SegmenterHeaderMismatch`. **Only the exact current archive hash is
   served — not a height range and not an arbitrary older archive.**
3. `Chain::segmenter()` (`chain.rs:892-921`) caches one segmenter per archive
   period; `init_segmenter` does a read-only rewind of the live txhashset to the
   archive header and snapshots the bitmap accumulator. The comment states the
   cost: "rewinding the txhashset by approx 720 blocks (12 hours)" once per
   archive period.
4. `Segmenter` (`chain/src/txhashset/segmenter.rs`) generates all four segment
   types from `txhashset.*_pmmr_at(&self.header)` and the cached bitmap
   snapshot; the served `output_root`/`bitmap_root` come from that snapshot.

The legacy `TxHashSetRequest` path (`p2p/src/protocol.rs:227-250`) serves
whatever the node's current `txhashset_archive_header()` is; the requesting
client validates the received archive against its own header.

### 2.3 Capabilities

Rust `Capabilities::default()` advertises `HEADER_HIST | TXHASHSET_HIST |
PEER_LIST | TX_KERNEL_HASH | PIBD_HIST | PIBD_HIST_1`
(`p2p/src/types.rs:405-411`). A stock Rust node therefore advertises PIBD by
default. C++ deliberately does not, until the serving path is real.

### 2.4 Client selection and fallback

`servers/src/grin/sync/state_sync.rs`:

- PIBD peer filter: connected, max difficulty (`>= max_diff`), capability
  `PIBD_HIST_1`, height within `PIBD_PEER_HEIGHT_SLACK_BLOCKS = 2` of the best
  PIBD peer (`state_sync.rs:395-420`).
- If no suitable PIBD peer exists for
  `TXHASHSET_ZIP_FALLBACK_TIME_SECS = 660` (`chain/src/pibd_params.rs`), PIBD is
  aborted and the legacy txhashset.zip path is used (same file, ~:430-450).
- Segment timeout is 20 s; the syncer re-requests segments and retries other
  peers. A server that advertises PIBD but silently refuses the hash the client
  picked causes failed/repeated PIBD attempts, not data corruption, and the
  client eventually falls back when no usable PIBD peer remains.

### 2.5 Provenance of the reference tree

`C:\DHW\Gplus\reference\grin-5.5.2` is a plain source snapshot: no own `.git`,
`Cargo.toml` `name = "grin"`, `version = "5.5.2"`. Deterministic manifest of the
tree (391 files, sorted `relpath<TAB>sha256<TAB>size`, SHA-256 over the UTF-8
manifest, `.git/` and `target/` excluded):

```
8176D489733D3BC2BEA303A87A65D6718D72D328E2938820FB193071240DE998
(manifest: 38 862 bytes, 391 files)
```

`C:\DHW\.tmp\grin55` and the vendored
`C:\DHW\Gplus\.toolchain\reference\grin_chain-5.5.2` appear to be the same
sources (spot-checked), but each must carry its own manifest if it is used as
evidence. The task names `dbaff34` as the authoritative Grin commit; it cannot
be verified from the snapshot (git there resolves the parent Flint repository).
Git-SHA provenance is **NOT VERIFIED**. The quotes below are from this snapshot,
identified by the manifest hash, not by a git SHA.

---

## 3. Contract v0 (normative proposal)

Naming: "serving node" = C++ GrinNode advertising PIBD; "snapshot" = one immutable
`TxHashSet` at horizon H.

### 3.1 Definitions

- `H_server(tip) = round_down(tip.CONFIRMED - 2880, 720)`, saturating at 0, with
  `STATE_SYNC_THRESHOLD`/`TXHASHSET_ARCHIVE_INTERVAL` from `Consensus.h`.
  Only defined for Testnet/Mainnet-compatible chains (see §2.1).
- `H_client(header_head) = round_down(header_head - 2880, 720)`; this is what a
  Rust peer asks for and what `StateSyncer::TryPibd` already implements.
- Snapshot identity = the canonical `BlockHeader` at H (hash, height, version,
  `num_outputs`, `num_kernels`, `output_mmr_size`, `kernel_mmr_size`,
  `output_root`, `range_proof_root`, `kernel_root`). **A file name, a height or a
  zip path is never an identity.**

### 3.2 Validation before publish (all mandatory, fail-closed)

1. Header H is on the confirmed chain: `GetBlockHeaderByHeight(H, CONFIRMED)`
   exists and its hash equals the snapshot key; H is a multiple of 720.
2. Zip: opens; every expected entry exists with expected names; per-entry
   decompression and close/CRC checks pass (the uncommitted auditor
   `ZipFile.cpp` already adds read-error, short-write and
   `unzCloseCurrentFile` checks — this document depends on that hardening);
   no unexpected entry is extracted; total uncompressed size and per-entry size
   are within the configured archive-size budget (zip-bomb guard).
3. Extracted state loads: kernel/output/rangeproof PMMR load succeeds;
   `outputs->GetSize() == header.output_mmr_size`,
   `proofs->GetSize() == header.output_mmr_size`,
   `kernels->GetSize() == header.kernel_mmr_size`.
4. `TxHashSet::ValidateRoots(header)` is true. This already checks the merged
   output root `HashParentWithIndex(output_root, UBMT, output_mmr_size)` for
   header v3+ (`src/PMMR/TxHashSetImpl.cpp:219-247`), i.e. the bitmap commitment
   is covered.
5. The loaded archive is **never** rewound, committed, compacted or reopened for
   write while serving. Only the prepare step may write (the leaf-set
   materialisation inside `OutputPMMR::Load` writes into the staging copy).
6. Only after 1-5 does the snapshot become `ready`; the archive file itself is
   already final-named only after validation (§3.8).

Errors at any step publish nothing, delete staging, keep the previous ready
snapshot, and set a bounded retry delay (monotonic clock).

### 3.3 Snapshot lifecycle

```
absent ──need(H)──> preparing ──extracted──> verifying ──ok──> ready
   ^                   │                        │               │
   │                   └──── error ─────────────┴──> failed ─────┘ (retry later)
   │                                                          
   └────────────── retiring (last lease released) <────────── ready(H') 
```

- **absent**: no validated archive for current H. Capability bits off.
- **preparing**: exactly one build for one key globally (single-flight). A
  second request for the same H attaches to the same future; requests for
  another H are refused while one is preparing (or queued as at most one
  "next" — a retention decision, not required).
- **verifying**: extraction/checks/roots; no reader can obtain a lease.
- **ready**: immutable snapshot; readers take leases by exact hash.
- **retiring**: removed from the hash map (new readers cannot acquire it), file
  deletion deferred until the lease count reaches zero.
- **failed**: recorded with monotonic retry-after; previous ready snapshot (if
  any) keeps serving until the new one is ready.

H=0 (genesis) is not servable; saturating horizon below the first archive
interval is `absent`.

### 3.4 Lease and locking rules

- The lease is `std::shared_ptr<const PibdArchiveSnapshot>` owning the three
  PMMRs and the staging directory. Acquirement is a short critical section
  under the snapshot-registry mutex; **no chain lock and no filesystem I/O is
  held while a segment is generated or sent.**
- Serving an archive does **not** take `IBlockChain::GetApiStateSnapshot()`; the
  archive is immutable and self-authenticating (roots in the header the client
  already consolidated). `GetApiStateSnapshot()` remains for the live flushed
  state path only.
- Serialisation order: acquire lease by requested hash → build response bytes →
  `SendAsync` → release. The response owns its bytes; a lease must survive
  until the last byte is serialised, not until the socket write completes.
- Deletion ordering: retire under registry mutex; erase last reference; the
  `shared_ptr` destructor is the only place that removes the directory; removal
  failures are logged and retried at next startup (Windows mmap can keep files
  locked briefly).
- Reorg: H is a historical immutable state. Canonical-chain membership of H is
  checked at **prepare** time (and defensively at acquire). A reorg that removes
  H cannot corrupt a lease; the snapshot is refused to new readers and the next
  H is prepared. Serving a still-valid old H after a >2880-block reorg is not
  supported (out of scope, matches Rust's horizon assumption).

### 3.5 Horizon transition, retention

- The current horizon is recomputed from the confirmed tip. It changes by +720
  when `tip >= H_old + 3600` (`H_new = H_old + 720`; the ±720 rounding window is
  exactly one archive interval).
- Preparation for `H_new` may start as soon as `H_new` is defined by the tip
  (`tip - 2880 >= H_new`), i.e. exactly when the old archive stops matching
  `H_server`. Prepare asynchronously; until `ready`, PIBD bits are off and all
  segment requests for `H_new` get the explicit refusal (§3.9).
- Retention: current ready snapshot + at most one retiring snapshot. Older
  archive files are removed only at startup or during a successful prepare, and
  only when unprotected by a lease.
- Old-H clients (`H = H_old` after transition) receive **the explicit refusal**,
  never data of another H. They may retry, pick another peer, or fall back to
  txhashset.zip. No ban (local incompleteness must not be fraud).
- Do not promise arbitrary historical rewind: preparation beyond the current
  horizon may need live blocks and spent positions. If `Rewind` cannot obtain
  them, the prepare fails closed and the node serves nothing at that H.

### 3.6 Capabilities

- Advertise `PIBD_HIST | PIBD_HIST_1` **iff** serving is enabled in config AND a
  `ready` snapshot exists AND `ready.header.hash == H_server(current confirmed
  tip)` at handshake time.
- Never advertise based on the existence of a zip file or on `preparing` state.
- Because the handshake value cannot change mid-connection: after a horizon
  rotation, peers on old connections held the old capability; the node must
  prepare the new snapshot promptly and refuse mismatching hashes; new
  connections tell the truth. This can cost a client one failed PIBD attempt and
  a fallback; that is safe and documented behaviour.
- The serving horizon is derived from the **CONFIRMED (body) head**, mirroring
  Rust `txhashset_archive_header()`. Consequence: immediately after a PIBD
  activation, or while a restart is catching headers up, `f(body_tip)` can be
  older than the horizon a client derives from its header head. The node then
  advertises no PIBD capability until body sync reaches the matching window; it
  never claims or serves a mismatched H. The accepted live flow (PIBD → body
  sync to the same tip) ends exactly in the matching window, so this costs
  availability only during catch-up, not correctness.
- `Capabilities::ADVERTISED` stays the compile-time safe default (PIBD off).
  The handshake must use a runtime value from the serving component; the
  concrete wiring is a Lead decision (see §5, F1).
- No advertising of `BLOCK_HIST` in this increment.

### 3.7 Resource limits (must be configurable, observable and enforced)

| resource | limit | enforcement |
|---|---|---|
| concurrent preparations | 1 global, one key | single-flight state machine; second H refused/deferred |
| ready snapshots loaded | 1 + 1 retiring | registry cap |
| archive on disk | `archive_cache_max_bytes` (default: 2 generations) | prepare refuses if `free + would_reclaim < 2.5 * zip_extracted_estimate`; quota counted before extraction |
| zip extracted size | per-entry and total cap derived from header MMR sizes (`DataFile` sizes + 10 % + bounded hash files) | checked from zip metadata **before** extraction; reject |
| free disk | `>= zip_size + extracted_size + safety` before extract | abort prepare, keep previous ready |
| RSS | one loaded archive; PMMRs are mmap (`AppendOnlyFile` → `IMappedFile`) | cap = 1 ready archive; do not pre-copy the whole live state while another archive is loaded |
| segment generation CPU | bitmap built once per snapshot identity and cached; per-request work bounded by per-type segment height ranges | cache key = identity; no global lock during generation |
| requests | per-connection 500/min (existing) **plus** global `MAX_PIBD_REQUESTS_PER_MINUTE_GLOBAL` and per-connection one in-flight response (synchronous) | monotonic clock, token buckets; excess dropped silently, liveness of block/ping paths unaffected |
| serving threads | bounded executor (default 4) for generation; requests beyond it are dropped, not queued unboundedly | queue length counter |
| staging dirs | `TXHASHSET/archive/.staging-<hash>` only; never `%TEMP%` | startup cleanup; crash leaves are removed |

Timekeeping: all internal windows use `std::chrono::steady_clock`; system clock
jumps must not open retry storms or disable the limiter.

### 3.8 Crash safety and recovery

- Prepare writes to `TXHASHSET/archive/.staging-<hash>/` on the same volume as
  the final archive. The final zip is written as
  `TxHashSet.<height>.<shorthash>.zip.partial`, validated (extract + roots),
  then atomically renamed to the final name. A crash at any point leaves either
  the old final name or a partial file — never a partial file under the final
  name.
- Startup: delete `.staging-*` directories and `*.partial` files; validate any
  final-named archive lazily on first use; do not trust file presence.
- The existing `pibd_state.complete` recovery stays as is (it recreates the
  archive from the open state at H immediately after a PIBD activation, when the
  flush is at H).
- Do **not** copy the live txhashset directory while blocks may be applying
  without holding a consistent lease: `SnapshotTxHashSet` currently takes only a
  ChainState batch (`BlockChainImpl.cpp:458`). A periodic prepare must either
  take the chain mutation guard (`StateWriteGuard`, which blocks `AddBlock`)
  for the copy+rewind, or be built from a verified copy that can be proven
  frozen. Record the actual stall duration.

### 3.9 Explicit refusal and fallback

- Requested block hash != ready snapshot hash, or no ready snapshot: **no
  response** (silently drop, log rate-limited). Never send a segment of another
  H, never ban the requester, never expose an internal error as a protocol
  message.
- No archive at all: PIBD bits off; legacy txhashset.zip serving stays
  functional (`TxHashSetPipe.cpp:211-221`); Rust peers use the legacy path
  (after 660 s without a suitable PIBD peer, or immediately if no peer
  advertises PIBD).
- A client that got the capability from an older handshake but asks for a
  horizon the node no longer has must be allowed to fail that attempt and
  retry/fall back; no ban.

### 3.10 Explicit non-guarantees

- No arbitrary historical H beyond the deterministic current horizon.
- No PIBD serving on Rust `AutomatedTesting`/`UserTesting` chain types unless
  their 20/10 constants are implemented and tested.
- No `BLOCK_HIST`/archival-block serving.
- No mainnet acceptance in this line; no guarantee for user wallets/secrets.
- G-09 stays NOT VERIFIED until §6/§7 scenarios run on the ordinary repo binary.

---

## 4. Gap analysis: ordinary repo node vs contract

| # | Contract item | Current state | Verdict |
|---|---|---|---|
| G1 | Deterministic server horizon `round_down(CONFIRMED-2880,720)` | no such function exists; only the client-side CANDIDATE formula in `StateSyncer.cpp:199-206` | **missing** |
| G2 | Snapshot creation independent of a PIBD activation | only after activation / crash marker | **missing** (periodic) |
| G3 | Snapshot identity + roots/sizes verification before publish | `ValidateRoots` exists and covers UBMT; no archive-level extract+verify path | **partial** |
| G4 | Immutable lease, deletion after last reader | live path has chain-lock lease (`BlockChain::GetApiStateSnapshot`); archive has none | **missing** |
| G5 | PIBD serving from archive H | `MessageProcessor.cpp:259-267` requires flushed-state hash | **missing** (main blocker) |
| G6 | Truthful capability tied to ready horizon | compile-time constant (`HandShake.cpp:110,128`) | **missing** |
| G7 | Single-flight preparation | none for archives; `SaveSnapshot` may run in the legacy path concurrently | **missing** |
| G8 | Disk/RSS/global request limits, staging quota | none for archives; request limit is per-connection only (`Connection.cpp:75-90`) | **missing** |
| G9 | Crash-safe archive publication | `fs::copy_file` directly to final name (`StateSyncer.cpp:378`), staging in `%TEMP%` (`TxHashSetManager.cpp:105-109`) | **missing** |
| G10 | Old-horizon refusal / retention | old files never pruned; arbitrary hash accepted by legacy serving | **partial/missing** |
| G11 | Per-type segment height ranges | blanket `>13` (`MessageProcessor.cpp:229`) vs Rust per-type ranges (9..14/11..16/7..12) | **mismatch** |
| G12 | Chain-type-aware constants | fixed 2880/720 (`Consensus.h:166,169`); Rust 20/10 for Automated/UserTesting | **documented gap** |
| G13 | Negative/concurrent tests | serving tests exist (`Test_PibdServing.cpp`); no archive lifecycle/lease/limits tests | **missing** |
| G14 | Ordinary-binary live acceptance | not run; experimental fixed-H binary only | **NOT VERIFIED** |
| G15 | Auditor ZIP hardening | uncommitted `ZipFile.cpp` adds checks; not yet integrated/verified by this line | **dependency** |

---

## 5. Review of `outputs\G09-DRAFT-archive-serving.patch`

Source: `C:\Users\nurli\Documents\Codex\2026-11-02\c-grin-1-2-9-c\outputs\G09-DRAFT-archive-serving.patch`
(204 lines; adds `src/P2P/PIBD/PibdArchiveCache.h`, warms it from
`MessageProcessor`, and falls back to it in the served-hash branch). It is
review material only; **do not apply as-is**.

Direction is right: load a persisted archive zip, check sizes and roots, serve
segments from it. Concrete findings:

- **F1. No capability change.** The patch leaves `Capabilities::ADVERTISED`
  untouched, so with `807b5b1` the new path is unreachable; flipping the bits
  without a readiness check would advertise an unvalidated/unavailable server.
  Required: handshake-time runtime capability from the serving component
  (§3.6), and the wiring must reach `HandShake`, which today sees only
  `ConnectionManager`/`SyncStatus` (`Connection.cpp:104`).
- **F2. Accepts a range, not the exact hash.** `.. > 10080` and "canonical
  header at requested height" (`patch lines 84-97`) let a peer force loading of
  any historical canonical archive. Rust serves exactly
  `segmenter.header().hash == requested` (`adapters.rs:493-545`). Serving
  another valid-hash state is not a corruption, but it turns every peer into a
  multi-GB load trigger and can thrash the single-slot cache. Required: exact
  `ready.header.hash == request.hash` only.
- **F3. `WarmPibdArchive()` on the hot path.** Called from the constructor and
  from `GetBlockHeight() const` (patch lines 42-62); `GetBlockHeight()` is
  called from the connection thread per `Ping` handling path
  (`Connection.cpp:260-262`). Every call hits the cache mutex and possibly a
  `stat`. Required: explicit prepare trigger (startup + tip/horizon change), no
  filesystem work in message-processing helpers.
- **F4. Single-flight poll-only model.** A load started in `Get()` is harvested
  only by a *later* `Get()` call (patch lines 146-164). If no second call comes,
  the result is never published; on shutdown a pending `std::async` future is
  joined implicitly. Required: explicit future with cancellation/lifetime tied to
  the serving component, and a completion path that publishes without requiring
  another request.
- **F5. Confused retry/hold windows.** `m_retryAfter` is set both after a
  successful load (`now + 1 min`) and after a missing file (15 s), and
  `m_holdUntil` (10 min) blocks serving another H (patch lines 150-158). This is
  not a state machine and has no measured rationale. Required: `absent →
  preparing → verifying → ready → retiring` with per-transition deadlines on
  `steady_clock`.
- **F6. Staging in the OS temp directory with random names**
  (patch lines 171-179): different volume from the final archive, no
  same-volume atomic rename, no disk-space/size checks, and random dirs leak on
  every crash (no startup cleanup). Required: `TXHASHSET/archive/.staging-<key>`,
  partial suffix, atomic rename, startup cleanup (§3.8).
- **F7. No zip precheck.** `Load()` extracts first and checks sizes/roots only
  after `TxHashSetZip::Extract` (patch lines 180-195). A corrupted or oversized
  zip is fully extracted to disk before being rejected. Required: entry set,
  per-entry/total size budget and CRC before/while extraction (§3.2).
- **F8. No resource limits.** Single current snapshot is implicit, but nothing
  bounds disk use, free space, loaded archives, global request rate or
  generation concurrency. Required: §3.7.
- **F9. Async lifetime.** `PibdArchiveCache` is a `shared_ptr` member of
  `MessageProcessor`; the async lambda captures `header` and a path, but the
  published `State` destructor removes files from whatever thread releases the
  last reference (a connection thread), while the loader may still run after
  the object dies. Required: explicit owner with shutdown join and lease-count
  deletion.
- **F10. No canonical/H check at prepare.** It loads whatever archive file
  matches the requested header without proving that header is the current
  derived horizon of the confirmed tip. Required: compute H from the confirmed
  tip at prepare time and validate the header chain link.
- **F11. `case TxHashSetArchive:` qualification — confirmed necessary by the
  integrated build.** The DRAFT qualified it (patch lines 104-105). Before the
  serving header existed it was unambiguous (only the `MessageTypes` enumerator
  was visible). `PibdArchiveServer.h` transitively includes
  `<P2P/TxHashSetArchive.h>`, whose namespace `TxHashSetArchive` then collides
  with the unqualified enumerator in `MessageProcessor.cpp` (which has
  `using namespace MessageTypes;`). The Lead's integrated build hit exactly
  this ambiguity; the fix is `case MessageTypes::TxHashSetArchive:`. Recorded as
  a compile-level lesson: a header that pulls `<P2P/TxHashSetArchive.h>` into a
  TU with `using namespace MessageTypes;` changes unqualified name lookup.
- **F12. Header version/UBMT.** `ValidateRoots` covers the merged root for v3+
  (`TxHashSetImpl.cpp:226-239`); keep it, but the archive must be rejected if
  the header version is below 3 *and* the stored state has an UBMT-bearing
  layout mismatch. State the version check explicitly in the contract.

Verdict: use the patch as the skeleton (load + size + root checks are the right
shape), but implement it as a component with the §3 state machine, exact-hash
routing, disk/CPU limits and startup cleanup; capability gating is a separate,
mandatory step.

---

## 6. Negative/concurrent test plan (fast, deterministic)

None of these are implemented in this increment. Recommended split: pure
unit tests in `tests/src/PMMR/Test_PibdServing.cpp` (already registered, no
CMake change needed), archive-lifecycle tests in a new
`tests/src/PMMR/Test_PibdArchive.cpp` (**requires the CMakeLists owner** —
currently uncommitted auditor change). Use isolated `TMP/TEMP/HOMEDRIVE/HOMEPATH`
per the line rules.

Segment-level (no node, no network):

- **T1 mutated proof**: serialise a valid range-proof segment, flip one byte in
  a proof, feed to `PibdSession::AcceptResponse`; must be rejected, no commit;
  server-side generation must not crash on the same input.
- **T2 wrong root**: bitmap segment with correct output root but wrong bitmap
  root/expected root; output segment with wrong bitmap root; kernel segment
  under a different horizon hash; all rejected, nothing published under the old
  hash.
- **T3 segment identity/bounds**: per-tree height ranges from §2.2
  (bitmap/kernel 8,9,13,14; output 10,11,15,16; rangeproof 6,7,11,12), huge
  `idx`, `idx` beyond the MMR, empty tree; controlled exception/empty response,
  process stays alive, no out-of-bounds read.
- **T4 mutated leaf data / missing sibling**: fully pruned segment with
  `MissingLeaf`; no substitution of a foreign node, no hang, no crash; verify
  the same request through `MessageProcessor` returns no response and does not
  ban.
- **T5 bitmap cache isolation**: serve H1, then a second `PibdSegmentService`
  with same size/root but different horizon hash; cached bitmap must not leak
  (identity includes hash; assert).
- **T6 concurrent segment generation**: 8 threads × 4 segment types against one
  service; one bitmap build (instrument counter), identical roots every time,
  no data race under ASan/TSan where available.

Archive lifecycle (unit, real zip files built by the test):

- **T7 corrupt zip**: truncated, wrong CRC, missing `output/pmmr_hash.bin`,
  extra entries, bad internal name; `Extract`/prepare returns failure; staging
  deleted; previous ready snapshot still served.
- **T8 root/size mismatch**: header with altered `output_mmr_size`,
  `kernel_mmr_size`, `output_root`, `kernel_root`, `num_outputs`; every case
  rejected before `ready`.
- **T9 disk-full**: prepare with insufficient free space (fake quota or a small
  volume/directory); abort before extraction or during write; no `.partial`
  final name; old ready intact; retry after a monotonic delay.
- **T10 crash before rename / after rename / before publish**: kill between
  stages; restart; `.staging-*`/`.partial` cleaned; no partially-validated
  archive is ever served; only a fully validated one becomes ready.
- **T11 single-flight**: 16 concurrent acquire requests for the same missing H;
  exactly one build, all get the same result; 2 different H requests while one
  prepares: one refusal, no parallel builds.
- **T12 lease/retire**: hold a lease for H, switch horizon, retire H; old
  reader still gets valid H bytes; directory is removed only after the last
  lease is released; no removal failure on Windows due to open mmap.
- **T13 old/new horizon during send**: barrier inside response serialisation;
  rotate H; verify the in-flight response is entirely old-H and a new request
  gets new-H or an explicit refusal — never a mixture.
- **T14 capability truthfulness**: no archive → bits off; preparing → off;
  ready+current → on; ready but stale horizon → off; failed → off. Reuse the
  existing assertions style of `tests/src/Net/Test_P2PCompat.cpp`.
- **T15 clock jump**: simulated system-clock jump does not disable or extend
  retry/rate limits (all internal windows on `steady_clock`).
- **T16 multi-client/global limits**: 8 connections × 64 segment requests;
  per-connection and global limits enforced; P2P ping/block handling latency
  stays bounded; no unbounded queue growth; no bans.

Wire/live (ordinary repo binary; needed for any G-09 claim):

- **T17** ordinary Rust 5.5.2 Testnet client with headers from zero, empty
  body/state, only C++ peer; record header sync → H → hash; prove C++ was
  selected (`PIBD_HIST_1` in handshake, max difficulty) → PIBD → roots → body
  sync → `no_sync` at the same tip.
- **T18** restart during download/staging/activation; peer drop/resume;
  controlled reorg; horizon change; invalid proof/root/hash injections; legacy
  fallback. Record p50/p95/p99, CPU, peak RSS, disk, lock stalls, and the exact
  binary SHA-256 for each run.
- **T19** negative run with the archive path disabled must fall back to
  txhashset.zip without a ban and without a corrupt state.

---

## 7. `Output Commitment not found` — diagnosis

### 7.1 What the string is

The literal string `Output Commitment not found` does not exist in the C++ tree
or in the saved logs of the G-09 runs (searched `C:\DHW\.tmp` and the Codex work
tree). It appears only in the auditor prose (`AUDIT_RESPONSE_2026-11-02.md:48,
108`). The nearest real code path is:

- `CoinView::GetOutputType` → `BLOCK_CHAIN_EXCEPTION_F("Output {} not found.",
  commitment)` (`src/BlockChain/CoinView.cpp:5-18`), reached from
  `TransactionInput::Deserialize` for protocol >= V3
  (`src/Core/Models/TransactionInput.cpp:26-31`). The exception is caught in
  `MessageProcessor::ProcessMessage` and logged as
  `BlockChain exception while processing <msg> from ...: Output <commit> not
  found.` (`src/P2P/MessageProcessor.cpp:113-121`).
- Secondary candidate: `TxHashSet::GetRoots` throws
  `Output position not found for input {}` (`TxHashSetImpl.cpp:255-263`) during
  apply/validation.

### 7.2 Mechanism (static)

The V3 wire format carries only input commitments; C++ reconstructs the input
"features" eagerly **during deserialization**:

```
BlockMessage::Deserialize -> Transaction::Deserialize
  -> TransactionInput::Deserialize (protocol >= V3)
       -> Global::GetCoinView()->GetOutputType(commitment)
            -> BlockDB->GetOutputPosition(commitment)   // must already exist
```

But positions for a block's *new* outputs are added only when that block is
applied (`TxHashSetImpl.cpp:200-203`, `ApplyBlock`), and inputs are looked up in
the live BlockDB. Block download is batched: `REQUEST_WINDOW = 15`,
`MAX_BATCH = 100`, `BATCH_PER_PEER = 10`
(`src/P2P/Sync/BlockSyncPolicy.h:28-30`), requests go out for many heights
(`BlockSyncer.cpp:127-131`), and each received `Block` is deserialized
immediately on the connection thread (`MessageProcessor.cpp:421-444`) before
`BlockPipe` applies up to 8 of them on parallel threads
(`src/P2P/Pipeline/BlockPipe.cpp:38-56`). Therefore block `N+1`, whose input
spends an output created in block `N`, can be deserialized before `N` has been
applied and its output position committed. The deserialization then throws, the
block is dropped from processing (no ban), and sync has to re-request it. The
same ordering hazard exists after a PIBD activation at H: the state install
writes positions for unspent outputs up to H (`TxHashSetProcessor.cpp:156-161` →
`SaveOutputPositions`), but positions created by blocks H+1.. are only added by
their own `ApplyBlock`; any out-of-order deserialization of dependent blocks in
the downloaded window can fail before the pipeline ever sees the block.

Rust does not have this failure mode: V3 `Inputs::CommitOnly` carries only
commitments and their features are resolved later in the apply/validation stage
(`core/src/core/transaction.rs`, `Inputs` enum, `CommitOnly`/`FeaturesAndCommit`;
`TxInput` features are only present for V2 compatibility). C++ moved the
resolution into deserialization, creating an ordering dependency.

### 7.3 Reproduction (plan; not run — no binary build in this increment)

Minimal unit-level case (offline, no network):

1. Prepare a chain state with one unspent output `o` (test fixture or mined
   synthetic block).
2. Build block `A` (height H+1) containing output `o`; apply `A` normally.
3. Build block `B` (height H+2) with one input spending `o`.
4. Fork/reset the chain to the state before `A`.
5. Serialise `B` for protocol V3, call `BlockMessage::Deserialize` (or
   `Transaction::Deserialize`) **before** applying `A`.
6. Expected current behaviour: `BlockChainException` "Output <commit> not
   found."; `A` then `B` applying in order succeeds.

Integration confirmation: run the ordinary repo binary against a Rust Testnet
peer after a real PIBD activation, with body sync window > 1; capture
`BlockChain exception while processing Block` lines. If the log shows the
`Output <commit> not found.` text for a height whose parent height has not yet
been applied, T7 is confirmed. `docs/modernization/AUDIT_RESPONSE_2026-11-02.md`
reports the stall but does not preserve such a log; the exact string was a
paraphrase, so **status: NOT VERIFIED** until this run is repeated.

### 7.4 Fix options (ownership: Line 2 / Lead; `TransactionInput` and
`TxHashSetImpl` are not Line 1 write scopes)

- (a) Recommended: keep V3 inputs commitment-only in the model and resolve
  `features` at apply/validation time under the chain write guard (parent state
  applied by definition); add regression test from §7.3. This matches Rust.
- (b) Cheap containment: serialise "deserialize + apply" per block with a
  single-flight parent barrier for body sync (performance cost; does not fix
  other out-of-order callers).
- (c) Keep eager resolution but re-resolve/retry when the position is missing
  at deserialization; hides the ordering error and can still produce a wrong
  feature if the branch is ambiguous — not recommended.

Do not ban a peer for this local data-ordering condition
(`MessageProcessor.cpp:113-121` currently does not ban — keep it that way).

---

## 8. Bounded implementation (uncommitted candidate; compiled in the integrated tree)

This section records the Line 1 candidate patch written after the contract was
reviewed. It is deliberately **inert by default**: PIBD capability bits stay off
and the archive serving branch is never entered until the acceptance gate is
met.

### 8.1 Files changed by Line 1 (working tree, uncommitted)

| file | change |
|---|---|
| `src/P2P/PIBD/PibdArchiveServer.h` | **new**, header-only serving component: header-free `ArchiveKey`, pure `Registry` state machine (`absent/preparing/ready/failed`, exact-hash acquire, stale-publish rejection), `Snapshot` lease with deferred last-release cleanup, bounded staging under `TXHASHSET/archive/.staging-<hash>`, zip/extracted-size and free-space caps, extract→MMR-size→`ValidateRoots` via `LoadAndValidate`, startup staging cleanup, global request token bucket, runtime capability query |
| `src/P2P/PIBD/PibdParams.h` | pure helpers: `ComputeArchiveHorizonHeight(head, threshold, interval)`, `IsServableArchiveHorizon`, per-tree segment-height ranges |
| `src/P2P/MessageProcessor.cpp` | archive-first exact-hash serving path that holds **no** chain mutation lease while generating archive segments; falls back to the existing open flushed-state path; per-tree height validation replaces the blanket `>13`; global archive request limiter |
| `include/P2P/Capabilities.h` | `Capabilities::Advertised(bool pibdServingReady)`; compile-time `ADVERTISED` mask unchanged (PIBD-free) |
| `src/P2P/Seed/HandShake.cpp` | handshake capability value comes from `PibdArchiveServer::ShouldAdvertise()` instead of the static enum |
| `src/P2P/Seed/Seeder.cpp` | owns the server: `Initialize(pBlockChain, ENABLED_BY_DEFAULT)` in `Seeder::Create`, `Shutdown()` in the destructor |
| `tests/src/PMMR/Test_PibdServing.cpp` | + `PibdParams::ArchiveHorizonSelection`, `PibdParams::SegmentHeightRanges` and the six `PibdArchiveServer::*` tests (see §8.5); no new CMake registration |
| `tests/src/Net/Test_P2PCompat.cpp` | + `Capabilities::AdvertisedGating` |
| `docs/modernization/G09_ARCHIVE_SERVING.md` | this document |

No CMake file, no `TxHashSetImpl`, no `ZipFile`, no other line's files were
touched. `PibdArchiveServer.h` is header-only because
`src/P2P/CMakeLists.txt` globs only `*.cpp`, `Seed/*.cpp`, `Sync/*.cpp`,
`Pipeline/*.cpp` — a new `PIBD/*.cpp` would silently not compile.

### 8.2 Default-off switch

`PibdArchiveServer::ENABLED_BY_DEFAULT = false` (`PibdArchiveServer.h`). While
false:

- no worker thread is started (`Start()` returns immediately);
- `ShouldAdvertise()` is always false, so `Capabilities::Advertised(false)` equals
  the existing `ADVERTISED` mask and `tests/src/Net/Test_P2PCompat.cpp:31-37`
  keeps passing;
- `Acquire()` returns `nullptr`, so `MessageProcessor` falls through to the
  existing live flushed-state path exactly as before (no behaviour change);
- the only added per-request cost is one uncontended mutex/copy of the instance
  handle plus one failed `Acquire` lookup.

To enable after acceptance, flip the single constant to `true` (and calibrate
`MAX_ARCHIVE_ZIP_BYTES`, `MAX_ARCHIVE_EXTRACTED_BYTES`,
`MIN_FREE_BYTES_AFTER_EXTRACT`, `MAX_GLOBAL_PIBD_REQUESTS_PER_MINUTE` from a
measured archive). Enabling starts the worker; it will:

1. compute `H = round_down(confirmed_tip - 2880, 720)` (Testnet/Mainnet
   constants from `Consensus.h`) every 5 s;
2. look for `TXHASHSET/archive/TxHashSet.<H>.<shorthash>.zip` (the file
   `StateSyncer::ArchivePibdState` already produces);
3. load it into staging, enforce size/space caps, check all three MMR sizes and
   `ValidateRoots` (merged output/UBMT root included), re-check the horizon, then
   publish;
4. answer PIBD segment requests whose hash equals the ready snapshot hash, with
   no chain mutation lock and no wrong-H data; everything else is a silent
   refusal (no ban);
5. advertise `PIBD_HIST|PIBD_HIST_1` only while `ready == expected horizon`.

#### Isolated-test opt-in (env), no production constant change

`GRINPP_PIBD_ARCHIVE_SERVING` (process environment) is the only test opt-in:
`1 | true | on | yes` (trailing/leading whitespace trimmed, case-insensitive)
enables archive serving for that one process; `ENABLED_BY_DEFAULT` stays `false`
and remains the hard default (`enabled = ENABLED_BY_DEFAULT || envOptIn`).
On `Environment::MAINNET` the node refuses the opt-in with `LOG_ERROR` and stays
disabled. Startup logs exactly one line: `PIBD archive serving: ENABLED
(opt-in GRINPP_PIBD_ARCHIVE_SERVING)` or `... DISABLED (default)`. Capability
bits still appear only when a validated ready archive matches the horizon
(§3.6). Pure parsing is covered by `PibdArchiveServer::ServingOptInParsing`; the
env→Seeder path and mainnet refusal are exercised in live preflight (§8.8 L0).

### 8.3 Explicit limitations of this increment

- **It serves an existing archive; it does not create new horizons.** Archive
  creation still happens only after a PIBD activation (or its crash recovery).
  When the confirmed tip advances one interval, the existing archive no longer
  matches the horizon, capabilities turn off and serving refuses — honest, but
  periodic re-archiving remains the next step (§3.5, §4 G2).
- Shutdown may wait for an in-flight archive load (multi-GB extract), because
  `LoadSnapshot` has no cancellation point yet; `Shutdown()` joins the worker.
- Limits are review placeholders, not measured values (no live archive was
  measured in this increment).
- Only Testnet/Mainnet horizon constants are implemented; Rust
  `AutomatedTesting`/`UserTesting` (20/10) chains are out of scope (§2.1).
- Compile evidence: the Lead's integrated build compiles the earlier candidate
  (after one `TxHashSetArchive` ambiguity fix, §8.6); the green 232/3926 run is
  on `FD7C0492…`. The env opt-in and `ServingOptInParsing` test are newer and
  need the next build; the isolated live attempt (§8.8) is not started.

### 8.4 Compile prerequisites (for the Lead-scheduled build slot)

- No CMake edit is required or made: the component is header-only and the P2P
  CMake glob does not include `PIBD/*.cpp`.
- C++17 is required (the tree already uses `inline static` in
  `PibdSegmentService.h`; `std::filesystem` comes through
  `deps/ghc/filesystem.h`). MSVC `/W4 /WX` is untouched.
- Out-of-line PMMR symbols used by the header (`KernelMMR::Load`,
  `TxHashSet` ctor, `TxHashSet::ValidateRoots`, `TxHashSetZip`) already reach
  the final link: `Tests` links `PMMR` explicitly
  (`tests/CMakeLists.txt:29`); `GrinNode` gets `PMMR` through
  `BlockChain`/`API` (plain-signature link interfaces, MSVC resolves static
  libraries order-independently). `GrinNode` already calls PMMR entry points
  today, so no new link dependency class is introduced.
- The CRC/read/short-write hardening in the auditor's uncommitted
  `src/PMMR/Zip/ZipFile.cpp` is a semantic prerequisite for §3.2 (the serving
  component treats `Extract == false` as failure; without that hardening the
  extractor could accept a corrupted entry).
- The working tree at build time contains other lines' uncommitted work
  (`src/PMMR/TxHashSetImpl.cpp/.h`, `src/BlockChain/Validators/BlockValidator.cpp`,
  `tests/src/PMMR/Test_Nrd*`, `tools/audit/nrd/`, `evidence/`,
  `tools/build/run-tests-isolated.ps1`). A build slot validates the combined
  tree, not Line 1 in isolation; record the exact tree state/diff hash.
- Expected new test cases in the next build (9 `TEST_CASE`s, no new files):
  `PibdParams::ArchiveHorizonSelection`, `PibdParams::SegmentHeightRanges`,
  `Capabilities::AdvertisedGating`, and the six `PibdArchiveServer::*` cases of
  §8.5. The previous integrated binary (222 cases / 3733 assertions, §8.6) does
  not contain them.
- Run the suite with isolated `TMP/TEMP/HOMEDRIVE/HOMEPATH` (line rules), and
  keep the `GrinNode.exe`/`Tests.exe` SHA-256 plus the log for the evidence
  record. A build/unit test is not live acceptance.

### 8.5 Executable tests added (executed; one fixture bug fixed)

All added to the already-registered `tests/src/PMMR/Test_PibdServing.cpp`
(no CMake change). Fixtures are synthetic: `WriteTestState()` builds a real
PIBD state on disk, a header is synthesised from the snapshot roots/sizes, and
the archive zip is rebuilt in the legacy `TxHashSet.<...>.zip` layout that
`TxHashSetZip::Extract` expects.

| test case | covers |
|---|---|
| `PibdArchiveServer::HorizonSelectionAndSpaceBoundary` | `ComputeHorizonHeight` at tiny heights (0, 1, 2878…2881, 3599, 3600, 4319, 4320, live 3 318 655 → 3 315 600), servable-height predicate, saturated `ComputeRequiredSpace` overflow, `HasSpaceFor` false on impossible size, positive boundary when the volume has room |
| `PibdArchiveServer::RegistryGatingAndExactHash` | disabled registry never advertises/acquires/prepares; absent → no capability; preparing is single-flight; wrong-horizon publish rejected; ready+match advertises; acquire is exact-hash only; ready height; monotonic failure backoff (`FAILURE_RETRY`) |
| `PibdArchiveServer::LeaseOutlivesHorizonSwitch` | a lease taken for an in-flight response survives a horizon switch; old/new-hash acquires are refused; stale publish rejected; last release triggers deferred cleanup exactly once (simulated "horizon switch during send") |
| `PibdArchiveServer::StagingCleanup` | `.staging-*` crash leftovers removed recursively, normal archive files preserved, missing directory is a no-op |
| `PibdArchiveServer::ValidateExtractedState` | valid state accepted with matching sizes/roots and flushed header; wrong output/range-proof/kernel root, wrong output/kernel MMR size and v2-header-with-merged-root all rejected with an error; v2 header with the plain output root accepted |
| `PibdArchiveServer::ArchiveZipExtractsAndValidates` | legacy zip extract→`LoadAndValidate` round trip; truncated zip rejected; bit-flipped (CRC) archive rejected; missing expected entry rejected |
| `PibdArchiveServer::DeferredGarbageDrain` | deferred-cleanup executor: queueing alone removes nothing; the executor removes exactly the queued directories, clears the queue, idempotent on empty input |
| `PibdArchiveServer::ServingOptInParsing` | env opt-in parsing only: `1/true/on/yes` (trim/case) enable; empty/`0/false/off/no/2/yesplease/enabled` do not; production constant pinned `false`; hard default wins without reading env |

Plus the earlier pure cases `PibdParams::ArchiveHorizonSelection`,
`PibdParams::SegmentHeightRanges`, `Capabilities::AdvertisedGating`. Total new
cases: **11** (8 `PibdArchiveServer::*` + 3 pure). The next build adds
`ServingOptInParsing` to the previous 232-case suite (expected 233 cases; exact
assertion count to be reported from the run, not predicted here).

**First execution** (Lead-run integrated binary `Tests.exe` SHA-256
`2032536D810F9DD78E57658FDEC39DCA06B71368551B2B1C6CAB08B79592140F`,
2026-11-03): `PibdArchiveServer::*` ran 6 cases, **5 passed / 1 failed**
(159 assertions, 158 passed). The single failure:

```
Test_PibdServing.cpp(611): FAILED: REQUIRE( destroyed == 1 )
with expansion: 0 == 1
```

Reproduced locally on the same SHA with isolated HOME/TMP:
`C:\DHW\.tmp\line1-lease-repro-01\lease-single.log` (1 case, 14 assertions,
13 passed / 1 failed, exit 1) and `lease-group.log` (6 cases, 5 passed / 1 failed,
158/159 assertions). **Independently reproduced by line3-fuzz** on the preserved
binary `tools/audit/nrd/evidence/bin/Tests-2032536D810F.exe` (same SHA,
isolated HOME/TMP): single filtered case in
`evidence/line3/line3-audit-pibd-lease-01/` fails identically; full-suite
evidence `tools/audit/nrd/evidence/isolated/full-20261103-033401/` (231 cases /
3917 assertions, 230 pass / 1 fail). **Root cause: test fixture bug, not lost
production cleanup.** The test kept `auto pFirst = MakeSnapshot(...)` alive, so after
`registry.SetExpected(h2)` dropped the registry's reference the snapshot still
had two owners (`pFirst` + `lease`); `lease.reset()` was not the last release and
the destructor correctly did not run. Production had a single deleter,
`Registry::SetExpected/Publish/SetAbsent` drop references under its mutex, and
the server drained the deferred-cleanup queue on `Stop()` and on prepare/failure
turns; the failure was therefore **not** a lost cleanup. The review did tighten
one cadence gap (see §8.7): the worker now drains on every turn, so a directory
retired by a reader cannot wait for the next prepare/failure turn.

Fix (test only, assertion not weakened): `pFirst.reset()` immediately after
`Publish`, `pSecond.reset()` after its publish, and
`REQUIRE(lease.use_count() == 1)` before the horizon-switch assertions to prove
the lease is the last owner, so a recurrence of this fixture bug fails at the
precondition instead of at `destroyed == 1`.

**Second execution, after the fix** (Line 1 approved build slot, normal Tests
build):

```
build:  pwsh tools/build/build.ps1 -Target Tests   -> BUILD OK (exit 0)
Tests.exe SHA-256:
  FD7C04922A2E016723531EC389E21D3AC1E8C0F99134F003595DB18149AB16E5
  (9,154,048 B, mtime 2026-11-03 03:44:47)
preserved copy:
  C:\DHW\.tmp\line1-build-03\Tests-FD7C04922A2E.exe  (SHA verified to match)
PibdArchiveServer::LeaseOutlivesHorizonSwitch: 1 case / 16 assertions PASS
PibdArchiveServer::*:                          7 cases / 168 assertions PASS
full suite:                                   232 cases / 3926 assertions PASS
```

All archive tests are green on the normal binary. The deferred-cleanup executor
is now covered directly by `PibdArchiveServer::DeferredGarbageDrain` (queueing
alone removes nothing; the executor removes exactly the queued directories and
is idempotent). The worker cadence itself (draining at the top of every turn)
needs a live chain/worker to exercise and is verified by code review in §8.7.

### 8.6 Build and test evidence (Lead-run, integrated tree)

Reported by the Lead for the integrated working tree:

- Integrated build **SUCCEEDS** after the Lead fixed the
  `case TxHashSetArchive` ambiguity (`MessageTypes::TxHashSetArchive`), which
  this patch triggered by pulling `<P2P/TxHashSetArchive.h>` into
  `MessageProcessor.cpp` (see §5 F11).
- Full isolated suite on the integrated binary: **222 cases / 3733 assertions
  PASS**. `Tests.exe` SHA-256:
  `06309AF0D81C9111FA3853777C09ECD0596DAAC2AB5433DF07D9931999C65B03`.
  That binary predates the six new archive tests in §8.5.
- Second integrated build (line2 retry): full suite **231 cases / 3917
  assertions**, with exactly one red — the Line 1 lease fixture bug described in
  §8.5 (test-side, fixed). `Nrd*` 20/375 and `KernelIndex*` 3/48 accepted by the
  Lead. `Tests.exe` SHA-256:
  `2032536D810F9DD78E57658FDEC39DCA06B71368551B2B1C6CAB08B79592140F`.
- `GrinNode.exe` built; Lead-reported SHA-256 prefix `ED218E55…` (full digest not
  yet recorded here — must be captured verbatim in the evidence record).
- **Line 1 approved build slot (2026-11-03):** `tools/build/build.ps1 -Target
  Tests` BUILD OK; new `Tests.exe` SHA-256
  `FD7C04922A2E016723531EC389E21D3AC1E8C0F99134F003595DB18149AB16E5`
  (preserved at `C:\DHW\.tmp\line1-build-03\Tests-FD7C04922A2E.exe`).
  Isolated HOME/TMP reruns: lease case 1/16 assertions PASS, archive group
  7 cases/168 assertions PASS, full suite 232 cases/3926 assertions PASS.
- **Line 1 opt-in build (2026-11-03):** `-Target "GrinNode;Tests"` BUILD OK;
  `Tests.exe` `8F10BFD67047DD25FAB3C201C29FA125096EF4DD7E8EFFFE33B61627CF1D73D6`
  (236 cases / 3952 assertions PASS; archive subset 8 cases/187 assertions;
  `ServingOptInParsing` 19 assertions PASS), `GrinNode.exe`
  `5CA47DA9589889E3CB7E44A715DD43BA8D592687634678EC708FFF7C86A21266`
  (both preserved in `C:\DHW\.tmp\line1-build-05\`). Live attempt result: §8.9
  (L0 PASS, L1 FAIL/BLOCKED; no ordinary PASS claimed).
- **Line 1 build 06 (2026-11-03):** `-Target "GrinNode;Tests"` BUILD OK (exit 0);
  `Tests.exe` `930148B6146A5E89B0B7FAC4A32B57212434B511E14F778A00AD6D6D24C1BB6E`
  (242 cases / 3980 assertions PASS; archive 8/187; `P2P::AdvertisedTipUsesHeaderHead`
  1/8 PASS), `GrinNode.exe`
  `9548B83F2741EE5911B6A72BBADDE59EFD32A3AF784A8280DFAC904E6D342B79` (preserved
  in `C:\DHW\.tmp\line1-build-06\`). Suite grew by the advert case + 5 line2
  `NrdFalseBan` cases. Live: handshake TD fixed and no overshoot, but Rust stayed
  `header_sync` without reaching H selection -> L1 PARTIAL/BLOCKED (§8.9).
- **Line 1 build 07 FINAL (2026-11-03):** `-Target "GrinNode;Tests"` BUILD OK
  (exit 0); `Tests.exe`
  `C8C386BE576A62604404E0FA02E56EE9E8F06B50EA0B15AD109F36F5AFF5B7FE` (245 cases /
  4013 assertions PASS; archive 8/187; advert 2 cases/15 assertions), `GrinNode.exe`
  `F9EF06B4CB0962B50E6DDBD186E1C61F5872FAFF9E41A779EE7C317A2ED25BDA`; both
  preserved in `C:\DHW\.tmp\line1-build-07\`. Line3 independent audit PASS
  (normal binding updated; ASan stale for this SHA under no-build).
- **Independent normal-binary audit (line3-fuzz): PASS.** Repo
  `bin\Release\Release\Tests.exe` == the preserved
  `C:\DHW\.tmp\line1-build-03\Tests-FD7C04922A2E.exe` == SHA-256
  `FD7C04922A2E016723531EC389E21D3AC1E8C0F99134F003595DB18149AB16E5`
  (9,154,048 B, PE timestamp `0x6AE8F63F`), with the line2 preserved copy also
  matching and `build-console.log` ending `BUILD OK`. Their isolated runs with
  independent runner/preserved copy (`-KeepBinary`, isolated HOME/TMP, exit 0)
  matched exactly: lease 1/16, archive group 7/168, `Nrd*` 20/375,
  `KernelIndex*` 3/48, full suite 232/3926 PASS. Source binding: compiled roots
  identical between repo and their ASan copied tree (`PibdArchiveServer.h`
  `56EE9A87…`, `Test_PibdServing.cpp` `4646F4DF…`, `TestMain.cpp`
  `6BAC1744…`); only evidence-only files differ. Evidence:
  `evidence/line3/normal-binary-fd7c-audit-20261103.md`, `EVIDENCE_LINE3` §12,
  `CI_REPRO`.
- **Independent ASan validation (line3-fuzz, source-based):** the fixture fix
  (test file SHA-256
  `4646F4DF256415DF561F66A6C16569AF068825D043F64B38D9A8CB604B3E9D52`) passes
  under a full ASan rebuild (`TestsMini.exe` SHA-256
  `2DA4727EE676E6663B6EEBC00F5ECC73848EA808819436F4887891F8BE3166ED`,
  source manifest `857AA38E…`): lease case 1 case/16 assertions PASS, archive
  subset 7 cases/168 assertions PASS, zero ASan reports; evidence
  `evidence/line3/asan-line1-archive-subset-20261103.md`. This is **source-based
  with the uncommitted fix**, not bound to the normal binary SHA; the normal
  binary rerun above is the authoritative normal-binary evidence. DB/rocksdb
  paths are **not** ASan-covered (line3 caveat).
- This is **build/unit evidence only**. It is not live PIBD/archive acceptance;
  the ordinary-binary live scenario remains **NOT VERIFIED** (§6 T17–T19).

### 8.7 Compile review: ambiguous names, locks, lifetimes

- **Ambiguous names.** After adding the serving header the only TU that mixes
  `using namespace MessageTypes;` with `<P2P/TxHashSetArchive.h>` is
  `MessageProcessor.cpp`; the one collision (`TxHashSetArchive` enumerator vs
  namespace) was hit by the build and fixed. `HandShake.cpp` and `Seeder.cpp` do
  not use unqualified `MessageTypes` names. `TxHashSetArchive::Dir/Path` are
  always qualified. No other unqualified identifier from the newly included
  headers collides in the reviewed TUs.
- **Lock order.** `PibdArchiveServer::Instance()` (instance mutex) → `Registry`
  (state mutex) in the handshake path; the worker never takes the instance
  mutex. `Registry::SetExpected/Publish/SetAbsent` may destroy the last
  `Snapshot` reference while holding the state mutex, and the `Snapshot`
  destructor only enqueues the staging path (garbage mutex); `DrainGarbage`
  never takes the state mutex. No inversion path was found.
- **Leases.** `Acquire()` returns a `shared_ptr`; the response is serialised
  while the lease lives and `SendAsync` serialises synchronously
  (`Connection.cpp:52-58`), so releasing the lease after the call is safe.
  Staging removal happens on the last release, after `txHashSet.reset()`
  releases the file mappings; a Windows removal failure is logged and retried
  at the next startup cleanup. Review follow-up: the worker now calls
  `DrainGarbage()` at the top of every turn. Before that, the steady-state
  `Tick()` early-return (ready matches expected) meant a directory retired by a
  reader was drained only on the next prepare/failure turn or `Stop()` —
  delayed, never lost. This cadence change is not related to the red
  `LeaseOutlivesHorizonSwitch` line, which was a test-fixture bug (§8.5). The
  deferred-cleanup executor itself is unit-tested
  (`PibdArchiveServer::DeferredGarbageDrain`: queueing alone deletes nothing;
  the executor removes exactly the queued directories, is idempotent and clears
  the queue). The worker call-site cadence (top of every turn) needs a live
  chain/worker and is verified by review plus the green `Stop()`/suite paths.
- **Worker lifetime.** The worker captures a raw `this`; the instance is held by
  `s_instance` until `Shutdown()` moves it out and `Stop()` joins the thread
  before releasing the shared pointer. `Start()` is idempotent.
- **No chain lock during archive generation.** `MessageProcessor` acquires the
  archive lease *before* `GetApiStateSnapshot()` and only takes the chain lease
  on the flushed-state fallback; archive segment generation therefore does not
  block block application. The live path keeps the previous behaviour.
- **Residual risks** (documented, not fixed): `Shutdown()` waits for an
  in-flight multi-GB load (no cancellation point); capability state can be up
  to `WORKER_INTERVAL` (5 s) stale after a horizon rotation, during which a new
  peer may receive the old capability and then get a correct refusal; no
  periodic re-archive yet.

### 8.8 Approved isolated live plan (NOT STARTED)

Approved by the Lead: Phase A env opt-in only; Rust client
`C:\Users\nurli\Documents\Codex\2026-11-02\c-grin-1-2-9-c\work\G09RustLive.exe`,
SHA-256 `E5219C9144FFED1C9A7D5368919ADCFF32FD39D48F449AA13521E1D100E776B8`
(verified present locally, do not rebuild); C++ reuses the documented fixture
profile `C:\DHW\.tmp\interop\pp-archsrv` (tip 3 318 655) — **fixture-based C++
side, partial; never a full public-sync PASS**. Rust headers and state start
from zero in its own isolated profile. Public Floonet sync is **not
authorized**.

Gates (each with hashes, logs, isolated HOME/TMP):
- **L0 preflight:** GrinNode SHA/build log; Rust SHA; C++/Rust configs (initial
  and post-`Config::Load`); startup log `PIBD archive serving: ENABLED
  (opt-in GRINPP_PIBD_ARCHIVE_SERVING)`; no non-loopback connections; fixture
  source manifest hash; ports 25536 (C++ listener) / 25537 (Rust listener);
  `--floonet --no-wallet --headless --config-path`.
- **L1 headers from zero:** empty Rust DB, `peers_allow` only C++; Rust header
  sync served by C++; prove allowlist and serving.
- **L2 PIBD:** Rust-selected H (`txhashset_archive_header_header_only`) equals
  C++ `PIBD archive: ready at height H`; segments served; roots validate;
  activation → BodySync → `no_sync` at identical tip/hash; capture p50/p95,
  RSS, disk.
- **L3 restart:** C++ restart during Rust download (before activation) and after
  ready; resume/retry, capability re-advertised only after validated archive; no
  wrong-H data.
- **L4 horizon transition / reorg:** expected **BLOCKED** without a +720-block
  source; report honestly, no synthetic PASS.
- **L5 negative proof/root/ID:** wire-level mutated requests/segments;
  controlled refusal, no hang/corruption; labelled wire-negative, not
  ordinary-node PASS.
- **L6 report:** PASS/FAIL/NOT VERIFIED/BLOCKED per scenario with exact SHAs and
  logs.

C++ isolation mechanism (verified in code, §2.5/`Seeder.cpp:194-231` and
`Seeder::Accept`): `PREFERRED_PEERS`/`ALLOWED_PEERS` restricted to loopback,
`MIN_PEERS=0`, `MAX_PEERS=2`, `P2P_PORT=25536`, `P2P_PEER_PORT=25537`; data root
pinned with `DATA_PATH`; `HOMEDRIVE/HOMEPATH/HOME/TMP/APPDATA` under
`C:\DHW\.tmp\g09-live-<ts>\`.

### 8.9 Isolated live attempt 2026-11-03 (fixture-based; PARTIAL — L1 BLOCKED)

Run root: `C:\DHW\.tmp\g09-live-01` (isolated HOME/TMP/APPDATA; C++ `DATA_PATH`
pinned). Binaries: GrinNode `5CA47DA9589889E3CB7E44A715DD43BA8D592687634678EC708FFF7C86A21266`
(preserved `C:\DHW\.tmp\line1-build-05\GrinNode-5CA47DA95898.exe`), Tests
`8F10BFD67047DD25FAB3C201C29FA125096EF4DD7E8EFFFE33B61627CF1D73D6`, Rust
`G09RustLive.exe` `E5219C91…E776B8`. C++ side is **fixture-based/partial**:
the Lead-named profile `C:\DHW\.tmp\interop\pp-archsrv` has an **empty
TXHASHSET** (no state, no archive), so the documented prior live profile
`work\g09-cpp-live-real\.GrinPP\FLOONET\NODE` was used instead (source manifest
SHA-256 `45EA449CF80C3A25AA5E97CBC362EA25CA0053C2627A2BDC92A44316E284390E`;
archive `TxHashSet.3315600.3615bb625917.zip`, SHA-256
`13F9781E207F787F5F58367F9C98947D4A645D660E1CA23186AD55D9D064C530`).
Rust headers/state start empty. No public Floonet sync; the C++ seeder resolved
5 DNS seeds but every candidate was rejected by
`PREFERRED_PEERS`/`ALLOWED_PEERS=127.0.0.1` (no public connection; C++
in/out = 1/0).

- **L0 preflight: PASS.** Opt-in line observed:
  `PIBD archive serving: ENABLED (opt-in GRINPP_PIBD_ARCHIVE_SERVING)`;
  `PIBD archive: ready at height 3315600 (zip 3118746579 B, extracted
  3117469056 B)`; C++ REST `/v1/status`: height 3318655, hash
  `2945348ef765160a160008cfd7c2b41aa44ed3cea77d6bcfe2083acf0fc4bbf1`.
  Rust handshake logged
  `HEADER_HIST | TXHASHSET_HIST | PEER_LIST | PIBD_HIST | PIBD_HIST_1` —
  dynamic gating advertised PIBD only after the validated archive was ready.
  Evidence: `...\evidence\L0-preflight.txt`.
- **L1 headers from zero: FAIL/BLOCKED.** Rust header extension advanced to
  **3318784**, then no header activity after 04:51:43:
  - Rust: `header_sync`, header head 3318784, highest_height 3318655, tip 0,
    1 connection; after 04:51:43 only `monitor_peers` lines.
  - C++: confirmed height 3318655 / hash `2945…bbf1`; candidate header head
    3320005; eight 512-header batches sent 04:51:41.100–04:51:43.286; only
    Pings afterwards; inbound=1, outbound=0.
  - The headers are present in the C++ store beyond the stall
    (`GET /v1/headers/3318785` FOUND, hash
    `45a75f97502f05b6ec8caef64c5b4728e60305bb7c2c83e6732c05008d1fa2f4`, TD
    `1207039626711`; `GET /v1/headers/3320005` FOUND, hash `4988ad64…`, TD
    `1207079050230`) → this is **not** a header-MMR/GetHeaders data-boundary bug.
  - **Root cause (real C++ protocol-semantics defect):** C++ handshake/Pong
    advertises the **CONFIRMED (body)** height/TD — 3318655 /
    `1207035333622` (`MessageProcessor.cpp:160-166`; SyncStatus seeded from the
    confirmed chain in `P2PServerImpl.cpp:54-60`) — while `GetHeaders` serves
    **CANDIDATE** headers up to 3320005 (`BlockLocator.cpp:54-78`). Rust received
    a 512-batch crossing the advertised tip and applied headers to 3318784;
    header 3318785's TD `1207039626711` now exceeds the advertised peer TD.
    grin 5.5.2 `servers/src/grin/sync/header_sync.rs:70-78` then returns
    `Ok(false)` (`peer_diff <= sync_head.total_difficulty`), never requests
    again, and leaves the status at `HeaderSync`. Rust's own server advertises
    `chain.header_head()` (`servers/src/common/adapters.rs:352`). This mismatch
    triggers whenever candidate head > confirmed head — i.e. exactly the
    from-zero header-sync-from-C++ case — and is deterministic.
  - Fixture is additionally not advanceable: candidate 3320005 vs bodies
    3318655 with no bodies beyond 3318655; public sync is not authorized.
- **L2 PIBD/activation/no_sync: NOT VERIFIED** (never reached; 0 PIBD segment
  requests on the C++ side; archive serving was not exercised live).
- **L3 restart: NOT EXECUTED** (blocked by L1). **L4 horizon/reorg: BLOCKED**
  (no +720-block source; fixture cannot advance without public sync).
  **L5 wire-negative: NOT EXECUTED** (needs the wire harness).
- **L6: no G-09 PASS.** Fixture-based C++ only; L0 PASS, L1 FAIL/BLOCKED,
  L2–L5 NOT VERIFIED/NOT EXECUTED.
- **P0 fix implemented (option 1 + atomic snapshot; BUILD-07
  `C8C386BE…`/`F9EF06B4…`, audited PASS).**
  No `GetHeaders` clamping. `include/P2P/AdvertisedTip.h` defines the advertised
  tip and `ReadAdvertisedTip(provider)` invokes the provider **exactly once**, so
  height and total difficulty always come from one header/tip snapshot; a null
  snapshot yields `nullopt` so callers withhold the message.
  `SyncStatus` now stores `HeaderTip{height,TD}` under one mutex
  (`GetHeaderTip()` / `UpdateHeaderStatus()` take the same lock; the old split
  header atomics are gone), and `P2P::GetAdvertisedTip(SyncStatus)` /
  `GetAdvertisedDifficulty(SyncStatus)` read that one pair. `HandShake.cpp`
  Hand/Shake TD uses the locked header pair. `MessageProcessor::GetAdvertisedTip()`
  returns one `GetTipBlockHeader(CANDIDATE)` snapshot (the call itself runs under
  a ChainState read lease); `Connection::CheckPing` sends it or falls back to the
  locked `SyncStatus` pair, otherwise withholds the Ping (no split reads).
  `P2PServerImpl` seeds header/block status from single tip-header snapshots
  instead of `GetHeight`+`GetTotalDifficulty` pairs. `ChainState::UpdateSyncStatus`
  documents that `BlockChain::UpdateSyncStatus` holds the read lease across its
  fallback lookups.
  Regression tests (`tests/src/Net/Test_P2PCompat.cpp`):
  `P2P::AdvertisedTipUsesHeaderHead` (header values, both fields move together)
  and `P2P::AdvertisedTipReadsOneSnapshot` (provider called once; the candidate
  tip changes between the two reads a split implementation would make, and the
  returned pair must stay on the first snapshot; null snapshot is withheld).
  **Build-07 status:** built and green (245/4013; advert 2/15; see §8.6).
  **Final live refusal attempt (build-07 `F9EF06B4…`):** Rust restarted on the
  persisted header chain (header extension 3320000..3320005, body tip 0) and
  entered `AwaitingPeers(false)` with 1 connection; it never transitioned to
  HeaderSync/state sync and sent C++ only Pings (no GetHeaders/PIBD/segment/
  archive request after restart). **H selection never happened**, so the
  expected H=3317040 vs archive 3315600 safe-refusal was **NOT REACHED /
  NOT VERIFIED**. Evidence: `...\evidence\L1-refusal-blocked.txt`,
  `rust3-grin.log`, `cpp3-Node.log`; nodes stopped; run root 17.7 GB.
  Classification: **BLOCKED/partial** (peer-wait/header-resume condition on the
  fixture profile), not a PASS.
  **Live result on the pre-atomic fix (build `9548B83F…`):** handshake advertised
  header-head TD `1207079050230` (confirmed body TD was `1207035333622`); Rust
  header extension reached 3320000/3320005 with no 3318784 overshoot. However Rust
  stayed in `header_sync` and never selected H/requested the archive, so safe
  refusal at H=3317040 was **NOT VERIFIED** (`evidence\L1-advert-fix-partial.txt`).
  **Expected live outcome on the current fixture after the fix:** header head
  3320005 makes Rust select H = round_down(3320005-2880, 720) = **3317040**, not
  our archive's 3315600; the archive hash must then be refused/fall back. Record
  that as a **fixture/H mismatch BLOCKED, not a PASS**. H=3315600 requires a
  consistent fixture with header == body == 3318655 (or an archive at 3317040).
- Artifacts: `...\evidence\L1-stall-diagnosis.txt`, `rust-grin.log`,
  `cpp-Node.log`, configs; RSS before stop: Rust 235.6 MB, C++ 3433.5 MB; run
  root 17.7 GB; both processes stopped (PID 20708, 31692).
- Harness note: running two `Tests.exe` concurrently against one isolated
  profile produced `0xC0000409` with an empty log; a sequential fresh-profile
  run passed 236/3952. Runner misuse, not a product result (relevant to line3).

### 8.10 Task-7A restart peer-wait root cause (build-07 trace)

Reproduction: build-07 `GrinNode F9EF06B4…` on the fixture profile, Rust
`G09RustLive` restarted on persisted `chain_data` (header MMR at 3320005, body tip
0). Result: Rust `AwaitingPeers(false)`, tip 0, 1 connection; C++ inbound/outbound
1/0 and only Pings; 30 status samples over 150 s identical
(`C:\DHW\.tmp\task7-A\restart-status.jsonl`).

Fresh-from-zero comparison (`task7-A\rust-zero`, empty `chain_data`): `Initial ->
AwaitingPeers(false) -> HeaderSync` (height 0, TD 100000, highest_diff
1207079050230) `-> BodySync`; same handshake capabilities/TD. The difference is the
local header-resume state, not min-peers gating (`skip_sync_wait = true`,
outbound peer present).

Trace-level restart (`task7-A\restart-trace.log`, `file_log_level = Trace`):
- Rust `header_head: 1207079050230 @ 3320005 [4988ad64…]`.
- `state_sync: head.height: 0, tail.height: 0. header_head.height: 3320005,
  highest_height: 3318655` (repeated); `body_sync: cannot sync full blocks
  earlier than horizon. will request txhashset`.
- `HeaderSync::check_run` early-returns because peer_diff (CONFIRMED TD
  1207035333622) <= local header TD (1207079050230).
- `StateSync::check_run` only starts PIBD when
  `header_head.height == highest_height`; 3320005 != 3318655, so PIBD never
  starts and the status never leaves `AwaitingPeers`.

Root cause: `src/P2P/MessageProcessor.cpp` `case Ping` (build-07 lines 178-188)
replied with **CONFIRMED** height/TD (`GetHeight/GetTotalDifficulty(CONFIRMED)`)
while outbound Ping/Hand/Shake used header-head values. Rust peer `live_info` is
overwritten by the Pong's body values, so `highest_height` = 3318655 while the
local header head is 3320005: header sync thinks it is caught up and state sync
waits for equality -> permanent `AwaitingPeers`.

Approved one-file fix (implemented uncommitted; build-08 pending): `case Ping`
builds `PongMessage` from one `GetAdvertisedTip()` header-head snapshot; if the
snapshot is `nullopt`, the Pong is withheld; never CONFIRMED and never mixed.
No other production changes. Expected effect: peer height/TD become
3320005/1207079050230 in both directions, `highest_height == header_head`, so
StateSync runs, Rust selects H=3317040, and the wrong-H refusal becomes
observable.

Build sequencing: build-07 `C8C386BE…`/`F9EF06B4…` remain accepted evidence for
their source snapshot but are no longer source-current (line2 post-build test
additions). build-08 is the frozen build containing line2 task-8 tests + this
Pong fix; no further source edits after build-08 without new approval.

### 8.11 Task-7B fixture inventory (read-only)

- C++ REST: body height 3318655, hash
  `2945348ef765160a160008cfd7c2b41aa44ed3cea77d6bcfe2083acf0fc4bbf1`, TD
  1207035333622; header height 3320005 (headers 3315600..3320005 present, 3320006
  absent). Header 3320005 hash `4988ad64…`, TD 1207079050230.
- Archive: `TxHashSet.3315600.3615bb625917.zip`, 3,118,746,579 B, SHA-256
  `13F9781E207F787F5F58367F9C98947D4A645D660E1CA23186AD55D9D064C530`.
- `CHAIN`: `candidate.chain` 106,240,192 B (3,320,006 × 32-byte hashes),
  `confirmed.chain` 106,196,992 B (3,318,656), `header_mmr.bin` 212,480,064 B,
  `chain.checkpoint` 52 B (GRINPPCK v1, CandidateEntries 3,320,006,
  ConfirmedEntries 3,318,656).
- Trim feasibility: **FEASIBLE via the documented checkpoint truncation
  contract** (not a raw trim):
  1. copy `NODE` unchanged to a new run root (`C:\DHW\.tmp\g09-live-02`, copy
     running),
  2. truncate `CHAIN\candidate.chain` to 3,318,656 × 32 B = 106,196,992 B,
  3. rewrite `CHAIN\chain.checkpoint` with `Sequence` unchanged,
     `MmrLeaves = 3,318,656`, `CandidateEntries = 3,318,656`,
     `ConfirmedEntries = 3,318,656`, and a recomputed FNV-1a checksum over the
     first 44 bytes (the file's documented integrity contract),
  4. on load, `BlockChainImpl` sees header MMR leaves 3,320,006 > candidate
     entries 3,318,656 and truncates the MMR to the candidate under that
     checkpoint contract, then writes a fresh checkpoint.
  No fabricated blocks: every retained hash is from the valid fixture. Validation
  and the acceptance run happen only after build-08.
- Archive-at-H=3317040 is **not sufficient** for a consistent same-tip fixture:
  C++ bodies end at 3318655, so a header head of 3320005 cannot reach `no_sync`
  (body sync would request 3318656..3320005, which the fixture does not have).
  The chosen B path is trim-to-3318655 + the existing archive at H=3315600.

## 9. Status matrix

| item | status | reason |
|---|---|---|
| Snapshot/capability contract | **DELIVERED (review)** | this document |
| Gap analysis vs ordinary node | **DELIVERED** | §4, static evidence |
| DRAFT-patch review | **DELIVERED** | §5 |
| Bounded archive serving (candidate) | **COMPILED (integrated build), default-off; behaviour NOT VERIFIED** | §8; gated by `ENABLED_BY_DEFAULT = false` |
| Dynamic capability gating (candidate) | **COMPILED; off by default; gating test green** | §8.2, §8.5 |
| Compile, integrated tree | **PASS: build-07 `C8C386BE…`/`F9EF06B4…`, 245/4013; independently audited** | §8.6; archive 8/187; advert 2/15; ASan stale under no-build |
| Negative/concurrent tests | **PASS: 11/11 new cases green (archive subset 8/187); line3 ASan subset (source-based)** | §8.5, §8.6 |
| `Output Commitment not found` | **NOT VERIFIED (root cause candidate)** | §7; static analysis only |
| G-09 ordinary-binary live acceptance | **NOT VERIFIED** | §8.9: L0 PASS, L1 FAIL/BLOCKED, L2+ NOT VERIFIED; fixture-based C++ |
| Env opt-in `GRINPP_PIBD_ARCHIVE_SERVING` (Phase A) | **PASS (compiled build-07 `C8C386BE…`; activation observed live)** | §8.2, §8.9; mainnet refusal in code; default-off preserved |
| L1 header sync from zero (ordinary binaries) | **FAIL/BLOCKED — real C++ advertised-tip defect** | §8.9: confirmed-tip Pong vs candidate-header serving; Rust `header_sync.rs:70-78` early-return; minimal repro captured |
| L1 advertised-tip P0 fix | **PASS: build-07 `C8C386BE…`/`F9EF06B4…`, 245/4013, advert 2/15, archive 8/187; independently audited** | §8.6, §8.9; advertisement + atomic snapshot implemented; live H selection still blocked |
| L1 fixture-mismatch refusal (H=3317040 vs archive 3315600) | **BLOCKED / NOT VERIFIED — H selection never reached** | §8.9: build-07 Rust restart stayed `AwaitingPeers(false)` (tip 0, 1 conn); C++ saw only Pings; refusal untested |
| Task-7A restart peer-wait root cause | **ROOT-CAUSED: Pong replied with CONFIRMED height/TD; one-file header-head snapshot fix implemented (uncommitted); build-08 pending** | §8.10; Rust Trace `header_head 3320005` vs `highest_height 3318655`; StateSync equality gate never satisfied |
| Task-7B consistent fixture | **BLOCKED offline: raw candidate trim refused by checkpoint guard; archive-at-3317040 plan requires valid snapshot generation after build-08** | §8.11; no synthetic data; checkpoint CandidateEntries=3320006 vs target 3318656 |
| Isolated live attempt (approved L0–L6) | **PARTIAL/BLOCKED: L0 PASS; L1 not completed (H selection/refusal NOT REACHED); L2–L5 NOT VERIFIED/NOT EXECUTED; no PASS** | fixture-based C++, Rust from zero + restart attempts, §8.9 |
| Repo state left | uncommitted edits only | no commit/reset/clean performed |

## 10. Evidence and open questions

### 10.1 Provenance / open questions for Lead

1. Rust reference SHA: the on-disk 5.5.2 snapshot has no `.git`; its identity for
   this document is Cargo version 5.5.2 + manifest
   `8176D489733D3BC2BEA303A87A65D6718D72D328E2938820FB193071240DE998`. The
   `dbaff34` commit named by the task is **NOT VERIFIED** from local data. A
   proper provenance record (release tarball URL/hash or a fresh clone at
   `dbaff34`) is required before acceptance evidence cites a Grin SHA.
2. Build slot: the opt-in build/rerun is done (`8F10BFD6…`, 236/3952). The live
   attempt was executed and is **PARTIAL/BLOCKED at L1** (§8.9); before another
   live run, decide the advertised-tip fix (header head vs clamped serving) and
   prepare a **consistent fixture** whose header head and body tip both map to
   the serving H (3315600). Live acceptance must not be inferred from unit
   tests; fixture-based C++ remains partial.
3. `tests/src/PMMR/CMakeLists.txt` is auditor-owned; a new
   `Test_PibdArchive.cpp` cannot be registered without the owner's consent.
   The alternative is appending to the existing `Test_PibdServing.cpp`.
4. Line 2 owns `TxHashSetImpl.cpp/.h`; §7.4 fix needs their agreement.
5. The uncommitted auditor `ZipFile.cpp` hardening is a prerequisite for §3.2
   (CRC/read/short-write checks). It must be reviewed and integrated before an
   archive can be trusted.
6. Rust ordinary-node client resolved by the Lead: `G09RustLive.exe` SHA-256
   `E5219C9144FFED1C9A7D5368919ADCFF32FD39D48F449AA13521E1D100E776B8`
   (locally verified, do not rebuild). The Lead-named fixture
   `C:\DHW\.tmp\interop\pp-archsrv` has an **empty TXHASHSET** (no state, no
   archive), so the documented prior live profile
   `work\g09-cpp-live-real` was used instead; public Floonet sync is not
   authorized. Result: L0 PASS, L1 FAIL/BLOCKED, L2+ NOT VERIFIED (§8.9).
7. The live stall exposed a real C++ advertised-tip defect (confirmed-tip
   Pong/Handshake vs candidate-header serving, §8.9). Option 1 (advertise the
   header head) plus the atomic-snapshot rework is in build-07
   (`C8C386BE…`/`F9EF06B4…`, 245/4013 PASS; independently audited). Live checks:
   build-06 fixed the handshake TD and removed the 3318784 overshoot, but Rust
   stayed in `header_sync`; build-07 restart on the persisted header chain stayed
   `AwaitingPeers(false)` with no GetHeaders/PIBD, so **H selection and the
   expected fixture/H-mismatch refusal were never reached**. Record L1
   **BLOCKED/partial, live G-09 NOT VERIFIED**; no synthetic PASS. Remaining
   blockers: peer-wait/header-resume condition, inconsistent fixture (header
   3320005 vs body 3318655; H=3317040 vs archive 3315600), L2–L5 unexecuted,
   Output Commitment repro unverified, ASan stale for build-07 under no-build.

### 10.2 Static evidence used (commands)

```
git -C C:\DHW\Gplus\src\grinplusplus rev-parse HEAD
  -> e0a5d2d68b990234e7bf1fb1b651f868dc30e9b6
git -C C:\DHW\Gplus\src\grinplusplus status --short
  -> M src/PMMR/Zip/ZipFile.cpp
     M tests/src/PMMR/CMakeLists.txt
     ?? tests/src/PMMR/Test_ZipFile.cpp
git -C C:\DHW\Gplus\src\grinplusplus log --oneline -12
  -> e0a5d2d, 807b5b1, 10453ea, 1c0bcc3, ad2eed2, a49dfe0, ...
Rust reference (no own .git; parent-repo git output is NOT Grin):
  Cargo.toml name=grin version=5.5.2
  tree manifest sha256 = 8176D489733D3BC2BEA303A87A65D6718D72D328E2938820FB193071240DE998
  (391 files, 38 862-byte manifest, .git/target excluded)
```

Integrated tree build/test (Lead-run, reported):
  build: SUCCEEDS after qualifying `case MessageTypes::TxHashSetArchive`
         (namespace `TxHashSetArchive` became visible through the serving header)
  Tests.exe SHA-256: 06309AF0D81C9111FA3853777C09ECD0596DAAC2AB5433DF07D9931999C65B03
  isolated suite: 222 cases / 3733 assertions PASS
  GrinNode.exe: built, Lead-reported SHA prefix ED218E55… (full digest pending)
  caveat: this binary predates the nine new archive/capability tests (§8.5)
  live PIBD/archive acceptance: NOT VERIFIED

Second integrated build + first execution of the archive tests (Lead-run):
  Tests.exe SHA-256: 2032536D810F9DD78E57658FDEC39DCA06B71368551B2B1C6CAB08B79592140F
  full suite: 231 cases / 3917 assertions; only red = Line 1 lease fixture
  Nrd* 20/375 PASS, KernelIndex* 3/48 PASS (accepted by Lead)
  PibdArchiveServer::*: 6 cases, 5 passed / 1 failed (159 assertions, 158 passed)

Local reproduction of the red test (isolated HOME/TMP, binary SHA verified):
  root = C:\DHW\.tmp\line1-lease-repro-01
  Tests.exe "PibdArchiveServer::LeaseOutlivesHorizonSwitch"
    -> 1 case, 14 assertions, 13 passed / 1 failed; exit 1
       Test_PibdServing.cpp(611): FAILED: REQUIRE( destroyed == 1 ) 0 == 1
    log: C:\DHW\.tmp\line1-lease-repro-01\lease-single.log
  Tests.exe "PibdArchiveServer::*"
    -> 6 cases, 5 passed / 1 failed; 159 assertions, 158 passed
    log: C:\DHW\.tmp\line1-lease-repro-01\lease-group.log
  root cause: test kept a second strong reference alive (fixture bug); fixed in
  Test_PibdServing.cpp by releasing it after Publish and adding
  REQUIRE(lease.use_count() == 1); assertion REQUIRE(destroyed == 1) unchanged.

Line 1 approved build slot + green rerun (normal Tests build):
  command: pwsh tools/build/build.ps1 -Target Tests  -> BUILD OK (exit 0)
  build log: C:\DHW\.tmp\line1-build-03\build-console.log
  Tests.exe SHA-256: FD7C04922A2E016723531EC389E21D3AC1E8C0F99134F003595DB18149AB16E5
    (9,154,048 B, mtime 2026-11-03 03:44:47)
  preserved: C:\DHW\.tmp\line1-build-03\Tests-FD7C04922A2E.exe (SHA verified)
  isolated HOME/TMP runs:
    lease case: 1 case / 16 assertions PASS        (line1-testrun-03\lease-single.log)
    archive group: 7 cases / 168 assertions PASS    (line1-testrun-03\archive-group.log)
    full suite: 232 cases / 3926 assertions PASS    (line1-testrun-03\full-suite.log)
  no assertion failures, exit 0 for all three runs

Independent ASan validation (line3-fuzz, source-based, uncommitted fix included):
  test file sha256: 4646F4DF256415DF561F66A6C16569AF068825D043F64B38D9A8CB604B3E9D52
  ASan TestsMini.exe sha256: 2DA4727EE676E6663B6EEBC00F5ECC73848EA808819436F4887891F8BE3166ED
  lease case 1 case/16 assertions PASS; archive subset 7 cases/168 assertions PASS
  zero ASan reports; evidence evidence/line3/asan-line1-archive-subset-20261103.md
  caveat: source-based, not bound to the normal binary SHA (FD7C0492… is authoritative)

Line 1 opt-in build + isolated live attempt (fixture-based, PARTIAL/BLOCKED at L1):
  build: pwsh tools/build/build.ps1 -Target "GrinNode;Tests" -> BUILD OK (exit 0)
  build log: C:\DHW\.tmp\line1-build-05\build-console.log
  GrinNode SHA-256: 5CA47DA9589889E3CB7E44A715DD43BA8D592687634678EC708FFF7C86A21266
  Tests SHA-256: 8F10BFD67047DD25FAB3C201C29FA125096EF4DD7E8EFFFE33B61627CF1D73D6
    full suite 236 cases / 3952 assertions PASS; archive subset 8 cases/187 assertions
    ServingOptInParsing 19 assertions PASS (sequential fresh isolated profile)
  run root: C:\DHW\.tmp\g09-live-01 (isolated HOME/TMP/APPDATA; DATA_PATH pinned)
  fixture: copy of work\g09-cpp-live-real NODE; source manifest sha256
    45EA449CF80C3A25AA5E97CBC362EA25CA0053C2627A2BDC92A44316E284390E
    archive sha256 13F9781E207F787F5F58367F9C98947D4A645D660E1CA23186AD55D9D064C530
  L0 PASS: opt-in log; archive ready H=3315600 (zip 3118746579 / extracted 3117469056);
    cpp tip 3318655 hash 2945348e...bbf1; Rust handshake caps include PIBD_HIST_1;
    cpp in/out=1/0, no non-loopback TCP (5 DNS seed resolutions, all rejected)
  L1 FAIL/BLOCKED: Rust header head 3318784, highest_height 3318655, tip 0,
    no GetHeaders after 04:51:43; cpp candidate 3320005 / confirmed 3318655,
    eight 512-header batches 04:51:41-43; headers 3318785/3320005 exist (REST).
    root cause: Pong advertises confirmed TD 1207035333622, served header 3318785
    TD 1207039626711; grin header_sync.rs:70-78 peer_diff <= sync_head TD -> stop;
    status remains HeaderSync. Real C++ advertised-tip defect.
  L2 PIBD activation/no_sync: NOT VERIFIED (0 PIBD segment requests); L3 restart
    NOT EXECUTED; L4 horizon/reorg BLOCKED (no +720 source); L5 wire-negative NOT EXECUTED
  artifacts: C:\DHW\.tmp\g09-live-01\evidence\{L0-preflight.txt,L1-stall-diagnosis.txt,
    rust-grin*.log,cpp-Node*.log,cpp-server_config.json,rust-grin.toml}
  RSS before stop: Rust 235.6 MB, C++ 3433.5 MB; run root 17.7 GB; PIDs 20708/31692 stopped
  harness note: two concurrent Tests.exe on one isolated profile -> 0xC0000409
    empty log; sequential fresh-profile run PASS (runner misuse, not product result)
  no G-09 PASS claimed

Files read for this document (C++): `src/P2P/MessageProcessor.cpp/.h`,
`include/P2P/Capabilities.h`, `include/P2P/TxHashSetArchive.h`,
`include/P2P/Common.h`, `src/P2P/PIBD/PibdSegmentService.h`,
`src/P2P/PIBD/PibdParams.h`, `src/P2P/Connection.cpp/.h`,
`src/P2P/Seed/HandShake.cpp`, `src/P2P/Seed/Seeder.cpp`,
`src/P2P/P2PServerImpl.cpp`, `src/P2P/Pipeline/TxHashSetPipe.cpp`,
`src/P2P/Pipeline/BlockPipe.cpp`, `src/P2P/Sync/StateSyncer.cpp`,
`src/P2P/Sync/BlockSyncer.cpp`, `src/P2P/Sync/BlockSyncPolicy.h`,
`src/PMMR/TxHashSetManager.cpp`, `src/PMMR/TxHashSetImpl.cpp/.h`,
`src/PMMR/TxHashSetImpl.h`, `include/PMMR/TxHashSet.h`,
`src/PMMR/Zip/TxHashSetZip.cpp`, `src/PMMR/Zip/ZipFile.cpp`,
`src/PMMR/Common/PruneableMMR.h`, `src/PMMR/Common/LeafSet.h`,
`include/Core/File/DataFile.h`, `include/Core/File/BitmapFile.h`,
`include/Core/File/AppendOnlyFile.h`, `src/Core/File/AppendOnlyFile.cpp`,
`src/BlockChain/BlockChainImpl.cpp/.h`, `src/BlockChain/ChainState.h`,
`src/BlockChain/CoinView.cpp`, `src/Core/Models/TransactionInput.cpp`,
`include/Consensus.h`, `include/BlockChain/BlockChain.h`,
`include/Core/Global.h`, `tests/src/PMMR/Test_PibdServing.cpp`,
`tests/src/Net/Test_P2PCompat.cpp`.

Files read for this document (Rust 5.5.2): `chain/src/chain.rs`,
`chain/src/txhashset/segmenter.rs`, `chain/src/pibd_params.rs`,
`core/src/global.rs`, `core/src/consensus.rs`,
`core/src/core/transaction.rs`, `servers/src/common/adapters.rs`,
`servers/src/grin/sync/state_sync.rs`, `p2p/src/protocol.rs`, `p2p/src/types.rs`.

Review-only artefacts: `...\outputs\G09-DRAFT-archive-serving.patch`,
`...\outputs\G09-acceptance-2026-11-03.md`,
`...\work\g09-rust-client-real\grin.toml`.

No build, no test binary execution, no live network run was performed for this
increment.
