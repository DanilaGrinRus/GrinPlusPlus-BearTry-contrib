# Task-10/11 root cause — Release NDEBUG deserialization of SpentOutputs

Status: **ROOT CAUSE CONFIRMED / FIX PRESENT BUT UNBUILT / RECOVERY AUDIT
PENDING (build-12)**. No build/commit by line3; original `g09-live-02` untouched.

## 1. Exact defect

`include/Core/Models/SpentOutput.h` (HEAD/build-11 has `F3F38B44…`, 3001 B;
build-11 source freeze entry):

```cpp
93  static SpentOutputs Deserialize(ByteBuffer& byteBuffer)
94  {
95      assert(byteBuffer.ReadU8() == 0);
96
97      const uint16_t numOutputs = byteBuffer.ReadU16();
```

`SpentOutputs::Serialize` writes the format version as a separate byte:

```cpp
84  serializer.Append<uint8_t>(/* version= */ 0);
85  serializer.Append<uint16_t>((uint16_t)m_spentOutputs.size());
```

The Release targets compile with `NDEBUG` (`GrinNode.vcxproj:156`,
`Tests.vcxproj`; CMake `CMAKE_BUILD_TYPE Release`). Under `NDEBUG`, `assert`
expands to a no-op and **does not evaluate** `ReadU8()`. The stream still points
at the version byte, so `ReadU16()` consumes the two bytes:

```text
stored:  00 00 01  <commitment 32 B> <leaf 8 B> <height 8 B>
         ^version ^count actual count=1 (bytes 01 00? format is BE: 00 01)
Release: ReadU8 omitted -> ReadU16 reads 00 00 -> numOutputs=0 -> empty map
```

For the failing 52-byte value:

```text
rawHex = 0000010989ac92...6288967 0000000000689f8d 000000000032a0a8
length = 52
version byte at [0] = 00
actual big-endian count at [1..2] = 0001 = 1
release misread at [0..1] = 0000 = 0
```

Python offset check:

```text
raw_len 52
old_release_read_u16_at_0 0
fixed_read_version 0 -> count 1 -> expected map entries 1
```

So the DB record itself is **not empty** and the canonical key is correct. The
deserializer returns an empty vector; `BuildMap()` returns `{}`; the preflight
reports `0 mapped spent position(s)` and recovery fails closed.

## 2. Live build-11 proof

Evidence source: `C:\DHW\Gplus\work\runs\task10-build11-live\cpp-profile\LOGS\Node.log`
(active while captured; snippet copied to `spent-diag-context.txt`).

Line 23840 (`GetSpentPositions` diagnostic, `GRINPP_SPENT_DIAG=1`):

```text
SPENT_DIAG GetSpentPositions db=... CURRENT=MANIFEST-000451
CF=SPENT_OUTPUTS key=30c524b8424f91e1b2b154112f19d77d4c0db6887d6a1cdc96ef2055e022bfcb
rawPresent=true rawBytes=52
rawHex=0000010989ac92c748aaf654f06fb9023746d3d34358b051dd2ac952bec39fecc62889670000000000689f8d000000000032a0a8
mapEntries=0
```

Line 23841:

```text
TryRecoverArchive(421) - TxHashSet archive recovery failed (bounded; no retry for this target):
archive preflight: block 3317930 hash 30c524b8424f91e1b2b154112f19d77d4c0db6887d6a1cdc96ef2055e022bfcb
has 1 input(s) but 0 mapped spent position(s) (target 3313440, flushed 3318655)
```

The same log still contains the earlier three original failures
(08:26:28, 08:45:10, 08:59:42) with identical commitment. The live build-11
run used `GrinNode-565BC2DBF30C.exe` (SHA
`565BC2DBF30C638F84B458012F1F07D091B8EF6DB9C933E821A58EF3CF6021A4`).

Key/DB view is now **known**: canonical `30c524b8…bfcb`, `MANIFEST-000451`,
`SPENT_OUTPUTS`, raw 52 B. The previous "actual Rewind key/view unknown" is
resolved.

