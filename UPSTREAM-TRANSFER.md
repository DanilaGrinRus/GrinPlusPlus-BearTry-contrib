# Upstream transfer guide — BearTry → GrinPlusPlus

This fork exists **only** to prepare small, reviewable contributions to
[`GrinPlusPlus/GrinPlusPlus`](https://github.com/GrinPlusPlus/GrinPlusPlus).
It is a true GitHub fork (`forked from GrinPlusPlus/GrinPlusPlus`).

The canonical project repository remains
[`DanilaGrinRus/GrinPlusPlus-BearTry`](https://github.com/DanilaGrinRus/GrinPlusPlus-BearTry);
its address, history, tags and release are not changed by this fork.

## Branch model

| Branch | Base | Purpose |
|---|---|---|
| `master` | upstream | untouched upstream default branch; PR base |
| `bear-main` | BearTry `main` (`c8501256…`) | full BearTry history, read-only reference for cherry-picks |
| `topic/NN-*` | `upstream/master` | one focused change per branch, ready for a PR |
| `contrib/guide` | `upstream/master` | this guide |

## Transfer order

Do not send one large PR. Port and review in this order:

1. **Small independent fixes (wave 1)**
   - `topic/01-bytebuffer-overflow` — ByteBuffer 64-bit length overflow (security).
   - `topic/02-p2p-compat` — capability bits / message IDs vs grin 5.5.2.
   - `topic/03-handshake-genesis` — handshake genesis + archive send checks.
   - `topic/04-accept-loop` — keep inbound accept loop alive after a rejected peer.
   - `topic/05-dns-retry` — retry DNS discovery when no candidate is available.
   - `topic/06-slow-peer-rotate` — rotate a slow txhashset peer instead of banning.
   - `topic/07-db-backup` — backup before schema migration.
   - `topic/08-capability-advertise` — advertise real capabilities, configurable P2P port.
   - `topic/09-pow-edge-bits` — PoW edge-bit / segment-limit validation.
   Only branches that apply cleanly to current `upstream/master` exist. Others need a
   rebase/port first (see table below).

2. **Robustness** — chain-store consistency/checkpoint, rewind/fail-closed, recovery.
   These depend on wave 1 and on each other; prepare a small series.

3. **API** — access levels, owner auth, method additions; document schema deviations.

4. **Large architecture (separate design document first)** — PIBD, KernelIndex,
   multi-peer scheduler, worker policy. Do not submit as a bulk patch.

5. **Not for upstream** — FastInit (experimental), PIHD (not implemented), local audit
   tooling.

## Porting commands

```bash
git fetch contrib
git fetch upstream-gh master
git checkout -b topic/my-change upstream-gh/master

# Option A: cherry-pick from the BearTry reference branch
git cherry-pick -x <commit-from-bear-main>

# Option B: apply a focused patch exported from BearTry and commit with context
git apply /path/to/change.diff
git add -A && git commit -m "…"

git push contrib topic/my-change
```

Open a PR only after explicit approval:

- base repository: `GrinPlusPlus/GrinPlusPlus`
- base branch: `master`
- head repository: `DanilaGrinRus/GrinPlusPlus-BearTry-contrib`
- head branch: `topic/...`
- attach sanitized evidence (no profiles, logs with local paths, wallets or secrets).

## Status of the first wave against `upstream/master`

| Topic | Source (BearTry dev commit) | Applies cleanly | Branch |
|---|---|---|---|
| ByteBuffer overflow | `fc9e988` | no — tests CMake context differs; needs small rebase | not created |
| P2P capability/message IDs | `a5c88bf` | no — message table conflict | not created |
| Handshake genesis / archive send | `e5287f2` | **yes** | `topic/03-handshake-genesis` |
| Accept loop | `bfc31a0` | **yes** | `topic/04-accept-loop` |
| DNS retry | `b37caa0` | no — Seeder context differs | not created |
| Slow peer rotation | `bbf9a56` | **yes** | `topic/06-slow-peer-rotate` |
| DB backup | `9c6bb34` | no — BlockChainImpl context differs | not created |
| Capability advertise / P2P port | `72c94f2` | no — config context differs | not created |
| PoW edge bits | `9d9f1ae` | no — file was renamed upstream | not created |

Branches that exist were built by applying the code-only diff of the source commit onto
`upstream/master` and committing with the original author/message plus a
`Prepared-from:` trailer.
