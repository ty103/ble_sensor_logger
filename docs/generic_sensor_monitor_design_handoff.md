# Generic Sensor Monitor: 設計引き継ぎ資料

作成日: 2026-06-16  
対象プロジェクト: BLE Sensor Logger  
目的: 様々なデバイス/プロジェクトで再利用できる、開発用センサーモニター + ロガー + 制御のFirmware + PC backend + WebGUI雛形を設計する。

## 1. この資料の位置づけ

本資料は、現在のBLE Sensor Logger実装を、より汎用的な開発用センサーモニター基盤へ再設計するための設計書兼引き継ぎ資料である。

今後は本資料を「現在の設計判断、現在の残課題、設計論点、次回作業キュー」の正本として維持する。実機確認・検証の詳細ログは `docs/records/` へ分離し、本資料には現在判断に必要な要約とリンクだけを残す。

既存資料との関係:

| 資料 | 位置づけ |
| --- | --- |
| `docs/README.md` | docs全体の入口、資料一覧、運用方針 |
| `docs/development_process.md` | 開発プロセス、成果物、レビュー/merge条件 |
| `docs/specs/current_implementation_spec.md` | 現行実装の外部仕様 |
| `docs/design/system_design.md` | FW/WebAppを含む全体設計 |
| `docs/design/firmware_design.md` | Firmware内部設計 |
| `docs/design/pc_webapp_design.md` | PC backend/WebApp設計 |
| `docs/adr/` | 重要な設計判断の理由 |
| `docs/records/validation_history_2026-06.md` | 実機確認・検証履歴 |
| `docs/old/phase1_2_ble_spec_and_plan.md` | 初期実装の仕様・開発計画 |
| `docs/old/test_spec.md` | 現行実装のテスト仕様・実機試験結果 |
| 本資料 | 次フェーズの汎用化設計判断、現在の残課題、設計論点、作業順の正本 |

## 2. プロジェクトの目的

本プロジェクトの目的は、特定のnRF52840 DKサンプルではなく、様々なデバイス/プロジェクトで使える汎用的な開発用雛形を作ることである。

主な利用シーン:

- センサーデバイスの試作時に、PC WebGUIでリアルタイム波形を確認する。
- センサーデータをCSVなどで保存し、後から解析できるようにする。
- PC側から測定開始/停止、sampling rate変更、range変更、calibration、marker挿入などを行う。
- 9軸IMU 100 Hzのような比較的高頻度データと、温度センサ 1 Hzのような低頻度データを同じ仕組みで扱う。
- Firmware側のセンサ実装を差し替えても、PC backend/WebGUI側の基本構造を使い回せるようにする。

追加で想定する実センサ評価ボード:

- X-NUCLEO-IKS01A2
- 搭載センサ:
  - LSM303AGR: 加速度センサ、地磁気センサ
  - LSM6DSL: 加速度センサ、ジャイロセンサ
  - LPS22HB: 気圧センサ
  - HTS221: 湿度センサ、温度センサ
- 実装優先度は `LSM6DSL` の26 Hz 6軸(accel + gyro)を最優先とする。

## 3. 現在の実装概要

現在の実装は、DeviceをBLE Peripheral / GATT Server、PCをCentral / GATT Clientとする。

Firmware:

- Target: nRF52840 DK
- SDK: NCS v3.2.2
- RTOS: Zephyr
- Event framework: App Event Manager
- Sensor: dummy accel + optional X-NUCLEO-IKS01A2 streams
- BLE: 独自GATT Service

PC:

- Python backend
- `bleak`によるBLE接続
- `aiohttp`によるlocal WebGUI backend
- BrowserはBLEを直接扱わず、HTTP/WebSocketでPython backendと通信する

現行Characteristic:

| Characteristic | Properties | 用途 |
| --- | --- | --- |
| Sensor Data | Notify | DeviceからPCへセンササンプルを送信 |
| Control | Write / Write Without Response | PCからDeviceへ開始/停止/リセットなどを送信 |
| Config | Read / Write | sampling intervalなどの設定 |
| Status | Read | Device状態、エラー、接続回数など |
| Capability | Read | 最小stream metadata、対応command/feature |

現行payloadはLittle Endian固定長binaryで、Protocol v3である。Sensor Dataは12 bytes header + stream-specific payloadの16-40 bytes frameで、`stream_id`を持つ。

## 4. 合意済みの設計判断

詳細な判断理由は `docs/adr/` を正とし、本資料には現在の結論だけを残す。

| ADR | Status | 現在の結論 |
| --- | --- | --- |
| `docs/adr/0001-custom-gatt-primary-transport.md` | Accepted | 主軸transportは独自GATTにする。NUS/Serialは将来のoptional transportとして扱う |
| `docs/adr/0002-browser-through-python-backend.md` | Accepted | BrowserはBLEを直接扱わず、Python backend経由でHTTP/WebSocket通信する |
| `docs/adr/0003-capability-schema-v1-stream-descriptors-only.md` | Accepted | Capability schema v1はstream descriptorまでに留め、field metadataはPC backendが補完する |
| `docs/adr/0004-wide-csv-stream-qualified-columns.md` | Accepted | CSVは単一wide CSVとし、値列は `s<stream_id>_<field>` にする |
| `docs/adr/0005-stream-scoped-config-v4.md` | Proposed | stream単位Configは固定長binary v4を最小実装にする |
| `docs/adr/0006-defer-lsm303agr-accel-stream.md` | Accepted | LSM303AGR accel `stream_id=11` は用途が明確になるまで通常runtime streamへ追加しない |

## 5. 現行Characteristic設計のセルフレビュー

### 5.1 良い点

現行の4本構成は、最小構成として妥当である。

- `Sensor Data`をNotifyにしており、センササンプルのpushに合う。
- `Control`をWriteにしており、PCからの即時操作に合う。
- `Config`をRead/Writeにしており、設定値の確認と変更に合う。
- `Status`をReadにしており、状態取得とエラー確認に合う。
- BLE callbackから重い処理を避け、eventへ変換する方針はZephyr BLE実装として健全。
- 固定長binaryはC/Python間で扱いやすく、初期実装の実機検証に向いている。

### 5.2 汎用化に向けた不足点

現行設計は特定payloadに寄っており、汎用雛形としては次の点が不足している。

