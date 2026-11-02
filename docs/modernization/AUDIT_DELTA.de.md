# AUDIT_DELTA (DE) — Grin++ 1.2.9 → Kompatibilität mit Grin v5.5.2
**Aktueller Stand 02.11.2026.** Basis 1.2.9 `5f511e6`; Arbeitsbaum HEAD `a49dfe0`. Quellmetriken:

- C++ hinzugefügt: **+17.093 Zeilen**; entfernt **−594**; 187 Dateien;
- Diff-/neuer C++-Textumfang: **≈ +721.7 KiB**;
- endgültige `GrinNode.exe`: **7.744.512 B = 7.563,0 KiB = 7,38 MiB**;
- **Exe-Zuwachs gegenüber 1.2.9: NOT MEASURED** — kein Originalbinär vorhanden; 7,7 MB ist die Endgröße, kein Delta;
- Artefakte: `GrinNode A3E35660…`, `Tests EC80635F…`; Suite **203 / 3.676 PASS**.
- **G-09 Stage 1:** identity-geprüfter P2P- + Rust-Lauf bei H=3.315.600 — PASS (7/7, 1673/1673, 1673/1673, 1637/1637); Roots stammen aus C++-REST, unabhängige H-Bestätigung und normale Rust-Node bleiben NOT VERIFIED.
- **PIBD/PIHD-Guard:** Height-Cap 13, Overflow-Prüfung `idx*capacity`, PIHD-Antwort an Connection/Peer gebunden, Fallback ohne Chain-Height-Fortschritt; PIHD ist experimentell, `PIHD_HIST` wird nicht beworben, Snapshot/Cache/Rate-Limit bleiben offen.

Basis: Grin++ 1.2.9 (`5f511e6`). Delta aktualisiert: 02.11.2026, HEAD `a49dfe0` (G-09/G-23/G-53/G-07/G-24).
Begleitdokumente: `AUDIT_PACKAGE.md` (Paketregeln), `GAP_MATRIX.md` (Status aller
Lücken), `TEST_RESULTS.md` (Nachweise E1–E84), `DB_CONSISTENCY.md` (Design der
Chain-Speicher).

Umfang: eine Karte pro Änderung am Code gegenüber der Basis oder pro untrennbarer
Gruppe. Jede Karte nennt Commit(s), Dateien, die Rust-Referenz, einen
Vorher/Nachher-Ausschnitt, den Nachweis sowie Risiko und Rücknahme. Geplante
Punkte ohne Code sind ausgeschlossen (siehe Ende); reine Dokumentations-Commits
stehen in der Nachweiskarte, nicht als Karten.

## Karten

### G-24 — Reproduzierbarer Build (Offline-Sysroot)
- Status: DONE
- Commits: `89eb22c`, `d034a2f`
- Dateien: `tools/deps/build_sysroot.py`, `tools/build/build.ps1`, `CMakeLists.txt`, `src/CMakeLists.txt`
- Rust-Referenz: n/a (Build-Infrastruktur)
- Vorher: configure löste Abhängigkeiten aus Netzwerk/System-vcpkg und nutzte das jeweilige MSVC/Ninja aus `PATH`; Builds waren offline nicht reproduzierbar.
- Nachher: `.toolchain/sysroot` liefert Header und Bibliotheken offline; Generator/Toolset sind gepinnt; `build.ps1 -Target GrinNode|Tests` ist der einzige Einstieg; Warnungen werden mit `/W4 /WX` gebaut.
- Diff: `tools/build/build.ps1`, Commit `89eb22c` — offline sysroot/`/W4 /WX` entry point.
- Nachweis: `BUILD.md`, E6–E9 (sauberer Build, keine deaktivierten Warnungen), SHA-256 in `BUILD.md`.
- Risiko, Rücknahme: Risiko ist ein veralteter/unvollständiger Sysroot; Rücknahme durch Zurücksetzen der Build-Skripte/CMake-Änderungen und Nutzung der Systemabhängigkeiten.

### G-19 — Speichergrenzen-sichere Deserialisierung
- Status: DONE
- Commits: `fc9e988`
- Dateien: `include/Core/Serialization/ByteBuffer.h`, `tests/src/Core/Serialization/Test_ByteBuffer.cpp`
- Rust-Referenz: `core/src/ser.rs` (Grenzen des Readers)
- Vorher: varint/vector/string-Reader konnten über die Nutzlast hinaus lesen; Größenrechnung wurde nicht gegen den Restpuffer geprüft.
- Nachher: jeder Lesezugriff wird gegen das Pufferende geprüft (protokollversionsabhängig); abgeschnittene und übergroße Eingaben scheitern sauber; Unit-Tests decken beides ab.
- Diff: `include/Core/Serialization/ByteBuffer.h`, Commit `fc9e988`
  ```diff
  --- a/include/Core/Serialization/ByteBuffer.h
  +++ b/include/Core/Serialization/ByteBuffer.h
  -        if (m_index + sizeof(T) > m_bytes.size()) {
  -            throw DESERIALIZATION_EXCEPTION("Attempted to read past end of ByteBuffer.");
  -        }
  +        RequireRemaining(sizeof(T));
  ```
- Nachweis: E2 (vorher/nachher reproduziert), `Test_ByteBuffer`.
- Risiko, Rücknahme: strenge Prüfungen können grenzgültige Nachrichten ablehnen; das Versionsgate begrenzt dies; Rücknahme durch Zurücksetzen von `ByteBuffer`.

### G-08 / G-10 / G-11 — Nachrichtentabelle, Capabilities, unbekannte Typen
- Status: DONE
- Commits: `a5c88bf`
- Dateien: `include/P2P/Capabilities.h`, `src/P2P/Messages/MessageTypes.h`, `src/P2P/Messages/MessageHeader.h`, `tests/src/Net/Test_P2PCompat.cpp`
- Rust-Referenz: `p2p/src/msg.rs:67-97`, `p2p/src/types.rs:381-399`
- Vorher: Nachrichtentabelle und Capability-Bits passten nicht zu grin v5.5.2; PIBD-Segmenttypen fehlten, unbekannte Typen wurden nicht explizit abgelehnt.
- Nachher: vollständige `EMessageType`-Tabelle inklusive Segment-Requests/Responses 21–28, Capability-Werte wie in Rust, unbekannte Typen werden vor dem Dispatch abgelehnt; Kompatibilitätstests pinnen beides.
- Nachweis: E3, `Test_P2PCompat`.
- Risiko, Rücknahme: eine Enum-Abweichung bricht Handshake/Dispatch; Tests pinnen die Wire-Werte; Rücknahme von Tabelle und Capability-Headern gemeinsam.

