# AI Quickstart: Build / Flash / RTT

BLE Sensor Logger Firmware を **AI が操作するための入口ファイル**。  
タスクを確認してから対応する Runbook を読んでください。

---

## タスク別ルーティング

| やりたいこと | 参照先 |
|-------------|-------|
| 環境変数の読み込み・確認 | [runbooks/common_prerequisites.md](runbooks/common_prerequisites.md) |
| ビルド（基本 / センサ有効 / RTT / I2Cプローブ） | [runbooks/build.md](runbooks/build.md) |
| フラッシュ（単一デバイス / 複数デバイス指定） | [runbooks/flash.md](runbooks/flash.md) |
| RTT ログ取得（スクリプト / 手動） | [runbooks/rtt_debug.md](runbooks/rtt_debug.md) |
| エラー・トラブル対処 | [runbooks/troubleshooting.md](runbooks/troubleshooting.md) |

> **AI 向け手順**: 上表でファイルを特定 → そのファイルのみ読む → コマンドを実行。  
> 複数タスク（例: ビルド＋フラッシュ）は該当ファイルを順に参照する。

---

## 典型フロー（クイックリファレンス）

### フロー1: 初回ビルド＋フラッシュ（センサ無し）

```bash
cd <repo_root>
source firmware/.env
west build -b nrf52840dk/nrf52840 firmware --build-dir build --pristine
west flash --build-dir build
```

詳細 → [runbooks/build.md#基本ビルドセンサ無し](runbooks/build.md#基本ビルドセンサ無し) / [runbooks/flash.md](runbooks/flash.md)

---

### フロー2: X-NUCLEO-IKS01A2 接続＋フラッシュ

```bash
cd <repo_root>
source firmware/.env
west build -b nrf52840dk/nrf52840 firmware --build-dir build --pristine --shield x_nucleo_iks01a2
west flash --build-dir build
```

詳細 → [runbooks/build.md#x-nucleo-iks01a2-接続時センサ有効](runbooks/build.md#x-nucleo-iks01a2-接続時センサ有効) / [runbooks/flash.md](runbooks/flash.md)

---

### フロー3: RTT デバッグログ有効化＋取得

```bash
cd <repo_root>
source firmware/.env
west build -b nrf52840dk/nrf52840 firmware \
  --build-dir build \
  --pristine \
  --shield x_nucleo_iks01a2 \
  --extra-conf $PWD/firmware/rtt_debug.conf
west flash --build-dir build --dev-id <JLINK_SERIAL>
firmware/scripts/rtt_capture.sh --build-dir build --serial <JLINK_SERIAL>
```

詳細 → [runbooks/build.md#rtt-ログ取得有効化](runbooks/build.md#rtt-ログ取得有効化vcom-が使えない場合) / [runbooks/rtt_debug.md](runbooks/rtt_debug.md)

---

## 主要パラメータ

| パラメータ | デフォルト | 例 | 用途 |
|-----------|----------|-----|------|
| `--build-dir` | `build` | `build` / `build-rtt` | ビルド出力ディレクトリ |
| `--shield` | （省略） | `x_nucleo_iks01a2` | センサシールド |
| `--extra-conf` | （省略） | `$PWD/firmware/rtt_debug.conf` | 追加 Kconfig（**絶対パス必須**） |
| `--dev-id` | （自動） | `1050278440` | フラッシュ対象デバイス |
| `--serial` | （自動） | `1050278440` | RTT 取得対象デバイス |

---

## トラブルシューティング早見表

| 症状 | 参照先 |
|------|-------|
| ビルド失敗 | [runbooks/troubleshooting.md#ビルド関連](runbooks/troubleshooting.md#ビルド関連) |
| フラッシュ失敗 | [runbooks/troubleshooting.md#フラッシュ関連](runbooks/troubleshooting.md#フラッシュ関連) |
| RTT ログが出ない | [runbooks/troubleshooting.md#rtt-ログ関連](runbooks/troubleshooting.md#rtt-ログ関連) |

---

**作成日**: 2026-06-23  
**最終更新**: 2026-06-23
