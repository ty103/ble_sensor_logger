# X-NUCLEO-IKS01A2 LSM6DSL stream 実装レビュー

作成日: 2026-06-18  
対象branch: `codex/design-iks01a2-streams`  
対象Device: nRF52840 DK serial `1050278440`

## 1. 実装概要

- Firmwareに `lsm6dsl_sensor` moduleを追加し、Zephyr sensor API経由でLSM6DSLを扱う構成にした。
- `--shield x_nucleo_iks01a2` build時にLSM6DSL devicetree nodeがreadyであれば、`stream_id=10` の `IMU6_INT16_V1` streamを有効にする。
- Sensor Data frame v3をstream-specific payloadへ拡張し、mixed sampleは26 bytes、IMU6 sampleは24 bytesとして扱う。
- Capabilityは起動時の実センサavailabilityを反映し、LSM6DSL未ready時は `stream_count=1`、ready時は `stream_count=2` とする。
- PC backend、interactive CUI、WebGUI、CSV、smoke scriptはIMU6 payloadのparseと表示に対応した。

## 2. 検証結果

| 種別 | コマンド / 内容 | 結果 |
| --- | --- | --- |
| PC自動テスト | `uv run --extra dev pytest` | PASS。15 passed |
| Firmware build | `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware --pristine --shield x_nucleo_iks01a2` | PASS |
| Device検出 | `nrfutil device list` | serial `1050278440`、PCA10056、VCOM `/dev/tty.usbmodem0010502784401` / `...4403` を確認 |
| Flash | `west flash --build-dir build/firmware --dev-id 1050278440` | PASS |
| BLE smoke | `pc_app/scripts/ble_e2e_smoke.py --first-window-s 3 --second-window-s 2` | PASS。scan/connect、Capability Read、notify、interval変更、Status Read、stop後通知停止を確認 |

BLE smokeの主な結果:

- device: `BLE_SENSOR_LOGGER`
- Capability: `streams=1`
- stream 1: `MIXED_SAMPLE`
- first window: 31 samples
- second window: 4 samples
- stop後: 0 samples
- missed samples: 0

## 3. 判定

- BLE接続、mixed stream、Control/Config/Status/Capabilityの基本動作は実機で正常に動作した。
- Firmware/PCともにLSM6DSL IMU6 stream対応はbuild・自動テスト済み。
- 今回接続された実機ではLSM6DSLがreadyにならず、Capabilityは `stream_count=1` になった。したがって、IMU6 streamの実データ通知は未確認である。
- LSM6DSL未ready時にCapabilityからstream 10を落とすfallbackは期待通り動作した。

## 4. 次回作業

1. X-NUCLEO-IKS01A2のArduino header装着、電源、I2C jumper、shield向き、LSM6DSL addressを確認する。
2. Capabilityに `stream_id=10` が出る状態で、26 Hz前後のsample count、accel/gyro値変化、stream単位sequence欠落検出を実機確認する。

## 5. 2026-06-19追記: LSM6DSL診断追加

- LSM6DSL未ready理由をStatus `last_error` で返す最小診断を追加した。
- 追加した診断値は `LSM6DSL_NO_DEVICETREE`、`LSM6DSL_NOT_READY`、`LSM6DSL_CONFIG_FAILED`。
- `stream_id=10` がCapabilityに出ない場合は、まず `status` を読み、`last_error` に応じてbuild条件またはX-NUCLEO-IKS01A2の物理接続を確認する。
- shield付きFirmwareをflashし、BLE smokeで `last_error=LSM6DSL_NOT_READY` を確認した。現時点の実機はdevicetree nodeはあるがZephyr deviceがreadyになっていない状態である。
- `ble_negative_smoke.py` で不正Configは `INVALID_CONFIG`、不正Control長は `INVALID_LENGTH` のまま検出できることを確認した。

## 6. 2026-06-19追記: IKS01A2同一I2C bus診断

- 起動時診断として `iks01a2_i2c_probe` を追加し、`HTS221@0x5f` と `LPS22HB@0x5d` の `device_is_ready()` / 最小sample fetchを確認できるようにした。
- `uv run --extra dev pytest` は16 passed。
- `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware --pristine --shield x_nucleo_iks01a2` は成功。
- `west flash --build-dir build/firmware --dev-id 1050278440` は成功。
- VCOM0ログでは、`HTS221: Failed to read chip ID.`、`LPS22HB: Failed reading chip id`、`LSM6DSL: failed to reboot device` が出た。
- probe結果も `HTS221@0x5f is not ready`、`LPS22HB@0x5d is not ready`、`lsm6dsl@6b is not ready` だった。
- BLE smokeは引き続きPASSし、`streams=1`、`last_error=LSM6DSL_NOT_READY`、mixed stream通知は正常だった。

判定:

- LSM6DSLだけでなく、同じArduino I2C bus上のHTS221/LPS22HBも見えていない。
- 次はLSM6DSL単体ではなく、X-NUCLEO-IKS01A2のArduino I2C routing、shield装着、3.3 V/GND、I2C solder bridge / jumper、DK側D14/SDA・D15/SCL接触を優先して確認する。

## 7. 2026-06-19追記: ロジックアナライザ用周期I2C probe

- ロジックアナライザで `P0.26` / `P0.27` に信号が観測できなかったため、取り逃がしを避ける目的で1秒周期のraw I2C WHO_AM_I readを追加した。
- 対象は `HTS221@0x5f`、`LPS22HB@0x5d`、`LSM6DSL@0x6b`、`LSM303AGR accel@0x19`、`LSM303AGR magn@0x1e`。
- `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware --shield x_nucleo_iks01a2` は成功。
- VS Code/Nordic Debug停止後、`west flash --build-dir build/firmware --dev-id 1050278440` は成功。
- VCOM0では1秒周期で各addressの `WHO_AM_I read failed: -5` が出ており、Firmware側は周期probeを実行している。

