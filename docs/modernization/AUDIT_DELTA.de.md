# AUDIT_DELTA (DE) — Grin++ 1.2.9 → Kompatibilität mit Grin v5.5.2

Status: **Entwurf (Skelett)**, erstellt am 31.10.2026. Noch kein finales
Audit-Artefakt. Basis: `5f511e6` (Upstream-Snapshot Grin++ 1.2.9).
Bereich: `5f511e6..HEAD`.

Umfang: alle Commits mit Codeänderungen. Reine Dokumentations-Commits sind keine
Karten; sie stehen in der Nachweis-Map. `Vorher`/`Nachher`/`Risiko` sind bewusst
`TODO`: sie müssen vor der Übergabe aus dem echten Diff gefüllt werden.

## So wird eine Karte gefüllt

1. Jedes `TODO` durch echten Inhalt ersetzen (keine Paraphrasen: Diff-Fragmente
   mit Datei und Zeilennummern einfügen).
2. `Rust-Referenz` — exakte `file:line` in grin v5.5.2.
3. `Nachweis` — Testname, `E`-Nummer oder Live-Artefakt; sonst `NOT VERIFIED`.
4. `Risiko, Rollback` — was brechen kann und wie man zurückrollt.

Regeln: Englisch ist kanonisch; Deutsch (`AUDIT_DELTA.de.md`) und Russisch
(`AUDIT_DELTA.ru.md`) sind Übersetzungen und müssen dieselben Karten-IDs behalten.
Code-Kommentare sind ausschließlich englisch.

## Karten

### G-24 — Reproduzierbarer Build (Offline-Sysroot)
- Status: DONE
- Commits: 89eb22c, d034a2f
- Dateien: `tools/deps/build_sysroot.py`, `tools/build/build.ps1`, `CMakeLists.txt`, `src/CMakeLists.txt`
- Rust-Referenz: n/a (Build-Infrastruktur)
- Vorher: TODO
- Nachher: TODO
- Nachweis: `BUILD.md`, E6–E9 (sauberer Build, `/W4 /WX`)
- Risiko, Rollback: TODO

### G-19 — Speichergrenzen-sichere Deserialisierung
- Status: DONE
- Commits: fc9e988
- Dateien: `include/Core/Serialization/ByteBuffer.h`, `tests/src/Core/Serialization/Test_ByteBuffer.cpp`
- Rust-Referenz: `core/src/ser.rs` (Grenzprüfungen im Reader)
- Vorher: TODO
- Nachher: TODO
- Nachweis: E2 (vorher/nachher reproduziert), `Test_ByteBuffer`
- Risiko, Rollback: TODO

### G-08 / G-10 / G-11 — Nachrichtentabelle, Capabilities, unbekannte Typen
- Status: DONE
- Commits: a5c88bf
- Dateien: `include/P2P/Capabilities.h`, `src/P2P/Messages/MessageTypes.h`, `src/P2P/Messages/MessageHeader.h`, `tests/src/Net/Test_P2PCompat.cpp`
- Rust-Referenz: `p2p/src/msg.rs:67-97`, `p2p/src/types.rs:381-399`
- Vorher: TODO
- Nachher: TODO
- Nachweis: E3, `Test_P2PCompat`
- Risiko, Rollback: TODO

### G-12 / G-14 / G-15 — Genesis im Handshake, Archiv senden/empfangen
- Status: DONE
- Commits: e5287f2
- Dateien: `src/P2P/Seed/HandShake.cpp`, `src/P2P/MessageProcessor.cpp`, `src/P2P/Pipeline/TxHashSetPipe.cpp`
- Rust-Referenz: `p2p/src/handshake.rs`, `p2p/src/msg.rs:735-789`, `p2p/src/protocol.rs:254-265`
- Vorher: TODO
- Nachher: TODO
- Nachweis: E4/E5 (Code-Ebene), live — `NOT VERIFIED`
- Risiko, Rollback: TODO

