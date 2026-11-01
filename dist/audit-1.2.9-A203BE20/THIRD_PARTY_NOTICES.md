# THIRD_PARTY_NOTICES — bundled dependencies

Project license: MIT (`LICENSE`, Copyright (c) 2018–2019 David Burkett).

This file is the distribution notice map. It is **not yet complete**: entries
marked OPEN still need the upstream license text or a verified replacement.

| Dependency | License | Local notice |
|---|---|---|
| bitcoin | MIT (Bitcoin Core) | `deps/bitcoin/COPYING` |
| reproc | MIT | `deps/reproc/LICENSE` |
| stduuid | MIT | `deps/stduuid/LICENSE` |
| cppcodec | MIT | `deps/cppcodec/LICENSE` |
| uint128 | BSD-2-Clause | `deps/uint128/LICENSE` |
| caches | MIT | `deps/caches/LICENSE.md` |
| scheduler | MIT | `deps/scheduler/include/scheduler/License.txt` |
| spdlog | MIT | `deps/spdlog/LICENSE.rst` |
| ghc::filesystem | BSD-3-Clause | embedded in `deps/ghc/filesystem.hpp` |
| jsoncpp | Public Domain / MIT | embedded in `deps/jsoncpp/json/json.h` |
| Catch2 | BSL-1.0 | embedded in `deps/Catch2/catch.hpp` |
| scrypt | BSD-2-Clause | embedded in `deps/scrypt/crypto_scrypt-ref.cpp` |
| chachapoly | Public Domain | embedded in `deps/chachapoly/chacha.h` |
| libsqlite3 | Public Domain | embedded in `deps/libsqlite3/sqlite3.h` |
| rfc6234 | IETF Trust terms | **OPEN** — see `deps/rfc6234/sha.h`; extract full notice |
| sha3 | Stephan Brumme terms | **OPEN** — `sha3.h` only links to disclaimer; add text |
| klib | MIT (upstream) | **OPEN** — no license text in the vendored files |

## Required before distribution

1. Add full license text for the three OPEN entries (rfc6234, sha3, klib).
2. Copy the embedded notices into standalone files or keep this map plus the
   referenced source headers in the distribution.
3. Re-scan with `AUDIT_LICENSES.md`; then D5 may move from FAIL to PASS.