## 8. 2026-06-19追記: TWIM切り替えでI2C疎通成功

- ロジックアナライザでは `0x5d WR: 0f` などのACKとSTOPが確認できたため、物理接続ではなくnRF側driverの扱いを疑った。
- nRF52840 DK標準の `arduino_i2c` は `nordic,nrf-twi` だったため、`firmware/boards/nrf52840dk_nrf52840.overlay` で `nordic,nrf-twim` へ切り替えた。
- pristine build後、`CONFIG_I2C_NRFX_TWIM=y`、`CONFIG_NRFX_TWIM=y` を確認した。
- VCOM0で `HTS221@0x5f=0xbc`、`LPS22HB@0x5d=0xb1`、`LSM6DSL@0x6b=0x6a`、`LSM303AGR accel@0x19=0x33` を確認した。
- `LSM303AGR magn@0x1e` は `0x00` で、期待値 `0x40` とは異なる。現時点の主目的であるLSM6DSL streamには影響しないため、必要時に別途確認する。
- BLE smokeはPASSし、Capabilityは `streams=2`、`stream_id=10 type=IMU6` を含み、Status `last_error=NONE` になった。
- 3秒windowでmixed stream 31 samples、IMU6 stream 76 samples、2秒windowでmixed stream 4 samples、IMU6 stream 51 samples、missed samples 0を確認した。

## 9. 2026-06-19追記: `nordic,nrf-twi` 失敗原因の切り分け

- `CONFIG_NRFX_TWI_LOG=y` と `NRFX_TWI_CONFIG_LOG_LEVEL=4` を一時的に有効化し、`LPS22HB@0x5d` だけに対象を絞ってTWI driverのイベントを確認した。
- TWI logではTX時に `NRF_TWI_EVENT_TXDSENT`、`NRF_TWI_EVENT_STOPPED`、`EVT_DONE`、RX時に `NRF_TWI_EVENT_RXDREADY`、`NRF_TWI_EVENT_STOPPED`、`EVT_DONE` が出た。`ADDRESS_NACK`、`DATA_NACK`、`BUS_ERROR` は出ていない。
- それでもアプリ側は `i2c_write()` / `i2c_read()` とも `-EIO` を受け取った。
- NCS v3.2.2同梱Zephyrの `drivers/i2c/i2c_nrfx_twi.c` では、`NRFX_TWI_EVT_DONE` で `dev_data->res = 0` を設定する一方、完了後に `data->res != NRFX_SUCCESS` を判定している。nrfx 4系では `NRFX_SUCCESS` が `0` ではないため、成功transferも `-EIO` になる。
- Nordic DevZoneにも同じNCS v3.2.0-v3.2.2の既知不具合が報告されており、mainでは `data->res != 0` へ修正済みとされている。
- 結論として、今回の現象はI2C bus上のNACK/STOP異常ではなく、Zephyr TWI wrapperの成功判定不一致が直接原因。nRF52840では `nordic,nrf-twim` が利用可能で実機疎通済みのため、本branchではTWIMを採用する。

## 10. 2026-06-20追記: HTS221 humidity/temperature stream実装

- Firmwareに `hts221_sensor` moduleを追加し、X-NUCLEO-IKS01A2上のHTS221からhumidity/temperatureを1 Hzで取得する構成にした。
- `stream_id=30`、`stream_type=TEMP_HUMIDITY`、`payload_format=HTS221_TEMP_HUMIDITY_INT16_V1` を追加した。
- payloadは `int16 humidity_centi_percent` と `int16 temperature_centi_c` の4 bytes。Sensor Data frame v3 headerと合わせて16 bytesとなる。
- Capabilityは起動時availabilityを反映し、HTS221 ready時だけ3本目のstreamとして `TEMP_HUMIDITY` を含める。
- PC backend、interactive CUI、WebGUI固定表示、CSV、smoke scriptはHTS221 payloadのparseと表示に対応した。
- `uv run --extra dev pytest` は18 passed。
- `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware --pristine --shield x_nucleo_iks01a2` は成功。
- 通常buildでは `CONFIG_BSL_IKS01A2_I2C_PROBE` は無効、`CONFIG_HTS221=y`、`CONFIG_HTS221_ODR="1"`、`CONFIG_I2C_NRFX_TWIM=y` を確認した。
- `west flash --build-dir build/firmware --dev-id 1050278440` は成功。
- BLE smokeはPASSし、Capabilityは `streams=3`、`stream_id=30 type=TEMP_HUMIDITY` を含み、Status `last_error=NONE` になった。
- 3秒windowでmixed stream 30 samples、IMU6 stream 76 samples、HTS221 stream 4 samples、2秒windowでmixed 3 samples、IMU6 50 samples、HTS221 2 samples、missed samples 0を確認した。
- 最新HTS221値としてhumidity `73.00 %RH`、temperature `28.50 degC` を確認した。

判定:

- LSM6DSL IMU6とHTS221 humidity/temperatureは同一Firmwareで同時に通知できる。
- HTS221の1 Hz streamは、低頻度環境streamの代表実装としてPC/WebGUI/CSVまで到達した。
- 次の実装候補はLPS22HB pressure 1 Hz。WebGUIは固定表示の追加が続いているため、Capability-driven UI生成の設計を先に進める価値が上がっている。
