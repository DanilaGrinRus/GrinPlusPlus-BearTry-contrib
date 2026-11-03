# Grin++ 1.2.9 — Kompatibilitäts-Auditpaket (DE)

## Aktueller Verweis — 03.11.2026

Der Paketstatus belegt keine Mainnet-Bereitschaft. Aktualisierter Stand und Grenzen: `AUDIT_DELTA.de.md`; operative Übersicht und Nachweise: `THREE_LINE_STATUS.md`. G-09: Szenario 7B PASS, vollständiges G-09 und öffentliches PIBD NOT VERIFIED; Wiederherstellung des alten Archivs FAIL, Grundursache NOT VERIFIED. Build-10 Gesamtsuite 254/253/1; Bodyless-NRD und Kill-Recovery NOT VERIFIED. Die operative Übersicht ist mit Primärnachweisen abzugleichen.

## Historisches Paket vom 01.11.2026

Die folgenden Zähler, Abnahmestände, Ergebnisse und Hashes gehören zu einem früheren Paket und beschreiben nicht den aktuellen Build.

Status: **durch Eigentümer-Abnahme geschlossen (01.11.2026)**; unabhängiger Prüfer nicht benannt.

## Inhalt

- `GAP_MATRIX.md` — 53 Einträge gegen grin v5.5.2 (`dbaff34`): 45 geschlossen,
  3 teilweise (G-09, G-23, G-24), 0 in Arbeit, 0 offen, 3 geplant
  (G-44, G-45, G-49), 1 zurückgestellt (G-07), 1 N/A (G-27).
- `AUDIT_DELTA.en/de/ru` — 29 Karten, 8 Pflichtfelder, je 18 Diff-Blöcke,
  `check_audit_delta.ps1` PARITY OK, 0 TODO.
- `TEST_RESULTS.md` — E1–E59 inkl. Live-Lauf G-52 → G-41 und G-51-Abschlussprüfung.
- `REMAINING.md`, `ACCEPTANCE_REPORT.md`, `DB_CONSISTENCY.md`,
  `AUDIT_COMMENTS.md`, `AUDIT_CHECKLIST.md`.
- Release-Build-Artefakte (SHA-256; nicht im Paket enthalten, gegen `bin/Release/Release` verifiziert):

```
GrinNode.exe  E16A014EC3AD1766637E06544790AAE815825A9DCFBF18EB1516F512799B6328  7.624.704
Tests.exe     CC6472B1E4A5081DC7770C275FEF927FA876AB257EBD0AD663A56EBCF273457E  8.651.264
```

- Tests: **187 Fälle / 2066 Assertions PASS**.

## Live-Nachweis (E54)

- PIBD am Horizont 3.314.160, validiert und aktiviert; Archiv
  `TxHashSet.3314160.1145e8daa674.zip` = **3.117.206.023 Bytes**.
- Legacy-Client lud die vollen 3,1 GB, entpackte und validierte sie
  (`Successfully validated TxHashSet`, 06:19), danach Block-Sync bis
  **FULLY_SYNCED auf 3.317.664** (06:53).
- Vollsynchronisation ab Genesis-Headern: ~2 h 54 min (Block-DB war vorbelegt,
  siehe E53).

## Bekannte Grenzen

- G-09: Live-PIBD-Serving an einen Rust-Client und vollständiger Resume sind
  nicht verifiziert.
- G-23: Hash-Level-MMR-Revalidierung (`--validate-on-load`) fehlt.
- G-24: CI-Anbindung und vcpkg-Pin offen; Offline-MSVC-Build ist verifiziert.
- G-51 durch Eigentümer-Abnahme geschlossen (01.11.2026); unabhängiger Prüfer nicht benannt. Lizenz-/Notice-Paket (`D5`) und `dist/audit-*` (`D9`) sind vollständig.
- Block-Sync des Servers nach der Archiv-Aktivierung blieb am Horizont stehen;
  der Client holte die Blöcke von externen Peers. Separate Analyse offen.
- Beliebig alte historische Snapshots unterhalb des Horizonts bleiben bewusst
  nicht unterstützt (Spent-Position-Lücke).
- Fuzz/Sanitizer, Differential-Fixtures, Reorg- und Wallet/API/GUI-Regression
  sind Phase 5–7 außerhalb dieser Matrix.

## Reproduktion

```powershell
pwsh tools/build/build.ps1 -Target Tests
$env:HOMEDRIVE='C:'; $env:HOMEPATH='\DHW\.tmp\testhome'
bin\Release\Release\Tests.exe --reporter compact
pwsh tools/audit/check_audit_delta.ps1
```

Nur Testnet/Floonet; keine Mainnet-Gelder oder echten Schlüssel.