### G-12 / G-14 / G-15 — Genesis im Handshake, Archiv senden/empfangen
- Status: DONE (Code); Live-Archivtransfer NOT VERIFIED
- Commits: `e5287f2`
- Dateien: `src/P2P/Seed/HandShake.cpp`, `src/P2P/MessageProcessor.cpp`, `src/P2P/Pipeline/TxHashSetPipe.cpp`
- Rust-Referenz: `p2p/src/handshake.rs`, `p2p/src/msg.rs:735-789`, `p2p/src/protocol.rs:254-265`
- Vorher: der Handshake prüfte den Genesis-Hash nicht; der Archivsender iterierte über `istream::read` und verlor den letzten Teilblock; der Empfänger hatte keine Größen-/Aufräumprüfung.
- Nachher: Genesis-Mismatch lehnt den Peer ab; die Archivübertragung nutzt `gcount()` und sendet jedes Byte, mit Größenprüfung und Löschen unvollständiger Dateien.
- Nachweis: E4/E5 (Codestufe); Live-Legacy-Archivtransfer weiterhin NOT VERIFIED (G-52).
- Risiko, Rücknahme: die Ablehnung falscher Genesis ist beabsichtigt; Änderungen sind auf `TxHashSetPipe` begrenzt; Rücknahme beider Dateien.

### G-25 — Version 1.2.9
- Status: DONE
- Commits: `348d87e`
- Dateien: `include/GrinVersion.h`, `include/P2P/Common.h`
- Rust-Referenz: n/a
- Vorher: `GRINPP_VERSION`/User-Agent meldeten 1.2.8.
- Nachher: Version und UA melden 1.2.9; `GrinVersion::MatchesRelease("Grin++ 1.2.9")` pinnt sie und `/v1/status` liefert denselben String.
- Nachweis: Unit-Test, Live-`/v1/status` (E30).
- Risiko, Rücknahme: die Version beeinflusst Peer-Banner; Rücknahme ist eine Zeile.

### G-29 — Accept-Loop überlebt abgelehnte Peers
- Status: DONE
- Commits: `bfc31a0`
- Dateien: `src/P2P/Seed/Seeder.cpp`
- Rust-Referenz: `p2p/src/serv.rs` (Accept-Loop)
- Vorher: `Seeder::Accept` kehrte nach einem abgelehnten Peer zurück, ohne `async_accept` erneut zu scharf zu schalten; der Listener starb nach der ersten Ablehnung.
- Nachher: der Accept-Handler schaltet den Listener immer wieder scharf; eine abgelehnte Verbindung beendet den Inbound-Dienst nicht mehr.
- Nachweis: E29 (`0.0.0.0:13414 LISTENING` nach Ablehnungen), Live-Handshakes.
- Risiko, Rücknahme: ein dauerhafter Accept-Fehler könnte drehen; der Fehlerpfad bleibt geschützt; Rücknahme stellt das alte Verhalten wieder her.

### G-30 — Beworbene Capabilities im Handshake + konfigurierbarer P2P-Port
- Status: DONE
- Commits: `72c94f2`
- Dateien: `src/P2P/Seed/HandShake.cpp`, `include/Core/Config.h`, `src/Core/Config.cpp`, `src/Core/Config/ConfigProps.h`, `src/Core/Config/P2PConfig.h`
- Rust-Referenz: `p2p/src/msg.rs` (Hand/Shake-Felder)
- Vorher: der Handshake bewarb ein hartkodiertes `FAST_SYNC_NODE`, die PIBD-Bits blieben unsichtbar; der Listen-Port wurde auch zum Wählen genutzt.
- Nachher: der Handshake bewirbt `ADVERTISED` (`FAST_SYNC_NODE | PIBD_HIST | PIBD_HIST_1`); `P2P_PEER_PORT` wählt den Wähl-Port; beides ist konfiguriert und getestet.
- Diff: `src/P2P/Seed/HandShake.cpp`, Commit `72c94f2`
  ```diff
  --- a/src/P2P/Seed/HandShake.cpp
  +++ b/src/P2P/Seed/HandShake.cpp
  -        Capabilities::FAST_SYNC_NODE,
  +        Capabilities::ADVERTISED,
  -        Capabilities::FAST_SYNC_NODE,
  +        Capabilities::ADVERTISED,
  ```
- Nachweis: E30 (ein echter Rust-Peer sieht `TXHASHSET_HIST | PEER_LIST | PIBD_HIST | PIBD_HIST_1`).
- Risiko, Rücknahme: Peers können zusätzliche Bits anders behandeln; PIBD-Bits sind per Policy abschaltbar; Rücknahme stellt den alten Wert wieder her.

### G-09a — PIBD-Wire-Layer, Segmentverifikation, Generator
- Status: DONE
- Commits: `62a146c`, `8e2f493`, `de3f1a5`, `c390ee9`
- Dateien: `src/PMMR/Models/Segment*.h`, `src/PMMR/Models/Bitmap*.h`, `src/P2P/Messages/Segment*Message.h`, `tests/src/PMMR/Test_Segment.cpp`, `tests/src/PMMR/Test_SegmentGenerator.cpp`
- Rust-Referenz: `core/src/core/pmmr/segment.rs`, `chain/src/txhashset/*`
- Vorher: es gab keine PIBD-Wire-Nachrichten oder Segmentmodelle; nur der Legacy-Zip-Pfad existierte.
- Nachher: Segment-Request/Response 21–28, Bitmap-Chunk/Block- und Segmentmodelle mit Verifikation sowie ein deterministischer Generator; Unit-Tests vergleichen Roots und Wire-Bytes.
- Nachweis: E26, Unit-Suiten `Test_Segment`, `Test_SegmentGenerator`.
- Risiko, Rücknahme: ein Wire-Mismatch macht PIBD unbrauchbar, Legacy bleibt intakt; `P2P.PIBD_ENABLED=false` deaktiviert den Pfad; Rücknahme entfernt Nachrichten/Modelle.

### G-09b — Bitmap-Zusammenbau, Download-Orchestrierung, Session/Scheduler, Snapshot
- Status: DONE
- Commits: `621efad`, `ec006a9`, `21df0b9`, `df81cc3`, `e167e8a`
- Dateien: `src/P2P/PIBD/BitmapSegmentAssembler.h`, `PibdDownload.h`, `PibdSession.h`, `PibdScheduler.h`, `PibdSnapshot.h`, `PibdTreeAssembler.h`, `PibdTreeRebuilder.h`
- Rust-Referenz: `chain/src/txhashset/desegmenter.rs`, `segmenter.rs`, `p2p/src/protocol.rs:294+`
- Vorher: kein Bitmap-Zusammenbau, keine Download-Orchestrierung, kein Request-Scheduler; PIBD war nicht ansteuerbar.
- Nachher: Bitmap-Segmente werden zusammengesetzt/validiert, Downloads mit In-Flight-Limit/Timeouts/Retries/Fallback geplant, Bäume aus Segmenten aufgebaut und über eine Snapshot-API bereitgestellt.
- Nachweis: E24/E25/E26, Unit-Tests je Modul.
- Risiko, Rücknahme: Speicher/CPU und Retry-Stürme sind durch Scheduler-Limits begrenzt; über `PIBD_ENABLED` abschaltbar oder Modul zurücksetzen.

