# 現行実装仕様書

作成日: 2026-06-17  
対象: 現時点の BLE Sensor Logger 実装  
実装範囲: Firmware、Python backend、interactive CUI、local WebGUI

## 1. 目的

本資料は、現時点で実装済みの外部仕様を定義する。将来のGeneric Sensor Monitor化に向けた設計案は扱わず、現在のFirmwareとPC Appが実際に提供している振る舞いを記録する。

将来方針や未決事項は `docs/generic_sensor_monitor_design_handoff.md` を参照する。

## 2. システム概要

現行実装は、nRF52840 DKをBLE Peripheral / GATT Server、PCをBLE Central / GATT Clientとして接続する。ブラウザはBLEへ直接接続せず、Python backendがBLE通信を担当し、WebGUIはHTTP/WebSocketでbackendと通信する。

構成:

| 要素 | 内容 |
| --- | --- |
| Device | nRF52840 DK |
| Firmware SDK | NCS v3.2.2 / Zephyr |
| Firmware event基盤 | App Event Manager |
| Sensor | dummy accel、X-NUCLEO-IKS01A2上のLSM6DSL、HTS221、LPS22HB、LSM303AGR magnetometer |
| BLE role | Peripheral / GATT Server |
| PC backend | Python、bleak、aiohttp |
| UI | interactive CUI、local WebGUI |
| Protocol | Little Endian固定長binary、Protocol v3 |

## 3. 対象範囲

- BLE Advertising
- Custom GATT Service
- Sensor Data Notification
- Control Write
- Config Read/Write
- Status Read
- Capability Read
- PC側scan/connect/disconnect
- Measurement start/stop
- Sampling interval変更
- Sequence reset
- Sensor Data parseと欠落検出
- WebGUIのリアルタイム表示とCSV download

## 4. 非対象範囲

- Status Notify
- Log/Event Characteristic
- NUS/Serial transport
- Fragmentation/reassembly
- BrowserからのWeb Bluetooth直接接続
- 長期loggingの永続保存

## 5. BLE仕様

### 5.1 Advertising

| 項目 | 値 |
| --- | --- |
| Device name | `BLE_SENSOR_LOGGER` |
| Service UUID | `12345678-1234-5678-1234-56789abcdef0` |
| Advertising mode | Connectable fast advertising |
| 最大接続数 | 1 |

### 5.2 GATT Service

| 名称 | UUID |
| --- | --- |
| BLE Sensor Logger Service | `12345678-1234-5678-1234-56789abcdef0` |

### 5.3 Characteristics

| 名称 | UUID | Properties | 方向 | 用途 |
| --- | --- | --- | --- | --- |
| Sensor Data | `12345678-1234-5678-1234-56789abcdef1` | Notify | Device -> PC | センササンプル通知 |
| Control | `12345678-1234-5678-1234-56789abcdef2` | Write / Write Without Response | PC -> Device | start/stop/status request/sequence reset |
| Config | `12345678-1234-5678-1234-56789abcdef3` | Read / Write | PC <-> Device | sampling intervalなど |
| Status | `12345678-1234-5678-1234-56789abcdef4` | Read | Device -> PC | 状態、エラー、接続回数 |
| Capability | `12345678-1234-5678-1234-56789abcdef5` | Read | Device -> PC | 最小stream metadata、対応command/feature |

## 6. Binary Protocol

### 6.1 共通ルール

| 項目 | 仕様 |
| --- | --- |
| byte order | Little Endian |
| payload長 | 固定長 |
| 現行version | 3 |
| 後方互換 | 試作段階のため旧Protocol v1/v2互換は持たない |
| sequence | `uint16`、stream単位、wrap可能 |
| timestamp | Device boot後、単位ms |

### 6.2 Sensor Data frame v3

Characteristic: Sensor Data Notification
サイズ: 16-34 bytes
Python header struct format: `<BBBBHIBB`

