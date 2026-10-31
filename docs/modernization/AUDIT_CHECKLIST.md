# AUDIT_CHECKLIST — handover checklist

Status: **open**; items are checked only with evidence. G-51 remains IN PROGRESS
until every line is PASS or explicitly accepted as a limitation.

## A. Build and tests

| # | Item | Evidence | Status |
|---|---|---|---|
| A1 | Clean offline build with `/W4 /WX` | `BUILD.md` (current: 171 cases / 1839 assertions) | PASS |
| A2 | Full unit suite on the final binary | `Tests.exe`, `TEST_RESULTS.md` | PASS (rebuild after the last code change needed) |
| A3 | No new compiler warnings | build log | PASS |
| A4 | Final artifact SHA-256 | `BUILD.md`, this package | PENDING (final rebuild) |
| A5 | Reproducible build entry point (offline sysroot) | `tools/build/build.ps1` | PASS |
| A6 | CI wiring + vcpkg pin | G-24 partial, phases 5–7 | NOT DONE |

## B. Functional evidence

| # | Item | Evidence | Status |
|---|---|---|---|
| B1 | P2P handshake / capabilities | E30/E33, `Test_P2PCompat` | PASS |
| B2 | PIBD wire, bitmap, horizon, roots | E31/E32/E35 | PASS |
| B3 | PIBD download → activation → validation | E32/E34/E42 | PASS |
| B4 | Full cycle to `FULLY_SYNCED` | E42 | PASS |
| B5 | Resume replay after restart | E41/E42 | PASS |
| B6 | DB checkpoint / no-loss repair | E43/E44/E48 | PASS |
| B7 | Atomic legacy activation | E49, `Test_TxHashSetStaging` | PASS |
| B8 | Full sync from zero timing | `pp-archsrv`, in progress | IN PROGRESS |
| B9 | Live archive serving (G-52) | same run, archive/legacy test | NOT VERIFIED |
| B10 | Live legacy fallback (G-41) | depends on B9 | NOT VERIFIED |
| B11 | Live PIBD serving to a Rust client (G-09) | unit only | NOT VERIFIED |

## C. Compatibility and safety

| # | Item | Evidence | Status |
|---|---|---|---|
| C1 | Message table/capabilities vs 5.5.2 | E30/E33 | PASS |
| C2 | Deserialization bounds | E19/E44/E50, `Test_ByteBuffer` | PASS |
| C3 | Parser limits (G-20) | E50, `Test_Segment` | PASS |
| C4 | PoW edge-bit validation (G-04) | E50, `Test_PoWEdgeBits` | PASS |
| C5 | FraudHeight rule (TD-only) | E48 | PASS |
| C6 | No timeout bans / peer rotation | E37/E42 | PASS |
| C7 | fsync ordering | E51 | PASS (directory fsync unavailable on Windows) |
| C8 | Hash-level MMR revalidation | G-23 partial | NOT DONE |
| C9 | NRD | G-07 deferred | ACCEPTED LIMITATION |
| C10 | Mainnet dry-run | G-45 planned | ACCEPTED LIMITATION (out of scope) |

## D. Documentation and package

| # | Item | Evidence | Status |
|---|---|---|---|
| D1 | `GAP_MATRIX` 52 entries, tally correct | 41 closed + 3 partial + 2 in work + 1 open + 3 planned + 1 deferred + 1 N/A | PASS |
| D2 | `AUDIT_DELTA.en/de/ru` parity | `check_audit_delta.ps1`: 29 cards, 8 fields, 18 diff blocks, 0 TODO | PASS |
| D3 | Comment review | `AUDIT_COMMENTS.md` | PASS (static/priority) |
| D4 | Secret/dump scan | `AUDIT_COMMENTS.md` §5 | PASS |
| D5 | License/notices | repository LICENSE/COPYING | PENDING |
| D6 | `AUDIT_README` in EN/DE/RU | not written | PENDING |
| D7 | Known-limitations section | `REMAINING.md`, this checklist | PASS |
| D8 | Final artifact + provenance hashes | after final rebuild | PENDING |
| D9 | Audited package `dist/audit-*` | after D8 | PENDING |

## E. Sign-off

| Role | Name | Date | Result |
|---|---|---|---|
| Author/engineer | DSH C++ Grin++ work | 01.11.2026 | partial (see pending) |
| Independent reviewer | — | — | not assigned |

**Handover rule:** no item marked PASS without a reproducible command or an
evidence file; "works on my machine" and a successful build alone are not PASS.