### G-09c — Staging-PMMR schreiben und atomar aktivieren
- Status: DONE
- Commits: `b681c2e`, `fcea044`
- Dateien: `src/P2P/PIBD/PibdStateWriter.h`, `src/P2P/PIBD/PibdActivation.h`
- Rust-Referenz: `chain/src/txhashset/txhashset.rs` (Snapshot), Aktivierungssemantik
- Vorher: für einen geladenen PIBD-Zustand gab es keinen crash-sicheren Staging-/Aktivierungspfad.
- Nachher: `PibdStateWriter` schreibt Staging-PMMR-Dateien; die Aktivierung nutzt ein temporäres Verzeichnis/einen Marker und schaltet nach der Validierung um; Tests injizieren Abstürze in beide Richtungen und prüfen die Wiederherstellung.
- Diff: `src/P2P/PIBD/PibdActivation.h`, Commit `fcea044`
  ```diff
  --- /dev/null
  +++ b/src/P2P/PIBD/PibdActivation.h
  +// Atomic activation of a downloaded PIBD state.
  +//
  +// The writer produces a complete state in a staging directory. Activation swaps
  +// it in with two renames and a marker file:
  +//
  +//   1. write <live parent>/pibd_activation.pending with (staging, backup) paths
  +//   2. rename live -> backup            (if a live state exists)
  +//   3. rename staging -> live
  ```
- Nachweis: `Test_PibdStateWriter`, `Test_PibdActivation`.
- Risiko, Rücknahme: ein unvollständiges Staging-Verzeichnis darf nie installiert werden; Marker und Tests erzwingen das; Rücknahme löscht Staging und behält den Live-Zustand.

### G-09d — PIBD-Serving aus den PMMR-Dateien der Node (inkl. pruned)
- Status: DONE (Unit); Live-Serving NOT VERIFIED
- Commits: `49942ab`, `30012d6`, `4f9a569`, `177c97a`
- Dateien: `src/P2P/PIBD/PibdSegmentService.h`, `src/P2P/MessageProcessor.cpp`, `include/BlockChain/BlockChain.h`, `src/BlockChain/BlockChainImpl.cpp`
- Rust-Referenz: `chain/src/txhashset/segmenter.rs`, `p2p/src/protocol.rs` (21/23/25/27)
- Vorher: die Node konnte `GetSegment` nicht beantworten; Segment-Serving existierte nicht.
- Nachher: `PibdSegmentService` liefert Bitmap/Output/Rangeproof/Kernel-Segmente aus der Node-PMMR, inklusive pruned-Pfaden; MessageProcessor routet Requests, BlockChain stellt den Segmenter bereit; ein Pruned-Bitmap-Test deckt den schwierigen Fall ab.
- Nachweis: `Test_PibdServing`; Live-Serving an einen Rust-Client NOT VERIFIED.
- Risiko, Rücknahme: falsche Segmente oder DoS bei Regressions der Horizont-Prüfung; Unit-Tests pinnen die Pfade; Rücknahme entfernt Routing und Service.

### G-09e — Verdrahtung in die Node: StateSyncer, P2P-Server, TxHashSet-Verarbeitung
- Status: DONE
- Commits: `7ed7ffb`, `edd6151`, `921ca42`
- Dateien: `src/P2P/Sync/StateSyncer.cpp`, `src/P2P/P2PServerImpl.cpp`, `src/BlockChain/Processors/TxHashSetProcessor.cpp`, `src/BlockChain/BlockChainImpl.cpp`
- Rust-Referenz: `chain/src/chain.rs` (TxHashSet anwenden), `p2p/src/protocol.rs`
- Vorher: StateSyncer nutzte nur `txhashset.zip`; der P2P-Server hatte keine PIBD-Ressourcen; Installation/Validierung des PIBD-Zustands fehlten.
- Nachher: `TryPibd` treibt die PIBD-Session vor dem Legacy-Fallback, der Server verdrahtet Resume/Serving, und `ProcessExistingTxHashSet` validiert und installiert den geladenen Zustand.
- Nachweis: E32–E34, E42 (Live-Aktivierung, Validierung, `FULLY_SYNCED`).
- Risiko, Rücknahme: eine fehlerhafte Installation kann den Zustand beschädigen; Validierung läuft vor der Aktivierung; `PIBD_ENABLED=false` ist die sichere Rücknahme.

### G-31 — Deterministischer Archive-Horizont
- Status: DONE
- Commits: `39b77a9`, `9027ebf`
- Dateien: `src/P2P/Sync/StateSyncer.cpp`, `include/Consensus.h`
- Rust-Referenz: `chain/src/chain.rs:985` (`txhashset_archive_header`)
- Vorher: PIBD forderte den Candidate-Tip-Hash an, Peers antworteten mit `SegmenterHeaderMismatch`.
- Nachher: der Horizont ist `archive_height = (head − STATE_SYNC_THRESHOLD)` abgerundet auf `TXHASHSET_ARCHIVE_INTERVAL = 720`; PIBD und Legacy-Archivanfragen nutzen diese Höhe.
- Diff: `src/P2P/Sync/StateSyncer.cpp`, Commit `39b77a9`
  ```diff
  --- a/src/P2P/Sync/StateSyncer.cpp
  +++ b/src/P2P/Sync/StateSyncer.cpp
  +        // archive_height = (header_head - STATE_SYNC_THRESHOLD) rounded down to a
  +        // multiple of TXHASHSET_ARCHIVE_INTERVAL (grin txhashset_archive_header_header_only).
  +        uint64_t archiveHeight = pTip->GetHeight();
  +        archiveHeight = archiveHeight > Consensus::STATE_SYNC_THRESHOLD
  +            ? archiveHeight - Consensus::STATE_SYNC_THRESHOLD
  +            : 0;
  +        archiveHeight -= archiveHeight % Consensus::TXHASHSET_ARCHIVE_INTERVAL;
  ```
- Nachweis: E31 (Live-Segment passt zu `header.output_root`), E42.
- Risiko, Rücknahme: ein Off-by-One im Intervall bricht die Segmentvalidierung; Orakel/Live-Tests pinnen es; Rücknahme stellt die Tip-Anfrage wieder her (bekannt fehlerhaft).

