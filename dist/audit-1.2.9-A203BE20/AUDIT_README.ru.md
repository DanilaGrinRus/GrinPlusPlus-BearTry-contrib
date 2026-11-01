# Grin++ 1.2.9 — аудиторский пакет совместимости (RU)

Статус: **готов к внешнему ревью**; независимая приёмка ещё открыта.

## Содержимое

- `GAP_MATRIX.md` — 53 записи против grin v5.5.2 (`dbaff34`): 44 закрыто,
  3 частично (G-09, G-23, G-24), 1 в работе (G-51), 0 открыто, 3 запланировано
  (G-44, G-45, G-49), 1 отложено (G-07), 1 N/A (G-27).
- `AUDIT_DELTA.en/de/ru` — 29 карточек, 8 обязательных полей, по 18 diff-блоков,
  `check_audit_delta.ps1` PARITY OK, 0 TODO.
- `TEST_RESULTS.md` — E1–E54, включая живой прогон G-52 → G-41.
- `REMAINING.md`, `ACCEPTANCE_REPORT.md`, `DB_CONSISTENCY.md`,
  `AUDIT_COMMENTS.md`, `AUDIT_CHECKLIST.md`.
- Артефакты (SHA-256):

```
GrinNode.exe  A203BE2089947493630552B2A74907C3E56D14F08977C28ECDFDCF686E0583E3  7 624 704
Tests.exe     38B9AF99205EA1FC3247305FAE133F0FA8C8F3A6B01E4F564B0818B361DE555F  8 643 072
```

- Тесты: **176 кейсов / 1907 проверок PASS**.

## Живое доказательство (E54)

- PIBD на горизонте 3 314 160, валидация и активация; архив
  `TxHashSet.3314160.1145e8daa674.zip` = **3 117 206 023 байт**.
- Legacy-клиент скачал все 3,1 ГБ, распаковал и валидировал
  (`Successfully validated TxHashSet`, 06:19), затем догнал блоки до
  **FULLY_SYNCED на 3 317 664** (06:53).
- Полный синк от genesis-заголовков: ~2 ч 54 мин (block DB была предзаполнена,
  см. E53).

## Известные ограничения

- G-09: живой PIBD-serving Rust-клиенту и полное завершение resume не проверены.
- G-23: hash-level ревалидация MMR (`--validate-on-load`) отсутствует.
- G-24: CI и пин vcpkg открыты; offline MSVC-сборка проверена.
- G-51: внешняя приёмка, пакет лицензий/notices и `dist/audit-*` открыты.
- Block sync сервера после активации архива остался на горизонте; клиент взял
  блоки у внешних пиров. Нужен отдельный разбор.
- Произвольные исторические snapshot ниже горизонта сознательно не
  поддерживаются (разрыв spent-позиций).
- Fuzz/санитайзеры, differential-fixtures, reorg и регресс кошелька/API/GUI —
  фазы 5–7 вне матрицы.

## Воспроизведение

```powershell
pwsh tools/build/build.ps1 -Target Tests
$env:HOMEDRIVE='C:'; $env:HOMEPATH='\DHW\.tmp\testhome'
bin\Release\Release\Tests.exe --reporter compact
pwsh tools/audit/check_audit_delta.ps1
```

Только testnet/floonet; mainnet-средства и реальные ключи не используются.
