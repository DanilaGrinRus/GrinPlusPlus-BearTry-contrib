# Full implementation audit — node

## Scope and method

Reviewed the node implementation by module against grin 5.5.2 and the documented
contracts in `docs/modernization/`:

- P2P: capabilities, handshake, message framing, PIBD serving, peer DB;
- sync: header/block sync, PIBD, legacy archive;
- consensus: header/block validation, PoW, kernel/output features, NRD;
- chain/DB: batch/rollback, checkpoints, TxHashSet application;
- PMMR: positions, prune list, segment proof, serving lookups;
- API/seed/TxPool: handlers, peer selection, pool interfaces;
- build/tests: `/W4 /WX`, full suite, audit package.

This is a static + targeted-test audit, not a formal proof or a live-wallet test.
Live PIBD serving remains separately blocked (G-09).

## Baseline

- `GAP_MATRIX.md`: 53 entries; 45 closed, 3 partial, 1 open (G-07), 3 planned,
  1 N/A.
- Full suite before this audit: 187 cases / 2069 assertions PASS.
- Build: CMake/Ninja, MSVC `/W4 /WX`, offline sysroot.

## Module results

| Module | Result | Main evidence / notes |
|---|---|---|
| Build/toolchain | PASS | Clean `/W4 /WX` build; `Tests` and `GrinNode` produced |
| PMMR | PASS formulas, 2 fixes | Position/family/peaks/shift parity; compacted-hash and bounds fixes |
| PIBD server | PASS protocol, blocked live | Wire layout/constants/bitmap/roots match; kernel null fixed |
| P2P framing/caps | PASS review | Per-type caps match reference, 4x forward-compat; `HEADER_HIST` advertised |
| Peer DB | FIXED | It deleted `DB/PEERS` on every start |
| Chain/DB | 2 fixes, 1 risk | Checkpoint UB fixed; input-bitmap wipe removed; ApplyBlock assert fixed |
| Consensus kernels | **OPEN (NRD)** | Rust 5.5.2 supports NRD; C++ does not enforce recent-index rules |
| Features/deserialization | FIXED | Unknown kernel/output feature bytes now rejected |
| CoinView | FIXED | Missing output returned `DEFAULT`; now errors |
| API | PASS review | v1/v2 handlers present; shutdown is local-only and unauthenticated |
| Seed/TxPool | PASS review | Seeder target tests, pool interfaces reviewed |
| Wallet | NOT VERIFIED | Interface-level review only; no live wallet run in this audit |

## Findings

### Fixed

1. **F-A1 (DB, high)** `PeerDB::OpenDB` called `fs::remove_all(DB/PEERS)` on every
   startup. Peers, ban state and last-seen data were lost at every restart.
   Removed the wipe; the DB is opened/created in place.
2. **F-A2 (consensus/deser, high)** `TransactionKernel::Deserialize` and
   `OutputIdentifier::Deserialize` accepted arbitrary feature bytes. Rust rejects
   unknown variants as corrupted data. Added strict ranges.
3. **F-A3 (chain, high)** `TxHashSet::ApplyBlock` used `assert` for a missing
   output leaf. In release this is UB/termination on inconsistent DB input.
   Replaced with a controlled `false`.
4. **F-A4 (DB, medium)** `DbCheckpoint::Read` dereferenced unaligned
   `uint64_t*` from a byte buffer (UB). Replaced with `memcpy` helpers.
5. **F-A5 (DB, medium)** `BlockDB::OpenDB` wiped the unused `INPUT_BITMAP`
   column on every start. Removed.
6. **F-A6 (consensus, medium)** `CoinView::GetOutputType` returned `DEFAULT` for
   a missing output, masking invalid input deserialization. Now throws.
7. **F-A7 (PIBD server, medium)** `GetKernelSegment` dereferenced a null
   `GetHashAt`; fixed (PIBD audit).
8. **F-A8 (PMMR, medium/high)** `GetHashAt` refused compacted hashes and the
   prune-ignoring fallback lacked bounds checks (SIGSEGV). Added guarded
   fallback (PMMR audit).

### Open

1. **F-O1 (consensus, high): NRD not enforced.** Rust 5.5.2 has
   `KernelFeatures::NoRecentDuplicate`, a recent-kernel index, relative-height
   and duplicate rules. C++ only parses the relative height; it does not maintain
   the index or enforce the rules. G-07 is now **OPEN (NRD)**, not “absent in
   v5.5.2”. An NRD-enabled network is not supported.
2. **F-O2 (data, blocker): G-09 live serving.** Compacted sibling hashes are
   missing from the C++ PIBD state; local states and sidecars cannot supply
   them. A full horizon snapshot is required (E56–E61).
3. **F-O3 (DB, low): checkpoint rename fallback** removes the old file before
   rename when Windows refuses replacement. The window is small and the file is
   checksummed, but the operation is not atomic in that fallback.
4. **F-O4 (config, low): P2P port casts** `(uint16_t)` from JSON without an
   explicit range check; values >65535 wrap.
5. **F-O5 (wallet, NOT VERIFIED):** no live wallet transaction/restore run in
   this audit; node audit results are not evidence for wallet funds safety.

## Verdict

- Node protocol/consensus/DB audit: **PASS with fixes**, except NRD (open) and
  live PIBD serving (blocked by missing data).
- No unqualified “all green” claim: G-07 and G-09 remain open/partial.
- Full suite after fixes: **187 cases / 2069 assertions PASS**.

## Artifacts

- `GrinNode 102F8702…`, `Tests FBEECC34…`;
- audit package `dist/audit-1.2.9-102F8702`.
