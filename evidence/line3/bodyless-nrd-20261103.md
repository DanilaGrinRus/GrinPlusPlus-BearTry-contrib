# Bodyless NRD — attempted full scenario (2026-11-03)

Status: **FAIL / BLOCKED for the full bodyless reorg+restart scenario.**
A real production classification bug was found and fixed, but the original
bodyless test still cannot be claimed PASS.

## Build

- Source HEAD at start: `39c9900`; working changes eventually built as
  **Tests `A686C6091D0F0886AE3572FF2CF5A3A559EB7C37A841C81648FBE13FF4015D29`**,
  GrinNode `0EF111EFBEDF1F53CF0CBD46185A5159DABFBE6BD924DA54F8C5DEE0F2026502`
  (build-30).
- Normal `/W4 /WX` build PASS; full default suite **272/272, 4314 assertions**
  on this binary, but the bodyless test is explicitly hidden (see below), so this
  is **not** a bodyless PASS.

## Real bug found and fixed

`TransactionKernel::IsCoinbase()` used a bitwise test. Kernel features are
variants, not flags: `NO_RECENT_DUPLICATE = 3` overlaps `COINBASE_KERNEL = 1`.
The NRD kernel was therefore treated as coinbase; the bodyless test failed at
`VerifyCoinbase`. Fixed to exact equality (matches Rust
`KernelFeatures::is_coinbase`). Regression test
`tests/src/BlockChain/Test_KernelFeatures.cpp` PASS (1 case; coinbase true, NRD
false, plain false).

## Test-only seam

To exercise the original real `TestServer`/BlockDB/OpenExisting/reorg/restart
path without a 552,960-block Floonet chain, a test-only seam permits an NRD
kernel at the normal v1 header **only in AUTOMATED_TESTING and only when the test
sets it**; reset is RAII. Production and the default test configuration keep the
HF3 rejection. This is a test configuration, not a production-consensus change.

## What the strengthened test reached

- phase 1: real TestServer chain, block 30 with NRD kernel, bodies cleared from
  real BlockDB; coinbase verification now passes after the IsCoinbase fix;
- phase 2: `OpenExisting()` rebuilds the bodyless NRD index; block 31 accepted;
  reorg branch blocks 31b/32b accepted;
- phase 3: `TestServer::OpenExisting()` after the reorg **hangs** in startup.
  `Node.log` ends with:
  `Confirmed chain is ahead of the candidate chain (33 > 31); truncating the
  confirmed chain.` No completion, no error; test timeout at 120 s.

The test is hidden (`[.]`) as a reproducer so the default suite does not hang;
the full bodyless scenario is **not** a PASS. The hidden tag is explicitly
recorded here and must not be presented as equivalent coverage.

## Evidence

- `C:\DHW\.tmp\build-30\Tests.exe` (A686C609…), full run
  `evidence/line3/b30-full` (272/272), kernel test
  `evidence/line3/b30-kernelfeatures`.
- Bodyless hidden run: `evidence/line3/bodyless-stage-b29` / `bodyless-seam-b28`
  (`TIMEOUT`, stdout/stderr, Node.log at
  `C:\DHW\.tmp\line3-iso\bodyless-stage-b29\home\.GrinPP\AUTOMATED_TESTING\LOGS\Node.log`).
- `tests/src/BlockChain/Test_NrdBodylessRestart.cpp` — real scenario kept,
  hidden, with exact stage comments.

## Remaining work

Fix the confirmed-chain truncation/restart path (or reproduce on a real Floonet
bodyless snapshot) and remove the hidden tag only when the complete scenario
passes. Until then: **bodyless NOT VERIFIED; do not claim closure.**