### G-32 / G-35 — Bitmap-Blockgröße (64 Chunks) und MSB-first-Bitreihenfolge
- Status: DONE
- Commits: `39b77a9`, `c6ab7ff`
- Dateien: `src/PMMR/Models/BitmapBlock.h`, `src/PMMR/Models/BitmapChunk.h`, `src/P2P/PIBD/BitmapSegmentAssembler.h`, `tests/src/PMMR/Test_Segment.cpp`
- Rust-Referenz: `chain/src/txhashset/bitmap_accumulator.rs:470,213`
- Vorher: ein `BitmapBlock` hatte 16 Chunks und indizierte Bits LSB-first; sparse (positive/negative) Segmente wurden falsch gelesen, echte Segmente deserialisierten nicht.
- Nachher: 2^16 Bits = 64 Chunks × 1024; Bit `n` liegt in Byte `n/8`, Bit `7−(n%8)` (MSB-first, wie `bitvec::to_bytes`); Raw-Chunks serialisieren die 1024 Bytes ohne Längenpräfix.
- Diff: `src/PMMR/Models/BitmapChunk.h`, Commit `c6ab7ff`
  ```diff
  --- a/src/PMMR/Models/BitmapChunk.h
  +++ b/src/PMMR/Models/BitmapChunk.h
  -        return (m_bytes[idx / 8] & (uint8_t)(1u << (idx % 8))) != 0;
  +        return (m_bytes[idx / 8] & (uint8_t)(1u << (7 - (idx % 8)))) != 0;
  -        const uint8_t mask = (uint8_t)(1u << (idx % 8));
  +        const uint8_t mask = (uint8_t)(1u << (7 - (idx % 8)));
  ```
- Nachweis: E32/E35, Orakelvektoren (`grin_chain 5.5.2`): 7/7 Segment-Roots und der Gesamt-Bitmap-Root stimmen.
- Risiko, Rücknahme: falsche Bitreihenfolge korrumpiert Roots still; Orakel-/Unit-Vektoren erkennen es; Rücknahme nur mit deaktiviertem PIBD.

### G-09f — Resume-Journal für PIBD
- Status: DONE (Unit + Live-Replay)
- Commits: `f73c7e8`
- Dateien: `src/P2P/PIBD/PibdResume.h`, `src/P2P/PIBD/PibdSync.h`
- Rust-Referenz: `chain/src/txhashset/desegmenter.rs` (Resume-Position)
- Vorher: ein Neustart verwarf alle geladenen Segmente, PIBD begann bei null.
- Nachher: append-only Journale je Typ (`bitmap.log`, `output.log`, `rangeproof.log`, `kernel.log`, `manifest`) mit u32-BE-Längenrahmen werden vor neuen Requests abgespielt; das Manifest bindet das Journal an den Horizont und wird erst nach Aktivierung gelöscht.
- Nachweis: E41/E42 (nach Neustart keine wiederholten Bitmap-Requests; Journal wuchs und Download lief weiter).
- Risiko, Rücknahme: ein veraltetes/kaputtes Journal könnte falsche Segmente liefern; Horizont-/Deserialisierungsprüfungen verwerfen es; Rücknahme löscht `NODE/pibd_resume`.

### G-43 — Peer-Rotation, Retry-Backoff, Rate-Schwelle, Journal-Erhalt
- Status: DONE
- Commits: `47b6bf1`, `5de693d`, `a5a976a`, `b377220`
- Dateien: `src/P2P/PIBD/PibdScheduler.h`, `src/P2P/Sync/StateSyncer.cpp`
- Rust-Referenz: `p2p/src/peers.rs` (Peer-Auswahl), Timer-Semantik von `pibd.rs`
- Vorher: das erste stockende Segment brach PIBD ab und deaktivierte es für 120 Minuten; ein langsamer, aber lebender Peer wurde nie rotiert; nach einem Horizont-Reset konnte der Peer-Zeiger null sein (Absturz).
- Nachher: ein Segment wird bis 660 s ohne akzeptierte Antwort wiederholt; `RotatePibdPeer` probiert andere Peers, dann bis zu drei Runden über das Peer-Set mit 30/60/120 s Backoff; `RateWindow` rotiert Peers mit weniger als 10 Segmenten pro 180 s; `Reset()` behält das Journal; nach einem Horizontwechsel wird der Peer neu gewählt.
- Nachweis: E40 (Null-Peer-Regression behoben), E41/E42 (Live-Rotation 204→66, voller Zyklus).
- Risiko, Rücknahme: Rotations-Churn in langsamen Netzen; Schwellen sind Konstanten; Rücknahme setzt Scheduler/Rotation zurück, Legacy-Fallback bleibt.

### G-38 — Block-Sync-Policy: keine Bans wegen Timeout
- Status: DONE
- Commits: `4dbae1b`, `c06888b`, `7997ca5`
- Dateien: `src/P2P/Sync/BlockSyncPolicy.h`, `src/P2P/Sync/BlockSyncer.cpp`, `src/P2P/Sync/HeaderSyncer.cpp`, `src/P2P/Seed/Seeder.cpp`, `include/P2P/SyncStatus.h`, `src/P2P/MessageProcessor.cpp`, `tests/src/Net/Test_BlockSyncPolicy.cpp`
- Rust-Referenz: `servers/src/grin/sync/body_sync.rs:149,191-204`, `header_sync.rs:135-141`
- Vorher: `BlockSyncer` bannte einen Peer nach zwei Timeouts wegen `FraudHeight`, `m_slowPeers` verfiel nie, das Anfragefenster ignorierte Peer-Höhen, Seeder/Syncer schwiegen nach den Bans.
- Nachher: ein Timeout wird gezählt und der Block neu zugewiesen; Anfragen sind auf `min(candidate tip, network height, per-connection height)` und Batch ≤100 begrenzt; Retry 6 s/1 s + Backoff; langsamer Peer 60 s Cooldown; `FraudHeight` spiegelt jetzt exakt Rust: strikt größere TOTAL-DIFFICULTY plus 120 s ohne Header-Fortschritt (die frühere Höhe-ODER-Schwierigkeit-Regel führte live zu drei falschen Bans; behoben, E48); Seeder loggt den Grund; `/v1/status` zeigt Wartezeit und Zähler.
- Diff: `src/P2P/Sync/BlockSyncPolicy.h`, Commit `4dbae1b`
  ```diff
  --- /dev/null
  +++ b/src/P2P/Sync/BlockSyncPolicy.h
  +// Behaviour mirrored from grin v5.5.2 body_sync.rs / header_sync.rs:
  +//   * a timeout is never fraud: reassign the block and count it;
  +//   * FraudHeight requires a peer that claims more work than we have and has
  +//     made no progress for a long time;
  ```
