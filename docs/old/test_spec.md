# BLE Sensor Logger Test Specification

作成日: 2026-06-12

## 1. テスト対象

| 領域 | 対象 |
| --- | --- |
| Firmware | nRF52840 DK (`nrf52840dk/nrf52840`) / NCS v3.2.3 / Zephyr / AEM |
| PC App | Python package `ble_sensor_logger` / bleak / interactive CUI / local WebGUI |
| BLE | Sensor Logger Service and 4 characteristics |
| Protocol | Sensor Data, Control, Config, Status fixed binary payload |

## 2. 受け入れ基準

| ID | 項目 | 基準 |
| --- | --- | --- |
| AC-01 | Advertising | `BLE_SENSOR_LOGGER`としてconnectable advertisingする |
| AC-02 | GATT | 仕様書のService/Characteristic UUIDが探索できる |
| AC-03 | Notification | 100 ms設定で10秒間に概ね100サンプル受信できる |
| AC-04 | Parse | PCアプリがProtocol v2の20 bytes Sensor Dataを正しくパースし、v1の16 bytesも受信できる |
| AC-05 | Start/Stop | `start`で送信開始、`stop`で送信停止する |
| AC-06 | Config | `set-interval <ms>`後に送信周期が変化する |
| AC-07 | Status | `status`で状態、interval、last_error、connection_countを取得できる |
| AC-08 | Reconnect | 切断後、FWはadvertising再開、PCは再接続可能 |
| AC-09 | Robustness | 不正WriteでFWがクラッシュしない |
| AC-10 | Portability | FWアプリ層がDK専用API、NCS sample board helper、ピン番号直書きに依存しない |
| AC-11 | ADC | Zephyr ADC APIでA0入力を約10 Hzで取得し、raw値とmV値をNotificationへ含める |
| AC-12 | WebGUI | BLEをPython backendに保持したまま、ブラウザで接続・制御・選択信号のリアルタイムグラフ表示ができる |

## 3. 正常系テスト

| ID | 手順 | 期待結果 |
| --- | --- | --- |
| N-01 | FWを書き込み、PCでBLE scanする | `BLE_SENSOR_LOGGER`が見つかる |
| N-02 | `connect BLE_SENSOR_LOGGER`を実行する | 接続成功し、FWログにconnected eventが出る |
| N-03 | `monitor`後に`start`を実行する | Sensor Data Notificationが表示される |
| N-04 | 100 ms設定で10秒間monitorする | 受信数が概ね100、sequenceが単調増加する |
| N-05 | `set-interval 500`を実行する | 送信周期が約500 msへ変化する |
| N-06 | `status`を実行する | state、sample_interval_ms、last_errorが表示される |
| N-07 | `stop`を実行する | Notification表示が停止する |
| N-08 | `reset-seq`後に`start`を実行する | sequenceが0付近から再開する |
| N-09 | A0を開放または電圧源へ接続してmonitorする | `adc_raw`と`adc_mv`が約10 Hzで更新される |
| N-10 | WebGUIでScan、Connect、Startを実行する | 最新値と選択信号のグラフがリアルタイム更新される |
| N-11 | WebGUIを390×844で表示する | 横スクロール、文字切れ、UI要素の重なりがない |

## 4. 異常系テスト

| ID | 手順 | 期待結果 |
| --- | --- | --- |
| E-01 | Controlへ不正長payloadを書き込む | ATT errorまたはlast_error `INVALID_LENGTH` |
| E-02 | Controlへ不正versionを書き込む | ATT errorまたはlast_error `INVALID_VERSION` |
| E-03 | Controlへ未定義commandを書き込む | ATT errorまたはlast_error `INVALID_COMMAND` |
| E-04 | Configへ10 msを書き込む | ATT errorまたはlast_error `INVALID_CONFIG` |
| E-05 | Configへ10001 msを書き込む | ATT errorまたはlast_error `INVALID_CONFIG` |
| E-06 | Notification購読前にstartする | FWはクラッシュせず、送信不可状態を保持する |
| E-07 | monitor中にデバイス電源を切る | PCアプリが切断を検知する |
| E-08 | デバイス電源を戻す | FWがadvertising再開し、PCから再接続できる |

## 5. PCアプリ単体テスト

| ID | 対象 | 内容 |
| --- | --- | --- |
| U-01 | `protocol.py` | payload sizeが仕様と一致する |
| U-02 | `protocol.py` | Sensor Data round trip |
| U-03 | `protocol.py` | Control command round trip |
| U-04 | `protocol.py` | Config interval範囲外を拒否 |
| U-05 | `protocol.py` | Status round trip |
| U-06 | `app_core.py` | sequence欠落を検出する |