Sensor Dataは12 bytesの共通headerとstream-specific payloadで構成する。現行実装では `stream_id=1` の `DUMMY_ACCEL3` を常時サポートし、X-NUCLEO-IKS01A2上のLSM6DSLがFirmware起動時にreadyになった場合のみ `stream_id=10` の `IMU6` と `stream_id=13` の `ORIENTATION_MOTION`、HTS221がreadyになった場合のみ `stream_id=30` の `TEMP_HUMIDITY`、LPS22HBがreadyになった場合のみ `stream_id=20` の `PRESSURE`、LSM303AGR magnetometerがreadyになった場合のみ `stream_id=12` の `MAG3` をCapabilityへ含め、測定中に通知する。

Header:

| Offset | Size | 型 | 名称 | 説明 |
| ---: | ---: | --- | --- | --- |
| 0 | 1 | uint8 | version | `3` |
| 1 | 1 | uint8 | message_type | `1` = SENSOR_SAMPLE |
| 2 | 1 | uint8 | stream_id | `1` = DUMMY_ACCEL3、`10` = LSM6DSL IMU6、`12` = LSM303AGR MAG3、`13` = LSM6DSL ORIENTATION_MOTION、`20` = LPS22HB PRESSURE、`30` = HTS221 TEMP_HUMIDITY |
| 3 | 1 | uint8 | flags | 予約。現行では0 |
| 4 | 2 | uint16 | sequence | stream単位のサンプル連番 |
| 6 | 4 | uint32 | timestamp_ms | Device boot後ms |
| 10 | 1 | uint8 | payload_format | `1` = DUMMY_ACCEL3_INT16_V1、`2` = IMU6_INT16_V1、`3` = HTS221_TEMP_HUMIDITY_INT16_V1、`4` = LPS22HB_PRESSURE_INT32_V1、`5` = MAG3_INT16_V1、`6` = ORIENTATION_MOTION_INT16_V1 |
| 11 | 1 | uint8 | payload_len | payload byte数 |

DUMMY_ACCEL3_INT16_V1 payload:

| Offset | Size | 型 | 名称 | 説明 |
| ---: | ---: | --- | --- | --- |
| 12 | 2 | int16 | accel_x_mg | dummy accel X |
| 14 | 2 | int16 | accel_y_mg | dummy accel Y |
| 16 | 2 | int16 | accel_z_mg | dummy accel Z |

IMU6_INT16_V1 payload:

| Offset | Size | 型 | 名称 | 説明 |
| ---: | ---: | --- | --- | --- |
| 12 | 2 | int16 | accel_x_mg | LSM6DSL accel X |
| 14 | 2 | int16 | accel_y_mg | LSM6DSL accel Y |
| 16 | 2 | int16 | accel_z_mg | LSM6DSL accel Z |
| 18 | 2 | int16 | gyro_x_mdps | LSM6DSL gyro X |
| 20 | 2 | int16 | gyro_y_mdps | LSM6DSL gyro Y |
| 22 | 2 | int16 | gyro_z_mdps | LSM6DSL gyro Z |

HTS221_TEMP_HUMIDITY_INT16_V1 payload:

| Offset | Size | 型 | 名称 | 説明 |
| ---: | ---: | --- | --- | --- |
| 12 | 2 | int16 | humidity_centi_percent | HTS221 relative humidity、0.01 %RH単位 |
| 14 | 2 | int16 | temperature_centi_c | HTS221 ambient temperature、0.01 degC単位 |

LPS22HB_PRESSURE_INT32_V1 payload:

| Offset | Size | 型 | 名称 | 説明 |
| ---: | ---: | --- | --- | --- |
| 12 | 4 | int32 | pressure_pa | LPS22HB pressure、Pa単位 |

MAG3_INT16_V1 payload:

| Offset | Size | 型 | 名称 | 説明 |
| ---: | ---: | --- | --- | --- |
| 12 | 2 | int16 | mag_x_ut | LSM303AGR magnetometer X、uT単位 |
| 14 | 2 | int16 | mag_y_ut | LSM303AGR magnetometer Y、uT単位 |
| 16 | 2 | int16 | mag_z_ut | LSM303AGR magnetometer Z、uT単位 |

ORIENTATION_MOTION_INT16_V1 payload:

| Offset | Size | 型 | 名称 | 説明 |
| ---: | ---: | --- | --- | --- |
| 12 | 2 | int16 | pitch_naive_cdeg | accel由来pitch、0.01 degree単位 |
| 14 | 2 | int16 | roll_naive_cdeg | accel由来roll、0.01 degree単位 |
| 16 | 2 | int16 | zenith_naive_cdeg | accel由来zenith、0.01 degree単位 |
| 18 | 2 | int16 | pitch_complementary_cdeg | 相補フィルタpitch、0.01 degree単位 |
| 20 | 2 | int16 | roll_complementary_cdeg | 相補フィルタroll、0.01 degree単位 |
| 22 | 2 | int16 | zenith_complementary_cdeg | 相補フィルタzenith、0.01 degree単位 |
| 24 | 2 | int16 | pitch_mahony_cdeg | Mahony filter pitch、0.01 degree単位 |
| 26 | 2 | int16 | roll_mahony_cdeg | Mahony filter roll、0.01 degree単位 |
| 28 | 2 | int16 | zenith_mahony_cdeg | Mahony filter zenith、0.01 degree単位 |
| 30 | 2 | int16 | yaw_mahony_cdeg | Mahony filter yaw、0.01 degree単位。現行はmagnetometer補正なしのためdriftし得る |
| 32 | 2 | int16 | accel_norm_mg | 合成加速度、mg単位 |

### 6.3 Control payload

Characteristic: Control Write  
サイズ: 4 bytes  
Python struct format: `<BBH`

| Offset | Size | 型 | 名称 | 説明 |
| ---: | ---: | --- | --- | --- |
| 0 | 1 | uint8 | version | `3` |
| 1 | 1 | uint8 | command | command enum |
| 2 | 2 | uint16 | value | 予約。現行commandでは未使用 |

Commands:

| 値 | 名称 | 動作 |
| ---: | --- | --- |
| `0x01` | START_MEASUREMENT | FirmwareをMEASURINGへ遷移し、dummy accel samplingと、readyな場合はLSM6DSL IMU / HTS221 / LPS22HB / LSM303AGR magnetometer samplingを開始 |
| `0x02` | STOP_MEASUREMENT | FirmwareをIDLEへ遷移し、dummy accel / LSM6DSL / HTS221 / LPS22HB / LSM303AGR magnetometer samplingを停止 |
| `0x03` | REQUEST_STATUS | Status更新を要求。現行では状態publish後、PCがStatus Readする |
| `0x04` | RESET_SEQUENCE | dummy sensor、LSM6DSL stream、HTS221 stream、LPS22HB stream、LSM303AGR magnetometer streamのsequenceを0へ戻す |

### 6.4 Config payload

Characteristic: Config Read/Write  
サイズ: 8 bytes  
Python struct format: `<BBBBHH`

現行Config payloadは固定長binaryのv4 schemaであり、stream単位Configの最小実装として `SET_STREAM_INTERVAL` を扱う。2026-06-21時点ではFirmwareで実際にinterval変更できる対象は `stream_id=1` の `DUMMY_ACCEL3` である。その他のstreamはCapability上のmin/max intervalが同一の固定rate streamとして扱う。

| Offset | Size | 型 | 名称 | 説明 |
| ---: | ---: | --- | --- | --- |
| 0 | 1 | uint8 | version | `4` |
| 1 | 1 | uint8 | op | `0x01` = `SET_STREAM_INTERVAL` |
| 2 | 1 | uint8 | stream_id | 設定対象stream。現行Firmwareでは `1` のみvalid |
| 3 | 1 | uint8 | flags | 予約。現行では0 |
| 4 | 2 | uint16 | sample_interval_ms | 対象streamのsampling周期 |
| 6 | 2 | uint16 | reserved | `0` 固定 |

制約:

| 項目 | 値 |
| --- | --- |
| 最小interval | 20 ms |
| 最大interval | 10000 ms |
| 初期interval | 100 ms |
| `op` | `SET_STREAM_INTERVAL` 以外はinvalid |
| `stream_id` | 現行Firmwareでは `1` 以外はinvalid |
| `flags` | 0以外はinvalid |
| `reserved` | 0以外はinvalid |

### 6.5 Status payload

Characteristic: Status Read
サイズ: 16 bytes
Python struct format: `<BBHHHHHHH`