| 不足点 | 説明 |
| --- | --- |
| Sensor Data payloadが固定field | `accel_x/y/z`, `battery`, `adc`に固定され、9軸IMUや温度などを自然に表せない |
| Capability discoveryが最小schemaのみ | PC側がfield単位のsensor種別、単位、scale、最大rate、軸数をまだ十分に自動認識できない |
| stream単位の設定がない | IMUは100 Hz、温度は1 Hzのような個別設定が難しい |
| StatusがNotifyできない | エラー、計測状態変更、drop発生などをPCへpushしづらい |
| Log/Event channelがない | 開発用としてFirmware eventやwarningをWebGUIへ流せない |
| MTU/fragmentation方針が未整理 | 9軸100 Hzなどでpayloadが20 bytesを超える場合の扱いが未定 |

## 6. 再設計後のCharacteristic案

次フェーズでは、以下のCharacteristic構成を基本案とする。

| Characteristic | Properties | 方向 | 用途 |
| --- | --- | --- | --- |
| Sensor Data | Notify | Device -> PC | 高頻度センササンプル送信 |
| Control | Write / Write Without Response | PC -> Device | start/stop/reset/calibrate/markerなど |
| Config | Read / Write | PC <-> Device | streamごとのrate/range/enable/batching設定 |
| Status | Read / Notify | Device -> PC | 状態、エラー、drop数、active streams |
| Capability | Read | Device -> PC | 対応sensor/stream/unit/scale/rate/payload format |
| Log/Event | Notify | Device -> PC | Firmware log、warning、debug event |

最小実装では、`Sensor Data`、`Control`、`Config`、`Status`、`Capability`までを優先し、`Log/Event`はoptionalとしてよい。

2026-06-17時点で、現行Service UUIDのまま `Capability` Read Characteristic
`12345678-1234-5678-1234-56789abcdef5` を追加した。2026-06-18時点でProtocol v3へ進め、2026-06-21時点では常時有効な `DUMMY_ACCEL3` streamと、readyなoptional sensor streamをSensor Data frame v3で扱う。field単位metadataはPC backendが `payload_format` から補完する。

## 7. Application protocol方針

### 7.1 Characteristic分割とmessage typeの両立

独自GATTではCharacteristic自体に意味があるため、payload内の`message_type`は省略可能に見える。しかし、NUS/Serialなどのstream transportへ載せる将来を考えると、上位protocolはmessage typeを持つ方がよい。

方針:

- 独自GATTではCharacteristicで大分類する。
- payloadには`version`、`message_type`、`stream_id`などの共通headerを持たせる。
- NUS/Serialでは同じframeをそのまま流し、`message_type`で多重化する。

想定frame:

```text
frame {
  version
  message_type
  stream_id
  sequence
  timestamp
  flags
  payload_len
  payload
}
```

実装済みのSensor Data frame v3:

```text
header {
  uint8  version = 3
  uint8  message_type = SENSOR_SAMPLE
  uint8  stream_id
  uint8  flags
  uint16 sequence
  uint32 timestamp_ms
  uint8  payload_format
  uint8  payload_len
}
payload {
  stream-specific binary
}
```

現行payloadは、常時有効な `stream_id=1`、`payload_format=DUMMY_ACCEL3_INT16_V1`、`payload_len=6` のdummy accel3 sampleと、LSM6DSLがreadyな場合だけ有効な `stream_id=10`、`payload_format=IMU6_INT16_V1`、`payload_len=12` のIMU6 sample、同じくLSM6DSL由来の `stream_id=13`、`payload_format=ORIENTATION_MOTION_INT16_V2`、`payload_len=28` のorientation sample、HTS221がreadyな場合だけ有効な `stream_id=30`、`payload_format=HTS221_TEMP_HUMIDITY_INT16_V1`、`payload_len=4` のhumidity/temperature sample、LPS22HBがreadyな場合だけ有効な `stream_id=20`、`payload_format=LPS22HB_PRESSURE_INT32_V1`、`payload_len=4` のpressure sample、LSM303AGR magnetometerがreadyな場合だけ有効な `stream_id=12`、`payload_format=MAG3_INT16_V1`、`payload_len=6` のmagnetometer sampleである。試作段階のため、旧Sensor Data v1/v2 payloadや旧orientation v1 payloadとの後方互換は持たない。

### 7.2 stream_id

複数センサを扱うため、Sensor Dataには`stream_id`を持たせる。

例:

| stream_id | 種別 | rate | データ例 |
| ---: | --- | ---: | --- |
| 1 | Debug dummy accel | 10 Hz default | dummy accel x/y/z。Protocol上のstream typeは `DUMMY_ACCEL3` |
| 10 | LSM6DSL 6-axis IMU | 26 Hz | accel/gyro |
| 11 | LSM303AGR accel | TBD | accel |
| 12 | LSM303AGR magnetometer | 10 Hz | mag |
| 13 | LSM6DSL derived orientation | 26 Hz | pitch/roll/zenith + composite accel |
| 20 | LPS22HB pressure | 1 Hz候補 | pressure |
| 30 | HTS221 humidity/temperature | 1 Hz | humidity/temperature |

`stream_id`の意味、単位、scale、payload formatはCapabilityで通知する。

### 7.3 timestampとsequence

Sensor Dataには欠落検出と時系列復元のため、少なくとも以下を含める。

- `sequence`: stream単位の連番
- `timestamp`: Device boot後の時刻

検討事項:

- timestamp単位をmsにするかusにするか。
- 100 Hz以上を考えるとusまたはtick-based timestampも候補。
- sequence幅をuint16にするかuint32にするか。

現時点では、雛形としては`uint32 timestamp_us`または`uint32 timestamp_ms`、`uint16 sequence`を候補とする。長時間loggingや高rateではwrap処理をPC側で扱う。

## 8. センサーデータ設計の方向性

### 8.1 9軸100 Hz

9軸IMUの想定payload:

- accel x/y/z: int16
- gyro x/y/z: int16
- mag x/y/z: int16

データ部だけで18 bytes。共通headerを加えると、おおむね28-36 bytes程度になる。

100 Hzの場合、payload 36 bytesなら約3.6 KB/sであり、BLE Notifyで扱える範囲にある。ただし、default ATT MTU 23では1 notifyに収まらないため、以下のいずれかが必要になる。

- ATT MTU拡張を前提にする。
- payloadを小さくする。
- batchingする。
- fragmentation/reassemblyを設計する。