- Nachweis: E37 (32-Minuten-Live-Fenster), E42 (4 222 Reassignments, null Fraud-Bans), E48 (TD-only nach Live-Falschbans).
- Risiko, Rücknahme: ohne Bans kann ein nicht antwortender Peer den Catch-up bremsen; Cooldown/Rotation begrenzen es; Rücknahme stellt den Ban-Pfad wieder her.

### G-42 — Legacy-State-Sync: Peer-Rotation statt Ban
- Status: DONE
- Commits: `bbf9a56`
- Dateien: `src/P2P/Sync/StateSyncer.cpp`
- Rust-Referenz: `servers/src/grin/sync/syncer.rs`, `state_sync.rs`
- Vorher: `RequestState` bannte den Peer mit `BadTxHashSet`, sobald der Zip 10 s keinen Fortschritt machte oder das 120-Minuten-Limit erreichte; die Node konnte ohne Peers bleiben.
- Nachher: ein langsamer Peer wird rotiert (`Rotating txhashset sync peer`); ein wirklich ungültiges Archiv wird weiterhin nach fehlgeschlagener Validierung von der Pipeline gebannt.
- Diff: `src/P2P/Sync/StateSyncer.cpp`, Commit `bbf9a56`
  ```diff
  -        LOG_WARNING_F("Banning peer: {}", m_pPeer);
  -        m_pPeer->Ban(EBanReason::BadTxHashSet);
  +        LOG_INFO_F("Rotating txhashset sync peer: {}", m_pPeer);
  ```
- Nachweis: E41/E42.
- Risiko, Rücknahme: ein defekter Peer kann erneut versucht werden; Rotation wird geloggt; Rücknahme stellt den Ban wieder her.

### G-46 / G-47 — Reparatur der Chain-Speicher und Ping-Null-Guard
- Status: DONE (Load-Time-Mitigation)
- Commits: `0331ea1`
- Dateien: `src/BlockChain/ChainConsistency.h`, `src/BlockChain/BlockChainImpl.cpp`, `src/P2P/MessageProcessor.cpp`, `tests/src/BlockChain/Test_ChainConsistency.cpp`
- Rust-Referenz: n/a (Load-Time-Invarianten)
- Vorher: nach unsauberem Shutdown konnten Header-MMR und Candidate/Confirmed-Chain auseinanderlaufen; das Laden las über das MMR hinaus (0xC0000005). Der Ping-Handler dereferenzierte einen fehlenden Confirmed-Tip.
- Nachher: `ComputeChainRepair` vergleicht Leaf-/Entry-Zahlen und kürzt die Speicher vor dem Öffnen durch `ChainState` auf ein konsistentes Ziel; der Ping-Handler prüft den Tip und antwortet oder loggt eine Warnung statt abzustürzen.
- Diff: `src/BlockChain/ChainConsistency.h`, Commit `0331ea1` — load-time counts repair and Ping null guard.
- Nachweis: E39 (Unit + Live-Injektion in beide Richtungen).
- Risiko, Rücknahme: die Reparatur kann nicht committete Header verwerfen; Recovery-Kopien und Checkpoint begrenzen den Verlust; Rücknahme stellt `NODE/recovery-*` wieder her.

### G-48 — Atomarer Checkpoint für Chain-Speicher
- Status: DONE (Checkpoint; Hash-Validierung siehe G-23)
- Commits: `fdef6e6`, `1c4d020`, `7997ca5`
- Dateien: `src/BlockChain/DbCheckpoint.h`, `src/BlockChain/ChainState.*`, `src/BlockChain/BlockChainImpl.cpp`, `include/BlockChain/DbRecovery.h`
- Rust-Referenz: n/a (Crash-Konsistenz-Design; `DB_CONSISTENCY.md` §4)
- Vorher: `header_mmr.bin`, `candidate.chain` und `confirmed.chain` wurden unabhängig committet; ein Absturz zwischen den Commits hinterließ unterschiedliche Höhen, und der Ladepfad vertraute den Rohzahlen.
- Nachher: `ChainState::Commit` schreibt `chain.checkpoint` (52 Bytes, FNV-1a) per Temp+rename nach allen Store-Commits; beim Laden werden Invarianten geprüft, ein nachlaufendes MMR aus der Block-DB neu aufgebaut, nur überzählige Leaves verworfen, ein fehlender confirmed-Tail ergänzt, eine größenverifizierte Recovery-Kopie verlangt und bei einer Chain hinter dem Checkpoint der Start abgebrochen; `db_repaired`/`db_last_repair` werden veröffentlicht. Live: Hard-Kill seq 26→27; MMR −2000 Leaves neu aufgebaut, Header erhalten (E43/E44/E48).
- Diff: `src/BlockChain/DbCheckpoint.h`, Commit `fdef6e6`
  ```diff
  --- /dev/null
  +++ b/src/BlockChain/DbCheckpoint.h
  +struct DbCheckpointData
  +{
  +    uint64_t Sequence = 0;
  +    uint64_t MmrLeaves = 0;
  +    uint64_t CandidateEntries = 0;
  +    uint64_t ConfirmedEntries = 0;
  +};
  +// "GRINPPCK" magic, version, FNV-1a checksum; written via temp + rename
  ```
- Nachweis: E43, E44, E48.
- Risiko, Rücknahme: ein zu konservativer Checkpoint verwirft nicht committete Header; Recovery-Kopien bleiben; Rücknahme löscht `chain.checkpoint` und fällt auf die Zähler-Heuristik zurück.

### G-50 — Wiederholte DNS-Peer-Erkennung
- Status: DONE
- Commits: `b37caa0`
- Dateien: `src/P2P/Seed/Seeder.cpp`
- Rust-Referenz: `p2p/src/peers.rs` (Seeding)
- Vorher: DNS wurde einmal abgefragt (`m_usedDNS`); bei veralteten Seeds blieb die Node dauerhaft peerlos.
- Nachher: solange kein Kandidat existiert, wird DNS alle fünf Minuten erneut abgefragt; die Anzahl der zurückgegebenen Peers wird geloggt und hinzugefügt, mit bestehendem Seeder-Backoff und Reason-Log.
- Diff: `src/P2P/Seed/Seeder.cpp`, Commit `b37caa0`
  ```diff
  -    } else if (!m_usedDNS.exchange(true)) {
  -        std::vector<SocketAddress> peerAddresses = DNSSeeder::GetPeersFromDNS();
  +    } else if (m_lastDnsAttempt + std::chrono::minutes(5) <= now) {
  +        m_lastDnsAttempt = now;
  +        std::vector<SocketAddress> peerAddresses = DNSSeeder::GetPeersFromDNS();
  ```
