# API integration kit — compatibility audit

## Artifact

- Kit: `C:\Users\nurli\Downloads\GrinPlusPlus-API-Ready-Integration-Kit.zip`
- SHA-256: `6CC1732C0FB44283A39633A4AB7C0CBEC925746BDAC45F65C62679B484050574`
- Kit base: `baeeac3a508bb5f96eba43793c6e4efa948707b0`
- Kit delivery claims: API implementation ready; 207 cases / 6352 assertions
  **claimed, logs not supplied**; PIBD/PIHD/wallet live interop explicitly NOT
  VERIFIED.

## Applicability

- 65 changed/new paths; **0 are byte-identical** to the current tree.
- 15 files are completely absent here: `ApiAccess`, `KernelIndex`,
  `KernelIndexState`, API concurrency/pagination/kernel/config tests.
- `git apply --check` of the cumulative patch fails at
  `tests/src/Core/CMakeLists.txt`; this is a separate branch, not a merge-ready
  patch.
- The kit itself says: do not overlay `ChainState`, `PMMR`, `P2P`; merge with
  semantic conflict resolution and rerun PIBD regressions.

## Critical API differences found

### A1 (security/compat, high): foreign API exposes admin config write

Current `src/API/Node/NodeServer.cpp` registers `update_config` on
`/v2/foreign`. The kit removes it from foreign and registers it on
`/v2/owner` (together with `get_config`). A foreign/read-only client can
currently attempt a state-changing admin operation; this is both a security
issue and a route-parity mismatch.

### A2 (security, high): owner/admin endpoints have no secret authentication

The kit adds `Net/Servers/RPC/ApiAccess`:

- `.api_secret` 64-hex token, owner-only file/DACL checks, fail-closed;
- `Authorization: Bearer <secret>` or `X-API-Secret`;
- PUBLIC / PRIVATE / ADMIN levels enforced centrally by `RPCServer`;
- loopback Host/Origin checks;
- `/v1/shutdown` requires ADMIN.

Current tree has no `ApiAccess`, no per-method access level and no auth check for
owner methods or `/v1/shutdown`. Binding to `EServerType::LOCAL` is not
authentication.

### A3 (API behavior, high): `get_config`/`update_config` missing from owner

Current `/v2/owner` has only `get_status`, `get_peers`,
`get_connected_peers`; the kit owner also has `get_config` and `update_config`.
Clients using the owner config methods get “method not found”.

### A4 (API correctness, high): no chain-bound KernelIndex

The kit adds `KernelIndex` + `KernelIndexState` (tip hash/height, kernel root,
count, checksum, generation-based rebuild, apply/rewind in chain batch, verified
lookups). Current `get_kernel` handler has no such index; behavior under reorg,
PIBD activation and restart differs. Kit also adds pmmr-position and unspent
pagination tests.

### A5 (durability, high): config update is not transactional

Current `UpdateConfigHandler` mutates runtime fields first and then writes the
JSON with `ofstream(..., ate)`. The kit uses one
`Validate → Persist → Apply → Publish` transaction with pre-rename verification,
backup, rollback, flush and canonical read-back (F-01/F-02/F-02R). A crash or
write failure can leave runtime and disk divergent.

### A6 (concurrency, medium/high): self-lock/ABBA fixes absent

Kit reports fixes for `ChainState` write-lock re-entry (`OpenTxHashSet`),
API/BlockDB lock ordering, TxHashSet/PMMR synchronization and shared PMMR read
lease for PIBD serving. Current tree still has the older `ChainState` structure;
no equivalent test suite.

### A7 (restart, medium): height-zero genesis check

Kit changes height-zero startup to check the block DB instead of treating
`candidateHeight == 0` as always fresh. Current `ChainState::Create` still
re-adds genesis when candidate height is zero; needs a restart regression test.

### A8 (API hardening, medium): pagination/numeric/position limits

Kit handlers add pagination, numeric-safety and PMMR-position bounds tests
(`Test_UnspentPagination`, `Test_NodeApiPmmrPositions`). Current tree has no
equivalent tests; handler behavior may differ on large inputs.

## What the kit does **not** fix

- G-09 PIBD serving: kit explicitly says it does not fix segment/Merkle proof
  generation and should not be marked as solving it.
- PIBD/PIHD live interop, wallet operations, Linux runtime: NOT VERIFIED.
- The claimed 207/6352 build/test logs are not in the kit.

## Recommendation

Do not apply the overlay. Port in this order:

1. **P0 route/security** — move `update_config` to owner, add owner
   `get_config`, import `ApiAccess`, enforce PUBLIC/PRIVATE/ADMIN in
   `RPCServer`, protect `/v1/shutdown`, add auth tests.
2. **P1 durability/concurrency** — config transaction (F-01/F-02/F-02R),
   chain-state lock ordering, TxHashSet/PMMR synchronization.
3. **P2 API semantics** — KernelIndex and pagination/position limits with the
   kit tests adapted to this tree.
4. Full build/tests plus PIBD serving regression after each step.

P0 (routes + ApiAccess + protected shutdown) is merged and live-verified.\nP1 config transaction, read-lock and genesis guard are also merged. Remaining:\nstate-epoch/ABBA synchronization and P2 KernelIndex/pagination. G-53 remains\n**PARTIAL**.

## Status schema compatibility (2026-11-04)

- Contract: [API_STATUS_SCHEMA_2026-11-04.md](API_STATUS_SCHEMA_2026-11-04.md).
- `/v1/status` keeps legacy fields and adds the Rust `Status` object under
  `rust` plus `tip`/`connections`/`chain_type`/`sync_status_rust` aliases.
- `/v2/owner get_status` uses the same live builder (`NodeStatus::Build`).
- Verified on patched build-59 `GrinNode ECB1B55E…`: `/v1/status.rust` and owner
  `Ok` match on chain/tip/sync/connections/protocol/user_agent; owner accepts
  Bearer and X-API-Secret, no secret gives expected `-32001`; foreign
  `get_version`/`get_tip` work. Full suite **280/280, 4403 assertions PASS**.
  Evidence: `evidence/legacy-build59-20261104/integration-20261104/API_CHECK_20261104.md`.
- Public Floonet API and fresh single-SHA state cycle remain **NOT VERIFIED**.