## 3. Fix present (unbuilt)

Current worktree `SpentOutput.h` SHA `D27A607D32F6CA587EF66D2086C5F6A10E6B8469A5EF3EAEAB803DEA5269E211`
(3341 B):

```cpp
const uint8_t version = byteBuffer.ReadU8();
if (version != 0) {
    throw DESERIALIZATION_EXCEPTION_F("Unsupported SpentOutputs version {}", version);
}
const uint16_t numOutputs = byteBuffer.ReadU16();
```

This consumes the version byte unconditionally and rejects bad versions. The
fix is **not built**; build-11 binaries still contain the old code
(build-11 freeze entry `F3F38B44…`).

New regression test `tests/src/Database/Test_SpentOutputs.cpp` (current SHA
`6157AC56…`) has two cases:

- `SpentOutputs Release-mode deserialize consumes version byte` — serializes one
  `SpentOutput`, deserializes, requires vector/map size 1 and key/location
  equality; also requires non-zero version to throw.
- `BlockChain preflight requires every input in spent positions` — mapped/missing
  input coverage.

Not yet compiled/run.

## 4. Build-12 audit acceptance

Normal **Release** build (not only ASan):

1. Full suite + `SpentOutputs*`, `BlockChain*preflight*`, archive, Nrd,
   KernelIndex, handler/schema/duplicate-range, ArchivePublication, advert.
   The new Release-mode test must be present and PASS; test count expected to
   increase by 2 (do not preclaim exact total).
2. Demonstrate fail-before-fix: same serialized 52-byte value / test against the
   old `F3F38B44…` header must yield 0 map entries; fixed header 1. Record as a
   focused repro, not a build-11 PASS.
3. Live recovery from a **copy** of the g09 fixture: build-12 must decode raw
   52 B -> mapEntries=1 at canonical `30c524b8…`, preflight pass, and archive
   H=3313440 published atomically (staged -> validate -> fsync -> rename), no
   partial final, H=3315600 unaffected, original untouched.
4. `GRINPP_SPENT_DIAG` line must show `rawBytes=52`, `mapEntries=1`; no
   `archive preflight ... 0 mapped` for this block.
5. Verify no other production `assert(side-effecting-read)` remains; current
   searches found none matching the SpentOutputs pattern (only non-reader
   asserts in BlockProcessor/Connection/WalletManager).
6. ASan refresh on build-12 sources; new SpentOutputs test is not ASan-BLOCKED
   (does not require rocksdb) and must run under ASan too; archive/server + Nrd
   subset as before; teardown; 0 reports.
7. Re-capture build-12 binary SHA/PE/source freeze; do not reuse build-11
   artifact or its normal/ASan binding for the fix.

## 5. Evidence

- `spent-diag-context.txt` SHA `22FDD497…` (log lines 23840-23841)
- `Node.build11-live.log` SHA `6552D5E3…` (3,372,495 B; full build-11 live log,
  captured after the node released the file)
- `SpentOutput.build11-HEAD.h` SHA `9C600C54…` (git HEAD text, old assert)
- `SpentOutput.build12-fixed.h` SHA `D27A607D…`
- `Test_SpentOutputs.build12.cpp` SHA `6157AC56…`
- build-11 old source freeze entry: `F3F38B44EBD7E5BC0D42863FA6BA87C0DCD98DB07B6292305F1CAED898ACD7CD`
- build-11 binaries: Tests `02DC3D9C…`, GrinNode `565BC2DB…`
- `NDEBUG`: `GrinNode.vcxproj:156`, `Tests.vcxproj:168`, CMake Release.
- Root cause manifest: `rootcause-20261103.json` (`15524033…` after full-log update).

No source edits, build, or commit by line3. This root-cause evidence supersedes
the earlier unresolved-key wording; the build-11 normal/ASan audit remains
valid only for its own snapshot and does not validate the fix.
