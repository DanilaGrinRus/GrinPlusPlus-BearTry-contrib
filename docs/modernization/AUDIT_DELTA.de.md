# AUDIT_DELTA (DE) — Grin++ 1.2.9 → Kompatibilität mit Grin v5.5.2

Basis: Grin++ 1.2.9 (`5f511e6`). Delta aktualisiert: 31.10.2026, HEAD `256cbe4`.
Begleitdokumente: `AUDIT_PACKAGE.md` (Paketregeln), `GAP_MATRIX.md` (Status aller
Lücken), `TEST_RESULTS.md` (Nachweise E1–E47), `DB_CONSISTENCY.md` (Design der
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
- Nachweis: `BUILD.md`, E6–E9 (sauberer Build, keine deaktivierten Warnungen), SHA-256 in `BUILD.md`.
- Risiko, Rücknahme: Risiko ist ein veralteter/unvollständiger Sysroot; Rücknahme durch Zurücksetzen der Build-Skripte/CMake-Änderungen und Nutzung der Systemabhängigkeiten.

### G-19 — Speichergrenzen-sichere Deserialisierung
- Status: DONE
- Commits: `fc9e988`
- Dateien: `include/Core/Serialization/ByteBuffer.h`, `tests/src/Core/Serialization/Test_ByteBuffer.cpp`
- Rust-Referenz: `core/src/ser.rs` (Grenzen des Readers)
- Vorher: varint/vector/string-Reader konnten über die Nutzlast hinaus lesen; Größenrechnung wurde nicht gegen den Restpuffer geprüft.
- Nachher: jeder Lesezugriff wird gegen das Pufferende geprüft (protokollversionsabhängig); abgeschnittene und übergroße Eingaben scheitern sauber; Unit-Tests decken beides ab.
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
- Nachweis: E31 (Live-Segment passt zu `header.output_root`), E42.
- Risiko, Rücknahme: ein Off-by-One im Intervall bricht die Segmentvalidierung; Orakel/Live-Tests pinnen es; Rücknahme stellt die Tip-Anfrage wieder her (bekannt fehlerhaft).

### G-32 / G-35 — Bitmap-Blockgröße (64 Chunks) und MSB-first-Bitreihenfolge
- Status: DONE
- Commits: `39b77a9`, `c6ab7ff`
- Dateien: `src/PMMR/Models/BitmapBlock.h`, `src/PMMR/Models/BitmapChunk.h`, `src/P2P/PIBD/BitmapSegmentAssembler.h`, `tests/src/PMMR/Test_Segment.cpp`
- Rust-Referenz: `chain/src/txhashset/bitmap_accumulator.rs:470,213`
- Vorher: ein `BitmapBlock` hatte 16 Chunks und indizierte Bits LSB-first; sparse (positive/negative) Segmente wurden falsch gelesen, echte Segmente deserialisierten nicht.
- Nachher: 2^16 Bits = 64 Chunks × 1024; Bit `n` liegt in Byte `n/8`, Bit `7−(n%8)` (MSB-first, wie `bitvec::to_bytes`); Raw-Chunks serialisieren die 1024 Bytes ohne Längenpräfix.
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
- Commits: `4dbae1b`, `c06888b`
- Dateien: `src/P2P/Sync/BlockSyncPolicy.h`, `src/P2P/Sync/BlockSyncer.cpp`, `src/P2P/Sync/HeaderSyncer.cpp`, `src/P2P/Seed/Seeder.cpp`, `include/P2P/SyncStatus.h`, `src/P2P/MessageProcessor.cpp`
- Rust-Referenz: `servers/src/grin/sync/body_sync.rs:149,191-204`, `header_sync.rs:135-141`
- Vorher: `BlockSyncer` bannte einen Peer nach zwei Timeouts wegen `FraudHeight`, `m_slowPeers` verfiel nie, das Anfragefenster ignorierte Peer-Höhen, Seeder/Syncer schwiegen nach den Bans.
- Nachher: ein Timeout wird gezählt und der Block neu zugewiesen, nie gebannt; Anfragen sind auf `min(candidate tip, network height, per-connection height)` und Batch ≤100 begrenzt; Retry 6 s (1 s mit Fortschritt) plus Backoff; ein langsamer Peer bekommt 60 s Cooldown; `FraudHeight` erfordert höhere behauptete Arbeit und 120 s ohne Header-Fortschritt; Seeder loggt Grund/Backoff; `/v1/status` zeigt Wartezeit und Zähler.
- Nachweis: E37 (32-Minuten-Live-Fenster), E42 (4 222 Reassignments, null Fraud-Bans).
- Risiko, Rücknahme: ohne Bans kann ein nicht antwortender Peer den Catch-up bremsen; Cooldown/Rotation begrenzen es; Rücknahme stellt den Ban-Pfad wieder her.

### G-42 — Legacy-State-Sync: Peer-Rotation statt Ban
- Status: DONE
- Commits: `bbf9a56`
- Dateien: `src/P2P/Sync/StateSyncer.cpp`
- Rust-Referenz: `servers/src/grin/sync/syncer.rs`, `state_sync.rs`
- Vorher: `RequestState` bannte den Peer mit `BadTxHashSet`, sobald der Zip 10 s keinen Fortschritt machte oder das 120-Minuten-Limit erreichte; die Node konnte ohne Peers bleiben.
- Nachher: ein langsamer Peer wird rotiert (`Rotating txhashset sync peer`); ein wirklich ungültiges Archiv wird weiterhin nach fehlgeschlagener Validierung von der Pipeline gebannt.
- Nachweis: E41/E42.
- Risiko, Rücknahme: ein defekter Peer kann erneut versucht werden; Rotation wird geloggt; Rücknahme stellt den Ban wieder her.

### G-46 / G-47 — Reparatur der Chain-Speicher und Ping-Null-Guard
- Status: DONE (Load-Time-Mitigation)
- Commits: `0331ea1`
- Dateien: `src/BlockChain/ChainConsistency.h`, `src/BlockChain/BlockChainImpl.cpp`, `src/P2P/MessageProcessor.cpp`, `tests/src/BlockChain/Test_ChainConsistency.cpp`
- Rust-Referenz: n/a (Load-Time-Invarianten)
- Vorher: nach unsauberem Shutdown konnten Header-MMR und Candidate/Confirmed-Chain auseinanderlaufen; das Laden las über das MMR hinaus (0xC0000005). Der Ping-Handler dereferenzierte einen fehlenden Confirmed-Tip.
- Nachher: `ComputeChainRepair` vergleicht Leaf-/Entry-Zahlen und kürzt die Speicher vor dem Öffnen durch `ChainState` auf ein konsistentes Ziel; der Ping-Handler prüft den Tip und antwortet oder loggt eine Warnung statt abzustürzen.
- Nachweis: E39 (Unit + Live-Injektion in beide Richtungen).
- Risiko, Rücknahme: die Reparatur kann nicht committete Header verwerfen; Recovery-Kopien und Checkpoint begrenzen den Verlust; Rücknahme stellt `NODE/recovery-*` wieder her.

### G-48 — Atomarer Checkpoint für Chain-Speicher
- Status: DONE (Checkpoint; Hash-Validierung siehe G-23)
- Commits: `fdef6e6`, `1c4d020`
- Dateien: `src/BlockChain/DbCheckpoint.h`, `src/BlockChain/ChainState.*`, `src/BlockChain/BlockChainImpl.cpp`, `include/BlockChain/DbRecovery.h`
- Rust-Referenz: n/a (Crash-Konsistenz-Design; `DB_CONSISTENCY.md` §4)
- Vorher: `header_mmr.bin`, `candidate.chain` und `confirmed.chain` wurden unabhängig committet; ein Absturz zwischen den Commits hinterließ unterschiedliche Höhen, und der Ladepfad vertraute den Rohzahlen.
- Nachher: nach allen vier Store-Commits schreibt `ChainState::Commit` `chain.checkpoint` (52 Bytes: Magic/Version/Sequence/`mmr_leaves`/Candidate/Confirmed/FNV-1a) per Temp+rename; `BlockChain::Create` repariert jeden Store auf `min(actual, checkpoint)` und schreibt einen frischen Checkpoint; die vier Dateien werden vor der Reparatur nach `NODE/recovery-*` kopiert, `db_repaired` erscheint in `/v1/status`.
- Nachweis: E43 (Hard-Kill-Neustart seq 26→27; Live-Mismatch-Reparatur), E44 (Recovery/`db_repaired`).
- Risiko, Rücknahme: ein zu konservativer Checkpoint verwirft nicht committete Header; Recovery-Kopien bleiben; Rücknahme löscht `chain.checkpoint` und fällt auf die Zähler-Heuristik zurück.

### G-50 — Wiederholte DNS-Peer-Erkennung
- Status: DONE
- Commits: `b37caa0`
- Dateien: `src/P2P/Seed/Seeder.cpp`
- Rust-Referenz: `p2p/src/peers.rs` (Seeding)
- Vorher: DNS wurde einmal abgefragt (`m_usedDNS`); bei veralteten Seeds blieb die Node dauerhaft peerlos.
- Nachher: solange kein Kandidat existiert, wird DNS alle fünf Minuten erneut abgefragt; die Anzahl der zurückgegebenen Peers wird geloggt und hinzugefügt, mit bestehendem Seeder-Backoff und Reason-Log.
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
- Nachweis: E44, Unit `TxHashSetLimits`.
- Risiko, Rücknahme: ein legitimes Archiv über 8 GiB würde abgelehnt (Mainnet-Archive liegen weit darunter); die Konstante steht in einem Header; Rücknahme entfernt die Prüfung.

### G-22 — Datenbank-Backup vor der Migration
- Status: DONE
- Commits: `9c6bb34`
- Dateien: `include/Database/DbBackup.h`, `src/BlockChain/BlockChainImpl.cpp`, `tests/src/Database/Test_DbBackup.cpp`
- Rust-Referenz: n/a (Betriebssicherheit)
- Vorher: die Migrationen v1→v2 und v2→v3 schrieben die DB ohne vorherige Kopie in-place um.
- Nachher: `DbBackup` kopiert das offene RocksDB-Verzeichnis rekursiv per Stream-Kopie (ohne `LOCK`) nach `DB.backup-v<version>-<timestamp>` vor der Migration; eine fehlgeschlagene Kopie bricht die Migration ab; eine frische DB (Version 0) wird nicht kopiert.
- Nachweis: E47, `Test_DbBackup`.
- Risiko, Rücknahme: zusätzlicher Speicher/Zeit bei Migrationen; nur echte Migrationen kopieren; Rücknahme entfernt Backup und Aufruf.

### G-23 — Strukturelle Header-MMR-Validierung beim Laden
- Status: PARTIAL (strukturelle Prüfung DONE; Hash-/Root-Validierung offen)
- Commits: `1c4d020`, `0331ea1`
- Dateien: `include/PMMR/Common/MmrStructure.h`, `include/PMMR/HeaderMMR.h`, `src/PMMR/HeaderMMRImpl.h`, `src/BlockChain/BlockChainImpl.cpp`, `tests/src/PMMR/Test_MmrStructure.cpp`
- Rust-Referenz: n/a (MMR-Invariante `N = 2L − popcount(L)`; siehe `DB_CONSISTENCY.md`)
- Vorher: die Rohknotenzahl in `header_mmr.bin` wurde vertraut; eine abgeschnittene Append-only-Datei konnte als gültiges MMR geladen werden.
- Nachher: `IsValidNodeCount` prüft `N = 2L − popcount(L)`, `LargestValidNodeCount`/`LeavesForNodes` leiten ein sicheres Ziel ab; beim Laden wird eine unmögliche Größe auf die größte gültige gekürzt, mit einem Guard, der ohne ableitbare Leaf-Zahl nicht kürzt; ein Regressionstest deckt 3,3 Mio. Leaves ab.
- Nachweis: E44 (die erste Version hatte eine falsche Binärsuchgrenze und kürzte ein gesundes MMR fälschlich; behoben, geschützt, Regressionstest).
- Risiko, Rücknahme: ein falsches Ungültig-Urteil kann Daten kürzen (der Vorfall); Guard, Regressionstest und Recovery-Kopien schützen; Rücknahme deaktiviert die strukturelle Prüfung und behält Zähler/Checkpoint.

### G-52 — Diagnose des historischen TxHashSet-Snapshots (OPEN)
- Status: OPEN (nur Diagnose; keine funktionale Korrektur)
- Commits: `7973d46`
- Dateien: `src/PMMR/TxHashSetManager.cpp`, `src/PMMR/TxHashSetImpl.cpp`, `src/P2P/Pipeline/TxHashSetPipe.cpp`
- Rust-Referenz: `chain/src/txhashset/txhashset.rs` (Archiv-Snapshot)
- Vorher: ein fehlgeschlagener historischer Snapshot wurde verschluckt (`catch (...) { return; }`), und Rewind meldete nur das Commitment; die Ursache blieb unsichtbar.
- Nachher: der Sender loggt Flushed-/Requested-Höhe, Snapshot-Pfad und Größe; Rewind loggt Blockhöhe, Input-Anzahl und Spent-Position-Anzahl; die funktionale Lücke bleibt offen.
- Nachweis: E45/E46 (`flushed 3317111 → requested 3314160`, Block `3316349`, 1 Input, 0 Spent-Positionen).
- Risiko, Rücknahme: nur Logging; Rücknahme entfernt die Log-Zeilen.
## Bekannte offene Punkte (keine Karten)

- **G-52 (OPEN)**: historischer `TxHashSet`-Snapshot/Serving scheitert an einer Lücke der Spent-Positionen (`Rewind: Spent output not found … block 3 316 349`, null Einträge für einen Input). Legacy-Serving und G-41 bleiben NOT VERIFIED; Optionen in `TEST_RESULTS.md` E46 und `REMAINING.md`.
- **G-41 (OPEN)**: Live-Legacy-`txhashset.zip`-Fallback nicht abgeschlossen (Peers drosseln den Handshake; Serverseite durch G-52 blockiert). Der Client fordert das Archiv jetzt auf der auf 720 gerundeten Höhe an.
- **Live-PIBD-Serving** an einen Rust-Client: nur Unit-getestet.
- **G-23 Hash-Ebene** der MMR-Revalidierung; **G-21** atomare Legacy-Aktivierung; **G-20** verbleibende Parser-Limits.
- **G-04** PoW-Edge-Bits, **G-07** NRD, **G-49** Rangeproof-Validierungsperformance, Differential-/Fuzz-Suiten, Mainnet (außerhalb dieses Audits).
- **PIHD**: N/A für grin v5.5.2; Detection-only by design.

## Ausgeschlossen aus dem Delta

- G-44 (PIHD-Stillstand-Scaffold), G-45 (Mainnet-Dry-Run), G-49 (Rangeproof-Performance), G-51 (dieses Paket): geplant, noch keine Codeänderungen.
- Reine Dokumentations-Commits (`docs: …`): in der Nachweiskarte gelistet, nicht als Karten.