開発用雛形としては、まずMTU拡張を推奨し、fragmentationは必要になった時点で追加する方針が現実的。

### 8.2 1 Hz温度センサ

温度センサは低頻度・小payloadであり、帯域上の問題はない。

むしろ重要なのは、WebGUI/backendがCapabilityを見て自動的に表示単位、scale、グラフ軸、logging列を決められることである。

例:

- unit: degC
- scale: 0.01
- raw type: int16
- rate: 1 Hz

### 8.3 X-NUCLEO-IKS01A2 stream方針

X-NUCLEO-IKS01A2を実センサ評価ボードとして想定する。最初に実装する実センサstreamは `LSM6DSL` の26 Hz 6軸(accel + gyro)とする。

優先順:

| 優先 | センサ | stream案 | rate案 | payload案 | 備考 |
| ---: | --- | --- | ---: | --- | --- |
| 1 | LSM6DSL | 6-axis IMU | 26 Hz | accel x/y/z + gyro x/y/z = int16 x6 | 最初に実装する実センサ |
| 2 | HTS221 | Humidity/Temperature | 1 Hz | humidity + temperature | 低頻度環境stream |
| 3 | LPS22HB | Pressure | 1 Hz | pressure | 低頻度環境stream |
| 4 | LSM303AGR | Magnetometer | TBD | mag x/y/z | 方位/地磁気確認用 |
| 5 | LSM303AGR | Accel | TBD | accel x/y/z | LSM6DSL accelと重複するため後回し |

LSM6DSL 26 Hz 6軸payload案:

```text
payload_format = IMU6_INT16_V1
payload_len = 12
fields:
  int16 accel_x_mg
  int16 accel_y_mg
  int16 accel_z_mg
  int16 gyro_x_mdps
  int16 gyro_y_mdps
  int16 gyro_z_mdps
```

このpayloadはSensor Data frame v3 header 12 bytesと合わせて24 bytesとなる。現行firmwareはATT MTU拡張を前提にしているため、1 notifyに収まる。

実装時の方針:

- Zephyr sensor APIで扱える場合は標準driverを優先する。
- Firmware側はsensor driver adapter -> stream manager -> protocol encoder -> custom GATT transportへ分離する。
- 最初はLSM6DSL streamを固定rate 26 Hzで実装し、その後stream単位Configでenable/rate変更へ進める。
- CapabilityにはLSM6DSL streamのunit、scale、payload format、default/min/max rateを載せる。

### 8.4 LSM6DSL派生姿勢stream方針

ウェアラブルデバイスのsleep/sleep解除閾値を検討しやすくするため、LSM6DSLの加速度を基準に、ピッチ角、ロール角、ゼニス角、合成加速度をdevice側で計算し、Sensor Data frame v3の独立streamとしてPCへ送る。

暫定stream案:

```text
stream_id = 13
stream_type = ORIENTATION_MOTION
payload_format = ORIENTATION_MOTION_INT16_V2
rate = LSM6DSL IMU6 streamと同じ26 Hz
payload_len = 28
fields:
  int16 pitch_naive_cdeg
  int16 roll_naive_cdeg
  int16 zenith_naive_cdeg
  int16 pitch_iir_cdeg
  int16 roll_iir_cdeg
  int16 zenith_iir_cdeg
  int16 pitch_complementary_cdeg
  int16 roll_complementary_cdeg
  int16 zenith_complementary_cdeg
  int16 pitch_mahony_cdeg
  int16 roll_mahony_cdeg
  int16 zenith_mahony_cdeg
  int16 yaw_mahony_cdeg
  int16 accel_norm_mg
```

角度は0.01 degree単位のcenti-degreeとする。ピッチ/ロール/yawは原則 `-18000..18000`、ゼニスは `0..18000`、合成加速度はmg単位とし、範囲外はint16へclampする。このpayloadはSensor Data frame v3 header 12 bytesと合わせて40 bytesとなる。現行firmwareはATT MTU拡張を前提にしているため、1 notifyに収まる。

軸定義はKConfigで明示する。最低限、device正面方向と、基準姿勢で重力方向が加速度センサのどの軸に対応するかを `+X/-X/+Y/-Y/+Z/-Z` から選ぶ。

```text
CONFIG_BSL_ORIENTATION_FRONT_AXIS=+X|-X|+Y|-Y|+Z|-Z
CONFIG_BSL_ORIENTATION_GRAVITY_AXIS=+X|-X|+Y|-Y|+Z|-Z
```

実装時は2つの軸が同じ物理軸を指す設定をbuild時に拒否し、残りの横方向軸は右手系で導出する。角度計算では、sensor raw accelをこのdevice座標系へ射影してから、pitch、roll、zenithを計算する。これにより、board実装ごとのセンサ取り付け向きをPC側へ漏らさず、firmware設定だけで姿勢定義を固定できる。

計算方針:

- 合成加速度は `sqrt(ax^2 + ay^2 + az^2)` をmgで計算する。
- naive pitch/roll/zenithは、加速度ベクトルを重力方向とみなして三角関数で計算する。
- IIR pitch/roll/zenithはgyroを使わず、device座標系へ射影した加速度から計算したnaive pitch/roll/zenithを一次IIR filterへ入力して計算する。cutoff frequencyはConfig v4 op `SET_IIR_CUTOFF_MILLIHZ` でGUIから変更できる。
- complementary pitch/rollはLSM6DSL gyro積分とaccel由来姿勢の相補フィルタで計算する。`alpha` はConfig v4 op `SET_COMPLEMENTARY_ALPHA` でGUIから変更できる。
- Mahony pitch/roll/zenith/yawはLSM6DSL accel + gyroを入力にしたMahony filterで計算する。現行yawはmagnetometer補正なしのためdriftし得る。`K_p` / `K_i` はConfig v4 op `SET_MAHONY_KP` / `SET_MAHONY_KI` でGUIから変更できる。
- complementary zenithとMahony zenithは、それぞれのpitch/rollからdevice重力軸と鉛直方向のなす角として再計算する。
- measurement start/stopとsequence resetはLSM6DSL streamと同期させる。LSM6DSLがunavailableの場合、姿勢streamもCapabilityから除外する。

PC/WebGUI方針:

