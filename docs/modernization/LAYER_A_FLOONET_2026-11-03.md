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

## A4 — exceptions and limits (closed with explicit exclusions)

- **Independent second public node — PASS:** separate profile `task14-a4-retry`,
  P2P 13514 → public 13414, REST 25550; 6 distinct real Rust peers, caps 95/223,
  UAs 5.4.0/5.5.1-alpha/5.5.1/5.5.2, 0 banned; restart resumed at 367,616 (not 0)
  and progressed to 529,408; owner no/wrong token → -32001, valid Bearer → OK,
  foreign get_tip/get_version → 200; non-loopback 192.168.1.33:25550 refused.
- **ASan F13/archive — PASS:** ASan TestsMini link set extended with StateSyncer
  dependencies (ASan-only script), source manifest `C1C321E9…`, TestsMini
  `C608090B…`; F13 1/38 and PibdArchiveServer 9/225 under ASan, teardown PASS,
  **0 ASan reports**. Normal `/W4 /WX` build unchanged.
- **Public PIBD serving — NOT VERIFIED, safely disabled:** serving remains default
  off, no PIBD/PIHD bits advertised; public serving was not exercised. This is an
  explicit exclusion and must not be described as a tested public service.
- **Bodyless — FAIL/BLOCKED, not PASS:** the test-only NRD seam now reaches the
  real TestServer/BlockDB/OpenExisting/reorg path and exposed a real bug:
  `TransactionKernel::IsCoinbase()` treated NRD (features=3) as coinbase by a
  bitwise test. Exact-match fix + regression PASS. After that fix, phase 3
  `OpenExisting()` after the reorg hangs in startup truncation (`Confirmed chain
  is ahead of the candidate chain (33 > 31)`); the test is hidden (`[.]`) as a
  reproducer. Default full suite is **272/272**, but this is not bodyless PASS.
- **RSS/resource observation:** peak RSS ~**9.7 GB** during public state validation,
  falling to ~2.8 GB after sync; no leak claim either way. A practical minimum for
  this validation profile is ≥12 GB free RAM; archive-serving resource limits are
  not measured.
- **Legacy public fallback — BLOCKED/PARTIAL:** public legacy txhashset was
  ~0.46 MB/s and was abandoned at ~501 MB; a fresh PIBD-off public profile stopped
  after headers 27,648. Ordinary PIBD client completed (Layer A A2). Legacy
  fallback remains supported in controlled/fixture runs but was not proven
  end-to-end public-to-public.
- No wallet/UI were used; no mainnet, no real funds, no secrets logged.


## Evidence

- `C:\DHW\Gplus\work\runs\task13-public-floonet\evidence`: pre/post status, diagnostics,
  restart window, peers, API probes.
- `evidence/line3/layerA-A3-live-20261103.md`, `layerA-public-sync-20261103.md`,
  `layerA-A2-public-pibd-validation-20261103.md`.
- Pre status hash `17c9300b…b4eb5b`; post status hash `1dc25a89…c906a41`.
- No GrinNode/G09 process remains.

## Layer B relation

B1/B2 hardening tests (build-18) are separate. B3/B4/B5 residuals
(bodyless/kill, DB corruption/power-loss, clean-checkout/CI/Mainnet dry-run)
remain NOT VERIFIED and are not implied by this A PASS.