- Nachweis: Live-Lauf (zwei DNS-Runden, Adressen hinzugefügt; Erreichbarkeit hängt vom Netzpfad ab).
- Risiko, Rücknahme: wiederholte DNS-Lookups; das Fünf-Minuten-Throttle begrenzt sie; Rücknahme stellt die Einmalabfrage wieder her.

### G-27 — PIHD nur Erkennung (kein Protokoll)
- Status: N/A für grin v5.5.2 (Bit erkannt, nie beworben)
- Commits: `a5c88bf`, `f73c7e8`, `348d87e`
- Dateien: `include/P2P/Capabilities.h`, `tests/src/Net/Test_P2PCompat.cpp`
- Rust-Referenz: `p2p/src/msg.rs` (v5.5.2 endet bei `KernelSegment = 28`)
- Vorher: PIHD wurde als implementierbarer Header-Sync-Modus behandelt, obwohl die Referenz keine PIHD-Nachrichten hat.
- Nachher: das `PIHD_HIST`-Bit (`0x80`) wird erkannt, aber nicht beworben und PIHD nie ausgehandelt; Header-Sync nutzt immer den Legacy-Pfad; ein Test pinnt das Verhalten.
- Nachweis: E33, `Test_P2PCompat`.
- Risiko, Rücknahme: keines für die Kompatibilität (die Referenz kann kein PIHD); Detection-only beibehalten.

### G-16 — Größenlimit für das TxHashSet-Archiv
- Status: DONE
- Commits: `1c4d020`
- Dateien: `include/P2P/Pipeline/TxHashSetLimits.h`, `src/P2P/Pipeline/TxHashSetPipe.cpp`, `tests/src/Net/Test_TxHashSetLimits.cpp`
- Rust-Referenz: n/a (defensive Grenze; die Referenz validiert das Archiv selbst)
- Vorher: `zipped_size` aus `TxHashSetArchiveMessage` wurde direkt zum Erzeugen/Schreiben der Archivdatei genutzt; null oder beliebig große Werte konnten die Platte füllen.
- Nachher: `IsPlausibleSize` verlangt `0 < size ≤ 8 GiB`; die Pipeline lehnt ein unplausibles Archiv vor dem Anlegen der Datei ab, setzt `TXHASHSET_SYNC_FAILED` und bannt `BadTxHashSet`.
- Diff: `include/P2P/Pipeline/TxHashSetLimits.h`, Commit `1c4d020` — 8 GiB archive bound before writing.
- Nachweis: E44, Unit `TxHashSetLimits`.
- Risiko, Rücknahme: ein legitimes Archiv über 8 GiB würde abgelehnt (Mainnet-Archive liegen weit darunter); die Konstante steht in einem Header; Rücknahme entfernt die Prüfung.

### G-04 — Validierung der PoW-Edge-Bits
- Status: DONE
- Commits: `9d9f1ae`
- Dateien: `include/PoW/PoWValidator.h`, `src/PoW/PoWValidator.cpp`, `tests/src/Consensus/Test_PoWEdgeBits.cpp`
- Rust-Referenz: `chain/src/pipe.rs` (`validate_pow_only`, `is_primary`/`is_secondary`), `core/src/pow/cuckatoo.rs` (`verify_size`)
- Vorher: nur `edge_bits == 29` wählte eine Cuckoo-Variante; jeder andere Wert (0, 24, 30, 64, 255) erreichte `Cuckatoo::Validate`, wo eine 64-Bit-Verschiebung um `edge_bits` undefiniert ist.
- Nachher: `PoWValidator::IsSupportedEdgeBits` akzeptiert Secondary 29 und Primary 31–63; `IsPoWValid` lehnt jede andere Größe vor dem Dispatch ab; die Grenztabelle ist unit-getestet.
- Diff: `src/PoW/PoWValidator.cpp`, Commit `9d9f1ae`
  ```diff
  +    if (!IsSupportedEdgeBits(header.GetEdgeBits())) {
  +        LOG_WARNING_F("Unsupported PoW edge bits {} for block {}", header.GetEdgeBits(), header);
  +        return false;
  +    }
  ```
- Nachweis: E50, `Test_PoWEdgeBits` (11 Assertions).
- Risiko, Rücknahme: künftige Primary-Größen über 63 bräuchten eine Konsensänderung; der Satz ist eine Konstante; Rücknahme entfernt den Guard.
### G-20 — Größenlimits der Parser
- Status: DONE (PIBD-/Segment-Parser; Fuzz außerhalb der Matrix)
- Commits: `9d9f1ae`
- Dateien: `src/PMMR/Models/Segment.h`, `tests/src/PMMR/Test_Segment.cpp`, `src/PMMR/Models/BitmapSegment.h`, `src/PMMR/Models/SegmentProof.h`
- Rust-Referenz: `core/src/ser.rs` (Grenzen), `core/src/core/pmmr/segment.rs` (`MAX_SEGMENT_READ_ITEMS`)
- Vorher: Segment-Zähler waren durch `MAX_SEGMENT_READ_ITEMS`, checked arithmetic und Message-Caps begrenzt, aber der Leaf-Vektor konnte bis zum Limit reservieren, bevor geprüft wurde, ob die Liste in die Nutzlast passt.
- Nachher: `numLeaves > remaining` wird vor der Reservierung abgelehnt; Limits für `SegmentProof`/`BitmapSegment`/`BitmapBlock`, checked arithmetic und Message-Cap bleiben; Tests decken riesige Counts, nicht-steigende Positionen und zu hohe Segmente ab.
- Diff: `src/PMMR/Models/Segment.h`, Commit `9d9f1ae`
  ```diff
  +        if (numLeaves > byteBuffer.GetRemainingSize()) {
  +            throw DESERIALIZATION_EXCEPTION("Segment leaf list does not fit in the payload");
  +        }
  ```
