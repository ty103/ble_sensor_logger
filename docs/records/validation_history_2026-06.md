# Generic Sensor Monitor: 実機確認履歴

作成日: 2026-06-21

この資料は `docs/generic_sensor_monitor_design_handoff.md` から分離した実機確認・検証履歴である。
handoff本体は現在の設計判断、残課題、次回作業キューを正本として扱い、過去の詳細ログは本資料を参照する。


### 2026-06-21: Debug dummy stream整理

対象branch: `codex/debug-dummy-stream`

目的:

- `stream_id=1` を `MIXED_SAMPLE_V1` から `DUMMY_ACCEL3_INT16_V1` へ整理し、実センサstreamと同じ固定field payloadの形に寄せる。
- dummy batteryとA0 ADCをactive Sensor Data pathから外す。A0 ADCは必要になった時点で独立したADC/board analog streamとして復活を検討する。

実装内容:

- Firmware:
  - `stream_id=1` のstream typeを `DUMMY_ACCEL3`、payload formatを `DUMMY_ACCEL3_INT16_V1` に変更した。
  - payloadを `int16 accel_x_mg`, `int16 accel_y_mg`, `int16 accel_z_mg` の6 bytesに縮小した。
  - `sensor_flags`、dummy battery、A0 ADCのmixed payload同梱を削除した。
  - `CONFIG_ADC` を無効化し、active buildからADC driverを外した。
- PC backend/CUI/WebGUI:
  - Python protocol parser/packer、Capability default、WebGUI field metadata、fallback Capability、CUI表示、BLE smoke scriptを `DUMMY_ACCEL3_INT16_V1` に同期した。
  - CSV列はCapability fields由来の `s1_accel_x_mg`、`s1_accel_y_mg`、`s1_accel_z_mg` を使い、旧 `sensor_flags` / battery / ADC列はactive fallbackから外した。
- Docs:
  - 現行仕様、system設計、Firmware設計、PC/WebApp設計、handoff、ADR-0005を現行名へ同期した。

確認内容:

- `cd pc_app && uv run --extra dev pytest`
  - 成功。30 tests passed。
- `node --check pc_app/web_frontend/app.js`
  - 成功。
- Firmware shield build:
  - `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware --pristine --shield x_nucleo_iks01a2`
  - 成功。NCS v3.2.2 toolchain bundle `e5f4758bcf` を使用し、`ZEPHYR_SDK_INSTALL_DIR=/opt/nordic/ncs/toolchains/e5f4758bcf/opt/zephyr-sdk` を指定した。
  - `CONFIG_ADC` は無効。build後のsizeは FLASH `199328 B`、RAM `37400 B`。
- Flash:
  - `west flash --build-dir build/firmware --dev-id 1050278440`
  - 成功。board serial `1050278440` へ書き込み、reset完了。
- BLE smoke:
  - `uv run --extra dev python scripts/ble_e2e_smoke.py --first-interval-ms 100 --second-interval-ms 500 --first-window-s 3 --second-window-s 3`
  - Capabilityは `preferred_mtu=24`、`streams=5`、`stream_id=1 type=DUMMY_ACCEL3 channels=3 format=DUMMY_ACCEL3_INT16_V1`。
  - 100 ms window: total 142 samples、`stream_id=1` は29 samples。
  - 500 ms window: total 116 samples、`stream_id=1` は5 samples。
  - Status `last_error=NONE`、optional sensorsすべて `NONE`、stop後1秒のsampleは0、missed samplesは0。
- BLE negative smoke:
  - `uv run --extra dev python scripts/ble_negative_smoke.py`
  - invalid Config v4 writeはrejectされ、Status `last_error=INVALID_CONFIG`。
  - invalid Control lengthはrejectされ、Status `last_error=INVALID_LENGTH`。

次作業:

1. Config v4次段、Status Notify、Log/Eventの優先順位を再評価する。
2. A0 ADCを再度BLE送信へ載せる必要が出た場合は、旧mixed payloadへ戻さず独立したADC/board analog streamとして設計する。

### 2026-06-21: stream単位Config v4最小実装

対象branch: `codex/stream-config-v4`

対象Device: nRF52840 DK serial `1050278440` + X-NUCLEO-IKS01A2

目的:

- Config Characteristicを固定長binary v4へ更新し、stream単位Configの最小実装として `SET_STREAM_INTERVAL` を導入する。
- 初期対象は `stream_id=1` の `MIXED_SAMPLE` interval変更に限定し、実センサstreamは固定rateのまま扱う。

実装内容:

- Firmware:
  - `struct bsl_config` を `version/op/stream_id/flags/sample_interval_ms/reserved` の8 bytes payloadへ変更した。
  - `version=4`、`op=SET_STREAM_INTERVAL`、`stream_id=1`、`flags=0`、`reserved=0`、20-10000 msのintervalだけをvalidにした。
  - `config_update_event` と `control` は、validなConfig v4を受けて `sensor_dummy_set_interval()` とStatus `sample_interval_ms` を更新する。
- PC backend/CUI:
  - Python `ConfigPayload` をConfig v4へ更新し、`set_stream_interval(stream_id, interval_ms)` を追加した。
  - 既存 `set_interval(interval_ms)` は `stream_id=1` への便利メソッドとして残した。
  - CUIに `set-stream-interval <stream_id> <ms>` を追加した。
- WebGUI:
  - Capabilityの `min_interval_ms` と `max_interval_ms` が異なるstreamをInterval controlのstream選択肢として表示する。
  - `/api/interval` は `{ "stream_id": 1, "interval_ms": 100 }` を受け取り、Config v4を書き込む。
- Docs:
  - ADR-0005をAcceptedへ更新した。
  - 現行仕様、Firmware設計、PC/WebApp設計、system設計をConfig v4現行仕様へ同期した。

確認内容:

- `cd pc_app && uv run --extra dev pytest`
  - 成功。30 tests passed。
- `node --check pc_app/web_frontend/app.js`
  - 成功。
- Firmware shield build:
  - `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware --pristine --shield x_nucleo_iks01a2`
  - 成功。NCS v3.2.2 toolchain bundle `e5f4758bcf` を使用し、`ZEPHYR_SDK_INSTALL_DIR=/opt/nordic/ncs/toolchains/e5f4758bcf/opt/zephyr-sdk` を指定した。
- Flash:
  - `west flash --build-dir build/firmware`
  - 成功。board serial `1050278440` へ書き込み、reset完了。
- BLE smoke:
  - `uv run --extra dev python scripts/ble_e2e_smoke.py --first-interval-ms 100 --second-interval-ms 500 --first-window-s 3 --second-window-s 3`
  - `streams=5`、Status `last_error=NONE`、optional sensorsすべて `NONE`。
  - 100 ms window: total 140 samples、`stream_id=1` は29 samples。
  - 500 ms window: total 117 samples、`stream_id=1` は6 samples。
  - 固定rate streamは継続して取得でき、`stream_id=10/12/20/30` も出力された。
  - stop後1秒のsampleは0、missed samplesは0。
- BLE negative smoke:
  - `uv run --extra dev python scripts/ble_negative_smoke.py`
  - invalid Config v4 writeはrejectされ、Status `last_error=INVALID_CONFIG`。
  - invalid Control lengthはrejectされ、Status `last_error=INVALID_LENGTH`。

判定:

- Config v4の最小実装として、`stream_id=1` のinterval変更はFirmware/PC/WebGUI/docs/実機smokeまで通った。
- 実センサstreamのrate変更、stream enable/disable、Config Response、TLV化は次段の設計・実装候補に残す。

次作業:

1. Config v4の次段として、`SET_STREAM_ENABLE` を入れるか、Status Notify / Log/Eventを先に入れるか再評価する。
2. 実センサstreamのrate変更が必要になった時点で、各Zephyr sensor driverのODR/range再設定可否と、measuring中の再設定手順を設計する。

### 2026-06-18: Capability最小実装

対象branch: `codex/feature-capability-char`
対象Device: nRF52840 DK serial `1050278440`

確認内容:

- `west flash --build-dir build/firmware --dev-id 1050278440` で書き込み成功。
- `pc_app/scripts/ble_e2e_smoke.py` でscan/connect、Capability Read、Sensor Data Notify、interval変更、Status Read、stop後の通知停止を確認。
- Capability Read結果は、v3移行前の最小schemaとしてstream metadataを読めることを確認した。
- 100 ms windowで31 samples、500 ms windowで6 samples、stop後1秒で0 samples、missed samplesは0。
- `pc_app/scripts/ble_negative_smoke.py` でinvalid Configが `INVALID_CONFIG`、invalid Control lengthが `INVALID_LENGTH` としてStatusに反映されることを確認。