### G-25 — Version 1.2.9
- Status: DONE
- Commits: 348d87e
- Dateien: `include/GrinVersion.h`, `include/P2P/Common.h`
- Rust-Referenz: n/a
- Vorher: TODO
- Nachher: TODO
- Nachweis: Test `GrinVersion::MatchesRelease`, `/v1/status` zeigt `Grin++ 1.2.9`
- Risiko, Rollback: TODO

### G-29 — Accept-Loop überlebt abgelehnte Peers
- Status: DONE
- Commits: bfc31a0
- Dateien: `src/P2P/Seed/Seeder.cpp`
- Rust-Referenz: `p2p/src/serv.rs` (Accept-Loop)
- Vorher: TODO
- Nachher: TODO
- Nachweis: E29 (`0.0.0.0:13414 LISTENING`), Live-Handshake
- Risiko, Rollback: TODO

### G-30 — Beworbene Capabilities im Handshake + konfigurierbarer P2P-Port
- Status: DONE
- Commits: 72c94f2
- Dateien: `src/P2P/Seed/HandShake.cpp`, `include/Core/Config.h`, `src/Core/Config.cpp`, `src/Core/Config/ConfigProps.h`, `src/Core/Config/P2PConfig.h`
- Rust-Referenz: `p2p/src/msg.rs` (Felder Hand/Shake)
- Vorher: TODO
- Nachher: TODO
- Nachweis: E30 (Rust-Peer sieht `PIBD_HIST | PIBD_HIST_1`)
- Risiko, Rollback: TODO

### G-09a — PIBD-Wire-Layer, Segmentverifikation, Generator
- Status: DONE
- Commits: 62a146c, 8e2f493, de3f1a5, c390ee9
- Dateien: `src/PMMR/Models/Segment*.h`, `src/PMMR/Models/Bitmap*.h`, `src/P2P/Messages/Segment*Message.h`, `tests/src/PMMR/Test_Segment.cpp`, `Test_SegmentGenerator.cpp`
- Rust-Referenz: `core/src/core/pmmr/segment.rs`, `chain/src/txhashset/*`
- Vorher: TODO
- Nachher: TODO
- Nachweis: Unit-Suiten, E26 (Root-Modell)
- Risiko, Rollback: TODO

### G-09b — Bitmap-Zusammenbau, Download-Orchestrierung, Session/Scheduler, Snapshot
- Status: DONE
- Commits: 621efad, ec006a9, 21df0b9, df81cc3, e167e8a
- Dateien: `src/P2P/PIBD/BitmapSegmentAssembler.h`, `PibdDownload.h`, `PibdSession.h`, `PibdScheduler.h`, `PibdSnapshot.h`, `PibdTreeAssembler.h`, `PibdTreeRebuilder.h`
- Rust-Referenz: `chain/src/txhashset/desegmenter.rs`, `segmenter.rs`, `p2p/src/protocol.rs:294+`
- Vorher: TODO
- Nachher: TODO
- Nachweis: Unit-Tests je Modul (E24/E25/E26)
- Risiko, Rollback: TODO

### G-09c — Staging-PMMR schreiben und atomar aktivieren
- Status: DONE
- Commits: b681c2e, fcea044
- Dateien: `src/P2P/PIBD/PibdStateWriter.h`, `src/P2P/PIBD/PibdActivation.h`
- Rust-Referenz: `chain/src/txhashset/txhashset.rs` (Snapshot), Aktivierungssemantik
- Vorher: TODO
- Nachher: TODO
- Nachweis: `Test_PibdStateWriter`, `Test_PibdActivation` (Abbruch in beide Richtungen)
- Risiko, Rollback: TODO