- `pc_app` は `ORIENTATION_MOTION_INT16_V2` をparseし、`/api/capability` のbackend補完field metadataへ角度と合成加速度のlabel/unit/scale/decimalsを追加する。Capability schema v1へfield descriptorは追加しない。
- 最新値カード、graph selector、CSV列は既存のCapability-driven経路に乗せる。CSV列名は `s13_pitch_naive_cdeg` のように `s<stream_id>_<field>` とする。
- WebGUIには姿勢専用の3D cuboid viewを追加する。Naive、IIR、相補フィルタ、Mahony filterの4方式を同時表示でき、方式ごとの表示/非表示を切り替えられる。各filter parameter変更はOrientationエリア内で行う。3D描画はThree.jsを使い、runtimeで外部CDNに依存しない形で導入する。

### 8.5 payload表現

候補:

| 方式 | 長所 | 短所 |
| --- | --- | --- |
| 固定長binary struct | 高速、小さい、C/Pythonで扱いやすい | streamごとにformat定義が必要 |
| TLV | 拡張しやすい、未知fieldをskip可能 | overheadが増える |
| CBOR | 汎用性が高い、MCUmgrとの親和性あり | 実装/デバッグがやや重い |
| Protocol Buffers/nanopb | schema管理しやすい | BLE小payloadではoverhead/生成物管理が増える |

現時点の推奨:

- Sensor Dataは効率重視で固定長binaryまたはstream-specific binary。
- Capability/Config/Statusは拡張性重視でTLVまたは小さなbinary schema。
- CBORはMCUmgr連携や外部tool互換を重視する場合に再検討する。

## 9. Capability設計の重要性

汎用雛形として最も重要なのはCapabilityである。

Capabilityがあることで、PC backend/WebGUIはFirmware固有のセンサ構成を事前にhardcodeしなくて済む。

現行Capability schema v1は固定長binaryで、Firmwareはstream descriptorまでを返す。field label、unit、scale、decimalsなどのWebGUI/CSV用metadataはPC backendが `payload_format` から補完する。

この判断の理由は `docs/adr/0003-capability-schema-v1-stream-descriptors-only.md` に残す。現行payloadとstream一覧の詳細は `docs/specs/current_implementation_spec.md` を正とする。

## 10. Config設計方針

Configはdevice全体ではなく、stream単位に拡張する。

必要な操作:

- stream enable/disable
- sampling rate変更
- notify rate変更
- batching設定
- sensor range変更
- filter設定
- calibration parameter設定

現行Configは、Config v3を破壊的に置き換えた固定長binaryのConfig v4としてstream単位設定を扱う。詳細な判断理由は `docs/adr/0005-stream-scoped-config-v4.md` に残す。

初期案:

```text
ConfigStreamPayloadV4 {
  uint8  version = 4
  uint8  op
  uint8  stream_id
  uint8  flags
  uint16 sample_interval_ms
  uint16 reserved
}
```

実装済み `op` は `SET_STREAM_INTERVAL`、`SET_COMPLEMENTARY_ALPHA`、`SET_MAHONY_KP`、`SET_MAHONY_KI`、`SET_IIR_CUTOFF_MILLIHZ` である。Characteristic UUIDは維持し、payload version `4` でstream-scoped Configとして扱う。`SET_STREAM_INTERVAL` のFirmware変更対象は `stream_id=1` の `DUMMY_ACCEL3` intervalに限定し、実センサstreamは固定rateのまま扱う。Orientation filter設定では既存8 bytes payloadの `sample_interval_ms` fieldをop-specific valueとして使い、alphaはpermille、Mahony gainはmilli gain、IIR cutoffはmilli-Hzで送る。

`SET_STREAM_ENABLE`、実センサstreamのrate変更、TLV形式、Config Request/Response、複数項目一括更新は、Config v4最小実装とorientation filter設定の実機確認後の次段候補として再検討する。

未決事項:

- Write後の結果をATT errorだけで表すか、Status/Eventでも返すか。

## 11. Control設計方針

Controlは即時性のある操作に使う。

想定command:

- start measurement
- stop measurement
- reset sequence
- request status
- request capability
- calibrate
- set marker
- clear error
- reboot

Controlは`Write With Response`を基本とし、低遅延操作や連続markerなどでは`Write Without Response`も許可する。

制御結果は以下のいずれかで返す。

- ATT error
- Status Notify
- Log/Event Notify
- Control Response message

初期実装ではATT error + Status更新でよいが、汎用雛形としてはControl Response messageの導入も検討する。

## 12. Status設計方針

StatusはReadだけでなくNotifyも持つ。

Statusに含めたい情報:

- device state
- active streams
- last error
- dropped sample count
- connection count
- current MTU
- battery state
- buffer usage
- logging/measurement state

Status Notifyの用途:

- start/stop完了通知
- error発生通知
- drop検出通知
- config適用完了通知
- connection parameter更新通知

## 13. Log/Event設計方針

開発用雛形としては、Firmware内部eventをWebGUIに流せる価値が高い。

用途:

- sensor driver warning
- BLE reconnect
- config validation error
- buffer overflow
- calibration result
- debug marker

Log/Eventは高頻度センサデータとは分ける。Sensor Dataの帯域を圧迫しないよう、levelやrate limitを持つ。

## 14. PC backend/WebGUI方針

### 14.1 backend

backendはBLE transportとApplication protocolを分離する。

```text
ble_client/custom_gatt
  -> transport adapter
  -> protocol parser
  -> app core
  -> web api / logger / CUI
```

NUS対応時は、`transport adapter`だけを差し替える。

### 14.2 WebGUI

WebGUIはCapabilityを読んで、以下を自動生成できることを目指す。

- stream一覧
- enable/disable control
- rate設定
- 最新値表示
- グラフ系列
- CSV列
- unit表示

初期WebGUIは完全自動生成でなくてもよいが、Capability由来のstream metadataを中心に設計する。

### 14.3 Logger

CSV loggerは単一wide CSVとしてdownloadする。値列は `/api/capability` のfield metadataから生成し、列名は `s<stream_id>_<field>` とする。sample行と一致しないstreamの値列は空欄にする。

この判断の理由は `docs/adr/0004-wide-csv-stream-qualified-columns.md` に残す。現行CSV fieldsの詳細は `docs/specs/current_implementation_spec.md` を正とする。

## 15. Firmwareアーキテクチャ方針

Firmware側は、センサ取得、protocol組み立て、transport送信を分離する。

```text
sensor drivers/adapters
  -> stream manager
  -> protocol encoder
  -> transport custom_gatt

control/config/status
  <- protocol decoder
  <- transport custom_gatt
```

