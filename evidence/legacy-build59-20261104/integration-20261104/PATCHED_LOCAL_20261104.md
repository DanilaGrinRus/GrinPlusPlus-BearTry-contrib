# Legacy patch v2 — применение и локальная проверка

Дата: 2026-11-04. Ветка/HEAD при интеграции: `9f0302f`; поверх него применён
патч v2 (26 путей, включая новый тест).

## Патч

- `C:\DHW\Gplus\evidence\legacy-build59-20261104\patches\legacy-build59-fixes.patch`
- SHA-256 `9FFB4569FFF789F44B6D7D206BA9C0146CCA677639B29A9E8690D9DC4C4059B6`.
- Проверочный run: `runs\legacy-20261104-090952-8ea4a0`, verdict **PASS**.
- Все 26 файлов в рабочем дереве свериены с `patched-files.json`:
  **26/26 SHA совпали**. Поэтому `git apply --check` на текущем дереве ожидаемо
  сообщает «patch does not apply» (патч уже нанесён); повторно не применялся.

## Сборка

- Из рабочего дерева с патчем собраны Release `Tests.exe` и `GrinNode.exe`
  штатной сборкой `/W4 /WX`.
- `Tests.exe` SHA-256 `BB165941E0BE2389D518B7906DAE78527BC58EB4AB3F531664067AD58070E5B0`
- `GrinNode.exe` SHA-256 `ECB1B55E31CEFCB03DE517DDC5EB9FFF86203E4981448B9A9D8BB9709DE28B3D`
- Logs: `C:\DHW\.tmp\legacy-build59-shared\build.log`.

## Тесты

- Focused: **13 test cases / 63 assertions PASS**
  (`TxHashSetManager propagates*`, `TxHashSetStaging::*`, `Zipper streams*`,
  `ZipFile rejects*`, `BlockPipe does not ban*`, `BlockPipe still bans*`,
  `TxHashSet::FindKernelByExcess*`).
- Full: **280 test cases / 4403 assertions PASS**, 0 failures.
- Evidence: `evidence\line3\legacy59-focused\manifest.json`,
  `evidence\line3\legacy59-full\manifest.json`.

Patch tests cover CRC-corrupted ZIP, extraction byte budget, staging
orphan/backup/installed/rollback-before-DB-commit/after-DB-commit, parallel MMR
load failure propagation and BlockPipe ban/no-ban classification.

## Live на отдельном профиле (частично)

- Профиль `C:\DHW\.tmp\legacy-local-01\cpp2`; Rust peer 5.5.2 loopback.
- Патченный `GrinNode.exe` (SHA выше) запущен на существующем активированном
  state; статус:
  - `FULLY_SYNCED`, height **3 318 655**,
  - chain `2945348e...`,
  - Rust-compatible поле `rust.sync_status = no_sync`, tip 3 318 655.
- Restart того же патченного бинарника: снова `FULLY_SYNCED`, без
  `Requesting TxHashSet` и без полной загрузки state.
- **Честно:** download/validation/activation архива на этом SHA не повторялись;
  state был активирован предыдущим бинарником. Полный single-SHA
  Rust→C++ download→verify→activate→body sync→restart — **NOT VERIFIED**.

## Не проверено

- ZIP64 > 4 GiB, power-loss, disk-full, публичный Floonet — **NOT VERIFIED**.
- Fresh archive download/validation/activation на этом SHA — **NOT VERIFIED**.
- Owner API 401 из-за DACL `.api_secret` не относится к патчу; `/v1/status`
  работает и отдаёт `rust`-объект.