- Nachweis: E50, `Test_Segment` (21 Fälle, inkl. riesiger Counts).
- Risiko, Rücknahme: fehlerhafte Nutzlasten werden früher abgelehnt; gültige Segmente sind unberührt; Rücknahme entfernt den Leaf-Fit-Guard.
### G-21 — Atomare Aktivierung des Legacy-TxHashSet
- Status: DONE
- Commits: `2122e32`
- Dateien: `src/PMMR/Zip/TxHashSetZip.*`, `include/PMMR/TxHashSetManager.h`, `src/PMMR/TxHashSetManager.cpp`, `src/BlockChain/Processors/TxHashSetProcessor.cpp`, `include/PMMR/TxHashSetStaging.h`, `tests/src/PMMR/Test_TxHashSetStaging.cpp`
- Rust-Referenz: `chain/src/txhashset/txhashset.rs` (State-Installation), `servers/src/grin/sync/state_sync.rs`
- Vorher: `TxHashSetZip::Extract` löschte und überschrieb das Live-`NODE/TXHASHSET` vor der Validierung; ein fehlgeschlagener oder unterbrochener Legacy-Sync hinterließ einen Teilzustand.
- Nachher: das Zip wird nach `TXHASHSET.staging` entpackt und dort validiert, während das Live-State unberührt bleibt; bei Fehler wird Staging entfernt und das Live-State wieder geöffnet; bei Erfolg `live → backup` / `staging → live` unter `txhashset_activation.pending`, und `TxHashSetStaging::Recover` beim Start stellt das Backup wieder her oder entfernt veraltetes Staging.
- Diff: `src/BlockChain/Processors/TxHashSetProcessor.cpp`, Commit `2122e32`
  ```diff
  -        ITxHashSetPtr pTxHashSet = TxHashSetManager::LoadFromZip(m_config, path, pHeader);
  +        const fs::path stagingPath = TxHashSetStaging::Staging(livePath);
  +        ITxHashSetPtr pStaged = TxHashSetManager::LoadFromZip(m_config, path, pHeader, stagingPath);
  ```
- Nachweis: E49, `Test_TxHashSetStaging` (3 Fälle).
- Risiko, Rücknahme: ein Rename-Fehler stellt das Backup wieder her; ein Absturz zwischen den Renames wird über den Marker wiederhergestellt; Rücknahme entfernt den Staging-Pfad.
### G-22 — Datenbank-Backup vor der Migration
- Status: DONE
- Commits: `9c6bb34`
- Dateien: `include/Database/DbBackup.h`, `src/BlockChain/BlockChainImpl.cpp`, `tests/src/Database/Test_DbBackup.cpp`
- Rust-Referenz: n/a (Betriebssicherheit)
- Vorher: die Migrationen v1→v2 und v2→v3 schrieben die DB ohne vorherige Kopie in-place um.
- Nachher: `DbBackup` kopiert das offene RocksDB-Verzeichnis rekursiv per Stream-Kopie (ohne `LOCK`) nach `DB.backup-v<version>-<timestamp>` vor der Migration; eine fehlgeschlagene Kopie bricht die Migration ab; eine frische DB (Version 0) wird nicht kopiert.
- Diff: `include/Database/DbBackup.h`, Commit `9c6bb34` — recursive stream backup before migration.
- Nachweis: E47, `Test_DbBackup`.
- Risiko, Rücknahme: zusätzlicher Speicher/Zeit bei Migrationen; nur echte Migrationen kopieren; Rücknahme entfernt Backup und Aufruf.

### G-23 — Strukturelle Header-MMR-Validierung beim Laden
- Status: PARTIAL (strukturelle Prüfung DONE; Hash-/Root-Validierung offen)
- Commits: `1c4d020`, `0331ea1`, `7997ca5`
- Dateien: `include/PMMR/Common/MmrStructure.h`, `include/PMMR/HeaderMMR.h`, `src/PMMR/HeaderMMRImpl.h`, `src/BlockChain/BlockChainImpl.cpp`, `include/BlockChain/DbRecovery.h`, `tests/src/PMMR/Test_MmrStructure.cpp`
- Rust-Referenz: n/a (MMR-Invariante `N = 2L − popcount(L)`; siehe `DB_CONSISTENCY.md`)
- Vorher: die Rohknotenzahl in `header_mmr.bin` wurde vertraut; eine abgeschnittene Append-only-Datei konnte als gültiges MMR geladen werden.
- Nachher: `IsValidNodeCount` prüft `N = 2L − popcount(L)`; beim Laden wird eine unmögliche Größe auf die größte gültige gekürzt, ein nachlaufendes MMR wird aus der Block-DB neu aufgebaut (Header bleiben erhalten), nur überzählige Leaves werden verworfen; jede Änderung verlangt eine größenverifizierte Recovery-Kopie; eine Chain hinter dem Checkpoint bricht den Start ab. Live-Injektion: MMR −2000 Leaves → Header 3 317 117 erhalten, candidate.chain unverändert (E48).
- Diff: `include/PMMR/Common/MmrStructure.h`, Commit `1c4d020` — `N = 2L − popcount(L)`; rebuild lagging MMR from block DB (E48).
- Nachweis: E44 (strukturelle Regression/Guard), E48 (No-Loss-Rebuild; Abbruch bei fehlendem DB-Header).
- Risiko, Rücknahme: ein falsches Ungültig-Urteil kann Daten kürzen (der Vorfall); Guard, Regressionstest und Recovery-Kopien schützen; Rücknahme deaktiviert die strukturelle Prüfung und behält Zähler/Checkpoint.

### G-52 — Diagnose des historischen TxHashSet-Snapshots (OPEN)
- Status: OPEN (Archiv-Persistenz/Serving teilweise implementiert; Live-Transfer NOT VERIFIED)
- Commits: `7973d46`, `7997ca5`
- Dateien: `src/PMMR/TxHashSetManager.cpp`, `src/PMMR/TxHashSetImpl.cpp`, `src/P2P/Pipeline/TxHashSetPipe.cpp`, `include/P2P/TxHashSetArchive.h`, `src/P2P/Sync/StateSyncer.cpp`
- Rust-Referenz: `chain/src/txhashset/txhashset.rs` (Archiv-Snapshot)
- Vorher: ein fehlgeschlagener historischer Snapshot wurde verschluckt (`catch (...) { return; }`), und Rewind meldete nur das Commitment; die Ursache blieb unsichtbar.
- Nachher: die Aktivierung speichert den Horizont-Zustand unter `NODE/TXHASHSET/archive/TxHashSet.<height>.<hash>.zip` (`ArchivePibdState`), und `TxHashSetPipe` liefert dieses Archiv per Hash ohne historischen Rewind aus; Snapshot-Fehler loggen Flushed-/Requested-Höhe sowie Block/Input/Spent-Zahlen. Die Spent-Position-Lücke bleibt für beliebige historische Snapshots offen, der Live-Transfer ist weiterhin NOT VERIFIED.
- Diff: `src/PMMR/TxHashSetManager.cpp`, Commit `7973d46`
  ```diff
  --- a/src/PMMR/TxHashSetManager.cpp
  +++ b/src/PMMR/TxHashSetManager.cpp
  +            LOG_INFO_F(
  +                "TxHashSet snapshot: flushed height {}, requested height {}.",
  +                pFlushedHeader != nullptr ? pFlushedHeader->GetHeight() : 0,
  +                pHeader->GetHeight());
  ```