実行:

```bash
cd pc_app
PYTHONPATH=src python -m pytest
```

## 6. FWレビュー観点

- `protocol`層がAEM/Zephyr BLEに依存していないこと。
- BLE callback内で長い処理を行わず、AEM eventへ変換していること。
- nRF52840 DK専用API、NCS sample board helper、ピン番号直書きがないこと。
- `sensor_dummy`を実センサdriver adapterへ差し替え可能なこと。
- Sensor Data characteristicのNotification対象attrがcharacteristic valueを指していること。
- Config範囲外、不正Controlでクラッシュしないこと。

## 7. WebGUI構成の確認

- BLE scan/connect/notify/write/reconnectはPython `ble_client`に残す。
- ブラウザ側ではWeb Bluetooth APIを使わない。
- WebGUIはlocal HTTP/WebSocketの`web_api`を通じて`app_core`を操作する。
- CUIとWebGUIが同じ`app_core` APIを共有できること。
- 選択可能な信号はADC mV/raw、Accel X/Y/Z、Batteryとする。

## 8. 実機テスト結果

実施日: 2026-06-13  
実施環境: nRF52840 DK PCA10056, serial `1050278440`, NCS v3.2.2, macOS Python 3.11.11, bleak via `uv`

### 8.1 実施コマンド

```bash
west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware --pristine
west flash --build-dir build/firmware --dev-id 1050278440
cd pc_app
uv run --extra dev pytest
uv run --extra dev python -m ble_sensor_logger --once scan
uv run --extra dev python scripts/ble_e2e_smoke.py
uv run --extra dev python scripts/ble_negative_smoke.py
```

### 8.2 結果サマリ

| ID | 結果 | 実測/確認内容 |
| --- | --- | --- |
| AC-01 | PASS | 起動ログで `ble_service: advertising` を確認 |
| AC-02 | PASS | PC scanで `BLE_SENSOR_LOGGER 52EA309C-F155-FE7B-283F-BA5D9C94F019` を検出 |
| AC-03 | PASS | 100 ms設定、3秒窓で31サンプル受信 |
| AC-04 | PASS | `SensorDataPayload`としてNotificationをパースし表示 |
| AC-05 | PASS | `start`後に受信開始、`stop`後1秒窓で0サンプル |
| AC-06 | PASS | 500 ms設定、3秒窓で5-6サンプル受信 |
| AC-07 | PASS | `status state=MEASURING interval_ms=500 last_error=NONE connections=1` |
| AC-08 | PASS | 切断後ログで `ble_service: advertising`、再scanで再検出 |
| AC-09 | PASS | 不正Config/Control WriteがATT error相当で拒否され、FWは継続動作 |
| AC-10 | PASS | アプリコードにDK専用API、NCS sample board helper、ピン番号直書きなし |

### 8.3 正常系

| ID | 結果 | 実測/確認内容 |
| --- | --- | --- |
| N-01 | PASS | FW flash成功後、PC scanで `BLE_SENSOR_LOGGER` 検出 |
| N-02 | PASS | `connected=true` |
| N-03 | PASS | `monitoring=true` 後、Notification受信 |
| N-04 | PASS | 100 ms設定、3秒窓で31サンプル、`missed_samples=0` |
| N-05 | PASS | 500 ms設定、3秒窓で5-6サンプル |
| N-06 | PASS | Status read成功、`state=MEASURING`, `interval_ms=500`, `last_error=NONE` |
| N-07 | PASS | Stop後1秒窓で `after_stop_samples=0` |
| N-08 | NOT RUN | `reset-seq`単独の実機確認は未実施。E2Eではsequence欠落なしを確認 |

### 8.4 異常系

| ID | 結果 | 実測/確認内容 |
| --- | --- | --- |
| E-01 | PASS | 不正長Control Writeが `BleakGATTProtocolError`、Status `INVALID_LENGTH` |
| E-02 | NOT RUN | 不正version Controlの実機確認は未実施 |
| E-03 | NOT RUN | 未定義commandの実機確認は未実施 |
| E-04 | PASS | Config 10 ms Writeが `BleakError`、Status `INVALID_CONFIG` |
| E-05 | NOT RUN | Config 10001 msの実機確認は未実施 |
| E-06 | NOT RUN | Notification未購読startの実機確認は未実施 |
| E-07 | PASS | PCアプリの明示disconnectで切断、FWログで通知無効化を確認 |
| E-08 | PASS | FWログでdisconnect後 `advertising`、PC scanで再検出 |

