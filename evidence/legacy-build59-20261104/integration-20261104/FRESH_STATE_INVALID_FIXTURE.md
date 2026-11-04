# Fresh-state attempt with patched build-59 — invalid fixture

Date: 2026-11-04 ~09:39–09:41.

Attempt: stop the running patched node, copy `cpp2` profile to `cpp3`, remove
`NODE/TXHASHSET` while retaining `NODE/CHAIN`/`NODE/DB`, then start patched
`GrinNode.exe` to force a fresh state request from the loopback Rust peer.

Result: **FAIL — fixture construction invalid**, not classified as a patch
regression.

```text
Load: NODE\TXHASHSET\... does not exist. Creating it now.
...
Exception thrown: GetDataAt: Failed to read data at position 6709210
```

Reason: the copied chain/DB still references an activated state at height
3,318,655, while TXHASHSET was deleted. The node therefore tried to read an
empty/incomplete kernel MMR at the persisted tip. This is not a clean
header-only profile and does not exercise download→validation.

Therefore:

- patched binary on the **complete** activated profile: `FULLY_SYNCED` and
  restart PASS (see `PATCHED_LOCAL_20261104.md`);
- fresh download/validation/activation on patched SHA: **NOT VERIFIED**;
- a valid fresh profile must be prepared separately (header-only profile or
  chain reset with a documented checkpoint), then run for the expected
  20–30 min range-proof validation.