この確認により、Capability Characteristicの最小実装は実機で読める状態になった。次作業はSensor Dataの`stream_id`付きframe header設計から進める。

### 2026-06-18: Sensor Data frame v3実装

対象branch: `codex/design-stream-frame`
対象Device: nRF52840 DK serial `1050278440`

確認内容:

- 試作段階のため後方互換を捨て、active protocolをv3へ更新した。
- Sensor Dataは26 bytes frameになり、12 bytes headerに `message_type`、`stream_id`、`sequence`、`timestamp_ms`、`payload_format`、`payload_len` を持つ。
- FirmwareはATT MTU拡張を前提に `CONFIG_BT_L2CAP_TX_MTU=247`、ACL buffer size 251を設定した。
- `west flash --build-dir build/firmware --dev-id 1050278440` で書き込み成功。
- `pc_app/scripts/ble_e2e_smoke.py` でscan/connect、Capability Read、Sensor Data Notify、interval変更、Status Read、stop後の通知停止を確認。
- Capability Read結果は `schema=1`、`flags=0x0003`、`features=0x0007`、`preferred_mtu=26`、`streams=1`。
- stream descriptorは `id=1`、`type=MIXED_SAMPLE`、`channels=5`、`format=MIXED_SAMPLE_V1`、`interval=100ms`、`range=20..10000ms`。
- 100 ms windowで31 samples、500 ms windowで6 samples、stop後1秒で0 samples、missed samplesは0。
- `pc_app/scripts/ble_negative_smoke.py` でinvalid Configが `INVALID_CONFIG`、invalid Control lengthが `INVALID_LENGTH` としてStatusに反映されることを確認。

この確認により、`stream_id`付きSensor Data frame v3は実機で通知・parseできる状態になった。次作業はIMU 9-axis 100 HzとTemperature 1 Hzの代表stream/payload定義へ進める。

### 2026-06-18: X-NUCLEO-IKS01A2 / LSM6DSL stream実装

対象branch: `codex/design-iks01a2-streams`
対象Device: nRF52840 DK serial `1050278440`

実装内容:

- Firmwareに `lsm6dsl_sensor` adapterを追加し、Zephyr標準shield `x_nucleo_iks01a2` buildでLSM6DSLを初期化する構成にした。
- LSM6DSL ready時は `stream_id=10`、`payload_format=IMU6_INT16_V1`、26 Hz固定intervalのIMU6 streamをCapabilityへ追加し、Sensor Data Notifyでaccel mg / gyro mdpsのint16 x6を送信する。
- LSM6DSL未ready時はFirmware起動とBLE Advertisingを継続し、Capabilityは `stream_count=1` としてmixed streamのみを返す。
- PC backend / CUI / WebGUI / CSV / smoke scriptはIMU6 payloadをparse・表示できるようにした。

確認内容:

- `uv run --extra dev pytest` でPC自動テストが成功。
- `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware --pristine --shield x_nucleo_iks01a2` でFirmware build成功。
- `west flash --build-dir build/firmware --dev-id 1050278440` で書き込み成功。
- `pc_app/scripts/ble_e2e_smoke.py --first-window-s 3 --second-window-s 2` でscan/connect、Capability Read、Sensor Data Notify、interval変更、Status Read、stop後の通知停止を確認。
- 今回接続された実機ではCapability Read結果が `streams=1` で、`stream_id=10` は出なかった。つまり、FirmwareのLSM6DSL adapterはbuildされているが、起動時にLSM6DSLがreadyになっていない。
- mixed streamは実機で正常に通知され、3秒windowで31 samples、2秒windowで4 samples、stop後0 samples、missed samplesは0だった。

次回作業:

1. X-NUCLEO-IKS01A2のArduino header装着、電源、I2C jumper、shield向き、LSM6DSLのI2C addressを物理確認する。
2. `stream_id=10` がCapabilityに出た状態で、26 Hz前後のsample count、accel/gyro値変化、stream単位sequence欠落検出を再確認する。

### 2026-06-19: LSM6DSL未ready診断の最小実装

対象branch: `codex/design-iks01a2-streams`

実装内容:

- `lsm6dsl_sensor` がoptional streamのavailability診断を保持するようにした。
- Status `last_error` に `LSM6DSL_NO_DEVICETREE`、`LSM6DSL_NOT_READY`、`LSM6DSL_CONFIG_FAILED` を追加した。
- LSM6DSLがreadyでない場合でもFirmware起動とBLE Advertisingは継続し、Capabilityは従来どおり `stream_count=1` とする。
- PC backend/CUI/WebGUIは既存のStatus表示で診断名を表示できる。Protocol enumとunit testを更新済み。

確認内容:

- `uv run --extra dev pytest` でPC自動テストが成功。16 passed。
- `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware --shield x_nucleo_iks01a2` でFirmware build成功。
- `west flash --build-dir build/firmware --dev-id 1050278440` で書き込み成功。
- `pc_app/scripts/ble_e2e_smoke.py --first-window-s 3 --second-window-s 2` でscan/connect、Capability Read、Sensor Data Notify、interval変更、Status Read、stop後の通知停止を確認。
- BLE smoke結果は `streams=1`、Status `last_error=LSM6DSL_NOT_READY`。つまり今回の実機はshield build済みでdevicetree nodeはあるが、起動時にLSM6DSL deviceがreadyになっていない。
- `pc_app/scripts/ble_negative_smoke.py` でinvalid Configが `INVALID_CONFIG`、invalid Control lengthが `INVALID_LENGTH` としてStatusに反映されることを確認。

次回の実機確認:

1. shield付きFirmwareを書き込み、接続直後に `capability` と `status` を読む。
2. 今回は `last_error=LSM6DSL_NOT_READY` なので、X-NUCLEO-IKS01A2のArduino header装着、向き、3.3 V/GND、I2C SCL/SDA jumper、LSM6DSL addressを優先確認する。
3. `last_error=LSM6DSL_NO_DEVICETREE` の場合は `--shield x_nucleo_iks01a2` 付きでbuildされているか確認する。
4. `last_error=LSM6DSL_CONFIG_FAILED` の場合はLSM6DSLは見えている可能性が高いため、Firmware logとZephyr driver設定を確認する。
5. Capabilityに `stream_id=10` が出たら、26 Hz前後のsample count、accel/gyro値変化、stream単位sequence欠落検出を確認する。

### 2026-06-19: LSM6DSL通信不可の追加切り分け方針

対象branch: `codex/design-iks01a2-streams`

ユーザー側確認:

- ユーザーがVS Code/Nordic拡張側でもshield付きbuildとVCOM0 debugを再現できる状態にした。
- shieldなしbuildではVCOM0に `no st,lsm6dsl devicetree node` が出る。この場合はI2C以前に `x_nucleo_iks01a2` shield overlayが入っていない。
- shield付きbuildではLSM6DSL devicetree nodeは存在するが、同じくLSM6DSL通信不可で `LSM6DSL_NOT_READY` 相当の状態が続いている。
- X-NUCLEO-IKS01A2のSB7実装により、資料上のLSM6DSL addressは `D6h`。これは8-bit write address表記であり、Zephyr devicetreeの7-bit address `0x6b` と一致する。
- nRF52840 DK側のArduino I2C `D14/SDA`、`D15/SCL` はidle時に約3.0 Vで、busがLow張り付きになっている状態ではない。

build / devicetree確認:

- 今回のCLI build成果物のapp本体 `.config` は `build/firmware/firmware/zephyr/.config`。`build/firmware/zephyr/.config` はsysbuild側であり、LSM6DSL/I2Cの有効状態を見る主対象ではない。
- `CONFIG_SENSOR=y`、`CONFIG_I2C=y`、`CONFIG_LSM6DSL=y` は有効。
- `x_nucleo_iks01a2` shield overlayにより、同じI2C bus上に以下のnodeが生成される:
  - `HTS221`: `0x5f`
  - `LPS22HB`: `0x5d`
  - `LSM6DSL`: `0x6b`
  - `LSM303AGR magnetometer`: `0x1e`
  - `LSM303AGR accel`: `0x19`

ログ設定:

- ユーザーが `firmware/prj.conf` に以下を追加済み。

```conf
# Sensor driver debug logging
CONFIG_SENSOR_LOG_LEVEL_DBG=y
CONFIG_I2C_LOG_LEVEL_DBG=y
```

- `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware --shield x_nucleo_iks01a2` 後、`build/firmware/firmware/zephyr/.config` で `CONFIG_SENSOR_LOG_LEVEL=4`、`CONFIG_I2C_LOG_LEVEL=4`、`CONFIG_LOG_MAX_LEVEL=4` を確認済み。現時点では追加Kconfigは不要。
- ログは `CONFIG_LOG_BACKEND_UART=y`、`zephyr,console=&uart0` のためVCOM0へ出る。RTTは `CONFIG_USE_SEGGER_RTT` 未設定。
- debug logが起動直後に欠ける場合の次候補は、`CONFIG_LOG_BUFFER_SIZE=4096` への拡大、または一時的な `CONFIG_LOG_MODE_IMMEDIATE=y` の追加。ただし現時点では未適用。