Zephyr標準APIを優先する。

- A0 ADC: 旧mixed payloadからは外した。必要になった時点で独立したADC/board analog streamとして復活を検討する
- 実センサ: Zephyr sensor APIまたは薄いdriver adapter
- 内部pub/sub: App Event Manager継続、または将来zbus検討
- Sensing Subsystem: 実センサ抽象化が複雑になった段階で検討

## 16. 互換性方針

現行Protocolはv3である。プロジェクトは試作段階のため、旧Protocol v1/v2との後方互換は捨ててよい方針とする。

方針:

- Service UUIDは現行のまま維持し、payload versionで破壊的変更を扱う。
- PC backendは旧Sensor Data v1/v2をparseしない。
- Capabilityは互換modeではなく、現在のstream/payload formatを表すmetadataとして使う。
- 汎用雛形として成熟した段階で、組織管理の128-bit UUIDや正式なversioning policyを再検討する。

## 17. git運用方針

再設計フェーズでは、git履歴とhandoff資料を連動させる。

基本方針:

- `master`を常に動く・参照可能な正本branchとして扱う。
- 作業branchはチャット単位ではなく作業テーマ単位で作る。
- 作業branch名は原則として `codex/<theme>` とする。
- チャット末尾では、必要に応じて本資料を更新する。
- `master`へmergeする条件に、本資料が最新であることを含める。
- 未完成・実験中の作業は作業branchに残し、本資料に未mergeであることと次回作業を記録する。
- チャットが終わっただけでは、未完成の変更を無理に`master`へmergeしない。
- 作業開始時は、可能であれば作業テーマが分かる短いチャット名を設定する。変更できない環境では、開始メッセージで推奨チャット名を宣言する。
- 本資料に基づいて作業を開始する場合は、実装前に今回のcommit予定境界と、`master`へのmerge予定条件を宣言する。

想定branch:

```text
master
codex/design-generic-protocol
codex/feature-capability-char
codex/feature-stream-frame
codex/webgui-capability-driven
```

作業完了の目安:

- 仕様または実装の目的が明確に完了している。
- 必要な検証が済んでいる、または未検証理由が明記されている。
- 本資料へ設計判断、現在の残課題、次回作業が反映されている。
- 作業開始時に宣言したcommit予定境界に到達している、または到達できなかった理由と次回作業が本資料に残っている。

## 18. 開発プロセスと設計書整備方針

仕様定義、設計、実装、実機検証、ドキュメント更新、git操作の標準フローは `docs/development_process.md` に定義する。

開発プロセスの基本方針:

- ユーザー要求をもとに対象範囲、非対象範囲、受け入れ条件を整理する。
- 変更の大きさに応じて仕様書・設計書を更新し、必要なレビュー/承認を置く。
- 実装後はセルフコードレビュー、自動テスト、Firmware build、可能な範囲の実機テストを行う。
- ユーザー実機確認で見つかった不具合は、仕様漏れ、設計漏れ、実装不具合、環境依存に分類して修正する。
- フェーズ終了時は、ドキュメント更新漏れがないことを確認してからcommit/mergeする。
- PC側の依存管理とテスト実行は `pip` ではなく `uv` を使う。

現行実装の仕様・設計書は以下へ分離した。これにより、本資料は汎用化の判断、現在の残課題、設計論点に集中し、作業ごとの不要なコンテキスト混入を避ける。

| 資料 | パス | 目的 |
| --- | --- | --- |
| 現行実装仕様書 | `docs/specs/current_implementation_spec.md` | 現行BLE/HTTP/WebSocket/UI仕様を定義する |
| 全体設計書 | `docs/design/system_design.md` | Firmware、PC backend、WebGUI/CUI、BLE/HTTP/WebSocket境界、状態遷移、sequenceを示す |
| Firmware設計書 | `docs/design/firmware_design.md` | Firmware module、event、状態、BLE serviceを示す |
| PC backend/WebApp設計書 | `docs/design/pc_webapp_design.md` | Python backend、CUI、WebGUI、CSV、WebSocketを示す |

## 19. 現在の残課題 / 次回作業キュー

この章は、次回作業で最初に確認する現在の作業キューである。以降の実機確認履歴にある `次作業` は、その時点の履歴メモとして扱い、現在の優先順位はこの章を正とする。

### 優先度高

