# Verification P1 — peer DB, negative wire, clean rebuild

## 1. PeerDB persistence

Before starting the node, a sentinel file was created in:

`C:\DHW\.tmp\interop\pp-archsrv\.GrinPP\FLOONET\NODE\DB\PEERS\keep.marker`

The node was started from that HOME. After startup the marker was still present.
This directly verifies the fix for `PeerDB::OpenDB` calling
`fs::remove_all(DB/PEERS)` on every start. The marker was removed after the test;
the node was stopped.

## 2. Negative feature tests

Added `tests/src/Core/Models/Test_FeatureValidation.cpp`:

- unknown output feature byte `0x02` is rejected;
- unknown kernel feature byte `0x04` is rejected;
- empty/truncated kernel payload is rejected.

Focused run: **3 cases / 3 assertions PASS**.

Full suite: **188 cases / 2128 assertions PASS**.

An initial full-suite run failed 27 tests because the process inherited the
default user `%TEMP%` (`C:\Users\nurli\AppData\Local\Temp`) where fixtures could
not create directories. Re-running with the project convention
`TMP=TEMP=C:\DHW\.tmp\deps` passed. This was an environment issue, not a code
regression.

## 3. Forced clean rebuild

Both targets were rebuilt with MSBuild `/t:Rebuild` (all sources recompiled):

- `GrinNode.vcxproj` — exit 0;
- `Tests.vcxproj` — exit 0.

Project sources compile under the project's warning policy. Two warnings remain
inside the pinned third-party `deps/reproc` (`sprintf` C4996, `C4133`); they are
not in Grin++ sources and do not fail the build.

Artifacts after rebuild:

- `GrinNode.exe A531C2C967B8046E4B580F38030817138B5A9102B3FFAF65C4DD602F63950BCD`
- `Tests.exe AED20E186DCC82023F27690F408C9E53D5404FBECF3FF586C9741303343A43F3`

## Verdict

| Check | Result |
|---|---|
| Peer DB survives restart | PASS |
| Unknown feature rejection | PASS |
| Truncated payload rejection | PASS |
| Full suite with project TMP | 188 / 2128 PASS |
| Forced full rebuild | PASS (third-party warnings only) |
