# Layer A A1/A3/A4 audit — build-16 public Floonet

Status: **A1 conditional PASS (artifact bound; source uncommitted); A3 PASS for
limited mode (owner auth, PIBD/PIHD off); A4 PARTIAL (bodyless exception
documented; build16 ASan not rerun). A2 full public sync/restart PENDING.**
Read-only; no build/source/profile edits. Layer B not touched.

## A1 — candidate freeze / SHA / flags

| Item | Evidence |
|---|---|
| Tests | `1592F3DF041116AC3C891E5AEEB894B6144C9AB1713893C3B265DDA1C8E44CB6` 9,542,656 B |
| GrinNode | `7E3AA29E1160244A28DDDFE3A2CCF68BD2A943A97D610B55845561453280CA18` 7,850,496 B |
| Build log | `build-console-20261103-145117.log` `95E621F0…` BUILD OK |
| Freeze | `build-16-freeze-snapshot-final.json` 1007 files, aggregate `7294EB10…` |
| Flags | C++ `/W4 /WX`; `/EHsc`; C deps `/W3` |
| Full | 262/261/1 (4281/4280), only bodyless |
| Owner tests | PASS 1/6 and 1/8 independently |

**Gap:** the A3 handler/test changes are **uncommitted** worktree changes; HEAD
is `921bcfd` (docs build-15). A1 artifact binding is PASS, but final candidate
commit/freeze is not closed until line2 commits or explicitly freezes the
worktree.

## A3 — security boundaries (limited public mode)

- Live build16: owner `get_status/get_peers/get_connected_peers` reject
  no-token and wrong-token with `-32001`; valid token succeeds.
- `get_config` PRIVATE; `update_config` ADMIN (no-auth/wrong rejected; no valid
  state-changing call made).
- Foreign public `get_tip/get_version` remain 200 without token.
- Bind exactly `127.0.0.1:25450` — no wildcard/remote API exposure.
- Config: `PIBD_ENABLED=false`, `PIHD_MODE=off`; startup log PIBD archive
  serving DISABLED (default). Public PIBD-serving is **not claimed**.
- A3 limited-mode: **PASS**. Negative wire/resource hardening for enabled
  PIBD-serving remains untested/out of this mode; do not advertise it.

## A4 — known exceptions / ASan boundaries

- Full build16: **262/261/1**, sole failure
  `Test_NrdBodylessRestart.cpp(103)` known test-only bodyless; 4281/4280
  assertions. No production/agreed-suite failure.
- Build16 ASan: **NOT RERUN** in this audit. Build15 ASan full build PASS but
  TestsMini F13/archive was link-blocked; standalone fuzz/ASan coverage has
  documented zlib/rocksdb/sysroot limitations.
- A4 is **PARTIAL/documentation-only**: exception is explicit, not replaced by
  green numbers; ASan/fuzz boundaries are stated. It is not evidence that
  build16 ASan/F13 archive path passed.
- No PIBD-serving/PIHD functionality is claimed from this run.

## A2 — public sync/restart (waiting)

- Header sync reached network tip **3,321,066**; before that: pre 2,503,680,
  post-restart 2,542,976, independent 2,613,120→2,742,144.
- At current sample: `DOWNLOADING_TXHASHSET`, chain height 0, outbound 4,
  network 3,321,066. Full sync/body sync not yet; restart after full sync not
  yet observed.
- Must wait for `FULLY_SYNCED`/chain tip, then line1 restart and persistence
  checks. No full-sync claim yet.

## Evidence

- `normal-binary-build16-audit-20261103.md` `8A23F99B…`
- `build-16-freeze-snapshot-final.json` `5E385E7A…` (artifact freeze)
- `layerA-A3-live-20261103.md` / `.json`
- `layerA-public-sync-20261103.md`;
  `layerA-public-sync-20261103/independent-sync-samples.jsonl` `851E79C4…`
- line1 `public-build16-sync.jsonl`, `build16-pre-status.json`,
  `build16-sync-status.json`

Layer B not started. No live promotion; build-13 7B remains final live binding.
