# G-09 stage 1: controlled P2P + Rust validation — PASS (fixed H)

Status: **PASS for the controlled wire-audit run at one fixed H.**
This is not yet full G-09 live acceptance (no ordinary Rust node state activation).

## Result

```
PASS bitmap=7/7 output=1673/1673 rangeproof=1673/1673 kernel=1637/1637
```

Total segments: **4990**.
Client: independent Rust 5.5.2 P2P client with a custom `NetAdapter`; it performs a real
handshake, requests every segment, deserializes the actual wire bytes with
`grin_core`/`grin_chain`, and validates each segment against the header roots.
C++ server: Grin++ 1.2.9 on loopback, serving its flushed state.

The identity pass additionally checks, for every response:

- response `block_hash` equals the requested H;
- `segment.id()` equals the requested `SegmentIdentifier`;
- output responses carry the bitmap root computed from the accumulated bitmap;
- all roots are checked with the Rust validator.

## Fixed state

- Height: **3,315,600**
- Block hash: `3615bb625917582f7d028fe4ff738d82cb939b71ac768e885519b8798544ec9f`
- output_mmr_size: **6,851,889**
- output_root: `fc2071977b1e3f4a71f40a9200a142b99b6424cafefb701bf671905b72f1dcbc`
- range_proof_root: `6bdb5037bfb2947bad1aff3ef168b70f2518bdf3133385ea8b92d2cfe33c03c0`
- kernel_mmr_size: **6,703,071**
- kernel_root: `60ab2c8838c33e57886d7c2aa2cb79b52fcfef6449eb195a276d8407b19a29a5`

Counts differ from the audit's pp-ckpt H=3,317,027 (1674/1674/1638) because this is a
different state one horizon later: output 1673, rangeproof 1673, kernel 1637.
Do not report this as 4993 or as the audit H.

## Artifacts

- Post-guard C++ GrinNode SHA-256: `0374EC4F6C8B0DEC1444E1AAA4994279EC12D02504AF09334BA99FB4F5B8B5E1`
- Rust wire client SHA-256: `A2561FE6E2AC4CAD76AF73DAA02BCE2972CA00E086853332BCB590289FB6B1D9`
- Post-guard full identity run: `C:\DHW\.tmp\pibd-wire-still2.log`
- Earlier identity run: `C:\DHW\.tmp\pibd-wire-full-identity.log`
- Client binary: `C:\Users\nurli\AppData\Local\Temp\pibd-wire-audit\target\release\pibd-wire-audit.exe`
- Header JSON used for roots: `C:\DHW\.tmp\interop\block3315600.json`
- Quiescent test server config: `C:\DHW\.tmp\interop\server_config-wire-still.json`
- C++ server log: `C:\DHW\.tmp\interop\pp-pibdnow\.GrinPP\FLOONET\LOGS\Node.log`

## Important limitations

1. Header roots were read from C++ REST block JSON (`/v1/blocks/3315600`), not independently
   fetched from a Rust header chain. Identity and segment validation passed, but root
   provenance still needs an independent header source. This is the main remaining caveat.
2. This does not exercise ordinary Rust node `awaiting_peers`/state activation.
3. The post-guard run used a **quiescent** C++ state (external peers disabled), because the
   API-sequence guard intentionally refuses a response if the state changes during assembly.
   Serving while blocks/reorg are actively changing needs a dedicated concurrent test.
4. Negative mutation test (flip proof byte / wrong root) was not run in this pass.
5. An earlier full run hit a transient C++ connection reset at kernel ~403; the clean rerun
   completed 1637/1637, so this needs separate crash/fault investigation.
6. NRD must be enabled in the Rust client (`global::init_global_nrd_enabled(true)`); without
   it, kernel segment 296 fails with `Serialization(CorruptedData)`. This was a client test
   configuration issue, not a C++ wire defect.
7. The run used a fixed H served by C++; it did not exercise normal Rust horizon selection.

## Verdict

**C++ PIBD serving: P2P + identity-checked Rust validation at fixed H=3,315,600 — PASS.**
**Independent header-root confirmation — NOT VERIFIED.**
**G-09 full live (ordinary Rust node activation) — NOT VERIFIED.**
