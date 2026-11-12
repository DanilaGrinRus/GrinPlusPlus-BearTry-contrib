# Beta — замороженный node-only кандидат

Дата freeze: 2026-11-10. Папка `beta\` — прежний `Node\Beta`, сохранён как есть.

## Артефакты

Runtime-пара (`start-node.ps1` запускает её):

| Файл | Размер | SHA-256 |
|---|---:|---|
| `beta\GrinNode.exe` | 8 462 336 | `B565612BCA31D9EFE4117FC37FD2C7680ED8B84D9830E47591EFA3C28BA3D64D` |
| `beta\Tests.exe` | 11 612 672 | `2B5BD9853979ED7AEFB4B2C3E6D9B27B6BB397E6E816CE2D52FC9E2E74B25DF3` |
| `beta\GrinDbTool.exe` | 3 891 712 | `8C0113D2E11A3655DDE2372515974E6B7D832076CC5229C82706616B89F00656` |

Accepted rebuild из recovered source (внутри `beta\Source`):

| Файл | Размер | SHA-256 |
|---|---:|---|
| `beta\Source\bin\Release\Release\GrinNode.exe` | 8 476 672 | `B52B6C5D84B25375463634F087E39CD65CF9BF2F3A1E4DB090E7AEAFEBB69C4A` |
| `beta\Source\bin\Release\Release\Tests.exe` | 11 655 168 | `96E80CBB9BF3C3C5382126A304BF0B628E4B38573E9B84CCAF602ECC471C6708` |

Source freeze: `B698079BBA24BFEDCD8F33BA4ACDDA669820ABAA1D6FC189C12B05B26D2A3609`
(1548 файлов, buildable, не byte-identical B565).
Манифесты: `beta\FROZEN.json`, `beta\FREEZE_SHA256SUMS.json`, `beta\SHA256SUMS.json`.

## Запуск

```powershell
.\beta\start-node.ps1 -ConfigPath <config.json>
# или
.\beta\GrinNode.exe --mainnet --headless --no-wallet --config-path <config.json>
```

Beta — только нода; кошелёк+UI и публичный inbound в неё не входят.

## Тесты и границы

- Full suite (frozen pair): **468/468 PASS, 19 309 assertions**.
- Rebuilt overlay: **471/471 PASS**, 19 340/19 343/19 346 assertions (×3).
- Linux: **464/464 PASS, 19 282 assertions**; mainnet sync на Linux — NOT VERIFIED.
- Что вошло: InboundAdmission/лимиты, P2P handshake/archive hardening,
  rate-limit loopback/public, SyncLogGate, TxHashSet progress, wallet send error,
  encryption format 2, DB tool, ENOSPC, `.api_secret` ACL, Tor temp/atomic replace,
  PIBD archive serving default-off с mainnet refusal.

## Ограничения

- Public Mainnet reorg — NOT VERIFIED; test-chain reorg PASS.
- Direct inbound/firewall — NOT VERIFIED; relay GCP PASS с ограничением source IP.
- Внешний >500/min false-ban — риск, live не воспроизведён.
- 24h stability, Linux/FreeBSD/macOS full sync, live Tor bridge, wallet+node — вне/NOT VERIFIED.

## RC1 и Linux

- `Node\RC1\GrinNode.exe` — reference RC1:
  `CD941F325B6E7595E9A777DDA90B486BCAFB0473322CBD95E3FCA5133344FD48`
  (в текущей папке beta отсутствует; сохранён только Beta).
- Linux-сборка: `beta\Linux\GrinNode` `078EF85D…`, `Tests` `729F42A6…`,
  image tar `49737E18…`.

## Provenance и сборка

- Recovered source `beta\Source` происходит из `source-vcpkg.tar.gz`
  (`FE1C2E53…`) + task-72 patch (`C4E042F1…`); `SOURCE_PROVENANCE.json` внутри.
- Сборка: см. [BUILD.md](BUILD.md); путь `beta\Source` и sysroot `.toolchain`
  (в чистке удалён; для пересборки нужен локальный sysroot, см. BUILD.md).
- Исторические VPS-эндпойнты (Windows `35.222.128.193:8080`, Linux builder
  `136.114.230.42:8080`) относились к тому прогону и больше не поддерживаются.