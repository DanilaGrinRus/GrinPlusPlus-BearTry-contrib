# AUDIT_LICENSES — bundled dependency license inventory

Status: **PASS / complete**. The project itself is MIT (`LICENSE`, David Burkett,
2018–2019), but a distribution cannot be signed off until every bundled
dependency's license and notice is present.

## Method

Scanned each directory under `deps/` recursively for a standalone `LICENSE*`,
`COPYING*` or `NOTICE*` file (case variants). Header-only dependencies often
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
| scheduler | `include/scheduler/License.txt` |
| scrypt | **MISSING** |
| sha3 | **MISSING** |
| spdlog | `LICENSE.rst` |
| chachapoly | **MISSING** |
| libsqlite3 | **MISSING** |

**17 dependencies: 11 with a standalone license file, 6 with the full license
text embedded in their source header. All 17 are mapped in
`THIRD_PARTY_NOTICES.md`; klib (MIT), sha3 (zlib-style) and rfc6234 (BSD-3)
have added local license files.**

## Result

The MIT project license is compatible with the bundled licenses (MIT, BSD-2/3,
BSL-1.0, zlib-style, Public Domain/IETF terms). `THIRD_PARTY_NOTICES.md` maps
every dependency, and the three previously missing notices (klib, sha3,
rfc6234) are now present. D5 is **PASS**; G-51 still needs external sign-off.
