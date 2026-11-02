# Grin++ 1.2.9 — Kompatibilitäts-Auditpaket (DE)

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
GrinNode.exe  D15174D98E75D706C781DAD760C1410AEC069784A30B1F4FB1D1270259779B74  7.624.704
Tests.exe     1D38C73B96563E0DC00613FA023A1E937817200B91445C741776E0B0CB776E9B  8.651.264
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