| Offset | Size | 型 | 名称 | 説明 |
| ---: | ---: | --- | --- | --- |
| 0 | 1 | uint8 | version | `3` |
| 1 | 1 | uint8 | state | device state enum |
| 2 | 2 | uint16 | sample_interval_ms | 現在のsample interval |
| 4 | 2 | uint16 | last_error | device error enum。通常のControl/Config/Notify error、またはoptional sensor errorの代表値 |
| 6 | 2 | uint16 | connection_count | 接続回数 |
| 8 | 2 | uint16 | lsm6dsl_error | LSM6DSL availability診断 |
| 10 | 2 | uint16 | hts221_error | HTS221 availability診断 |
| 12 | 2 | uint16 | lps22hb_error | LPS22HB availability診断 |
| 14 | 2 | uint16 | lsm303agr_magn_error | LSM303AGR magnetometer availability診断 |

Device states:

| 値 | 名称 |
| ---: | --- |
| `0` | IDLE |
| `1` | MEASURING |
| `2` | ERROR |

Device errors:

| 値 | 名称 | 主な発生条件 |
| ---: | --- | --- |
| `0` | NONE | エラーなし |
| `1` | INVALID_VERSION | Control version不正 |
| `2` | INVALID_LENGTH | Control payload長不正 |
| `3` | INVALID_COMMAND | Control command不正 |
| `4` | INVALID_CONFIG | Config不正 |
| `5` | NOT_CONNECTED | 接続なしでnotifyしようとした |
| `6` | NOT_SUBSCRIBED | Notification未購読でnotifyしようとした |
| `7` | LSM6DSL_NO_DEVICETREE | Firmware buildに `st,lsm6dsl` devicetree nodeが含まれていない |
| `8` | LSM6DSL_NOT_READY | LSM6DSL nodeはあるが、起動時にZephyr deviceがreadyでない |
| `9` | LSM6DSL_CONFIG_FAILED | LSM6DSLの26 Hz ODR設定に失敗した |
| `10` | HTS221_NO_DEVICETREE | Firmware buildに `st,hts221` devicetree nodeが含まれていない |
| `11` | HTS221_NOT_READY | HTS221 nodeはあるが、起動時にZephyr deviceがreadyでない |
| `12` | LPS22HB_NO_DEVICETREE | Firmware buildに `st,lps22hb-press` devicetree nodeが含まれていない |
| `13` | LPS22HB_NOT_READY | LPS22HB nodeはあるが、起動時にZephyr deviceがreadyでない |
| `14` | LSM303AGR_MAGN_NO_DEVICETREE | Firmware buildに `magn0` / `st,lis2mdl` devicetree nodeが含まれていない |
| `15` | LSM303AGR_MAGN_NOT_READY | LSM303AGR magnetometer nodeはあるが、起動時にZephyr deviceがreadyでない、またはODR設定に失敗した |

LSM6DSL/HTS221/LPS22HB/LSM303AGR magnetometer診断値は、optional streamがCapabilityに出ない理由をPC側から読むための最小診断である。診断値が出てもFirmware全体は起動を継続し、`DUMMY_ACCEL3` streamは利用できる。複数optional sensorが失敗した場合、`lsm6dsl_error`、`hts221_error`、`lps22hb_error`、`lsm303agr_magn_error` に各sensorの診断値を同時に返す。`last_error` は既存UI/CUI向けの代表値として、LSM6DSL、HTS221、LPS22HB、LSM303AGR magnetometerの順に最初の非 `NONE` optional sensor errorを入れる。

### 6.6 Capability payload

Characteristic: Capability Read
サイズ: `12 + 16 * stream_count` bytes
Python header struct format: `<BBHBBHHH`
Python stream struct format: `<BBBBBBHHHHbB`

CapabilityはGeneric Sensor Monitor化の最初の足場として追加したschema v1である。現時点では `DUMMY_ACCEL3_INT16_V1` を常時記述し、Firmware起動時にLSM6DSLがreadyになった場合のみ `IMU6_INT16_V1` と `ORIENTATION_MOTION_INT16_V1`、HTS221がreadyになった場合のみ `HTS221_TEMP_HUMIDITY_INT16_V1`、LPS22HBがreadyになった場合のみ `LPS22HB_PRESSURE_INT32_V1`、LSM303AGR magnetometerがreadyになった場合のみ `MAG3_INT16_V1` を追加streamとして記述する。

