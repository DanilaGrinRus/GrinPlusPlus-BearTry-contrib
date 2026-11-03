# L-B1 negative wire inventory — PIBD proof/root/segment-ID

Status: **STATIC READ-ONLY INVENTORY (no source/build/live access)**.
Build/source HEAD `921bcfd…`; source clean. Evidence-only.

## 1. Wire surface

| Wire item | Parser | Current tests |
|---|---|---|
| Segment request: `block_hash` + `SegmentIdentifier(height u8, idx u64)` | `SegmentRequestMessage::Deserialize` | `P2PCompat::PibdIds`, roundtrip vectors; no malformed request test |
| Segment response variants 22/24/26/28 | `SegmentResponseMessage::Deserialize` | `PibdSession::EndToEndMessageFlow`, `RejectsMismatchedPayload`; `PibdDownload` out-of-order/duplicates; `PibdSync::BadSegment…` |
| `Segment<T>` payload | `Segment<T>::Deserialize` | positive roundtrip/FullSweep; no truncation/bounds negative |
| Bitmap segment | `BitmapSegment::Deserialize`, `BitmapSegmentAssembler` | `PibdLiveVector::BitmapSegmentReconstructs`, `PibdOracleVector`; positive only |
| Archive header/root | `TxHashSetArchiveMessage`, `PibdArchiveServer::LoadAndValidate` | `ArchiveZipExtractsAndValidates`, F13 `ExistingArchiveHeaderRootsValidated` |
| PIBD/PIHD IDs | `MessageTypes` | `P2PCompat::PibdIds` (IDs only) |

## 2. Negative matrix (untested / partial)

| ID | Case | Expected fail-closed | Coverage / gap |
|---|---|---|---|
| N1 | Invalid segment response message type | deserialize throw, no state change | Parser switch throws; **no direct test** |
| N2 | Truncated segment body / header / payload | typed exception, no partial segment | **GAP** |
| N3 | `SegmentIdentifier.height` > max / idx out of range | reject before request/assemble | Assembler checks idx; **no wire-level range test** |
| N4 | Wrong tree variant (e.g. Kernel payload under RangeProof type) | type mismatch reject | `PibdSession::RejectsMismatchedPayload` partial |
| N5 | Segment out of order | reject/reschedule | covered (`PibdDownload::RejectsSegmentsOutOfOrder`) |
| N6 | Duplicate same ID with different payload | reject/ignore, no overwrite | partial (`DuplicatesAndMismatchedTrees`) |
| N7 | Bitmap chunk count / bit count mismatch | reject | assembler checks; **no malformed-wire test** |
| N8 | Wrong output bitmap root / wrong proof root | reject, do not mark tree complete | Snapshot/StateWriter positive root checks; **no segment-wire negative** |
| N9 | Extra/missing leaves vs segment range | reject, no truncation/accept | **GAP** |
| N10 | Bad leaf hash size / pruned node / MMR size mismatch | typed reject | StateWriter/Snapshot tests partial; **no wire proof test** |
| N11 | Response block hash/ID differs from request | reject, no state mutation | `PibdSession` mismatch partial; **direct wire vector gap** |
| N12 | Malformed request (unknown type/hash/height) | reject/close, no crash | **GAP** |
| N13 | Archive zip truncated/corrupt/missing entry | reject, no publish/serve | covered (`ArchiveZipExtractsAndValidates`) |
| N14 | Container-valid archive with wrong roots/height | not ready/regenerate | covered build15 F13 (normal); ASan link-boundary |
| N15 | Oversized count / huge allocation | bounded reject | no wire count-cap negative test; **GAP** |
| N16 | Empty/zero-leaf segment with nonzero root | reject | **GAP** |

## 3. Minimal fixture/test plan (proposed; no source edits now)

Use raw wire bytes and the existing `SegmentResponseMessage::Deserialize`,
`Segment<T>::Deserialize`, `BitmapSegment`/assembler, and `PibdSession` seams.

- `PibdWireNegative::InvalidResponseType` — type 23/25/27/29 payload as segment
  response; expect typed exception.
- `PibdWireNegative::TruncatedBody` — truncate after hash / count / one leaf;
  expect exception, no partial object.
- `PibdWireNegative::IdentifierRange` — height 0/255, idx UINT64_MAX; expected
  policy reject.
- `PibdWireNegative::WrongTreePayload` — valid other tree bytes + wrong type.
- `PibdWireNegative::BitmapChunkMismatch` — wrong chunk count and bit count.
- `PibdWireNegative::WrongRoot` — valid segment with mutated output/rangeproof
  root; reject before marking complete.
- `PibdWireNegative::ExtraAndMissingLeaves` — count±1; reject, no truncation.
- `PibdWireNegative::ResponseIdMismatch` — request ID/hash vs response.
- `PibdWireNegative::OversizeCount` — max/adversarial count; bounded reject,
  no OOM.
- `PibdWireNegative::EmptyLeafNonzeroRoot` — reject.
- `PibdWireNegative::ArchiveWrongRoot` — F13 exists; ASan link boundary.

Pass criteria: typed exception/refusal, no partial PIBD state, no completed
tree, no archive publish, no ban unless policy says invalid; logs/IDs exact.

## 4. Limits / known exceptions

- Build15 normal F13 passes; ASan F13/archive blocked by TestsMini link set.
- No source/build/live/profile changes; no malformed wire tests added yet.
- Existing positive tests cannot substitute for negative parser tests.