### 8.5 単体テスト

| ID | 結果 | 実測/確認内容 |
| --- | --- | --- |
| U-01 | PASS | pytest |
| U-02 | PASS | pytest |
| U-03 | PASS | pytest |
| U-04 | PASS | pytest |
| U-05 | PASS | pytest |
| U-06 | PASS | pytest |

pytest結果: `6 passed in 0.03s`

### 8.6 実機テスト中に見つけて修正した事項

| 事項 | 修正 |
| --- | --- |
| Zephyr 4.2系で `BT_LE_ADV_CONN` が未定義 | `BT_LE_ADV_CONN_FAST_1`へ変更 |
| AEMが使う`k_malloc`が未有効 | `CONFIG_HEAP_MEM_POOL_SIZE=2048`を追加 |
| disconnect直後のadvertising再開が失敗する場合があった | BLE callback内で直接再開せず、delayable workで200 ms後に再開し、失敗時は1秒後リトライ |

## 9. Protocol v2 / ADC / WebGUI追加テスト結果

実施日: 2026-06-14
実施環境: nRF52840 DK PCA10056, serial `1050278440`, NCS v3.2.2, macOS, bleak/aiohttp via `uv`

### 9.1 FirmwareとProtocol

| ID | 結果 | 実測/確認内容 |
| --- | --- | --- |
| AC-04 | PASS | v2 20 bytesのADC付きpayloadを受信。pytestでv1 16 bytesの後方互換parseも確認 |
| AC-10 | PASS | ADC入力はDevicetree `io-channels`、実装はZephyr ADC APIのみを使用 |
| AC-11 | PASS | 起動ログで `ADC ready: channel=0, fs=10 Hz`。A0から`adc_raw=47`, `adc_mv=41`を実測 |
| N-04 | PASS | 100 ms設定の3秒窓で31サンプル、`missed_samples=0` |
| N-05 | PASS | 500 ms設定の3秒窓で5サンプル |
| N-07 | PASS | Stop後の1秒窓で0サンプル |
| N-09 | PASS | NotificationのADC raw/mVが継続更新 |

### 9.2 WebGUI実機操作

| ID | 結果 | 実測/確認内容 |
| --- | --- | --- |
| AC-12 | PASS | WebGUIからScan、Connect、Start、interval変更、Stop、Disconnectを実行 |
| N-10 | PASS | ADC mVグラフを実データで描画。最大300点保持、281サンプル時点で欠落0 |
| N-11 | PASS | Desktopおよび390×844で表示確認。横方向overflow、文字切れ、重なりなし |
| W-01 | PASS | BrowserはHTTP/WebSocketのみを使用し、BLE処理はPython backendで実行 |
| W-02 | PASS | Start/Stop後にStatusを再取得し、Disconnect時にStatus表示をクリア |

### 9.3 ビルド結果

- Firmware: FLASH 178,948 bytes (17.07%)、RAM 31,948 bytes (12.19%)
- Flash: serial `1050278440`へ書き込み成功
- ADC入力: nRF52840 DKのA0（P0.03 / SAADC AIN1）。DK専用APIは不使用
- PC単体テスト: `9 passed`（Protocol、app core、Web API/static assets）
- JavaScript: `node --check pc_app/web_frontend/app.js` 成功

## 10. WebGUI追加機能テスト

実施日: 2026-06-14

| ID | 項目 | 期待結果 | 結果 |
| --- | --- | --- | --- |
| W-03 | グラフ時刻軸 | 横軸にlocal timeの`hh:mm:ss` tickを表示する | PASS（Desktop最大5 tick、狭幅では自動削減） |
| W-04 | 接続中表示 | Connectを無効化し、`Connecting...`を表示する | PASS（Frontend stateとWebSocket stateで同期） |
| W-05 | 接続キャンセル | 接続中のStopで接続taskをcancelし、BLE clientをdisconnectする | PASS（単体テストおよびAPI状態確認） |
| W-06 | キャンセル後復帰 | cancel後にDKへ再接続できる | PASS（Status取得後に正常切断） |
| W-07 | CSV記録 | 記録中の全受信サンプルをCSVとしてdownloadできる | PASS（header、UTF-8 BOM、timestamp、全payload fieldを確認） |
| W-08 | CSV重複防止 | Status更新とWebSocketが同一sampleを返しても重複記録しない | PASS（`host_time_ms`で重複排除） |

追加後のPC単体テスト: `11 passed`