1. LSM6DSL派生姿勢stream `stream_id=13` のIIR拡張とGUI filter設定集約を完了済みとする。
   - 2026-06-23 `codex/mahony-orientation-stream` で、Firmware/PC/WebGUI/docsを更新し、実機build/flash/BLE smoke/実ブラウザ確認まで完了。
   - 2026-06-23 `codex/orientation-iir-filter` で、IIR pitch/roll/zenithを追加し、`ORIENTATION_MOTION_INT16_V2` payloadへ更新。PC自動テスト、Web frontend構文確認、Firmware shield build、flash、BLE smokeまで完了。実ブラウザ確認は未実施。
   - Naive、IIR、相補フィルタ、Mahony filterの4方式を同一 `ORIENTATION_MOTION_INT16_V2` payloadで送る。Mahony yawはmagnetometer補正なしのためdriftし得る。
   - GUIからConfig v4でIIR cutoff frequency、相補フィルタ `alpha`、Mahony `K_p` / `K_i` をOrientationエリア内で設定変更できる。
   - 並行開発前提の進め方:
     - [x] まずこのhandoffの `stream_id=13` interface lockを正とし、docs-onlyの準備commitを共通baseへ入れる。その後、Firmware / PC backend / WebGUIの各チャットは同じbase commitから作業branchまたはworktreeを作る。
     - [x] 推奨branch名は `codex/stream13-firmware`、`codex/stream13-pc-backend`、`codex/stream13-webgui`、統合用は `codex/stream13-orientation-integration` とする。実装branch同士を直接mergeせず、統合branchへ順に取り込む。
     - [x] interface lockを変更したくなった場合は、個別branch内で独自判断せず、先にhandoffを更新して3タスクへ周知する。特に `stream_id`、`stream_type`、`payload_format`、payload byte order、field名、scale、CSV列名、Capability stream descriptorは固定契約として扱う。
     - [x] 取り込み順は、PC backend -> Firmware -> WebGUIを基本とする。PC backendはsynthetic frame/Capabilityで先にparserとmetadataを固め、Firmware取り込み後にBLE smokeを行い、最後にWebGUIを実backend + 実ブラウザで確認する。
     - [x] 最終的な `master` mergeは、統合branchでPC自動テスト、Firmware shield build、flash、BLE smoke、WebGUI実ブラウザ確認が済み、最後にユーザー実機確認が完了してから行う。
   - `stream_id=13` interface lock:
     - `stream_id`: `13`
     - `stream_type`: `ORIENTATION_MOTION` / enum value `6`
     - `payload_format`: `ORIENTATION_MOTION_INT16_V2` / enum value `7`
     - `data_type`: `INT16`
     - `unit`: `MIXED`
     - `channel_count`: `14`
     - `payload_len`: `28`
     - `rate/default_interval`: LSM6DSL IMU6と同じ26 Hz / `38 ms`
     - `Capability`: LSM6DSLがreadyな場合のみ `stream_id=10` と一緒に含める。`BSL_CAPABILITY_MAX_STREAMS` は6、`preferred_mtu` は少なくともSensor Data header 12 bytes + payload 28 bytes = 40 bytesを表す値へ更新する。
     - `Sensor Data`: Little Endian、Sensor Data frame v3 headerは既存通り。payloadは次の順序で固定する。

       ```text
       int16 pitch_naive_cdeg
       int16 roll_naive_cdeg
       int16 zenith_naive_cdeg
       int16 pitch_iir_cdeg
       int16 roll_iir_cdeg
       int16 zenith_iir_cdeg
       int16 pitch_complementary_cdeg
       int16 roll_complementary_cdeg
       int16 zenith_complementary_cdeg
       int16 pitch_mahony_cdeg
       int16 roll_mahony_cdeg
       int16 zenith_mahony_cdeg
       int16 yaw_mahony_cdeg
       int16 accel_norm_mg
       ```

     - field名とCSV列名:

       | field | 表示単位 | scale | CSV列 |
       | --- | --- | ---: | --- |
       | `pitch_naive_cdeg` | degree | 0.01 | `s13_pitch_naive_cdeg` |
       | `roll_naive_cdeg` | degree | 0.01 | `s13_roll_naive_cdeg` |
       | `zenith_naive_cdeg` | degree | 0.01 | `s13_zenith_naive_cdeg` |
       | `pitch_iir_cdeg` | degree | 0.01 | `s13_pitch_iir_cdeg` |
       | `roll_iir_cdeg` | degree | 0.01 | `s13_roll_iir_cdeg` |
       | `zenith_iir_cdeg` | degree | 0.01 | `s13_zenith_iir_cdeg` |
       | `pitch_complementary_cdeg` | degree | 0.01 | `s13_pitch_complementary_cdeg` |
       | `roll_complementary_cdeg` | degree | 0.01 | `s13_roll_complementary_cdeg` |
       | `zenith_complementary_cdeg` | degree | 0.01 | `s13_zenith_complementary_cdeg` |
       | `pitch_mahony_cdeg` | degree | 0.01 | `s13_pitch_mahony_cdeg` |
       | `roll_mahony_cdeg` | degree | 0.01 | `s13_roll_mahony_cdeg` |
       | `zenith_mahony_cdeg` | degree | 0.01 | `s13_zenith_mahony_cdeg` |
       | `yaw_mahony_cdeg` | degree | 0.01 | `s13_yaw_mahony_cdeg` |
       | `accel_norm_mg` | mg | 1 | `s13_accel_norm_mg` |

   - 並行作業の担当境界:
     - Firmware taskは `firmware/` 配下を主に編集し、PC/WebGUIのコードは触らない。必要なinterface変更は上記interface lockに合っているかだけ確認する。
     - PC backend taskは `pc_app/src/ble_sensor_logger/protocol.py`、`web_api.py`、`ui_shell.py`、BLE smoke script、Python testsを主に編集する。`pc_app/web_frontend/` は触らず、WebGUI用には `/api/capability` のfield metadataとWebSocket sample JSONが正しく出るところまでを責務にする。
     - WebGUI taskは `pc_app/web_frontend/` を主に編集し、Python backendのprotocol/parserは触らない。開発中はfallback Capabilityまたはfixture sampleで3D表示を確認し、統合後に実backendへ接続して確認する。
     - `pc_app/tests/test_web_api.py` はPC backendとWebGUIの両方が触りやすいので、PC backend側はAPI JSON/field metadata、WebGUI側はstatic asset/HTML/JS存在確認に範囲を限定する。重複変更が出た場合は統合branchで解消する。
   - Firmware task:
     - [x] `firmware/src/protocol/protocol.h` / `protocol.c` に `BSL_STREAM_ID_LSM6DSL_ORIENTATION_MOTION=13`、`BSL_STREAM_TYPE_ORIENTATION_MOTION`、`BSL_PAYLOAD_FORMAT_ORIENTATION_MOTION_INT16_V2`、28 bytes payload、`BSL_SENSOR_DATA_MAX_PAYLOAD_SIZE` 更新、`BSL_CAPABILITY_MAX_STREAMS=6` を追加する。
     - [x] LSM6DSL sampling pathでIMU6 sampleと同じfetch結果からorientation sampleを生成し、measurement start/stop、sequence reset、availabilityをLSM6DSL streamと同期する。
     - [x] KConfigで `CONFIG_BSL_ORIENTATION_FRONT_AXIS` と `CONFIG_BSL_ORIENTATION_GRAVITY_AXIS` を追加し、`+X/-X/+Y/-Y/+Z/-Z` から選べるようにする。同じ物理軸の組み合わせはbuild時またはinit時に拒否する。
     - [x] naive pitch/roll/zenith、IIR pitch/roll/zenith、complementary pitch/roll/zenith、Mahony pitch/roll/zenith/yaw、`accel_norm_mg` を固定小数点で計算し、int16範囲へclampする。
     - [x] build/flash/RTTの初動では `firmware/docs/ai_quickstart.md` と `firmware/docs/runbooks/` を参照し、ホーム配下の個人Skillに依存しない。
     - 2026-06-23 `codex/mahony-orientation-stream`: Firmware側Mahony拡張とConfig setterを追加し、shield build / flash / BLE smokeで確認済み。
   - PC backend task:
     - [x] `pc_app/src/ble_sensor_logger/protocol.py` にpayload format、parse/pack、validation、default Capabilityを追加する。
     - [x] `pc_app/src/ble_sensor_logger/web_api.py` にfield metadataを追加し、角度はdegree表示、合成加速度はmg表示にscaleする。
     - [x] CUI表示、BLE smoke、protocol/web/app_core pytestを更新する。negative smokeはmalformed write中心のため変更不要。
     - [x] 2026-06-23: IIR拡張後のsynthetic frame/Capability/APIによるPC自動テストを実施し、`uv run --extra dev pytest` が35件pass。Firmware build、flash、BLE smokeまで完了。
   - WebGUI task:
     - [x] fallback Capability、最新値カード、graph selector、CSV列へorientation fieldsを追加する。
     - [x] Naive、IIR、相補フィルタ、Mahony filterの4つの直方体を同時表示できる3D cuboid viewを追加する。3D描画はThree.jsを使い、外部CDNなしで動くようにする。
     - [x] 3D viewは未接続/未到着時、4方式表示時、方式別表示/非表示切替時の表示状態を持つ。
     - [x] filter parameter変更UIをOrientationエリアへ集約し、生データのLive表示を折りたたみ可能にする。
     - [x] 統合後、実backendからの `/api/capability` と WebSocket sampleで `stream_id=13` の最新値、graph、CSV列、3D cuboidが更新されることを実ブラウザで確認する。
   - 検証task:
     - [x] `pc_app/` で `uv run --extra dev pytest` を実行する。IIR拡張後に35件pass。
     - [x] Web frontendの構文確認を行う。`node --check pc_app/web_frontend/app.js` 成功。
     - [x] Firmware shield buildを行う。`build/orientation-iir` で成功。
     - [x] nRF52840 DK + X-NUCLEO-IKS01A2へflashし、Capability Readで `streams=6`、`stream_id=13`、`ORIENTATION_MOTION_INT16_V2`、`channels=14` を確認する。
     - [x] BLE smokeで `stream_id=10` と `stream_id=13` のsampleが同じ測定中に流れること、`pitch_iir_cdeg` / `roll_iir_cdeg` / `zenith_iir_cdeg` がnotify payloadに含まれることを確認する。
     - [ ] WebGUIを実ブラウザで確認し、数値表示、graph、4方式3D cuboid、Orientation内filter設定、Raw live折りたたみが動くことをPlaywright screenshotまたは同等の方法で確認する。
     - 2026-06-23 `codex/orientation-iir-filter`でPC自動テスト35件、Web frontend構文確認、Firmware shield build、flash、BLE smokeまで完了。実ブラウザ確認は未実施。
     - 2026-06-23 `codex/mahony-orientation-stream`でPC自動テスト35件、Web frontend構文確認、Firmware shield build、flash、BLE smoke、実ブラウザ確認まで完了。`firmware/.env` はこのworktreeに無かったため、`firmware/.env.template` をsourceした。