Firmware Capability schema v1は、stream単位のmetadataまでを表す固定長binary schemaとする。WebGUI/CSVで使うfield単位のlabel、unit、scale、decimal表現は、PC backendが `payload_format` から補完して `/api/capability` の `streams[].fields[]` に付与する。Firmware由来のfield descriptorが必要になった場合は、schema v1へ後付けせず、Capability schema version 2またはTLV化として別途設計する。

Header:

| Offset | Size | 型 | 名称 | 説明 |
| ---: | ---: | --- | --- | --- |
| 0 | 1 | uint8 | version | `3` |
| 1 | 1 | uint8 | schema_version | `1` |
| 2 | 2 | uint16 | capability_flags | bit0 custom GATT、bit1 fixed binary |
| 4 | 1 | uint8 | stream_count | `1` から `6` |
| 5 | 1 | uint8 | reserved | `0` |
| 6 | 2 | uint16 | supported_commands | command値をbit位置にしたbitmask |
| 8 | 2 | uint16 | supported_features | bit0 interval config、bit1 status read、bit2 sensor notify |
| 10 | 2 | uint16 | preferred_mtu | 現行ではSensor Data frame v3 max sizeの34 |

Stream descriptor共通形式:

| Offset | Size | 型 | 名称 | 説明 |
| ---: | ---: | --- | --- | --- |
| 12 | 1 | uint8 | stream_id | stream識別子 |
| 13 | 1 | uint8 | stream_type | `1` = DUMMY_ACCEL3、`2` = IMU6、`3` = TEMP_HUMIDITY、`4` = PRESSURE、`5` = MAG3、`6` = ORIENTATION_MOTION |
| 14 | 1 | uint8 | channel_count | stream内channel数 |
| 15 | 1 | uint8 | data_type | `1` = INT16、`2` = INT32 |
| 16 | 1 | uint8 | unit | `0` = MIXED、`1` = PA、`2` = UT、`3` = MG |
| 17 | 1 | uint8 | payload_format | payload format enum |
| 18 | 2 | uint16 | stream_flags | bit0 enabled by default、bit1 mixed units |
| 20 | 2 | uint16 | default_interval_ms | `100` |
| 22 | 2 | uint16 | min_interval_ms | `20` |
| 24 | 2 | uint16 | max_interval_ms | `10000` |
| 26 | 1 | int8 | scale_exponent | 現行では `0` |
| 27 | 1 | uint8 | reserved | `0` |

実装済みstream:

| stream_id | stream_type | channel_count | data_type | unit | payload_format | default/min/max interval |
| ---: | --- | ---: | --- | --- | --- | --- |
| 1 | `DUMMY_ACCEL3` | 3 | `INT16` | `MG` | `DUMMY_ACCEL3_INT16_V1` | 100 / 20 / 10000 ms |
| 10 | `IMU6` | 6 | `INT16` | `MIXED` | `IMU6_INT16_V1` | 38 / 38 / 38 ms |
| 13 | `ORIENTATION_MOTION` | 11 | `INT16` | `MIXED` | `ORIENTATION_MOTION_INT16_V1` | 38 / 38 / 38 ms |
| 30 | `TEMP_HUMIDITY` | 2 | `INT16` | `MIXED` | `HTS221_TEMP_HUMIDITY_INT16_V1` | 1000 / 1000 / 1000 ms |
| 20 | `PRESSURE` | 1 | `INT32` | `PA` | `LPS22HB_PRESSURE_INT32_V1` | 1000 / 1000 / 1000 ms |
| 12 | `MAG3` | 3 | `INT16` | `UT` | `MAG3_INT16_V1` | 100 / 100 / 100 ms |

## 7. PC backend HTTP API

Base URL: `http://127.0.0.1:8765`

