# PIBD server audit — C++ vs Rust grin 5.5.2

## Scope

Server side only (what our node sends to a PIBD client):

- `src/P2P/MessageProcessor.cpp` — request dispatch, flushed-header check;
- `src/P2P/PIBD/PibdSegmentService.h` — bitmap/output/rangeproof/kernel segments;
- `src/PMMR/Models/SegmentGenerator.h`, `Segment.h`, `SegmentProof.h`;
- `src/P2P/Messages/SegmentResponseMessage.h`, `SegmentRequestMessage.h`;
- `include/P2P/Capabilities.h`.

Reference: `reference/grin-5.5.2` (`chain/src/txhashset/segmenter.rs`,
`core/src/core/pmmr/segment.rs`, `p2p/src/msg.rs`, `chain/src/pibd_params.rs`,
`chain/src/txhashset/bitmap_accumulator.rs`).

## Wire and constant parity

| Item | Rust 5.5.2 | C++ | Result |
|---|---|---|---|
| Bitmap segment height | `BITMAP_SEGMENT_HEIGHT = 9` | `PibdParams::BITMAP_SEGMENT_HEIGHT = 9` | PASS |
| Output segment height | `11` | `11` | PASS |
| Rangeproof segment height | `11` | `11` | PASS |
| Kernel segment height | `11` | `11` | PASS |
| Segment request | `block_hash + SegmentIdentifier` | `SegmentRequestMessage` same order | PASS |
| Bitmap response | `block_hash + BitmapSegment + output_root` | `CreateOutputBitmap` | PASS |
| Output response | `block_hash + Segment<OutputIdentifier> + output_bitmap_root` | `CreateOutputs` | PASS |
| Rangeproof response | `block_hash + Segment<RangeProof>` | `CreateRangeProofs` | PASS |
| Kernel response | `block_hash + Segment<TxKernel>` | `CreateKernels` | PASS |
| Segment encoding | identifier, hash positions/hashes, leaf positions/leaves, proof | `Segment<T>::Serialize` / `Deserialize` | PASS (wire test) |
| Bitmap MMR leaf count | `ceil(n_outputs / 1024)` | `BuildBitmapMmr` same formula | PASS |
| Leaf required rule | own bit or sibling bit, plus final MMR leaf | `IsSegmentLeafRequired` same | PASS |
| Output/bitmap roots | `output_pmmr.root()` / bitmap snapshot root | `GetOutputMmrRoot` / `GetBitmapMmrRoot` | PASS under flushed-header check |
| Kernel segment | unpruned, `prunable=false` | `bitmap=nullptr` | PASS |

The bitmap construction was also exercised by a live client that accepted
bitmap and output segments from this server.

## Findings

### F1 (fixed): kernel hash null dereference

`GetKernelSegment` used `*m_pKernelMMR->GetHashAt(...)` without a null check.
A compacted kernel node would dereference null and terminate the process instead
of returning `MissingHash`. Fixed to return `std::nullopt`; commit includes the
rebuild. This is a robustness/correctness fix, not the cause of G-09.

### F2 (by design): service serves the installed state, not an arbitrary header

Rust `Segmenter` is created per `archive_header` and uses rewound
`output_pmmr_at(header)` plus a bitmap snapshot. C++ `PibdSegmentService` reads
the currently installed `TxHashSet` PMMRs directly. `MessageProcessor` only
serves when the requested block hash equals `GetFlushedBlockHeader()->GetHash()`;
a request for another horizon is ignored (logged, no wrong data).

Consequence: serving a different historical horizon would require installing
that state first. This is a deliberate limitation, not a wire mismatch.

### F3 (data, not protocol): compacted hashes are missing

On a PIBD state produced by `PibdStateWriter`, segment generation can throw
`Source is missing the sibling hash at position ...`. The same request against
a Rust node succeeds because Rust's PMMR backend keeps the hashes that a compacted
C++ hash file drops. A sidecar of all received segment hashes (6,015,442 entries
per tree) still did not contain the specific compacted position.

Therefore the failure is in **state retention**, not in the segment/response
format. Details and evidence: `TEST_RESULTS.md` E56–E61.

## Verdict

- Server protocol/serialization/constants: **PASS** (static comparison, wire
  round-trip, live client accepted bitmap/output segments).
- Kernel null robustness: **fixed and rebuilt**.
- Live PIBD serving on this node's compacted state: **NOT VERIFIED / BLOCKED**
  by missing compacted sibling hashes. It cannot be promoted to PASS until a
  full horizon state (or the missing hashes) is available.

## Evidence

- `Test_PibdServing` wire round-trip and live-state tests;
- `Test_PibdServing::ServesStateThatValidates`;
- full suite 188 cases / 2131 assertions PASS;
- artifacts `GrinNode A531C2C9…`, `Tests 08E89008…`.
