# Normal-binary AUDIT build-16 (1592F3DF / 7E3AA29E) — bodyless-only

Status: **FAIL overall, known bodyless only; owner-auth tests PASS; live API
pending line1 restart.** Build-13 remains final 7B live binding; build-16 is
the A3 owner-auth hardening candidate.

## Identity / source binding

| Artifact | SHA-256 | Size | PE timestamp |
|---|---|---|---|
| Tests.exe | `1592F3DF041116AC3C891E5AEEB894B6144C9AB1713893C3B265DDA1C8E44CB6` | 9,542,656 B | `0x6AE9948F` |
| GrinNode.exe | `7E3AA29E1160244A28DDDFE3A2CCF68BD2A943A97D610B55845561453280CA18` | 7,850,496 B | — |

- Build log `C:\DHW\.tmp\build-16\build-console-20261103-145117.log`
  `95E621F0…` BUILD OK; object/source timestamps show handlers/tests compiled
  after A3 patch (OwnerHandlerAccess obj 14:56:46, RpcAuth obj 14:58:11;
  Tests 15:00:15; GrinNode 15:00:45).
- Build freeze `evidence/line3/build-16-freeze-snapshot-final.json`
  (`44E50214…`, 1007 files).
- `/W4 /WX` C++ project flags; `/EHsc`; C deps `/W3`.
- Source is uncommitted line2 worktree (3 handlers, 2 CMake, 2 new tests);
  no line3 source/build edits.

## Normal audit

Serialized isolated runner, combined line3 independent run:

- Full: **262 cases / 261 passed / 1 failed**; **4281 assertions / 4280
  passed**. Sole failure known test-only `Test_NrdBodylessRestart.cpp(103)`;
  zero `mg_start`.
- `Owner handlers require PRIVATE access; config levels unchanged`: **PASS**
  1 case / 6 assertions.
- `Owner RPC methods reject missing/wrong token and accept valid token`:
  **PASS** 1 case / 8 assertions.

Build-15 bodyless-only baseline was 260/259/1; build-16 adds the two A3 tests
(262).

## A3

Static fix: PRIVATE overrides on `GetStatusOwnerHandler`,
`GetPeersOwnerHandler`, `GetConnectedPeersOwnerHandler`; get_config PRIVATE,
update_config ADMIN, foreign public unchanged. The two tests pass under build16.
Live owner API after line1 restart is **PASS** (15:12 build16 PID33856; no/wrong -32001, valid OK; public foreign 200; loopback bind);
line1 was still on build15 `GrinNode-A7D28BCC…` when this audit ran.

## Classification

- Build16 normal: **FAIL bodyless-only**; production/agreed-suite zero.
- A3 unit/static: **PASS**; live owner API: **NOT VERIFIED**.
- No build16 ASan run in this step; no live promotion.