| Method | Path | Request | Response概要 |
| --- | --- | --- | --- |
| GET | `/api/scan` | なし | `{ "devices": [...] }` |
| POST | `/api/connect` | `{ "target": "<name_or_address>" }` | `{ "ok": true, "target": "..." }` |
| POST | `/api/connect/cancel` | なし | `{ "ok": true, "cancelled": true/false }` |
| POST | `/api/disconnect` | なし | `{ "ok": true }` |
| POST | `/api/start` | なし | `{ "ok": true }` |
| POST | `/api/stop` | なし | `{ "ok": true }` |
| POST | `/api/reset-sequence` | なし | `{ "ok": true }` |
| POST | `/api/interval` | `{ "stream_id": 1, "interval_ms": 100 }` | `{ "ok": true, "stream_id": 1, "interval_ms": 100 }` |
| POST | `/api/orientation-filter` | `{ "complementary_alpha": 0.98, "mahony_kp": 0.5, "mahony_ki": 0.0 }` | `{ "ok": true, "complementary_alpha": 0.98, "mahony_kp": 0.5, "mahony_ki": 0.0 }` |
| GET | `/api/status` | なし | 接続状態、monitoring状態、Status、latest sample。Statusには `last_error` と `optional_sensors.{lsm6dsl,hts221,lps22hb,lsm303agr_magn}.error` を含む |
| GET | `/api/capability` | なし | 接続中DeviceのCapability metadata。WebGUI用にbackendが `payload_format` 由来の `fields` metadataを付与する |
| GET | `/ws` | WebSocket upgrade | state/sample event |
| GET | `/` | なし | WebGUI HTML |

## 8. WebSocket message

### 8.1 state message

```json
{
  "type": "state",
  "connected": true,
  "connecting": false,
  "monitoring": true,
  "missed_samples": 0
}
```

### 8.2 sample message

```json
{
  "type": "sample",
  "sample": {
    "version": 3,
    "message_type": 1,
    "stream_id": 1,
    "flags": 0,
    "sequence": 1,
    "timestamp_ms": 1234,
    "payload_format": 1,
    "payload_len": 6,
    "accel_x_mg": 0,
    "accel_y_mg": 70,
    "accel_z_mg": 930,
    "host_time_ms": 1781640000000,
    "missed_samples": 0
  }
}
```

## 9. UI仕様

### 9.1 interactive CUI

起動:

```bash
cd pc_app
uv run python -m ble_sensor_logger
```

Commands:

| Command | 動作 |
| --- | --- |
| `scan` | 対象Deviceをscan |
| `connect [name_or_address]` | 接続 |
| `disconnect` | 切断 |
| `monitor` | Sensor Data Notification購読開始 |
| `unmonitor` | Sensor Data Notification購読停止 |
| `start` | Measurement開始 |
| `stop` | Measurement停止 |
| `set-interval <ms>` | `stream_id=1` へのConfig v4 Write |
| `set-stream-interval <stream_id> <ms>` | 指定streamへのConfig v4 Write。現行Firmwareで変更可能なのは `stream_id=1` |
| `status` | Status Read |
| `capability` | Capability Read |
| `reset-seq` | sequence reset |
| `exit` / `quit` | 切断して終了 |

### 9.2 WebGUI

起動:

```bash
cd pc_app
uv run --extra dev \
  python -m ble_sensor_logger --web --host 127.0.0.1 --port 8765
```

提供機能:

- Scan
- Connect
- Connect cancel
- Disconnect
- Start / Stop
- Sequence reset
- Sampling interval変更
- Status表示。`last_error` は代表値、LSM6DSL/HTS221/LPS22HB/LSM303AGR Mag欄はsensor別errorを表示する
- Capability metadataに基づく最新値表示
- Capability metadataに基づくSignal選択とリアルタイムchart表示。最大3個のgraphを描画でき、各graphは最大3つのSignalを選択できる
- GraphごとにY軸rangeのAuto/manual指定、X軸rangeのAuto/任意秒数window指定
- `stream_id=13` のorientation field metadataがある場合の3D cuboid表示。Naive、相補フィルタ、Mahony filterの3方式を同時表示でき、各方式の表示/非表示を切り替えられる
- Orientation filter設定。GUIから相補フィルタ `alpha`、Mahony filter `K_p`、`K_i` を `/api/orientation-filter` 経由でFirmwareのConfig characteristicへWriteできる
- 最大5000点のchart履歴
- CSV記録開始/停止とdownload

CSV fields:

```text
host_time_iso,host_time_ms,version,message_type,stream_id,flags,sequence,
timestamp_ms,payload_format,payload_len,<stream-qualified Capability fields...>,
missed_samples
```

