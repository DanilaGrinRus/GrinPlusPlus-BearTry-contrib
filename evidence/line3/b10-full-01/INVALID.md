# INVALID — not build-10 test evidence

Run `b10-full-01` is NOT a valid build-10 full-suite audit.

Reason: a parallel line2 Tests process held the fixed wallet port
`127.0.0.1:3421`; the run produced 15 wallet/Tor `mg_start failed` failures plus
the known test-only bodyless failure (254 cases / 238 passed / 16 failed).

Do not count these 16 failures as build-10 failures. The clean serialized rerun
is `evidence/line3/b10-full-02/` (254 / 253 / 1; bodyless only). The original
manifest and logs are preserved for traceability only.
