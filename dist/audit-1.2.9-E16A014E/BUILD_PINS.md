# Build pins and reproducibility (G-24)

## Toolchain

- Windows x64, MSVC 14.4x (Visual Studio 2022 Build Tools).
- CMake generator: Visual Studio 17 2022, x64.
- C++ flags: `/W4 /WX`, static CRT `/MT`.
- Offline sysroot: `C:\DHW\.Gplus\.toolchain\sysroot`.
- Short temp directory is required: `TMP=TEMP=C:\DHW\.tmp\deps`.

## Dependency pin

The local sysroot is built from the same versions as vcpkg pin
`6185aa76504a5025f36754324abf307cc776f3da` plus the project overlay ports:

| dependency | version / commit |
|---|---|
| zlib/minizip | 1.2.11 |
| libsodium | 1.0.18 |
| secp256k1-zkp | 5b39fbf |
| RocksDB | 6.6.4 (551a110) |
| civetweb | 2c1caa6 |
| CRoaring | 0.2.66 |
| fmt | 6.2.1 |
| asio | 1.12.2 |
| mio | cafa313 |
| Catch2 | 2.12.1 |

`tools/deps/build_sysroot.py` records downloaded archive SHA-256 values in
`SOURCES.sha256`. The build must use the sysroot actually linked into the
artifacts, not a stale binary in `bin/`.

## Local reproducible gate

`tools/build/ci.ps1` performs:

1. short TMP/TEMP setup;
2. `GrinNode` and `Tests` build (unless `-SkipBuild`);
3. isolated `HOMEPATH` test run;
4. SHA-256 of both executables;
5. a JSON manifest under `evidence/ci/<git-rev>/`.

It intentionally does not clean the worktree or delete user data.

## Open G-24 item

The hosted GitHub Actions workflow still needs an actual green run on a clean
checkout with this pin. Local gate success is **not** hosted-CI evidence.
