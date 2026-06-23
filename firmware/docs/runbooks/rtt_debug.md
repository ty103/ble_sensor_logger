# Runbook: RTT デバッグ

BLE Sensor Logger Firmware の RTT ログ取得手順。

**前提**: [共通前提: 環境セットアップ](common_prerequisites.md) を先に確認してください。  
**前提**: RTT ログを取得するには `rtt_debug.conf` を有効にしてビルドすること（[build.md#rtt-ログ取得有効化](build.md#rtt-ログ取得有効化vcom-が使えない場合) を参照）。

---

## 必要環境

- SEGGER JLink ドライバ
- `JLinkRTTLogger` コマンド（SEGGER ツール）
- `nrfutil` コマンド

### 環境変数ファイルの準備

`firmware/.env.template`を参考に、`firmware/.env`を作成しておくこと。

---

## スクリプトによる RTT 取得（推奨）

### スタンドアロン RTT 取得

```bash
cd <repo_root>
source firmware/.env
firmware/scripts/rtt_capture.sh --build-dir build --serial <JLINK_SERIAL_NUMBER>
```

このコマンドは以下を実行します：
1. `JLinkRTTLogger` をバックグラウンドで起動
2. RTT チャネル 0 から自動ログ取得を開始
3. デバイスをリセット
4. ログを `build/rtt_capture.log` に保存

### フラッシュ + RTT 取得（一括実行）

新規ビルドをフラッシュしてから RTT ログを取得する場合：

```bash
cd <repo_root>
source firmware/.env
firmware/scripts/rtt_capture.sh --flash --build-dir build --serial <JLINK_SERIAL_NUMBER>
```

### ヘルプ

```bash
firmware/scripts/rtt_capture.sh --help
```

**デフォルト設定**:
- RTT チャネル: `0`（固定）
- 出力ログ: `build/rtt_capture.log`
- Ready timeout: `0.8` 秒
- 取得ウィンドウ: `0.8` 秒

---

## 手動 JLinkRTTLogger での取得

スクリプトを使わずに手動実行する場合：

```bash
JLinkRTTLogger
```

プロンプトに以下を入力：

| プロンプト | 入力値 |
|-----------|-------|
| Device name | `NRF52840_XXAA` |
| Target interface | （デフォルト SWD で Enter） |
| Interface speed | `4000` |
| RTT Control Block address | （自動で Enter） |
| RTT Channel | `0` |
| Output file | `build/rtt_capture.log` |

プログラムが "Getting RTT data" を表示したら、デバイスをリセットするかアプリケーションがデータを送信するのを待ちます。

---

## ログ確認

```bash
cat build/rtt_capture.log
```

または `tail -f` でリアルタイム確認：

```bash
tail -f build/rtt_capture.log
```

---

## パラメータリファレンス

| パラメータ | デフォルト | 例 | 用途 |
|-----------|----------|-----|------|
| `--build-dir` | `build` | `build` / `build-rtt` | ビルド出力ディレクトリ |
| `--serial` | （自動） | `1050278440` | J-Link シリアル番号 |
| `--flash` | （省略） | `--flash` | フラッシュ後に RTT 取得 |

---

## トラブルシューティング

→ [troubleshooting.md#rtt-ログ関連](troubleshooting.md#rtt-ログ関連)

---

**作成日**: 2026-06-23