### G-09d — PIBD-Serving aus den PMMR-Dateien der Node (inkl. pruned)
- Status: DONE
- Commits: 49942ab, 30012d6, 4f9a569, 177c97a
- Dateien: `src/P2P/PIBD/PibdSegmentService.h`, `src/P2P/MessageProcessor.cpp`, `include/BlockChain/BlockChain.h`, `src/BlockChain/BlockChainImpl.cpp`
- Rust-Referenz: `chain/src/txhashset/segmenter.rs`, `p2p/src/protocol.rs` (21/23/25/27)
- Vorher: TODO
- Nachher: TODO
- Nachweis: `Test_PibdServing` (pruned Bitmap), Live-Serving — `NOT VERIFIED`
- Risiko, Rollback: TODO

### G-09e — Verdrahtung in die Node: StateSyncer, P2P-Server, TxHashSet-Verarbeitung
- Status: DONE
- Commits: 7ed7ffb, edd6151, 921ca42
- Dateien: `src/P2P/Sync/StateSyncer.cpp`, `src/P2P/P2PServerImpl.cpp`, `src/BlockChain/Processors/TxHashSetProcessor.cpp`, `src/BlockChain/BlockChainImpl.cpp`
- Rust-Referenz: `chain/src/chain.rs` (TxHashSet anwenden), `p2p/src/protocol.rs`
- Vorher: TODO
- Nachher: TODO
- Nachweis: E32–E34 (Live-Aktivierung, Validierung, `FULLY_SYNCED`)
- Risiko, Rollback: TODO

### G-31 — Deterministischer Archive-Horizont
- Status: DONE
- Commits: 39b77a9, 9027ebf
- Dateien: `src/P2P/Sync/StateSyncer.cpp`, `include/Consensus.h`
- Rust-Referenz: `chain/src/chain.rs:985` (`txhashset_archive_header`)
- Vorher: TODO
- Nachher: TODO
- Nachweis: E31 (Live-Segment passt zu `header.output_root`), E42
- Risiko, Rollback: TODO

### G-32 / G-35 — Bitmap-Blockgröße (64 Chunks) und MSB-first-Bitreihenfolge
- Status: DONE
- Commits: 39b77a9, c6ab7ff
- Dateien: `src/PMMR/Models/BitmapBlock.h`, `src/PMMR/Models/BitmapChunk.h`, `src/P2P/PIBD/BitmapSegmentAssembler.h`, `tests/src/PMMR/Test_Segment.cpp`
- Rust-Referenz: `chain/src/txhashset/bitmap_accumulator.rs:470,213`
- Vorher: TODO
- Nachher: TODO
- Nachweis: Live-Bitmap-Segment gültig, Orakel 7/7 Roots (G-35), E32
- Risiko, Rollback: TODO

### G-09f — Resume-Journal für PIBD
- Status: DONE (Unit), Live-Replay PASS
- Commits: f73c7e8
- Dateien: `src/P2P/PIBD/PibdResume.h`, `src/P2P/PIBD/PibdSync.h`
- Rust-Referenz: `chain/src/txhashset/desegmenter.rs` (Fortsetzungsposition)
- Vorher: TODO
- Nachher: TODO
- Nachweis: E41/E42 (Journal-Replay, keine wiederholten Bitmap-Anfragen)
- Risiko, Rollback: TODO

### G-43 — Peer-Rotation, Retry-Backoff, Rate-Schwelle, Journal-Erhalt
- Status: DONE
- Commits: 47b6bf1, 5de693d, a5a976a, b377220
- Dateien: `src/P2P/PIBD/PibdScheduler.h`, `src/P2P/Sync/StateSyncer.cpp`
- Rust-Referenz: `p2p/src/peers.rs` (Peer-Auswahl), Timer aus `pibd.rs`
- Vorher: TODO
- Nachher: TODO
- Nachweis: E40 (Null-Peer-Regression behoben), E41, E42 (Rotation 204→66, voller Zyklus)
- Risiko, Rollback: TODO

