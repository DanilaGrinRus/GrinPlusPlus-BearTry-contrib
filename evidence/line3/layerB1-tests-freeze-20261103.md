# Layer B1 tests freeze — task-15 (tests-only, no build yet)

Status: **FROZEN FOR build-17 REVIEW; NOT COMPILED/RUN (no build order).**
Only tests changed; no production source, line1 profile, or build16 touched.

## Frozen files

| File | SHA-256 |
|---|---|
| `tests/src/P2P/Test_PibdNegativeAndLimits.cpp` | `993CB409F1954AA649B0C1F016DCF7C94B7D51A1C4A3CAE90D50B4F9BE331F20` |
| `tests/src/P2P/CMakeLists.txt` | `11DBEFB5356456BE7C83FD869EE1A8D2235ED830EEA3791F1BABA2FA70D4C3C2` |

Own subtree CMake; root tests CMake untouched.

## Cases / fixtures

1. `PibdWireNegative::TruncatedSegmentIdentifier` — 4-byte height+partial
   idx; expect `DeserializationException`.
2. `PibdWireNegative::IdentifierHeightOutOfRange` — height 33; expect typed
   rejection.
3. `PibdWireNegative::IdentifierOffsetOverflow` — height 9, idx UINT64_MAX;
   `LeafOffset()` must throw.
4. `PibdWireNegative::NonSegmentResponseType` — 32-byte block hash + Block
   type; parser must refuse non-segment response.
5. `PibdWireNegative::TruncatedRequestHash` — 8-byte body; request parser must
   refuse before reading a 32-byte hash.
6. `PibdResource::SingleFlightAndExactHash` — Registry enabled, expected hashA,
   one preparation; exact hashA lease only, hashB refused.
7. `PibdResource::ConcurrentLeasesDuringHorizonSwitch` — held hashA lease
   survives SetExpected(hashB); 8 threads ×50 exact-hash probes receive no
   stale/wrong leases; no ready advertise after switch.

## Coverage gaps / production needs

- New file does **not** construct a wrong proof/root archive; existing
  `PibdArchiveServer::ExistingArchiveHeaderRootsValidated` (F13) and
  `PibdDownload` mismatch cases cover parts. A dedicated wrong-proof/root
  fixture still needed.
- Per-connection/global request budgets and RSS/disk boundedness need
  production test hooks or process-level harness; Registry covers single-flight
  and exact-hash refusal only.
- Tests are expected to pass against current code by inspection; they are
  guard/regression tests, **not red-before**. Failure-before/after is not
  measured because no build was run.
- Build-17 inclusion requires compile/test; if compile fails, tests are not
  frozen valid.

## Next

Build slot → run the 7 cases + full suite; report actual/expected and any
production change required. No production source edits.

Layer A remains waiting: full public sync still `DOWNLOADING_TXHASHSET`; no
restart yet.
