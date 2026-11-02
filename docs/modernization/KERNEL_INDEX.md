# KernelIndex / get_kernel fast path

## Design

- Persistent `excess -> OutputLocation` index in the existing RocksDB (`default`
  column): `KERNELPOS/<generation>/<excess>` entries plus one
  `KERNELIDX/STATE` record.
- The state record binds the index to:
  - confirmed tip hash and height;
  - kernel root;
  - kernel count and entry count;
  - a commutative XOR checksum of every `(excess, position, height)` entry.
- A rebuild writes a new generation, commits it, reads it back, and only then
  publishes the state record. A crash before the state write leaves the previous
  generation authoritative.
- `BlockChain::Create` calls `EnsureKernelIndex` best-effort: a missing index is
  logged and `get_kernel` falls back to the bounded direct scan. The node must
  not fail to start because an optimization could not be built.

## get_kernel

`GetKernelHandler` first queries the persistent index. It trusts the entry only
when the state matches the current tip hash/height, kernel root, kernel counts
and TxHashSet kernel count. The indexed kernel is read back and its excess is
compared before returning. Any mismatch falls back to
`TxHashSet::FindKernelByExcess`, which uses 1-based PMMR positions (N-06).

## Tests

- `KernelIndex is chain-bound and detects corrupted mappings` (48 assertions):
  state binding, missing excess, corrupted mapping repaired into a new
  generation.
- `KernelIndex ignores an interrupted unpublished rebuild`.
- `KernelIndex rebuilds when the state record is stale`.
- Full suite: **194 cases / 2224 assertions PASS**.

## Artifacts

- `GrinNode D15174D9…`
- `Tests 1D38C73B…`

## Limits

- Live RPC and reorg paths were not separately exercised against a running Rust
  peer.
- The index is rebuilt at startup; after a PIBD state activation without a
  restart, `get_kernel` may use the direct-scan fallback until the next
  `EnsureKernelIndex` call.