`<stream-qualified Capability fields...>` は `/api/capability` の `streams[].fields[]` をstream順に展開した列で、列名は `s<stream_id>_<field>` とする。現行fallbackでは `s1_accel_x_mg`、`s1_accel_y_mg`、`s1_accel_z_mg`、`s10_accel_x_mg`、`s10_accel_y_mg`、`s10_accel_z_mg`、`s10_gyro_x_mdps`、`s10_gyro_y_mdps`、`s10_gyro_z_mdps`、`s13_pitch_naive_cdeg`、`s13_roll_naive_cdeg`、`s13_zenith_naive_cdeg`、`s13_pitch_complementary_cdeg`、`s13_roll_complementary_cdeg`、`s13_zenith_complementary_cdeg`、`s13_pitch_mahony_cdeg`、`s13_roll_mahony_cdeg`、`s13_zenith_mahony_cdeg`、`s13_yaw_mahony_cdeg`、`s13_accel_norm_mg`、`s30_humidity_centi_percent`、`s30_temperature_centi_c`、`s20_pressure_pa`、`s12_mag_x_ut`、`s12_mag_y_ut`、`s12_mag_z_ut` を含む。sample行と一致しないstreamの値列は空欄にする。CSVは単一wide CSVとしてdownloadし、stream別CSV分割は現時点では行わない。

## 10. 受け入れ条件

| ID | 条件 |
| --- | --- |
| AC-01 | PCからDevice nameまたはService UUIDでscanできる |
| AC-02 | PCからconnect/disconnectできる |
| AC-03 | Sensor Data Notificationを購読し、Protocol v3 16-34 bytes frameをparseできる |
| AC-04 | 旧Protocol v1/v2 payloadを受け付けない |
| AC-05 | start/stopでDevice stateがMEASURING/IDLEへ遷移する |
| AC-06 | `stream_id=1` へ20-10000 msのsampling intervalをConfig v4 Writeできる |
| AC-07 | 不正Control/Configに対しATT errorまたはStatus last_errorで検出できる |
| AC-08 | sequence欠落をPC側で検出できる |
| AC-09 | WebGUIでリアルタイム値とchartを表示できる |
| AC-10 | CSVをdownloadできる |
| AC-11 | PC backendからCapabilityをReadし、最小stream metadataをparseできる |
| AC-12 | optional sensorがCapabilityに出ない場合、Statusのsensor別error fieldで同時に最小診断を取得できる |
| AC-13 | HTS221がreadyな場合、Capabilityに `stream_id=30` が出て、1 Hz前後のtemperature/humidity sampleをparseできる |
| AC-14 | LPS22HBがreadyな場合、Capabilityに `stream_id=20` が出て、1 Hz前後のpressure sampleをparseできる |
| AC-15 | LSM303AGR magnetometerがreadyな場合、Capabilityに `stream_id=12` が出て、10 Hz前後のmagnetometer sampleをparseできる |

## 11. 既知の制約

- Capability discoveryはschema v1としてstream metadataまで実装済みである。WebGUIの最新値カード、Signal選択、CSV列は `/api/capability` の `streams[].fields[]` から生成するが、このfield metadataはPC backendが `payload_format` から補完する。
- StatusはNotifyされないため、WebGUIは操作後にStatus Readする。
- Config v4の最小実装では `SET_STREAM_INTERVAL` のみ扱い、`SET_STREAM_ENABLE`、range/filter/calibration設定、Config Responseは未実装である。
- Firmware Capability payload内のfield単位unit/scale metadataは未実装である。現行WebGUI/CSV向けには、PC backend補完のfield metadataを正式仕様として使う。
- Fragmentationは未実装であり、現行Sensor Data frameの最大は34 bytesである。将来payloadが大きくなる場合はATT MTU拡張を前提とする。
- `connection_count` は接続回数として公開しているが、現行実装ではBLE層とcontrol層の状態同期を今後整理する余地がある。
- optional sensor診断はStatusのsensor別error fieldで同時に取得できる。ただしI2C addressやpin状態など、より詳細なruntime診断はまだ返さない。
- LSM303AGR accel `stream_id=11` は予約候補だが、現行runtime streamには含めない。追加判断は `docs/adr/0006-defer-lsm303agr-accel-stream.md` を参照する。