次回作業方針:

1. LSM6DSL単体故障またはLSM6DSL周辺配線問題の可能性を切り分けるため、X-NUCLEO-IKS01A2上の他センサへI2C接続できるか確認する。
2. 優先確認対象は、同じArduino I2C bus上の `HTS221@0x5f` または `LPS22HB@0x5d`。低頻度センサでpayload設計が単純なため、まず `device_is_ready()` と最小sample fetchだけを追加してI2C疎通を確認する。
3. 他センサがreadyになる場合は、I2C bus全体ではなくLSM6DSL固有の問題として、LSM6DSLのCS/I2C mode、WHO_AM_I read結果、chip id不一致、実装/はんだ、LSM6DSL周辺solder bridgeを調べる。
4. 他センサもreadyにならない場合は、Arduino I2C routing、IKS01A2のI2C jumper/solder bridge、3.3 V供給、GND、shield装着方向、nRF52840 DK側D14/D15接続を再確認する。
5. I2C driver debug logをVCOM0で取得し、`lsm6dsl` driverの `failed reading chip id` / `invalid chip id` と、nrfx I2C側のerror codeを記録する。

### 2026-06-19: IKS01A2同一I2C bus上の複数センサ診断

対象branch: `codex/design-iks01a2-streams`
対象Device: nRF52840 DK serial `1050278440`

実装内容:

- `firmware/src/modules/iks01a2_i2c_probe.c` を追加し、起動時にX-NUCLEO-IKS01A2上の `HTS221@0x5f` と `LPS22HB@0x5d` を診断するようにした。
- 診断はstream追加ではなく、`device_is_ready()` と最小 `sensor_sample_fetch()` / `sensor_channel_get()` のログ出力に限定した。
- `LSM6DSL@0x6b` の既存availability診断と合わせ、同一Arduino I2C bus全体が見えているかを切り分ける目的で使う。

確認内容:

- `uv run --extra dev pytest` を `pc_app/` から実行し、16 passed。
- `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware --pristine --shield x_nucleo_iks01a2` でFirmware build成功。
- `nrfutil device list` でserial `1050278440`、VCOM0 `/dev/tty.usbmodem0010502784401` を確認。
- `west flash --build-dir build/firmware --dev-id 1050278440` で書き込み成功。
- VCOM0起動ログで以下を確認:
  - `HTS221: Failed to read chip ID.`
  - `LPS22HB: Failed reading chip id`
  - `LSM6DSL: failed to reboot device` / `Failed to initialize chip`
  - `iks01a2_i2c_probe: HTS221@0x5f is not ready`
  - `iks01a2_i2c_probe: LPS22HB@0x5d is not ready`
  - `lsm6dsl_sensor: lsm6dsl@6b is not ready`
- `pc_app/scripts/ble_e2e_smoke.py --first-window-s 3 --second-window-s 2` でscan/connect、Capability Read、Sensor Data Notify、interval変更、Status Read、stop後の通知停止を確認。
- BLE smoke結果は `streams=1`、Status `last_error=LSM6DSL_NOT_READY`。mixed streamは3秒windowで31 samples、2秒windowで3 samples、stop後0 samples、missed samples 0。

判定:

- 同一Arduino I2C bus上の `HTS221@0x5f`、`LPS22HB@0x5d`、`LSM6DSL@0x6b` がすべてchip ID readまたは初期化に失敗している。
- したがって現時点の本命はLSM6DSL単体故障ではなく、X-NUCLEO-IKS01A2のArduino I2C routing、shield側I2C jumper/solder bridge、3.3 V/GND供給、shield装着方向、またはnRF52840 DK側Arduino header接触である。
- BLE/PC protocol側は引き続き正常で、FirmwareはI2Cセンサ未ready時にもAdvertisingとmixed streamを維持できている。

次の物理確認:

1. nRF52840 DKの電源SWがONで、DKのPCA10056/J-Link側ではなくターゲットnRF52840へ給電されていることを確認する。
2. X-NUCLEO-IKS01A2がArduino headerに正しい向きで奥まで挿さっていることを確認する。特にD14/SDA、D15/SCL、3.3 V、GNDの列がずれていないこと。
3. DK側Arduino I2CはSoC pin `P0.26` / `P0.27` で、Arduino名では `D14/SDA` / `D15/SCL`。この2本がshield側SDA/SCLへつながっている前提で見る。
4. X-NUCLEO-IKS01A2側のI2C関連solder bridge / jumperがArduino I2Cへ接続される設定になっていることを確認する。3.3 V系のVDD/VDDIOとGNDも確認対象。
5. 物理確認後、同じ診断入りFirmwareのままresetし、VCOM0で `HTS221@0x5f is ready` または `LPS22HB@0x5d is ready` が出るかを確認する。

### 2026-06-19: ロジックアナライザ観測用の周期I2C probe

対象branch: `codex/design-iks01a2-streams`
対象Device: nRF52840 DK serial `1050278440`

背景:

- ユーザーがnRF52840 DK側 `P0.26` / `P0.27`、Arduino名 `D14/SDA` / `D15/SCL` にロジックアナライザを接続してRunしたが、信号を観測できなかった。
- build済みdevicetreeでは `arduino_i2c` は `i2c0@40003000` で、pinctrlは `P0.26` / `P0.27` のままであることを確認した。
- 従来診断は起動直後のdriver初期化時にI2C accessが集中し、失敗後は継続的なbus accessがなかったため、ロジックアナライザで取り逃がす可能性があった。

実装内容:

- `iks01a2_i2c_probe` に1秒周期のraw I2C WHO_AM_I readを追加した。
- 対象address:
  - `HTS221@0x5f`
  - `LPS22HB@0x5d`
  - `LSM6DSL@0x6b`
  - `LSM303AGR accel@0x19`
  - `LSM303AGR magn@0x1e`
- ACKがなくてもmaster側のI2C transaction自体を継続的に出し、ロジックアナライザでSCL/SDAの有無を確認しやすくする目的である。

確認内容:

- `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware --shield x_nucleo_iks01a2` でbuild成功。
- VS Code/Nordic DebugがJ-Linkを掴んでいたため一度flashに失敗したが、Debug停止後に `west flash --build-dir build/firmware --dev-id 1050278440` でflash成功。
- VCOM0ログで1秒周期のread試行を確認した。現状は全対象で `WHO_AM_I read failed: -5`。

次の確認:

1. ロジックアナライザをDK側 `P0.26` / `P0.27`、またはArduino header `D14/SDA` / `D15/SCL` に接続し、1秒以上recordする。
2. 周期的なSCL/SDA波形が見える場合は、nRF側I2C masterは駆動できているため、shield接続・pull-up・sensor側ACK不在を優先して切り分ける。
3. それでも波形が全く見えない場合は、測定点違い、header接触、DK側Arduino I2C routing、またはpinmuxが実物の測定点と一致していない可能性を優先して確認する。

### 2026-06-19: IKS01A2 I2C疎通成功とTWIM切り替え

対象branch: `codex/design-iks01a2-streams`
対象Device: nRF52840 DK serial `1050278440`

ユーザー確認:

- ロジックアナライザで `0x5d WR: 0f`、`0x6b WR: 0f` などがdecodeされ、9個目のSCL High中にSDAがLowを維持してACKが返っていることを確認した。
- `0x5d WR: 0f` 後にはSCL High中にSDA Low -> HighのSTOP条件も確認できた。
- つまり、DK側 `P0.26` / `P0.27` からX-NUCLEO-IKS01A2側I2C busへの物理接続とACKは成立していた。

原因:

- nRF52840 DKのboard標準devicetreeでは `arduino_i2c` が `compatible = "nordic,nrf-twi"` になっていた。
- ロジックアナライザ上はACK/STOPが見えているにもかかわらず、Zephyr `i2c_nrfx_twi` driverは `i2c_write()` / `i2c_write_read()` を `-EIO` として返していた。
- そのため、LSM6DSL/HTS221/LPS22HBのZephyr driver初期化が失敗し、Capabilityは `streams=1`、Statusは `LSM6DSL_NOT_READY` になっていた。
- 追加で `CONFIG_NRFX_TWI_LOG=y` と `NRFX_TWI_CONFIG_LOG_LEVEL=4` を一時的に有効化し、`LPS22HB@0x5d` だけをprobeして確認したところ、TWI自体は `NRF_TWI_EVENT_TXDSENT` / `NRF_TWI_EVENT_STOPPED` / `EVT_DONE` まで到達していた。`ADDRESS_NACK`、`DATA_NACK`、`BUS_ERROR` は出ていなかった。
- NCS v3.2.2同梱Zephyrの `drivers/i2c/i2c_nrfx_twi.c` では、`event_handler()` が `NRFX_TWI_EVT_DONE` で `dev_data->res = 0` を設定する一方、transfer完了後の判定は `if (data->res != NRFX_SUCCESS)` のままだった。nrfx 4系では `NRFX_SUCCESS` が `0` ではなく `0x0BAD0000` 系の値として残っているため、成功したtransferも常に `-EIO` へ落ちる。
- 同じ問題はNordic DevZoneでも報告されており、NCS v3.2.0-v3.2.2で `nordic,nrf-twi` が成功transferを `-EIO` として返す既知不具合として扱われ、mainでは `data->res != 0` へ修正済みとされている。
- したがって、今回の「ACK/STOPが見えるのにread transactionへ進まない」現象は、bus上のNACKやSTOP異常ではなく、Zephyr TWI wrapperの成功判定不一致によりアプリが `i2c_write()` の時点で失敗扱いにして後続readを実行しなかったことが直接原因である。

対処:

- `firmware/boards/nrf52840dk_nrf52840.overlay` で `arduino_i2c` を `compatible = "nordic,nrf-twim"` へ切り替えた。
- pristine build後、`build/firmware/firmware/zephyr/.config` で `CONFIG_I2C_NRFX_TWIM=y`、`CONFIG_NRFX_TWIM=y`、`CONFIG_NRFX_TWI0` 無効を確認した。
- `nordic,nrf-twi` へのlocal patchも理論上は可能だが、NCS管理下のZephyr driverに手を入れるより、nRF52840で利用可能な `nordic,nrf-twim` を選ぶ方を本プロジェクトの方針とする。

参考:

- Nordic DevZone: [In NCS v3.2.0-3.2.2 i2c_nrfx_twi_transfer returns -EIO because it is checking for NRFX_SUCCESS, but the success value has changed to 0](https://devzone.nordicsemi.com/f/nordic-q-a/127138/in-ncs-v3-2-0-3-2-2-i2c_nrfx_twi_transfer-returns--eio-because-it-is-checking-for-nrfx_success-but-the-success-value-has-changed-to-0)
- Nordic DevZone: [Bug Report: I2C TWI driver always returns -EIO due to NRFX_SUCCESS comparison mismatch](https://devzone.nordicsemi.com/f/nordic-q-a/126148/bug-report-i2c-twi-driver-always-returns--eio-due-to-nrfx_success-comparison-mismatch)
- Zephyr docs: [`nordic,nrf-twim`](https://docs.zephyrproject.org/latest/build/dts/api/bindings/i2c/nordic%2Cnrf-twim.html)、[`nordic,nrf-twi`](https://docs.zephyrproject.org/latest/build/dts/api/bindings/i2c/nordic%2Cnrf-twi.html)

確認内容:

- `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware --pristine --shield x_nucleo_iks01a2` でbuild成功。
- `west flash --build-dir build/firmware --dev-id 1050278440` でflash成功。
- VCOM0で以下のWHO_AM_I readを確認:
  - `HTS221@0x5f`: `0xbc`
  - `LPS22HB@0x5d`: `0xb1`
  - `LSM6DSL@0x6b`: `0x6a`
  - `LSM303AGR accel@0x19`: `0x33`
  - `LSM303AGR magn@0x1e`: `0x00`。magnetometerだけ期待値 `0x40` と異なるため、必要時に別途確認する。
- `pc_app/scripts/ble_e2e_smoke.py --first-window-s 3 --second-window-s 2` でscan/connect、Capability Read、Sensor Data Notify、interval変更、Status Read、stop後通知停止を確認。
- BLE smoke結果は `streams=2`、Status `last_error=NONE`。
- Capabilityに `stream_id=10`、`type=IMU6`、`payload_format=IMU6_INT16_V1` が出た。
- 3秒windowでmixed stream 31 samples、IMU6 stream 76 samples。2秒windowでmixed stream 4 samples、IMU6 stream 51 samples。missed samples 0。

判定:

- X-NUCLEO-IKS01A2上のI2Cセンサ接続は成立した。
- LSM6DSL streamは実機で有効化され、`stream_id=10` のIMU6データ通知まで確認できた。
- この環境ではnRF52840 DKのArduino I2Cは `nordic,nrf-twi` ではなく `nordic,nrf-twim` を使う方針とする。

次作業:

1. 周期I2C probeは診断用なので、常設するか、debug build限定にするか判断する。2026-06-19にdebug build限定へ変更済み。
2. IMU6 sampleの値変化を、shieldを軽く動かして確認する。
3. `LSM303AGR magn@0x1e` のWHO_AM_Iが `0x00` になる理由は、必要になった時点で別途確認する。

### 2026-06-19: RTT-only logging検証

対象branch: `codex/design-iks01a2-streams`
対象Device: nRF52840 DK serial `1050278440`

目的:

- 試作基板でVCOM0が使えない場合に、Codex側でSEGGER RTTログを見ながら開発できるか確認した。
- 今回は本来の次作業とは別件だが、今後の試作基板bring-upで使うため手順と制約を残す。

実装/設定:

- `firmware/rtt_debug.conf` を追加し、RTT-only logging用のextra confとした。
- 有効化した主なKconfig:
  - `CONFIG_USE_SEGGER_RTT=y`
  - `CONFIG_LOG_BACKEND_RTT=y`
  - `CONFIG_LOG_BACKEND_RTT_MODE_DROP=y`
  - `CONFIG_CONSOLE=n`
  - `CONFIG_LOG_BACKEND_UART=n`
- `RTT_CONSOLE` と `LOG_BACKEND_RTT` を同時に使う構成も試したが、SEGGER `JLinkRTTClient` の画面出力に大量の非表示文字が混ざった。最終的にはconsoleを使わず、log backendだけRTTへ出す構成を採用した。

確認内容:

- `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware-rtt --pristine --shield x_nucleo_iks01a2 --extra-conf rtt_debug.conf` でbuild成功。
- `build/firmware-rtt/firmware/zephyr/.config` で `CONFIG_USE_SEGGER_RTT=y`、`CONFIG_LOG_BACKEND_RTT=y`、`CONFIG_CONSOLE=n`、`CONFIG_LOG_BACKEND_RTT_MODE_DROP=y` を確認。
- `west flash --build-dir build/firmware-rtt --dev-id 1050278440` でflash成功。
- `JLinkExe -device nRF52840_xxAA -SelectEmuBySN 1050278440 -if SWD -speed 4000 -AutoConnect 1` と `JLinkRTTClient` でRTTログ表示を確認した。
- `JLinkRTTClient` の画面表示ではログ行自体は読めるが、非表示文字が混ざるため長時間作業には不向きだった。
- `JLinkRTTLogger` でchannel `0` を `build/rtt_logger_capture.log` へ保存すると、ANSI color escapeを除けば通常のZephyr logとしてきれいに取得できた。
- RTTログで `HTS221@0x5f`、`LPS22HB@0x5d`、`LSM6DSL@0x6b`、`LSM303AGR accel@0x19` のWHO_AM_I read成功ログを確認した。
- `pc_app/scripts/ble_e2e_smoke.py --first-window-s 3 --second-window-s 2` を `pc_app/` から `uv run --extra dev python ...` で実行し、BLE smoke成功。
- BLE smoke結果は `streams=2`、Status `last_error=NONE`、mixed stream 31/3 samples、IMU6 stream 76/49 samples、missed samples 0。

判定:

- Codex側でRTTログを取得しながらnRF52840 DK firmwareを開発できる。
- 試作基板向けには `firmware/rtt_debug.conf` をextra confとして使い、ログ閲覧はリアルタイム画面表示より `JLinkRTTLogger` のファイル取得を優先する。
- VCOM0前提のdebugが使えない場合でも、build/flash/RTT log capture/BLE smokeまでagent側で実施可能。

### 2026-06-19: 周期I2C probeのdebug build限定化

対象branch: `codex/design-iks01a2-streams`

目的:

- ロジックアナライザ観測用に追加した周期I2C WHO_AM_I probeを、通常Firmwareに常設しない方針へ整理した。
- 通常buildではLSM6DSL stream本体だけを動かし、bring-up / RTT / VCOM診断時だけ明示的にprobeを有効化する。

実装内容:

- `firmware/Kconfig` を追加し、`CONFIG_BSL_IKS01A2_I2C_PROBE` をdefault offで定義した。
- `firmware/CMakeLists.txt` では `target_sources_ifdef(CONFIG_BSL_IKS01A2_I2C_PROBE ...)` により、probe有効時だけ `iks01a2_i2c_probe.c` をcompileする。
- `firmware/src/main.c` では `IS_ENABLED(CONFIG_BSL_IKS01A2_I2C_PROBE)` でincludeと `iks01a2_i2c_probe_run()` をguardした。
- `firmware/iks01a2_i2c_probe_debug.conf` を追加し、診断buildでは `--extra-conf iks01a2_i2c_probe_debug.conf` で有効化できるようにした。
- `firmware/README.md` に、通常shield build、RTT-only build、probe診断build、RTT+probe buildの使い分けを追記した。

確認内容:

- 通常shield build:
  - `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware --pristine --shield x_nucleo_iks01a2` 成功。
  - `build/firmware/firmware/zephyr/.config` で `# CONFIG_BSL_IKS01A2_I2C_PROBE is not set` を確認。
  - `compile_commands.json` に `iks01a2_i2c_probe.c` が含まれないことを確認。
- probe診断build:
  - `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware-probe --pristine --shield x_nucleo_iks01a2 --extra-conf iks01a2_i2c_probe_debug.conf` 成功。
  - `build/firmware-probe/firmware/zephyr/.config` で `CONFIG_BSL_IKS01A2_I2C_PROBE=y` を確認。
  - `compile_commands.json` に `iks01a2_i2c_probe.c` が含まれることを確認。
- RTT+probe build:
  - `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware-rtt-probe --pristine --shield x_nucleo_iks01a2 -- -DEXTRA_CONF_FILE="rtt_debug.conf;iks01a2_i2c_probe_debug.conf"` 成功。
  - `rtt_debug.conf` と `iks01a2_i2c_probe_debug.conf` が同時にmergeされることをconfigure logで確認。

判定:

- 周期I2C probeはdebug build限定とする。
- 通常Firmwareでは、周期的なraw I2C read logを出さず、Capability/Status/LSM6DSL IMU6 streamの通常動作を優先する。

次作業:

1. IMU6 sampleの値変化を、shieldを軽く動かして確認する。
2. `LSM303AGR magn@0x1e` のWHO_AM_Iが `0x00` になる理由は、必要になった時点で別途確認する。
3. 次の実装テーマとして、HTS221 humidity/temperatureまたはLPS22HB pressureの低頻度stream化を検討する。どちらも2026-06-20に実装・実機確認済み。

### 2026-06-20: HTS221 humidity/temperature stream実装

対象branch: `codex/design-iks01a2-streams`
対象Device: nRF52840 DK serial `1050278440`

目的:

- LSM6DSL IMU6に続く低頻度環境streamとして、X-NUCLEO-IKS01A2上のHTS221 humidity/temperatureをBLE Sensor Data frame v3へ追加した。
- Capabilityに実機availabilityを反映し、PC backend/CUI/WebGUI/CSV/smoke scriptで新payloadをparseできるようにした。

実装内容:

- Firmwareに `hts221_sensor` adapterを追加し、Zephyr sensor APIで `SENSOR_CHAN_HUMIDITY` と `SENSOR_CHAN_AMBIENT_TEMP` を1秒周期で取得する。
- `stream_id=30`、`stream_type=TEMP_HUMIDITY`、`payload_format=HTS221_TEMP_HUMIDITY_INT16_V1` を追加した。
- payloadは `int16 humidity_centi_percent` と `int16 temperature_centi_c` の4 bytesとし、0.01 %RH / 0.01 degC単位で送信する。
- `CONFIG_BSL_IKS01A2_I2C_PROBE` は引き続きdebug build限定のまま、通常buildには周期raw I2C probeを含めない。
- PC側 `protocol.py`、interactive CUI、WebGUI固定表示、CSV列、BLE smoke script、pytestをHTS221 payloadに対応させた。
- この時点のCapability最大stream数は3となり、実機では `MIXED_SAMPLE`、`IMU6`、`TEMP_HUMIDITY` を返す。

確認内容:

- PC自動テスト:
  - `cd pc_app && uv run --extra dev pytest`
  - `18 passed`
- 通常shield build:
  - `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware --pristine --shield x_nucleo_iks01a2`
  - 成功。`build/firmware/firmware/zephyr/.config` で `CONFIG_HTS221=y`、`CONFIG_HTS221_ODR="1"`、`CONFIG_I2C_NRFX_TWIM=y`、`# CONFIG_BSL_IKS01A2_I2C_PROBE is not set` を確認。
- Flash:
  - `west flash --build-dir build/firmware --dev-id 1050278440`
  - 成功。
- BLE smoke:
  - `cd pc_app && uv run --extra dev python scripts/ble_e2e_smoke.py --first-window-s 3 --second-window-s 2`
  - Capabilityは `streams=3`。`stream_id=1 MIXED_SAMPLE`、`stream_id=10 IMU6`、`stream_id=30 TEMP_HUMIDITY` を確認。
  - 3秒windowでmixed 30 samples、IMU6 76 samples、HTS221 4 samples。
  - 2秒windowでmixed 3 samples、IMU6 50 samples、HTS221 2 samples。
  - Status `last_error=NONE`、stop後0 samples、missed samples 0。
  - 最新HTS221値としてhumidity `73.00 %RH`、temperature `28.50 degC` を確認。

判定:

- HTS221 humidity/temperature streamは実機で有効化され、Capability ReadとSensor Data Notifyで確認済み。
- LSM6DSL IMU6とHTS221 TEMP_HUMIDITYを同時に通知してもstream単位sequence欠落は0だった。
- 次の低頻度環境stream候補は `LPS22HB@0x5d` pressure 1 Hz。2026-06-20に実装・実機確認済み。

次作業:

1. LPS22HB pressure streamは2026-06-20に `stream_id=20`、`payload_format=LPS22HB_PRESSURE_INT32_V1`、Pa単位int32として実装・実機確認済み。
2. WebGUIの固定表示を増やし続ける前に、Capability metadataからstream一覧と表示項目を生成する範囲を決める。
3. Status `last_error` は1値のみなので、複数optional sensorのavailabilityを同時に伝えるにはStatus拡張またはLog/Eventが必要。2026-06-20にStatusへsensor別error fieldを追加して解消済み。
4. 次の実センサ候補は `LSM303AGR` magnetometer/accel。ただし先にWebGUIのCapability driven化またはStatus/Log/Event拡張を入れる選択肢もある。

### 2026-06-20: LPS22HB pressure stream実装

対象branch: `codex/design-iks01a2-streams`
対象Device: nRF52840 DK serial `1050278440`

目的:

- HTS221 humidity/temperatureに続く低頻度環境streamとして、X-NUCLEO-IKS01A2上のLPS22HB pressureをBLE Sensor Data frame v3へ追加した。
- Capabilityに実機availabilityを反映し、PC backend/CUI/WebGUI/CSV/smoke scriptでpressure payloadをparseできるようにした。

実装内容:

- Firmwareに `lps22hb_sensor` adapterを追加し、Zephyr sensor APIで `SENSOR_CHAN_PRESS` を1秒周期で取得する。
- `stream_id=20`、`stream_type=PRESSURE`、`payload_format=LPS22HB_PRESSURE_INT32_V1` を追加した。
- payloadは `int32 pressure_pa` の4 bytesとし、ZephyrのLPS22HB pressure値をPa単位へ変換して送信する。
- Capability最大stream数を4へ拡張し、readyな場合は `MIXED_SAMPLE`、`IMU6`、`TEMP_HUMIDITY`、`PRESSURE` を返す。
- Status `last_error` に `LPS22HB_NO_DEVICETREE` と `LPS22HB_NOT_READY` を追加した。
- PC側 `protocol.py`、interactive CUI、WebGUI固定表示、CSV列、BLE smoke script、pytestをLPS22HB payloadに対応させた。

確認内容:

- PC自動テスト:
  - `cd pc_app && uv run --extra dev pytest`
  - `20 passed`
- 通常shield build:
  - NCS workspaceから `west build -b nrf52840dk/nrf52840 <repo-root>/firmware --build-dir <repo-root>/build/firmware --pristine --shield x_nucleo_iks01a2`
  - 成功。`build/firmware/firmware/zephyr/.config` で `CONFIG_LPS22HB=y`、`CONFIG_I2C_NRFX_TWIM=y`、`# CONFIG_BSL_IKS01A2_I2C_PROBE is not set` を確認。
- Flash:
  - `west flash --build-dir <repo-root>/build/firmware`
  - 成功。board serial `1050278440` に書き込み。
- BLE smoke:
  - `cd pc_app && uv run python scripts/ble_e2e_smoke.py --first-window-s 3 --second-window-s 2`
  - Capabilityは `streams=4`。`stream_id=1 MIXED_SAMPLE`、`stream_id=10 IMU6`、`stream_id=30 TEMP_HUMIDITY`、`stream_id=20 PRESSURE` を確認。
  - 3秒windowでmixed 29 samples、IMU6 75 samples、LPS22HB 4 samples、HTS221 4 samples。
  - 2秒windowでmixed 4 samples、IMU6 51 samples、LPS22HB 2 samples、HTS221 2 samples。
  - Status `last_error=NONE`、stop後0 samples、missed samples 0。
  - 最新LPS22HB値としてpressure `99597 Pa` を確認。

判定:

- LPS22HB pressure streamは実機で有効化され、Capability ReadとSensor Data Notifyで確認済み。
- LSM6DSL IMU6、HTS221 TEMP_HUMIDITY、LPS22HB PRESSUREを同時に通知してもstream単位sequence欠落は0だった。
- 低頻度環境streamの最初の2本、HTS221とLPS22HBは実装済みになった。

次作業:

1. WebGUIの固定表示を増やし続ける前に、Capability metadataからstream一覧、表示単位、CSV列を生成する範囲を決める。
2. Status `last_error` は1値のみなので、複数optional sensorのavailabilityを同時に伝えるにはStatus拡張またはLog/Eventが必要。2026-06-20にStatusへsensor別error fieldを追加して解消済み。
3. 次の実センサ候補は `LSM303AGR` magnetometer/accel。`LSM303AGR magn@0x1e` のWHO_AM_Iが過去に `0x00` だった点は、着手時に再確認する。

### 2026-06-20: WebGUI複数stream表示のスパイク修正

対象branch: `codex/design-iks01a2-streams`

現象:

- 実機とWeb UIで、Battery、Accel Zなど複数signalのRealtime graphに周期的なスパイクまたは落ち込みが見えた。
- 例としてBatteryは本来mixed sampleだけが持つfieldだが、IMU6/HTS221/LPS22HB sampleではPython側のdataclass default値 `0` がWebへ送られ、WebGUIの単一historyへ混入していた。
- Accel系も、温湿度/気圧streamのdefault `0` が同じfield名として描画対象に入るため、実センサ値ではなく表示/serialization由来の落ち込みが発生していた。

原因:

- `SensorDataPayload` はstream-specific payloadを1つのdataclassに展開しており、payloadに存在しないfieldをdefault `0` で保持していた。
- `web_api.py` が `asdict(sample)` をそのままWebSocket sampleとして送信し、`app.js` が全streamを1つの `history` に入れて選択中metricを描画していた。
- このため「未定義fieldの0」と「実際の測定値0」を区別できなかった。

修正方針:

- WebSocket sample JSONでは、payload formatに存在しないfieldを `null` にする。
- WebGUIのRealtime graphは選択metricが `null` のsampleを点列から除外する。
- 最新値カードは、対象fieldを持つsampleを受け取ったときだけ更新する。

判定:

- Firmwareの実センサ読み取りやBLE Notifyのsequence欠落ではなく、PC backend/WebGUIの複数stream表示処理の不具合として扱う。
- 今後Capability driven UIへ進める際も、field metadataに存在しない値は0埋めせず、missing/nullとして扱う。

追調査:

- Batteryは修正後に安定したが、Accel Zにはまだスパイクが見えた。
- Batteryはmixed streamだけが持つfieldなので `null` 除外で解消した。一方、Accel Zはmixed streamとLSM6DSL IMU6 streamの両方が `accel_z_mg` を持つため、field名だけで描画すると2つの別streamを同じ系列へ混ぜてしまう。
- WebGUIのSignal定義を `field` だけではなく `stream_id + field` に変更し、`Dummy Accel Z` と `LSM6DSL Accel Z` を別Signalとして扱う。
- 最新値カードのAccel/GyroはLSM6DSL IMU6 streamの値として表示し、Battery/ADCはmixed streamの値として表示する。
- Protocol上の `MIXED_SAMPLE` / `stream_id=1` は既存名のまま維持する。ただしこのstreamにはdummy accel、dummy battery、実測ADCが混在するため、UI表示ではstream全体を `Dummy` と呼ばず、field単位で `Dummy Accel`、`Dummy Battery`、`ADC` と呼ぶ。

### 2026-06-20: WebGUI Capability metadata driven化

対象branch: `codex/webgui-capability-driven`

目的:

- stream追加のたびにWebGUIの固定カード、Signal option、CSV列を手作業で増やす状態を減らす。
- `stream_id + field` の分離を維持したまま、Capability metadataから表示対象を組み立てる。

実装内容:

- `web_api.py` の `/api/capability` responseに、backendが `payload_format` から補完する `fields` metadataを追加した。
- `fields` metadataは `field`、`label`、`unit`、`scale`、`decimals` を持つ。現行Firmware Capability schemaにはまだfield descriptorを入れていない。
- `app.js` は `streams[].fields[]` から `fieldDefinitions` / `metricDefinitions` を作り、最新値カード、Signal select、CSV value列を生成する。
- 未接続時も現行4stream相当のfallback CapabilityでUIを初期表示する。
- HTMLから固定の最新値カードと固定Signal optionを外した。

確認内容:

- `cd pc_app && uv run --extra dev pytest`
  - `22 passed`
- `node --check pc_app/web_frontend/app.js`
  - 成功
- WebGUI smoke:
  - `8765` は使用中だったため、検証用に `127.0.0.1:8766` で起動。
  - Browser確認で最新値カード15項目、Signal option 15項目が生成されることを確認。
  - 1280x720と390x844 viewportで横スクロールなし、主要要素のoverflowなし、console errorなし。

判定:

- WebGUIの最新値カード、Signal選択、CSV列はCapability metadata drivenになった。
- ただしfield metadataはFirmwareから送られる真のCapability payloadではなく、PC backendの `payload_format` 補完である。Firmware Capability schemaへfield descriptorを入れるかどうかは次段の設計判断とする。

次作業:

1. Firmware Capability schemaへfield metadataを入れるか、backend補完を当面の正式仕様とするかの判断は2026-06-20に完了済み。
2. CSVを単一wide tableのままにするか、stream別CSVへ分けるかを必要に応じて整理する。
3. LSM303AGR magnetometer/accelの追加可否を再probeして判断する。

### 2026-06-20: 複数optional sensorのStatus診断拡張

対象branch: `codex/optional-sensor-status`

目的:

- Status `last_error` が1値だけで、LSM6DSL/HTS221/LPS22HBの複数同時失敗をPC側から同時に読めない状態を解消する。
- `last_error` は既存表示向けの代表値として残しつつ、sensor別のavailability診断をStatus Readで取得できるようにする。

実装内容:

- Status payloadを8 bytesから14 bytesへ拡張し、`lsm6dsl_error`、`hts221_error`、`lps22hb_error` を追加した。
- Firmware `control` / `ble_service` は起動時、start/stop、Config更新時にsensor別error fieldを更新し、`last_error` にはLSM6DSL、HTS221、LPS22HBの順で最初の非 `NONE` を代表値として入れる。
- PC protocol parser、CUI status表示、`/api/status` response、WebGUI Status bandを更新し、Web APIでは `status.optional_sensors.{lsm6dsl,hts221,lps22hb}.error` を返す。
- `docs/specs/current_implementation_spec.md`、`docs/design/system_design.md`、`docs/design/firmware_design.md`、READMEを更新した。

確認内容:

- `cd pc_app && uv run --extra dev pytest`
  - `24 passed`
- NCS v3.2.2 toolchain bundle `e5f4758bcf` でshield付きFirmware build成功。
  - `west build -b nrf52840dk/nrf52840 <repo-root>/firmware --build-dir <repo-root>/build/firmware --pristine --shield x_nucleo_iks01a2`
- nRF52840 DK serial `1050278440` へflash成功。
- BLE smoke成功。
  - Capability: `streams=4`、`stream_id=1/10/30/20`
  - first window: total `114` samples、stream 1=`30`、stream 10=`76`、stream 20=`4`、stream 30=`4`
  - second window: total `57` samples、stream 1=`3`、stream 10=`50`、stream 20=`2`、stream 30=`2`
  - Status: `last_error=NONE`, `optional_sensors=lsm6dsl:NONE,hts221:NONE,lps22hb:NONE`
  - stop後 `after_stop_samples=0`、`missed_samples=0`
- `node --check pc_app/web_frontend/app.js` 成功。
- WebGUI layout確認:
  - `127.0.0.1:8766` で起動し、1280x720と390x844でStatus bandの横スクロールがないことを確認。
  - 長いerror名向けにStatus値へ `overflow-wrap: anywhere` を追加した。

判定:

- 複数optional sensorのavailability/errorは、Status Read / CUI / Web API / WebGUIからsensor別に確認できる。
- 現行4streamのCapability、Notify、stop後停止、missed sampleなしは実機smokeで維持できている。

次作業:

1. Firmware Capability schemaへfield metadataを入れるか、backend補完を当面の正式仕様とするかの判断は2026-06-20に完了済み。
2. I2C addressやpin状態など、sensor別errorより詳細な診断が必要ならLog/Event channelで扱う。

### 2026-06-20: Capability field metadata方針決定

対象branch: `codex/capability-field-metadata`

目的:

- WebGUI Capability metadata driven化の後続判断として、Firmware Capability schemaへfield descriptorを入れるか、PC backend補完を正式仕様として維持するかを決めた。
- 固定長binaryのschema v1を不用意に肥大化させず、次の実センサ追加やCSV整理へ進める状態にする。

判断:

- Firmware Capability schema v1へfield descriptorは入れない。
- schema v1は、Firmwareが返すstream descriptor、payload format、interval、stream flagsまでを正とする。
- WebGUI/CSVで使うfield label、unit、scale、decimal表現は、PC backendが `payload_format` から補完して `/api/capability` の `streams[].fields[]` として返す現行仕様を維持する。
- Firmware由来のfield descriptorが必要になった場合は、schema v1へ後付けせず、Capability schema version 2またはTLV化として設計する。

更新内容:

- `docs/generic_sensor_monitor_design_handoff.md` の現在キューを更新し、field metadata判断を「判断済み」に移した。
- `docs/specs/current_implementation_spec.md` に、Firmware schema v1の責務とPC backend補完metadataの責務を明記した。
- `docs/design/system_design.md`、`docs/design/firmware_design.md`、`docs/design/pc_webapp_design.md` のCapability責務を同じ方針へ揃えた。

確認内容:

- docs-only変更のためFirmware buildと実機smokeは不要。
- `cd pc_app && uv run --extra dev pytest` でPC側仕様との整合を確認する。

次作業:

1. LSM303AGR magnetometer/accelの追加可否は2026-06-20に再probe済み。次はmagnetometer streamを最小実装するか判断する。
2. CSVを単一wide tableのままにするか、stream別CSVへ分けるかを必要に応じて整理する。
3. I2C addressやpin状態など、sensor別errorより詳細な診断が必要ならLog/Event channelで扱う。

### 2026-06-20: LSM303AGR magnetometer/accel再probe

対象branch: `codex/lsm303agr-reprobe`
対象Device: nRF52840 DK serial `1050278440`

目的:

- 次の実センサ候補であるX-NUCLEO-IKS01A2上のLSM303AGR magnetometer/accelをstream化できるか判断する。
- 過去のraw I2C probeで `LSM303AGR magn@0x1e` のWHO_AM_Iが `0x00` だった理由を切り分ける。

実装内容:

- debug build限定の `firmware/src/modules/iks01a2_i2c_probe.c` に、LSM303AGR magnetometer/accelのZephyr sensor API probeを追加した。
- `firmware/iks01a2_i2c_probe_debug.conf` で `CONFIG_LIS2MDL=y`、`CONFIG_LIS2DH=y`、それぞれのtrigger none設定を明示した。
- raw WHO_AM_I probeをtargetごとのregister指定へ変更した。HTS221/LPS22HB/LSM6DSL/LSM303AGR accelは `0x0f`、LIS2MDL互換のLSM303AGR magnetometerは `0x4f` を読む。
- LSM303AGR sensor API probeは周期probeにも出すようにし、RTT drop modeでboot直後ログが欠けても確認できるようにした。

確認内容:

- RTT+probe build:
  - `west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware-rtt-probe --shield x_nucleo_iks01a2 --extra-conf rtt_debug.conf --extra-conf iks01a2_i2c_probe_debug.conf`
  - 成功。app本体 `.config` で `CONFIG_BSL_IKS01A2_I2C_PROBE=y`、`CONFIG_LIS2MDL=y`、`CONFIG_LIS2DH=y`、`CONFIG_I2C_NRFX_TWIM=y`、`CONFIG_LOG_BACKEND_RTT=y` を確認。
- Flash:
  - `west flash --build-dir build/firmware-rtt-probe --dev-id 1050278440`
  - 成功。
- RTT log:
  - `JLinkRTTLogger` channel 0で `build/rtt_lsm303agr_probe_fixed.log` にcapture。
  - `LSM303AGR accel@0x19 WHO_AM_I[0x0f]=0x33 expected=0x33` を確認。
  - `LSM303AGR magn@0x1e WHO_AM_I[0x4f]=0x40 expected=0x40` を確認。
  - `LSM303AGR magn@0x1e is ready` と `mag_xyz=(...)` を確認。
  - `LSM303AGR accel@0x19 is ready` と `accel_xyz=(...)` を確認。

判定:

- LSM303AGR magnetometer/accelは、現在のboard/shield/devicetree構成でZephyr sensor API経由のstream候補として扱える。
- `LSM303AGR magn@0x1e` の過去の `WHO_AM_I=0x00` はhardware不良ではなく、debug probeがLIS2MDL互換sensorのWHO_AM_I register `0x4f` ではなく `0x0f` を読んでいたことによる誤診断だった。
- 次のstream実装候補はLSM303AGR magnetometerを優先する。LSM303AGR accelはLSM6DSL accelと重複するため、必要性が出るまで後回しでよい。

次作業:

1. LSM303AGR magnetometer streamを実装する場合は、`stream_id=12`、mag x/y/zの3軸payload format、Capability metadata、PC parser/WebGUI/CSV/smokeをまとめて更新する。
2. その後にLSM303AGR accel `stream_id=11` を追加するか、LSM6DSL accelとの差分用途を整理してから判断する。
3. debug probeで明示した `CONFIG_LIS2MDL_TRIGGER_NONE` / `CONFIG_LIS2DH_TRIGGER_NONE` と周期probe本体は通常runtimeへ常設しない。通常shield buildではshield overlay由来でLIS2MDL/LIS2DH driver自体は有効になり得るが、stream実装時に必要な処理だけ通常pathへ昇格する。

### 2026-06-20: LSM303AGR magnetometer stream実装

対象branch: `codex/lsm303agr-magnetometer-stream`
対象Device: nRF52840 DK serial `1050278440`

目的:

- X-NUCLEO-IKS01A2上のLSM303AGR magnetometerを、地磁気/方位確認用の最小streamとして通常runtimeへ追加する。
- `stream_id=12`、`MAG3_INT16_V1`、mag x/y/zの3軸payloadをFirmware/PC/WebGUI/CSV/smoke/docsへ同期する。

実装内容:

- Firmwareに `lsm303agr_magn_sensor` moduleを追加し、`DT_ALIAS(magn0)` / Zephyr `st,lis2mdl` driver / `SENSOR_CHAN_MAGN_XYZ` でsample取得するようにした。
- payloadは `MAG3_INT16_V1`、`payload_len=6`、`mag_x_ut` / `mag_y_ut` / `mag_z_ut` のint16とした。Zephyr LIS2MDL driverのmicro-gauss相当値を `uT = micro_gauss / 10000` で変換する。
- Capability schema v1へ `stream_id=12`、`stream_type=MAG3`、`payload_format=MAG3_INT16_V1`、10 Hz相当の100 ms interval descriptorを追加した。
- Status payloadを16 bytesへ拡張し、`lsm303agr_magn_error` と `LSM303AGR_MAGN_NO_DEVICETREE` / `LSM303AGR_MAGN_NOT_READY` を追加した。
- PC側 `protocol.py`、interactive CUI、WebGUI backend metadata、Web fallback Capability、CSV列、BLE smoke script、pytestをMAG3 payloadへ対応させた。
- `README.md`、`docs/specs/current_implementation_spec.md`、`docs/design/system_design.md`、`docs/design/firmware_design.md`、`docs/design/pc_webapp_design.md` を現行仕様へ同期した。

確認内容:

- PC自動テスト:
  - `cd pc_app && uv run --extra dev pytest`
  - 成功。26 tests passed。
- Firmware shield build:
  - `west build --pristine -b nrf52840dk/nrf52840 firmware --build-dir build/firmware -- -DSHIELD=x_nucleo_iks01a2`
  - 成功。app本体 `.config` で `CONFIG_LIS2MDL=y`、`CONFIG_I2C_NRFX_TWIM=y`、`# CONFIG_BSL_IKS01A2_I2C_PROBE is not set` を確認。
  - `CONFIG_LIS2MDL` は通常 `prj.conf` へ明示せず、shield Devicetree由来のKconfig defaultに任せる。
- Flash:
  - `west flash --build-dir build/firmware --dev-id 1050278440`
  - 成功。
- BLE smoke:
  - `uv run --extra dev python scripts/ble_e2e_smoke.py --first-window-s 3 --second-window-s 2`
  - 成功。Capabilityは `streams=5` で `id=12 type=MAG3 channels=3 format=MAG3_INT16_V1 interval=100ms` を確認。
  - 3秒windowでmixed 29 samples、IMU6 75 samples、MAG3 29 samples、LPS22HB 4 samples、HTS221 4 samples。
  - 2秒windowでmixed 4 samples、IMU6 51 samples、MAG3 20 samples、LPS22HB 2 samples、HTS221 2 samples。
  - Statusは `last_error=NONE`、`optional_sensors=lsm6dsl:NONE,hts221:NONE,lps22hb:NONE,lsm303agr_magn:NONE`。
  - `missed_samples=0`、最新magnetometer値として `latest_magnetometer_ut=-4,11,-84` を確認。

判定:

- LSM303AGR magnetometer streamは通常runtimeで有効化され、Capability ReadとSensor Data Notifyで実機確認済み。
- LSM6DSL IMU6、HTS221 TEMP_HUMIDITY、LPS22HB PRESSURE、LSM303AGR MAG3を同時に通知してもstream単位sequence欠落は0だった。
- LSM303AGR accel `stream_id=11` はLSM6DSL accelと重複するため、用途が明確になるまで後回しにする。

次作業:

1. CSV loggerの複数stream表現を整理する。現行は単一wide CSVで、MAG3追加によりfield列がさらに増えた。
2. LSM303AGR accel `stream_id=11` は比較用途、低電力構成、またはLSM6DSLを使わない構成の必要性が出た時点で判断する。
3. optional sensorの詳細診断が必要なら、Status fieldを増やすよりLog/Event channelでI2C scan結果やaddress候補を扱う。

### 2026-06-21: CSV logger複数stream表現整理

対象branch: `codex/csv-multistream-logging`

目的:

- LSM303AGR magnetometer追加で5 streamになったため、WebGUI CSV downloadの複数stream表現、同名fieldの扱い、欠損表現、stream別CSV分割の要否を整理する。

実装内容:

- CSVはstream別ファイルに分割せず、1つのwide CSVとしてdownloadする方針にした。
- CSV値列は `/api/capability` の `streams[].fields[]` から生成し、列名を `s<stream_id>_<field>` にした。例: `s1_accel_z_mg`, `s10_accel_z_mg`。
- sample行と一致しないstreamの値列は空欄にする。これにより、同じ `accel_z_mg` を持つmixed streamとLSM6DSL streamを列で分離する。
- WebGUIの重複sample判定を `host_time_ms` だけから、`stream_id`、`sequence`、`timestamp_ms`、`payload_format` の一致へ変更した。同じhost時刻に複数stream sampleが届いてもCSV/Chartから落とさない。
- `docs/design/pc_webapp_design.md`、`docs/design/system_design.md`、`docs/specs/current_implementation_spec.md` をCSV仕様へ同期した。

確認内容:

- `node --check pc_app/web_frontend/app.js`
  - 成功。
- `cd pc_app && uv run --extra dev pytest`
  - 成功。27 tests passed。
- Web配信確認:
  - `uv run python -m ble_sensor_logger --web --host 127.0.0.1 --port 8767`
  - `curl -fsS http://127.0.0.1:8767/` でHTMLを取得。
  - `curl -fsS http://127.0.0.1:8767/static/app.js` で `csvValueColumns` / `sameSample` / `sampleMatchesStream` を含むapp.js配信を確認。

判定:

- CSV loggerの複数stream表現は、単一wide CSV + stream-qualified列 + 非該当stream空欄を現行仕様とする。
- stream別CSV分割は長時間loggingや解析tool要件が明確になった時点で再検討する。

次作業:

1. LSM303AGR accel `stream_id=11` は比較用途、低電力構成、またはLSM6DSLを使わない構成の必要性が出た時点で判断する。
2. optional sensorの詳細診断が必要なら、Status fieldを増やすよりLog/Event channelでI2C scan結果やaddress候補を扱う。

### 2026-06-20: WebGUI複数グラフ化

対象branch: `codex/gui-rich-graphs`

目的:

- WebGUIのRealtime graphを、複数stream/複数fieldの比較に使いやすい表示へ拡張する。
- 最大3個のgraphを描画し、各graphで最大3つのSignalを選択できるようにする。
- GraphごとにY軸rangeをAuto/manual、X軸rangeをAuto/任意秒数windowから選べるようにする。

実装内容:

- `pc_app/web_frontend/index.html` の単一canvas / 単一Signal selectを、3つのgraph panelへ置き換えた。Graph 1は初期有効、Graph 2/3はcheckboxで任意に有効化する。
- `pc_app/web_frontend/app.js` に `graphConfigs` を追加し、graphごとのenabled、最大3Signal、Y軸Auto/manual min/max、X軸Auto/seconds windowを保持するようにした。
- GraphのSignal selectは、従来どおり `/api/capability` の `streams[].fields[]` から生成する。`stream_id + field` のmetric keyは維持し、同名fieldが複数streamにある場合も混ぜない。
- Graph panel内のcontrol同士が重ならないよう、Controls領域を折りたたみ可能にし、CSS container queryでpanel幅に応じてSignal select/range controlを縦積みにするようにした。
- Chart描画値はCapability由来の `scale` を適用した表示単位の値に変更した。HTS221 humidity/temperatureなどはcenti単位ではなく `%RH` / `degC` 相当の値でY軸を決める。
- X軸Autoは保持履歴内の選択Signal sample全体、Secondsは最新sampleから指定秒数分のwindowを表示する。履歴上限は固定秒数window向けに300点から5000点へ増やした。
- `pc_app/tests/test_web_api.py` のfrontend静的確認を、新しいgraph panel/range control前提に更新した。
- `docs/specs/current_implementation_spec.md` と `docs/design/pc_webapp_design.md` を現行仕様へ更新した。

確認内容:

- `node --check pc_app/web_frontend/app.js` 成功。
- `cd pc_app && uv run --extra dev pytest`
  - `24 passed`
- `http://127.0.0.1:8766/` でWebGUI HTMLを取得し、3つのgraph panelが配信されることを確認した。
- In-app browser確認は、browser接続ツールがsandbox metadata errorで起動できなかったため未実施。

次作業:

1. 実機またはmock sampleで、Graph 1/2/3の有効化、最大3Signal同時描画、Y manual range、X seconds windowの視認確認を行う。
2. 必要ならgraph設定のlocalStorage保存、series legendの色表示、stream単位CSV出力を追加検討する。

### 2026-06-21: WebGUI操作後500 error修正

対象branch: `codex/gui-rich-graphs`

症状:

- WebGUIでConnect、Start、Stop後にtoastで `500 Internal Server Error` が出る。
- BLE操作自体は継続でき、graph描画も動いていた。

原因:

- 実機FirmwareはLSM303AGR magnetometer stream実装後のprotocolを返していた。
- このWebGUI worktreeのPC backend/parserは、`stream_type=5`、`payload_format=5`、Status 16 bytesをまだ知らない古いprotocol定義だった。
- そのため、操作後の `/api/status` と `/api/capability` refreshで `ProtocolError` が未処理となりHTTP 500になっていた。

修正内容:

- `pc_app/src/ble_sensor_logger/protocol.py` をMAG3対応へ同期した。
  - `stream_id=12`
  - `PayloadFormat.MAG3_INT16_V1`
  - `StreamType.MAG3`
  - `StreamUnit.UT`
  - Status payload 16 bytes / `lsm303agr_magn_error`
- `pc_app/src/ble_sensor_logger/web_api.py` にMAG3 field metadataと `optional_sensors.lsm303agr_magn.error` を追加した。
- WebGUIのfallback capabilityとStatus bandにも `LSM303AGR Mag` を追加した。
- `docs/specs/current_implementation_spec.md` をMAG3/Status 16 bytesの現行仕様へ同期した。

確認内容:

- `node --check pc_app/web_frontend/app.js` 成功。
- `cd pc_app && uv run --extra dev pytest`
  - `26 passed`
- 実機API確認:
  - `GET /api/scan`: `BLE_SENSOR_LOGGER` 検出。
  - `POST /api/connect`: HTTP 200。
  - `GET /api/status`: HTTP 200。`optional_sensors.lsm303agr_magn.error=NONE`。
  - `GET /api/capability`: HTTP 200。`stream_id=12` / `stream_type=MAG3` / `payload_format=MAG3_INT16_V1` を確認。
  - `POST /api/start`: HTTP 200。
  - Start後 `GET /api/status`: HTTP 200。`latest_sample.stream_id=12` / `payload_format=5` をparseできることを確認。
  - `POST /api/stop`: HTTP 200。
  - Stop後 `GET /api/status`: HTTP 200。
