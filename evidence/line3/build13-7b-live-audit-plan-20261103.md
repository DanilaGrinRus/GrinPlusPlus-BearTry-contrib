# Build-13 full 7B live — independent audit + crash/publication/resource plan

Status: **PRE-AUDIT / LINE1 FULL 7B LIVE IN PROGRESS / NO BUILD OR SOURCE EDIT
UNTIL LIVE ENDS.** No commit. Build-13 is final candidate; build-09 remains
historical; build-12 historical; build-10 final only for its snapshot.

## 0. Candidate identity

- Tests `001DF4BC31F194DCDDE83FE736B06EB6FD942666F2BEFF62FF1FB5F1BB8DB3A8`
  (9,465,856 B, PE `0x6AE957C5`)
- GrinNode `F39BDFE5A8DAB22BE969406874EDCEFD023770BE1A1BA6330D7506B2C2571201`
  (7,846,912 B, PE `0x6AE956CB`)
- Build log `8C54DD1759AE3D4E22B7EC29B6AF1A1F5FB81125727613B6739D1A32B70F2FF7`,
  BUILD OK
- Source freeze `build-13-freeze-snapshot-final.json` SHA
  `4FC8DCE123D7DFD138B1AA9BB10EB7F9CC3683B4A76E35279DC7E868A261FAA3`
- Boundary fix `StateSyncer.cpp FDC5B4BB…`, `ArchivePublication.h F1E43737…`,
  regression `Test_SpentOutputs.cpp A80D1D97…`
- Normal 257/256/1 bodyless-only; ASan subset PASS 0 reports; H3313440/H3315600
  observed recovery evidence recorded (line1 formal report pending).

## 1. Non-interference rules while line1 live runs

- Do NOT run build, Tests, the isolated runner, or anything that binds
  `127.0.0.1:3421`.
- Do NOT edit source, test source, build scripts, config, or the live copy.
- Read-only access to line1 logs/evidence is allowed after files are stable;
  never copy/truncate a live Node.log without a read-share-safe method.
- Original `C:\DHW\Gplus\work\runs\g09-live-02` must remain untouched; line1 uses a copy.
- No commit.

## 2. Independent full 7B live audit (after live ends)

### Artifact/freeze

- Re-verify binary SHA/bytes/PE against the freeze before reading results.
- Verify source freeze aggregate and the boundary/SpentOutputs hashes; no
  source changes after build.
- Hash line1 evidence: logs, status/roots JSON, archive zip(s), configs,
  manifest/report.

### Scenario

- Fixed-H 7B scenario on the copied profile; record horizon/target hash,
  flushed tip, archive path, PIBD serving flags, peer topology.
- Confirm archive serving default/opt-in behavior and that only validated
  archives are advertised.
- Check body sync/no_sync/restart outcome, tip hash/height, roots, API status.
- Verify no hidden `mg_start`/port collision, no exception loop, no partial
  archive/staging served, no stale target marker.
- Classify each item PASS / FAIL / NOT VERIFIED; do not turn missing evidence
  into PASS. Existing 7B PASS is for its fixed-H scenario only.

### Recovery evidence integration

- H=3313440: build-13 observed archive
  `TxHashSet.3313440.84d7ac676356.zip` 3,116,449,317 B SHA `7FB23058…`;
  verify LoadAndValidate/atomic publish/no partial.
- H=3315600: new snapshot 3,118,756,420 B SHA `DA23A1C7…`; old
  `13F9781E…` preserved; serving ready across 3317930.
- Line1 formal report to be reconciled with independent observations.

## 3. Crash / publication test matrix (after live ends; copy only; no source edit)

Use build-13 binaries, fresh copies, isolated HOME/TMP/ports. Never the
original. For each case record pre/post tip/root/marker/archive hashes.

| ID | Injection | Expected |
|---|---|---|
| PUB-01 | kill after staged zip complete, before validation | staging removed/never advertised; final absent/old unchanged; no loop |
| PUB-02 | kill during Extract/LoadAndValidate | no final/publish; temp validation root removed or safely stale; restart handles |
| PUB-03 | kill after validation before `PublishStaged` | no partial final; old final unchanged; staging cleanup |
| PUB-04 | kill during rename/publish window | either old validated final or new validated final; never partial served |
| PUB-05 | truncated/corrupt final zip | `IsReadyArchive` rejects; not extracted/advertised; recovery regenerates or fail-closed |
| PUB-06 | missing required zip entry / wrong leaf suffix | readiness/validation rejects; no publish |
| PUB-07 | wrong target roots/height zip | LoadAndValidate rejects before publish; no serve |
| PUB-08 | stale `.staging`/validation dirs on startup | cleaned; not served as final |
| PUB-09 | existing valid final | idempotent readiness; no rewrite; no duplicate archive |
| PUB-10 | same-target failure restart | bounded, marker semantics, no retry loop; tip unchanged |

Acceptance: no partial/invalid archive at final path is ever served; failed
recovery preserves tip/roots and does not clear unrelated state; atomic rename
semantics hold; error messages include target/flushed/heights.

## 4. Resource tests (copy only)

| ID | Resource/condition | Expected |
|---|---|---|
| RES-01 | low disk/ENOSPC during staged copy/extract | fail before publish; temp cleaned; no partial final; error surfaced |
| RES-02 | disk full during rename/fsync | final path remains old-valid or absent; no servable partial |
| RES-03 | large archive/zip64 boundary near 3.1 GB | size/entry validation; no integer truncation; bounds honored |
| RES-04 | high CPU/memory during archive validation | bounded temp dirs; no leak; process remains responsive/fails closed |
| RES-05 | concurrent serving + recovery on copy | readers see old-valid/new-valid only; no torn zip |
| RES-06 | file lock/read-only archive dir | clear failure; no destructive partial state |
| RES-07 | cleanup/restart after crash | stale staging/temp cleaned; no stale target marker loop |
| RES-08 | repeated recovery/restart | bounded attempts; no archive duplication; no tip/root mutation |

Resource checks are NOT VERIFIED until executed on a fresh copy; no assumption
from unit tests.

## 5. Deliverables after live ends

- `build13-7b-live-audit-20261103.md` + JSON with artifact/evidence hashes,
  scenario status, PASS/FAIL/NOT VERIFIED.
- Crash/publication/resource evidence matrix with command/run ids and hashes.
- Update `EVIDENCE_LINE3`, `CI_REPRO`, `SANITIZERS`, task-10/11 manifest.
- No commit; no final G-09/Mainnet claim beyond what is actually observed.

## 6. Current blockers

- Line1 full 7B live in progress; no audit execution yet.
- Crash/publication/resource tests not run; need post-live copies and explicit
  run scope.
- H3313440/H3315600 observed evidence exists but line1 formal report pending.
- Full G-09/Mainnet readiness remains NOT VERIFIED; build-13 is final
  candidate, not a blanket Mainnet claim.