2. Config v4の次段、Status Notify、Log/Eventの優先順位を再評価する。
   - Config v4次段候補: `SET_STREAM_ENABLE`、実センサstreamのrate変更、Config Response。
   - Status Notify候補: start/stop/config/errorのpush通知。
   - Log/Event候補: optional sensor詳細診断、I2C scan結果、Firmware warning。
   - 次回作業では、実装前にどれを最初に入れるかをADRまたはhandoffへ短く固定する。

### 優先度中

1. 実センサstreamのrate変更設計を必要になった時点で行う。
   - LSM6DSL/HTS221/LPS22HB/LSM303AGR magnetometerのZephyr driver ODR再設定可否を確認する。
   - measuring中に再設定するか、stop中だけ許可するかを決める。

### 判断済み

- Capability schema v1へfield descriptorは入れない。詳細は `docs/adr/0003-capability-schema-v1-stream-descriptors-only.md`。
- CSV loggerは単一wide CSV + `s<stream_id>_<field>` 列名とする。詳細は `docs/adr/0004-wide-csv-stream-qualified-columns.md`。
- stream単位Configは固定長binary v4を最小実装とする。`SET_STREAM_INTERVAL` は `stream_id=1` で実装・実機確認済み。詳細は `docs/adr/0005-stream-scoped-config-v4.md`。
- LSM303AGR accel `stream_id=11` は用途が明確になるまで通常runtime streamへ追加しない。詳細は `docs/adr/0006-defer-lsm303agr-accel-stream.md`。

### 後回し

2. NUS/Serial transport adapterを設計する。
   - 理由: 主軸は独自GATTだが、transport非依存なApplication protocolへ育てる余地を残す。
   - 方針: すぐ実装せず、custom GATTが安定してから必要性を再評価する。

3. App Event Manager継続かZephyr zbus移行かを判断する。
   - 理由: 現行実装ではApp Event Managerで足りている。汎用雛形化でmodule間契約が増えた時点で再評価する。

## 20. 設計論点バックログ

次チャット以降で詰めるべき論点:

1. 新Service/Characteristic UUID体系
2. Capability payload形式の拡張方針（現行schema v1はstream descriptorまで。field metadataがFirmware由来で必要になった時点でschema v2/TLV化を検討する）
3. Sensor Data frame形式の拡張方針（現行v3 headerからbatching/fragmentationへ進めるか）
4. stream_idとpayload format idの管理方法。現行は `stream_id=1`, `DUMMY_ACCEL3_INT16_V1`, `stream_id=10`, `IMU6_INT16_V1`, `stream_id=13`, `ORIENTATION_MOTION_INT16_V2`, `stream_id=30`, `HTS221_TEMP_HUMIDITY_INT16_V1`, `stream_id=20`, `LPS22HB_PRESSURE_INT32_V1`, `stream_id=12`, `MAG3_INT16_V1`。
5. optional sensor未ready時の詳細診断方法。sensor別の最小診断はStatusに実装済みで、今後はI2C scan結果、address候補などをLog/Eventへ出すか検討する
6. MTU拡張の扱い。現行v3 frameは最大40 bytesである。将来の大きなpayloadではATT MTU拡張を前提にする
7. fragmentation/reassemblyを初期から入れるか
8. Config v4を `SET_STREAM_ENABLE`、実センサrate変更、TLV、Config Request/Responseのどれへ拡張するか
9. Status Notifyのpayload
10. Log/Eventを初期実装に含めるか
11. NUS transport adapterのframe boundary方式
12. CSV loggerの長時間logging対応。複数stream表現は `docs/adr/0004-wide-csv-stream-qualified-columns.md` で判断済み。今後はbackend保存、streaming download、stream別CSV分割の必要性が出た時点で再検討する
13. WebGUIの自動UI生成範囲。現行は最新値カード、Signal選択、CSV列、stream interval一覧を `/api/capability` から生成する。Config変更UIは `min_interval_ms != max_interval_ms` のstreamだけを対象にし、固定rate streamは表示のみとする
14. App Event Manager継続か、Zephyr zbus検討か

