# BLE Sensor Logger

nRF52840 DKのセンサデータをBLEで送信し、Python backendを介してCUIまたはWebGUIで表示・制御するサンプルシステムです。

## 現在の構成

- Device: nRF52840 DK、NCS v3.2.3、Zephyr RTOS、App Event Manager
- Sensor: 疑似加速度、X-NUCLEO-IKS01A2接続時のLSM6DSL 6軸IMU（26 Hz）、HTS221 humidity/temperature（1 Hz）、LPS22HB pressure（1 Hz）、LSM303AGR magnetometer（10 Hz）
- Protocol: Little Endian固定長binary、Sensor Data frame v3は16-24 bytes
- PC backend: Python、bleak、aiohttp
- UI: interactive CUI、およびlocal WebGUI
- BLE通信はPython backendのみが担当し、ブラウザではWeb Bluetooth APIを使用しない

FirmwareはnRF52840 DK専用APIやNCS sample board helperを使用しません。Board固有設定はDevicetree overlayへ分離します。

## ディレクトリ

```text
docs/       設計書、開発計画、テスト仕様・結果
firmware/   NCS/Zephyr firmware
pc_app/     Python backend、CUI、WebGUI、テスト
```

## PC Appセットアップ

```bash
cd pc_app
uv sync --extra dev
```

### WebGUI

```bash
uv run --extra dev python -m ble_sensor_logger --web --host 127.0.0.1 --port 8765
```

ブラウザで `http://127.0.0.1:8765` を開きます。以下を利用できます。

- Scan、Connect、Start/Stop、sampling interval変更、Status確認、sequence reset
- 横軸を時刻（hh:mm:ss）で表示する、選択信号のリアルタイムグラフ
- 接続中の状態表示と、Stopボタンによる接続キャンセル
- Capability metadataに基づく最新値カード、Signal選択、CSV保存

CSVにはhost timestamp、stream id、sequence、device timestamp、Capability由来のsensor field、欠落サンプル数を記録します。`Stop & save CSV`を押すとブラウザのdownload先へ保存されます。

### Interactive CUI

```bash
uv run python -m ble_sensor_logger
```

起動後は同じterminalでコマンドを入力します。

```text
scan
connect BLE_SENSOR_LOGGER
monitor
start
set-interval 100
status
stop
exit
```

## Firmwareビルド・フラッシュ・RTT

詳細な手順は [firmware/docs/ai_quickstart.md](firmware/docs/ai_quickstart.md) を参照してください。

初めて実行する場合は [firmware/docs/ai_quickstart.md](firmware/docs/ai_quickstart.md) から開始してください。

### よくあるケース

**基本的なビルド・フラッシュ** (センサ無し):
```bash
cd firmware
source .env
west build -b nrf52840dk/nrf52840 ../firmware --build-dir ../build --pristine
west flash --build-dir ../build
```

**X-NUCLEO-IKS01A2 接続時**:

X-NUCLEO-IKS01A2を接続してLSM6DSL/HTS221/LPS22HB/LSM303AGR magnetometer streamを有効にする場合は、Zephyr標準shieldを指定します。

```bash
cd firmware
source .env
west build -b nrf52840dk/nrf52840 ../firmware --build-dir ../build --pristine --shield x_nucleo_iks01a2
west flash --build-dir ../build
```

`capability` でoptional streamが出ない場合は、`status` の `optional_sensors` またはCUI出力の `optional_sensors=...` を確認します。`last_error` には既存表示向けの代表値が出ます。

| `last_error` | 確認ポイント |
| --- | --- |
| `LSM6DSL_NO_DEVICETREE` | `--shield x_nucleo_iks01a2` 付きでbuildしたか |
| `LSM6DSL_NOT_READY` | X-NUCLEO-IKS01A2のArduino header装着、向き、3.3 V/GND、I2C SCL/SDA jumper、LSM6DSL address |
| `LSM6DSL_CONFIG_FAILED` | LSM6DSLは見えているがODR設定に失敗しているため、Firmware logとZephyr driver設定 |
| `HTS221_NO_DEVICETREE` / `LPS22HB_NO_DEVICETREE` / `LSM303AGR_MAGN_NO_DEVICETREE` | `--shield x_nucleo_iks01a2` 付きでbuildしたか |
| `HTS221_NOT_READY` / `LPS22HB_NOT_READY` / `LSM303AGR_MAGN_NOT_READY` | X-NUCLEO-IKS01A2の装着、3.3 V/GND、Arduino I2C bus、対象sensor address |

詳細・RTT デバッグ・トラブルシューティング は [firmware/docs/ai_quickstart.md](firmware/docs/ai_quickstart.md) を参照してください。

## テスト

```bash
cd pc_app
uv run --extra dev pytest
```

初期実装の実機試験の手順と結果は[旧テスト仕様書](docs/old/test_spec.md)を参照してください。再設計フェーズの正本は[設計引き継ぎ資料](docs/generic_sensor_monitor_design_handoff.md)です。