- Nachweis: E45/E46 (Diagnose), E48 (Archiv-Persistenz-Code; live pending).
- Risiko, Rücknahme: nur Logging; Rücknahme entfernt die Log-Zeilen.
## Bekannte offene Punkte (keine Karten)

- **G-52 (OPEN)**: historischer `TxHashSet`-Snapshot/Serving scheitert an einer Lücke der Spent-Positionen (`Rewind: Spent output not found … block 3 316 349`, null Einträge für einen Input). Legacy-Serving und G-41 bleiben NOT VERIFIED; Optionen in `TEST_RESULTS.md` E46 und `REMAINING.md`.
- **G-41 (OPEN)**: Live-Legacy-`txhashset.zip`-Fallback nicht abgeschlossen (Peers drosseln den Handshake; Serverseite durch G-52 blockiert). Der Client fordert das Archiv jetzt auf der auf 720 gerundeten Höhe an.
- **Live-PIBD-Serving** an einen Rust-Client: nur Unit-getestet.
- **G-23 Hash-Ebene** der MMR-Revalidierung; **G-21** atomare Legacy-Aktivierung; **G-20** verbleibende Parser-Limits.
- **G-04** PoW-Edge-Bits, **G-07** NRD, **G-49** Rangeproof-Validierungsperformance, Differential-/Fuzz-Suiten, Mainnet (außerhalb dieses Audits).
- **PIHD**: N/A für grin v5.5.2; Detection-only by design.

## Ausgeschlossen aus dem Delta

- G-44 (PIHD-Stillstand-Scaffold), G-45 (Mainnet-Dry-Run), G-49 (Rangeproof-Performance), G-51 (dieses Paket): abgeschlossen (Auditpaket; Eigentümer-Abnahme 01.11.2026).
- Reine Dokumentations-Commits (`docs: …`): in der Nachweiskarte gelistet, nicht als Karten.
## Aktualisierung 02.11.2026 — HEAD `a49dfe0`

Basis unverändert (`5f511e6`). Dieser Stand enthält den G-09-Candidate-Merge,
die G-23-Hash-Revalidierung, KernelIndex/get_kernel, NRD-Regeln und das lokale
CI-Gate. Artefakte: `GrinNode E16A014E…`, `Tests CC6472B1…`; Suite **194 / 2227 PASS**.

### G-09 — PIBD-Serving (PARTIAL, Live läuft)

- Candidate-Patch zusammengeführt: `GetRawAt` prüft Compaction/Grenzen; ein
  erforderliches Blatt ohne Daten liefert `MissingLeaf`; ein optionales Blatt
  trägt einen prune-aware Hash; ein vollständig geprunter Segmentpfad läuft die
  **gesamte** `familyBranch` bis zum ersten gespeicherten Vorfahren.
- `PibdStateWriter`/`PibdTreeRebuilder` speichern hash-only Boundary-Leaves.
- Read-Path: `AppendOnlyFile`, `MMRHashUtil`, `DataFile` sind grenzgeprüft;
  `IsCompacted` berücksichtigt die Leaf-Set-Ausnahme; der prune-ignoring
  Fallback wurde entfernt.
- Serving-Gate: nur `NOT_SYNCING` und ein non-genesis Flushed-State.
- Offline-Sweep auf `pp-ckpt` H=3.317.027: Output **0/1674**, Rangeproof
  **0/1674**, Kernel **0/1638** Fehler; gesamt **4993/4993 PASS**.
- Output-/Rangeproof-Fixture (11,1100) stimmte byte-genau mit dem
  Rust-validierten Fixture überein; das externe Kernel-Fixture war unvollständig,
  der vollständige Sweep validiert alle Kernel-Segmente.
- Live: ein C++-PIBD-Client lädt den State am Horizont 3.315.600; der
  Rust-Client folgt nach der Aktivierung. **Live-G-09 ist NOT VERIFIED** —
  Offline-PASS ist keine Live-Abnahme.

### G-23 — Hash-/Root-Revalidierung (PARTIAL)

- Bei vorhandenem Checkpoint prüft der Startup jeden Header-Hash/Height/Link und
  den Header-MMR-Leaf-Hash; ein offener TxHashSet muss zu seinen Flushed-Header-
  Roots passen.
- Live-Startup mit 3,3M-Header-Kette: API nach 75,4 s bereit, kein Mismatch.
- Ein Ein-Byte-Korruptionstest für `header_mmr.bin` existiert; vollständige
  Startup-Fault-Injection und ein PMMR-Node-Sweep sind **NOT VERIFIED**.

### G-53 — KernelIndex/get_kernel (PARTIAL)

- Persistenter generationenbasierter `excess → OutputLocation`-Index mit
  Tip/Root/Count/Checksum; `get_kernel` Fast Path plus begrenzter Direct-Scan-
  Fallback; N-06 nutzt jetzt 1-based PMMR-Positionen.
- Drei Index-Tests / 48 Assertions. Live `/v2`, Reorg und Pagination sind NOT VERIFIED.

### G-07 — NRD (PARTIAL)

- Doppelte NRD-Kernel im Block und `diff < relative_height` werden abgelehnt;
  Zwei-Wochen-Cutoff; Rewind/Startup/PIBD-Rebuild; NRD vor HF3 verboten.
- Kein persistenter inkrementeller Index, kein vollständiger Block-/Reorg-NRD-Test,
  kein Live-NRD-Peer.

### G-24 — Build (PARTIAL)

- `tools/build/ci.ps1` (Build + isolierte Tests + Manifest) und `BUILD_PINS.md`
  hinzugefügt; Hosted-CI und vcpkg-Lauf auf sauberem Checkout sind NOT VERIFIED.

### N-Funde

- Behoben: N-01 (Commit-Threads), N-02 (Flush/Checkpoint-Replace), N-03
  (Recovery-Zweig), N-04 (Async-Rate-Counter), N-05 (16-MiB-Write-Queue), N-06
  (get_kernel-Positionen), N-07 (get_outputs-Fehler).
- Fault-Injection für N-01/N-02/N-05 und Lasttests sind NOT VERIFIED.

### Offen

- **G-09 live**: Rust-PIBD-Client abschließen, Peer/H/Antwortzahl festhalten.
- G-23 Fault-Injection/vollständiger PMMR-Node-Sweep.
- G-53 Live-`/v2`/Reorg/Pagination.
- G-07 persistenter Index/vollständiger Block-/Reorg-Test.
- G-24 Hosted-CI; G-49 Rangeproof-Beschleunigung ist zurückgestellt.