### G-38 — Block-Sync-Policy: keine Bans wegen Timeout
- Status: DONE
- Commits: 4dbae1b, c06888b
- Dateien: `src/P2P/Sync/BlockSyncPolicy.h`, `src/P2P/Sync/BlockSyncer.cpp`, `src/P2P/Sync/HeaderSyncer.cpp`, `src/P2P/Seed/Seeder.cpp`, `include/P2P/SyncStatus.h`, `src/P2P/MessageProcessor.cpp`
- Rust-Referenz: `servers/src/grin/sync/body_sync.rs:149,191-204`, `header_sync.rs:135-141`
- Vorher: TODO
- Nachher: TODO
- Nachweis: E37 (32-Minuten-Fenster), E42 (4 222 Neuverteilungen, 0 Fraud-Bans)
- Risiko, Rollback: TODO

### G-42 — Rotation des Legacy-Peers statt Ban
- Status: DONE (Code), Live-Rotation PASS
- Commits: bbf9a56
- Dateien: `src/P2P/Sync/StateSyncer.cpp`
- Rust-Referenz: `servers/src/grin/sync/syncer.rs` / `state_sync.rs`
- Vorher: TODO
- Nachher: TODO
- Nachweis: E41/E42 (`Rotating txhashset sync peer`)
- Risiko, Rollback: TODO

### G-46 / G-47 — Konsistenzreparatur der Chain-Stores und Null-Guard in Ping
- Status: DONE (Mitigation)
- Commits: 0331ea1
- Dateien: `src/BlockChain/ChainConsistency.h`, `src/BlockChain/BlockChainImpl.cpp`, `src/P2P/MessageProcessor.cpp`, `tests/src/BlockChain/Test_ChainConsistency.cpp`
- Rust-Referenz: n/a (Invarianten beim Laden)
- Vorher: TODO
- Nachher: TODO
- Nachweis: E39 (Unit + Live-Injektion in beide Richtungen)
- Risiko, Rollback: TODO

### G-48 — Atomarer Checkpoint für Chain-Stores
- Status: OPEN (Implementierung läuft; `DbCheckpoint` zum Entwurfszeitpunkt nicht committet)
- Commits: TODO (nach dem Merge füllen)
- Dateien: `src/BlockChain/DbCheckpoint.h`, `src/BlockChain/ChainState.*`, `src/BlockChain/BlockChainImpl.cpp`
- Rust-Referenz: n/a (Crash-Consistency-Design; siehe `DB_CONSISTENCY.md` §7)
- Vorher: TODO
- Nachher: TODO
- Nachweis: Crash-Injektionen stehen aus
- Risiko, Rollback: TODO

### G-50 — Wiederholte DNS-Peer-Erkennung
- Status: DONE (Code)
- Commits: b37caa0
- Dateien: `src/P2P/Seed/Seeder.cpp`
- Rust-Referenz: `p2p/src/peers.rs` (Seeding)
- Vorher: TODO
- Nachher: TODO
- Nachweis: Live-Lauf, zwei DNS-Abfragen, Peers hinzugefügt
- Risiko, Rollback: TODO

### G-27 — PIHD: nur Erkennung (kein Protokoll)
- Status: N/A für v5.5.2 (Bit wird erkannt, nicht beworben)
- Commits: a5c88bf (Bit), f73c7e8, 348d87e
- Dateien: `include/P2P/Capabilities.h`, `tests/src/Net/Test_P2PCompat.cpp`
- Rust-Referenz: `p2p/src/msg.rs` (in v5.5.2 endet es bei `KernelSegment = 28`)
- Vorher: TODO
- Nachher: TODO
- Nachweis: E33, `Test_P2PCompat`
- Risiko, Rollback: TODO

## Nicht in der Delta enthalten

- G-44 (schlafendes PIHD-Gerüst), G-45 (Mainnet-Dry-Run), G-49 (Rangeproof-
  Performance), G-51 (dieses Paket): geplant, noch kein Code.
- Reine Dokumentations-Commits (`docs: …`): in der Nachweis-Map, nicht als Karten.