## 21. フェーズ別ロードマップ

1. Characteristic構成を確定する。`Capability` Readは最小実装済み。
2. Capabilityの最小schemaを決める。固定長binary schema version `1` として実装済み。
3. Sensor Data frame headerを決める。Protocol v3として実装済み。
4. LSM6DSL 26 Hz 6軸を最初の代表streamとして実装し、TWIM切り替え後に実機で `stream_id=10` の実データを確認済み。
5. HTS221 humidity/temperature 1 Hzを低頻度環境streamとして実装し、実機で `stream_id=30` の実データを確認済み。
6. LPS22HB pressure 1 Hzを低頻度環境streamとして実装し、実機で `stream_id=20` の実データを確認済み。
7. LSM303AGR magnetometer 10 Hzを地磁気streamとして実装し、実機で `stream_id=12` の実データを確認済み。
8. Configはstream単位の固定長binary v4を最小実装として導入済み。`stream_id=1` の `SET_STREAM_INTERVAL` をFirmware/PC/WebGUIへ実装し、実機で100 msから500 msへのinterval変更を確認済み。
9. `stream_id=1` は `DUMMY_ACCEL3` / `DUMMY_ACCEL3_INT16_V1` へ整理済み。dummy batteryとA0 ADCはactive pathから外し、A0 ADCは必要になった時点で独立streamとして復活を検討する。
10. LSM6DSL派生姿勢streamとして `stream_id=13` を追加し、naive/IIR/complementary/Mahonyのpitch/roll/zenith、Mahony yaw、合成加速度をFirmwareで計算してWebGUI/CSV/3D表示へ流す。GUIからIIR cutoff、相補フィルタalpha、Mahony `K_p` / `K_i` をConfig v4で変更できる。
11. `docs/specs/current_implementation_spec.md` と `docs/design/` をDUMMY_ACCEL3現行仕様に合わせて更新済み。
12. PC backendのprotocol層をtransport非依存に再整理する。
13. Firmwareのprotocol encoder/decoderをtransportから分離する。
14. custom GATT transportで実装する。
15. WebGUIをCapability drivenに寄せる。最新値カード、Signal選択、CSV列は2026-06-20に実装済み。2026-06-23にstream interval一覧を追加し、変更可能な `DUMMY_ACCEL3` と固定rateの実センサstreamを同じ場所で確認できるようにした。field metadataは当面PC backendが `payload_format` から補完する正式仕様とする。
16. NUS transportをオプションとして設計だけ残す、または最小実装する。

## 22. 現時点の推奨結論

現在の5 Characteristic構成（Sensor Data / Control / Config / Status / Capability）は、初期実装として妥当である。ただし、汎用的な開発用センサーモニター雛形としては、以下を追加・再設計する必要がある。

- `Capability`を追加する。schema v1はstream descriptorまで実装済みで、field metadataは当面PC backendが `payload_format` から補完する。
- `Status`をRead/Notifyにする。
- `Sensor Data`を`stream_id`付きframeにする。Protocol v3として実装済み。
- `Config`をstream単位にする。Config v4最小実装として `stream_id=1` のinterval変更は実装済みで、次段はenable/disable、実センサrate変更、Config Responseの優先順位を再評価する。
- NUS/Serial transportへ載せられるよう、Application protocolをtransport非依存にする。
- 実センサavailability失敗理由をStatusまたはLog/Eventで読めるようにする。LSM6DSL/HTS221/LPS22HB/LSM303AGR magnetometerのsensor別最小診断はStatusへ追加済み。

主軸は独自GATT、NUSはオプションtransport。この方針が、開発時の見通し、WebGUIの自動化、将来の複数device/projectへの再利用性のバランスが最もよい。

## 23. 実機確認履歴

詳細な実機確認・検証履歴は `docs/records/validation_history_2026-06.md` へ分離した。

直近の重要な確認:

- 2026-06-20: LSM303AGR magnetometer `stream_id=12` を実装し、Capability Read / Sensor Data Notify / BLE smokeで実機確認済み。
- 2026-06-21: CSV loggerは単一wide CSV + `s<stream_id>_<field>`列名へ整理済み。
- 2026-06-21: WebGUI rich graph化と、Connect/Start/Stop後のHTTP 500 error修正を確認済み。
- 2026-06-21: Config v4 `SET_STREAM_INTERVAL` を `stream_id=1` で実装し、Firmware build / flash / BLE smoke / negative smokeで確認済み。
- 2026-06-21: `stream_id=1` を `DUMMY_ACCEL3_INT16_V1` へ整理し、dummy batteryとA0 ADCをactive pathから削除。PC自動テスト / Firmware build / flash / BLE smoke / negative smokeで確認済み。
- 2026-06-23: WebGUIのinterval表示を全streamへ広げ、`DUMMY_ACCEL3` はEditable、実センサstreamはFixedとして表示するよう更新。PC自動テストで確認済み。
- 2026-06-23: `codex/mahony-orientation-stream` でLSM6DSL派生姿勢stream `stream_id=13` をMahony拡張。Naive / 相補フィルタ / Mahony filterのpitch/roll/zenith、Mahony yaw、合成加速度をBLEに載せ、GUIで3方式同時表示・表示切替・filter parameter設定を実装。PC自動テスト35件、Web frontend構文確認、Firmware shield build、flash、BLE smoke、実ブラウザ確認まで完了。
- 2026-06-23: `codex/orientation-iir-filter` で `stream_id=13` を `ORIENTATION_MOTION_INT16_V2` へ更新し、IIR pitch/roll/zenith、IIR cutoff Config、Orientationエリア内filter設定、Raw live折りたたみを追加。PC自動テスト35件、Web frontend構文確認、Firmware shield build、flash、BLE smokeまで完了。

過去履歴内の `次作業` は当時のメモであり、現在の優先順位は `現在の残課題 / 次回作業キュー` を正とする。
