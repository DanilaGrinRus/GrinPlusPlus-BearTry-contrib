# AUDIT_LICENSES — bundled dependency license inventory

Status: **FAIL / open**. The project itself is MIT (`LICENSE`, David Burkett,
2018–2019), but a distribution cannot be signed off until every bundled
dependency's license and notice is present.

## Method

Scanned each directory under `deps/` for a standalone `LICENSE*`, `COPYING*`
or `NOTICE*` file (depth 1, case variants). Header-only dependencies often
carry the license in the source header; that is **not** a substitute for the
distribution notice and was not counted as present.

## Result

| Dependency | Standalone license file |
|---|---|
| bitcoin | `COPYING` |
| reproc | `LICENSE` |
| stduuid | `LICENSE` |
| cppcodec | `LICENSE` |
| uint128 | `LICENSE` |
| caches | `LICENSE.md` |
| ghc | **MISSING** |
| jsoncpp | **MISSING** |
| klib | **MISSING** |
| Catch2 | **MISSING** |
| rfc6234 | **MISSING** |
| scheduler | **MISSING** |
| scrypt | **MISSING** |
| sha3 | **MISSING** |
| spdlog | **MISSING** |
| chachapoly | **MISSING** |
| libsqlite3 | **MISSING** |

**17 dependencies, 6 with a standalone license file, 11 missing.**

## Required before handover

1. Add the upstream license text (and copyright line) for each missing
   dependency, or replace/vendor it under a compatible license.
2. Add a top-level `THIRD_PARTY_NOTICES` that maps dependency → license →
   source URL/version.
3. Confirm the MIT project license is compatible with every bundled license
   (notably SQLite public domain, jsoncpp MIT, spdlog MIT, Catch2 BSL-1.0,
   GHC BSD-3-Clause, klib MIT, scrypt BSD-2-Clause, sha3 CC0).
4. Re-run this inventory and record the result in `AUDIT_LICENSES.md` and
   `AUDIT_CHECKLIST.md` D5.

Until then D5 is **FAIL**, and G-51 remains IN PROGRESS.
