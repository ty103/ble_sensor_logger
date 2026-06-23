# 共通前提: 環境セットアップ

Build / Flash / RTT いずれの操作でも必要な前提環境を記載します。

---

## 環境変数読み込み

**毎回のビルド・フラッシュ前に必ず実行してください。**

```bash
cd <repo_root>
source firmware/.env
```

設定される内容：
- NCS toolchain の PATH
- Zephyr SDK のパス
- git 環境変数

---

## 環境確認コマンド

```bash
# Zephyr 環境変数確認
echo $ZEPHYR_BASE

# J-Link デバイス確認（接続済みの場合）
nrfutil device list

# west ツール確認
west --version
```

---

## 関連ファイル一覧

| ファイル | 用途 |
|---------|------|
| `firmware/.env` | 環境変数定義 |
| `firmware/prj.conf` | Firmware Kconfig デフォルト設定 |
| `firmware/boards/nrf52840dk_nrf52840.overlay` | Devicetree overlay |
| `firmware/rtt_debug.conf` | RTT ログ有効化設定 |
| `firmware/iks01a2_i2c_probe_debug.conf` | I2C 診断設定 |
| `firmware/scripts/rtt_capture.sh` | RTT ログ自動取得スクリプト |

---

**作成日**: 2026-06-23
