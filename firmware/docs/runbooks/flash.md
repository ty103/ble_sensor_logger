# Runbook: フラッシュ

BLE Sensor Logger Firmware のフラッシュ手順。

**前提**: [共通前提: 環境セットアップ](common_prerequisites.md) を先に確認してください。  
**前提**: ビルドが完了していること（[build.md](build.md) を参照）。

---

## 単一デバイス環境（デフォルト）

```bash
cd <repo_root>
source firmware/.env
west flash --build-dir build
```

---

## 複数デバイス接続時（シリアル番号指定）

複数の J-Link デバイスが接続されている場合：

```bash
cd <repo_root>
source firmware/.env
west flash --build-dir build --dev-id <JLINK_SERIAL_NUMBER>
```

`<JLINK_SERIAL_NUMBER>` の確認方法：

```bash
nrfutil device list
```

---

## パラメータリファレンス

| パラメータ | デフォルト | 例 | 用途 |
|-----------|----------|-----|------|
| `--build-dir` | `build` | `build` / `build-rtt` | ビルド出力ディレクトリ |
| `--dev-id` | （自動） | `1050278440` | フラッシュ対象デバイスのシリアル番号 |

---

## トラブルシューティング

→ [troubleshooting.md#フラッシュ関連](troubleshooting.md#フラッシュ関連)

---

**作成日**: 2026-06-23
