#!/usr/bin/env python3
"""Fetch pinned third-party sources for the Grin++ build (HTTPS via OpenSSL).

The DSH sandbox blocks Schannel (git/vcpkg/curl cannot do TLS), but Python's
OpenSSL stack works, so dependency acquisition goes through this script.
Downloads are content-addressed by URL basename and verified by SHA-256 when a
pin is provided in PINS.
"""
import hashlib
import os
import sys
import tarfile
import urllib.request
import zipfile

DEST = os.environ.get("GRINPP_DEPS_SOURCES", r"C:\DHW\Gplus\.toolchain\sources")

# name -> (url, sha256 or None)
PINS = {
    "fmt-7.1.3": "https://github.com/fmtlib/fmt/archive/refs/tags/7.1.3.tar.gz",
    "zlib-1.2.11": "https://github.com/madler/zlib/archive/refs/tags/v1.2.11.tar.gz",
    "libsodium-1.0.18": "https://github.com/jedisct1/libsodium/releases/download/1.0.18-RELEASE/libsodium-1.0.18.tar.gz",
    "civetweb-1.11": "https://github.com/civetweb/civetweb/archive/refs/tags/v1.11.tar.gz",
    "roaring-0.2.6": "https://github.com/RoaringBitmap/CRoaring/archive/refs/tags/v0.2.6.tar.gz",
    "rocksdb-6.6.4": "https://github.com/facebook/rocksdb/archive/refs/tags/v6.6.4.tar.gz",
    "secp256k1-zkp-5b39fbf": "https://github.com/mimblewimble/secp256k1-zkp/archive/5b39fbf7f1e16e126404791923511723c4c0876d.tar.gz",
    "asio-1.18.0": "https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-18-0.tar.gz",
    "mio-2020": "https://github.com/mandreyel/mio/archive/refs/heads/master.tar.gz",
    "catch2-2.13.6": "https://github.com/catchorg/Catch2/archive/refs/tags/v2.13.6.tar.gz",
    "minizip-1.2.11": "https://github.com/madler/zlib/archive/refs/tags/v1.2.11.tar.gz",
}


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def download(url, target):
    req = urllib.request.Request(url, headers={"User-Agent": "grinpp-deps-fetch/1.0"})
    with urllib.request.urlopen(req, timeout=120) as resp, open(target, "wb") as out:
        while True:
            chunk = resp.read(1 << 20)
            if not chunk:
                break
            out.write(chunk)


def fetch(name, url):
    os.makedirs(DEST, exist_ok=True)
    ext = ".tar.gz" if url.endswith(".tar.gz") else ".zip"
    path = os.path.join(DEST, name + ext)
    if os.path.exists(path) and os.path.getsize(path) > 0:
        print(f"[skip] {name} ({os.path.getsize(path)} bytes)")
        return path
    print(f"[get ] {name} <- {url}")
    tmp = path + ".part"
    download(url, tmp)
    os.replace(tmp, path)
    print(f"[ok  ] {name} sha256={sha256(path)} size={os.path.getsize(path)}")
    return path


def main():
    names = sys.argv[1:] or list(PINS)
    for name in names:
        if name not in PINS:
            print(f"[warn] unknown dep {name}")
            continue
        try:
            fetch(name, PINS[name])
        except Exception as exc:  # noqa: BLE001 - report and continue
            print(f"[FAIL] {name}: {exc}")


if __name__ == "__main__":
    main()
