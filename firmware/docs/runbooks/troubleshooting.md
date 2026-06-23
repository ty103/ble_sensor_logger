# Runbook: トラブルシューティング

Build / Flash / RTT 操作時の問題と対処法。

---

## ビルド関連

### `west build` が "Kconfig module" エラー

**症状**: `Kconfig.modules` not found

**対処**:
```bash
cd <repo_root>
source firmware/.env
west update
west build ...
```

---

### `--shield x_nucleo_iks01a2` が見つからない

**症状**: `Error: Shield x_nucleo_iks01a2 not found`

**対処**:
- NCS インストールが正しいか確認: `echo $ZEPHYR_BASE`
- Zephyr modules が最新か確認: `west update`

---

## フラッシュ関連

### `west flash` が J-Link を検出しない

**症状**: `Device with specified ID not found` または同等のエラー

**対処**:
```bash
# J-Link デバイス確認
nrfutil device list

# J-Link ドライバを再起動
sudo killall JLinkGDBServer JLinkRTTLogger
```

その後 USB 接続を再確認して再実行してください。

---

### 複数デバイス接続時のシリアル番号指定

**症状**: 正しいデバイスを指定する必要がある

**対処**:
```bash
# デバイス一覧を確認
nrfutil device list

# 該当シリアル番号を指定
west flash --build-dir build --dev-id 1050278440
```

---

## RTT ログ関連

### RTT ログが出力されない

**症状**: `build/rtt_capture.log` が空またはファイルが作成されない

**対処**:
1. デバイスが正しくフラッシュされているか確認
2. `rtt_debug.conf` を有効にしてビルドされているか確認（[build.md#rtt-ログ取得有効化](build.md#rtt-ログ取得有効化vcom-が使えない場合) 参照）
3. `JLinkRTTLogger` が正しく起動しているか確認:
   ```bash
   ps aux | grep JLinkRTTLogger
   ```

---

### JLinkRTTLogger が応答しない

**症状**: `JLinkRTTLogger` プロセスがハング

**対処**:
```bash
killall JLinkRTTLogger
# または
killall -9 JLinkRTTLogger
```

---

### スクリプトがシリアル番号を自動検出できない

**症状**: `ERROR: no J-Link devices found` または `ERROR: multiple J-Link devices found`

**対処**:
```bash
# デバイスを明示的に指定
firmware/scripts/rtt_capture.sh --build-dir build --serial <JLINK_SERIAL>
```

---

**作成日**: 2026-06-23
