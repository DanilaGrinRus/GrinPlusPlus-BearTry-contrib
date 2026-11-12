# Grin++ — BearTry contrib releases

Форк `GrinPlusPlus/GrinPlusPlus`, используемый как рабочая площадка и витрина релизов.
Ветка `main` — **только релизы**; полный исходный код и история — в ветке
[`Main`](https://github.com/DanilaGrinRus/GrinPlusPlus-BearTry-contrib/tree/Main)
(default) и остальных ветках форка.

## Релизы

| Релиз | Что это | Скачать |
|---|---|---|
| **Grin++ 2.0.0** | нода + кошелёк + UI; PIBD приём/раздача, выключатель Tor | [v2.0.0](https://github.com/DanilaGrinRus/GrinPlusPlus-BearTry-contrib/releases/tag/v2.0.0) |
| **Node Beta 1.2.9** | замороженный node-only Beta (Windows + Linux + Source) | [beta-1.2.9](https://github.com/DanilaGrinRus/GrinPlusPlus-BearTry-contrib/releases/tag/beta-1.2.9) |

Полные portable-архивы приложены к релизам. Ниже — установщик, исходники и памятки.

## Быстрый старт
1. Скачать архив релиза и распаковать.
2. Для 2.0: `Start Grin++.cmd` (portable) либо установщик + `GrinNode.exe`/`tor\` рядом.
3. Для Beta: `beta\start-node.ps1 -ConfigPath <config.json>` (только нода).

Порты 2.0: P2P 3414, REST 3413, wallet 3421 (loopback). Входящие — см. [docs/P2P.md](docs/P2P.md).

## Структура main

```text
README.md
releases/
  Grin++-2.0/   README, SHA256SUMS, установщик, sources/
  Beta/         README (описание freeze, артефакты, ограничения)
docs/
  P2P.md
  enable-p2p-firewall.cmd
```

Секретов, seed, `.api_secret` и данных сети нет.