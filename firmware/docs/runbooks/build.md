# Runbook: ビルド

BLE Sensor Logger Firmware のビルド手順。

**前提**: [共通前提: 環境セットアップ](common_prerequisites.md) を先に確認してください。

---

## 環境変数ファイルの準備

`firmware/.env.template`を参考に、`firmware/.env`を作成しておくこと

## 基本ビルド（センサ無し）

疑似加速度のみを使う場合：

```bash
cd <repo_root>
source firmware/.env
west build -b nrf52840dk/nrf52840 firmware --build-dir build --pristine
```

**出力**: `build/zephyr/app_update.bin` など

---

## X-NUCLEO-IKS01A2 接続時（センサ有効）

LSM6DSL / HTS221 / LPS22HB / LSM303AGR magnetometer を有効にする場合：

```bash
cd <repo_root>
source firmware/.env
west build -b nrf52840dk/nrf52840 firmware --build-dir build --pristine --shield x_nucleo_iks01a2
```

---

## RTT ログ取得有効化（VCOM が使えない場合）

RTT-only logging でビルドする場合（試作基板など）：

```bash
cd <repo_root>
source firmware/.env
west build -b nrf52840dk/nrf52840 firmware \
  --build-dir build \
  --pristine \
  --shield x_nucleo_iks01a2 \
  --extra-conf $PWD/firmware/rtt_debug.conf
```

> **注意**: sysbuild 環境では `--extra-conf` に**絶対パス**を使用してください（相対パスは `firmware/firmware/...` になる可能性があります）。

---

## I2C プローブ（bring-up 診断のみ）

X-NUCLEO-IKS01A2 の I2C アドレス確認時：

```bash
cd <repo_root>
source firmware/.env
west build -b nrf52840dk/nrf52840 firmware \
  --build-dir build \
  --pristine \
  --shield x_nucleo_iks01a2 \
  --extra-conf $PWD/firmware/iks01a2_i2c_probe_debug.conf
```

---

## RTT + I2C プローブ（同時有効）

```bash
cd <repo_root>
source firmware/.env
west build -b nrf52840dk/nrf52840 firmware \
  --build-dir build \
  --pristine \
  --shield x_nucleo_iks01a2 \
  -- -DEXTRA_CONF_FILE="rtt_debug.conf;iks01a2_i2c_probe_debug.conf"
```

---

## パラメータリファレンス

AI が状況に応じて差し替えるパラメータ：

| パラメータ | デフォルト | 例 | 用途 |
|-----------|----------|-----|------|
| `--build-dir` | `build` | `build` / `build-rtt` / `build-probe` | ビルド出力ディレクトリ |
| `--shield` | （省略） | `x_nucleo_iks01a2` | センサシールド有無 |
| `--extra-conf` | （省略） | `$PWD/firmware/rtt_debug.conf` | 追加 Kconfig（絶対パス必須） |
| `-DEXTRA_CONF_FILE` | （省略） | `"rtt_debug.conf;iks01a2_i2c_probe_debug.conf"` | 複数 config の同時指定 |

---

## トラブルシューティング

→ [troubleshooting.md#ビルド関連](troubleshooting.md#ビルド関連)

---

**作成日**: 2026-06-23
