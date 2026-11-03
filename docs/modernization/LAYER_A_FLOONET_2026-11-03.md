# Layer A — limited public Floonet acceptance (2026-11-03)

Status: **PASS for the limited public-Floonet client scenario on build-16**;
public PIBD **serving** was not tested and remained disabled. This is not a
Mainnet or full G-09 readiness claim.

## A1 — candidate/build

- A public-Floonet run: build-16 `GrinNode 7E3AA29E…CA18`, `Tests 1592F3DF…44CB6`.
- Source-bound build: `/W4 /WX`, known bodyless test-only failure remains.
- Build-15/18 are later hardening candidates; results below are build-16 only.

## A2 — public Floonet sync and restart

Isolated profile/data, **public** Floonet seeds/real Rust peers, P2P 13414,
REST 127.0.0.1:25450, no loopback allowlist, no fixture, no wallet.

- Headers from zero to public tip; selected state sync used ordinary **PIBD client**
  (legacy txhashset was too slow at ~0.46 MB/s and was abandoned at ~501 MB).
- PIBD state staged/activated/validated; body sync reached **FULLY_SYNCED**.
- Pre-restart: height/hash **3,321,243 / 17c9300b3c8062d18b68c872cc629d0cdeea77c9957267bb23c22d7118b4eb5b**,
  network height 3,321,243, 3 outbound, 0 inbound.
- Restart on the same profile: resumed at 3,321,243 (not genesis), kernel index
  rebuilt, caught up only to **3,321,250 / 1dc25a89940f37bef6b5c2ff633fcef2d57edf331bb3bae9c9b4b9fc9c906a41**,
  **FULLY_SYNCED**, 4 healthy real Rust peers, 0 banned.
- Restart log: **0 TxHashSet/state re-downloads**, **0 PIBD requests**, no genesis
  reset; only header/body catch-up.
- Peer user agents: MW/Grin 5.4.0/5.5.1/5.5.2; capabilities bits 95/223; real
  public peers, no loopback-only fixture.

## A3 — safe boundaries

- REST binds only **127.0.0.1:25450**; no remote wildcard listener.
- Owner `get_status/get_peers/get_connected_peers`: no token/wrong token → `-32001`;
  valid Bearer secret → success. `get_config` PRIVATE; `update_config` ADMIN;
  foreign `get_tip/get_version` remain public.
- PIBD archive serving **DISABLED** (default); PIHD **off**. Local advertised mask
  `HEADER_HIST | TXHASHSET_HIST | PEER_LIST`; no PIBD/PIHD capability advertised.
- **Public PIBD serving: NOT VERIFIED.** The ordinary PIBD *client* path was used;
  this is not proof of public serving.

## A4 — exceptions and limits

- Known test-only bodyless failure remains; it does not affect this node run.
- ASan F13/archive path is normal-only due TestsMini link boundary; no full ASan claim.
- Public validation peak RSS observed around **9.7 GB** during state validation;
  after sync RSS fell to ~2.8 GB. This is a resource observation, not a proven leak;
  resource limits/concurrency for serving remain unmeasured.
- Public sync time was long; legacy txhashset fallback was too slow, PIBD client
  completed. The fallback path itself remains PARTIAL for public use.
- No wallet/UI were used; no mainnet, no real funds, no secrets logged.

## Evidence

- `C:\DHW\.tmp\task13-public-floonet\evidence`: pre/post status, diagnostics,
  restart window, peers, API probes.
- `evidence/line3/layerA-A3-live-20261103.md`, `layerA-public-sync-20261103.md`,
  `layerA-A2-public-pibd-validation-20261103.md`.
- Pre status hash `17c9300b…b4eb5b`; post status hash `1dc25a89…c906a41`.
- No GrinNode/G09 process remains.

## Layer B relation

B1/B2 hardening tests (build-18) are separate. B3/B4/B5 residuals
(bodyless/kill, DB corruption/power-loss, clean-checkout/CI/Mainnet dry-run)
remain NOT VERIFIED and are not implied by this A PASS.
