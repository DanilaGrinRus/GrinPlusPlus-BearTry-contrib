# AUDIT_COMMENTS — code comment review

Status: **automated scan + priority manual review complete; repo-wide line-by-line
review is not claimed.** Completed 01.11.2026 as part of `AUDIT_PACKAGE.md` §4
and `GAP_MATRIX` G-51. Scope: the 112 source/test files changed since `5f511e6`.

## 1. Rules applied

1. Code comments are **English only**; a localized comment is a defect.
2. New files carry a header: purpose, Rust reference (`file:line`), invariants.
3. Constants copied from the reference state their source and formula.
4. Every deviation from the reference is explained and recorded in `GAP_MATRIX`.
5. A temporary workaround states its removal condition.
6. Comments explain **why**, not what; obvious statements are removed.
7. Stale claims ("never advertised", "only once") are treated as defects.
8. The auditor should not need the Rust tree to understand a constant or branch.

## 2. Automated scan (all changed files)

| Check | Result | Verdict |
|---|---|---|
| Cyrillic/localized comments | **0** | PASS (`// ...` and `/* ... */`) |
| Hard-coded `C:\DHW` paths in code | **0** | PASS |
| `TODO`/`FIXME`/`HACK`/`XXX` in changed files | **0** | PASS (8 triaged below) |
| Commented-out code in changed files | **0** | PASS (dead ChainState block removed) |
| Secret/credential literals | **0** | PASS (131 pattern hits are identifiers/key names/include paths) |
| Debug dumps / temporary prints | **0** added | PASS |

Repo-wide (703 code files, not only the diff): **74** inherited `TODO`s remain,
mostly wallet/API code from 1.2.9; localized comments **0**, hard-coded paths
**0**. The relevant inherited item is `BlockValidator.cpp` NRD (G-07), already
tracked as deferred; it is not introduced by this work.

## 3. Triaged TODOs removed or rewritten

| File | Before | Action |
|---|---|---|
| `src/BlockChain/ChainState.h` | commented-out writer/reader block (13 lines) | removed |
| `src/PMMR/TxHashSetImpl.cpp` | `// TODO: Implement` in `Compact()` | documented as an explicit no-op (append-only PMMR) |
| `src/P2P/Messages/MessageHeader.h` | `// TODO 4x the limits` | explained as the grin 5.5.2 forward-compat multiplier |
| `src/PMMR/TxHashSetManager.cpp` | snapshot copy TODO | documented per-height snapshot reuse |
| `src/P2P/ConnectionManager.cpp` | broadcast-to-8 TODO | documented `MAX_PEERS` bound |
| `src/P2P/MessageProcessor.cpp` | send-ban-reason TODO | documented that 5.5.2 has no wire field |
| `src/BlockChain/BlockChainImpl.cpp` | snapshot reader TODO | documented the read-only batch |
| `src/BlockChain/BlockChainImpl.cpp` | catch/delete-NODE TODO | points to E48 recovery; fatal by design, no data deletion |

`ServerAPI.cpp` `xxx` was an API-documentation placeholder, not a TODO.

## 4. Priority manual review

| Area | Files | Result |
|---|---|---|
| `src/P2P/PIBD` | 13 | headers/constants reference Rust and checked arithmetic |
| `src/P2P/Sync` | 6 | ban/timeout rules documented (G-38), no stale fraud claim |
| `src/BlockChain` + recovery | 8 | checkpoint/recovery invariants aligned with `DB_CONSISTENCY.md` |
| P2P wire + `Capabilities.h` | ~20 | message caps and PIHD "detected, never advertised" accurate |
| `include/Core/Serialization/ByteBuffer.h`, `Segment*` | 6 | bounds/limits documented (G-19/G-20) |
| `src/PMMR`/`include/PMMR` | 17 | limiter comments match the implemented checks |

## 5. Secret scan

Patterns searched in changed files: `seed`, `mnemonic`, `password`, `passphrase`,
`private key`, `api_secret`, `bearer`, `token =`, `secret =`, `BEGIN ... PRIVATE KEY`,
plus base64/hex-looking literals.

Result: **no credential values**. The 131 hits are property/key names
(`ConfigProps::PASSWORD`, `GetHashedControlPassword`), include paths
(`Seed/Seeder.h`) and comments. Local `.api_secret` files are runtime data and
are not copied into source, docs or the audit package.

## 6. Limitations and acceptance

* This is a **static** review; no external auditor sign-off.
* Priority files were read manually; the remaining changed files were covered by
  the automated rules above, not line-by-line.
* DE/RU translations of this report are still pending; EN is canonical.
* G-51 stays **IN PROGRESS** until the final build/hashes and the auditor
  checklist (`AUDIT_CHECKLIST.md`) are signed off.
