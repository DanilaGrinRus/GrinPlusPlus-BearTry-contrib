# Layer A public Floonet sync evidence (build-16)

Status: **IN PROGRESS / NOT FULLY SYNCED.** Independent read-only samples; no
source/build/profile edits.

## Run

- Build16 `GrinNode-7E3AA29E…CA18`, PID 33856, restart 15:12:12.
- API loopback `127.0.0.1:25450`; PIBD disabled, PIHD off, archive serving
  disabled by default.
- Pre-restart header **2,503,680**; post-restart **2,542,976**; independent
  samples:

| UTC | Header | Chain height | Network | Sync |
|---|---|---|---|---|
| 08:13:27 | 2,552,576 | 0 | 3,321,049 | SYNCING_HEADERS |
| 08:14:xx | 2,613,120 | 0 | 3,321,049 | SYNCING_HEADERS |
| 08:14:xx | 2,639,232 | 0 | 3,321,049 | SYNCING_HEADERS |
| 08:14:xx | 2,664,832 | 0 | 3,321,049 | SYNCING_HEADERS |
| 08:15:xx | 2,691,456 | 0 | 3,321,049 | SYNCING_HEADERS |
| 08:15:24 | 2,704,768 | 0 | 3,321,049 | SYNCING_HEADERS |

- Headers resumed above 2.5M, not genesis; outbound 4; no inbound.
- Block chain height remains 0 because header sync is still in progress; no
  full-sync claim.

## Not yet verified

- Header sync to network tip; body sync; chain tip hash/difficulty.
- Post-full-sync restart persistence, no re-download, marker cleanup.
- Long-run public peer diversity / resource behavior.
- No live promotion; build-13 7B remains final live binding.

## Evidence

- `evidence/line3/layerA-public-sync-20261103/independent-sync-samples.jsonl`
  `851E79C4…`
- `evidence/line3/layerA-A3-live-20261103/` copied line1 restart/API evidence
  (`build16-pre-status.json`, `build16-sync-status.json`, `public-build16-sync.jsonl`).
- `layerA-A3-live-20261103.md` / `.json`.
