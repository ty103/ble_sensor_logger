# Firmware

対象は nRF52840 DK、Zephyr board target は `nrf52840dk/nrf52840`。

## ビルド・フラッシュ・RTT ガイド

詳細な Build/Flash/RTT 手順は以下を参照してください：

- **[docs/ai_quickstart.md](docs/ai_quickstart.md)** - AI・人向けの入口（タスク別ルーティング表）
- **[docs/runbooks/](docs/runbooks/)** - タスク別詳細手順（build.md / flash.md / rtt_debug.md / troubleshooting.md）

### よく使うコマンド

基本ビルド：
```bash
source .env
west build -b nrf52840dk/nrf52840 firmware --build-dir build --pristine
west flash --build-dir build
```

X-NUCLEO-IKS01A2 接続時：
```bash
source .env
west build -b nrf52840dk/nrf52840 firmware --build-dir build --pristine --shield x_nucleo_iks01a2
west flash --build-dir build
```

詳細・RTT・トラブルシューティング は [ai_quickstart.md](docs/ai_quickstart.md) を参照。

## 移植性ルール

- nRF52840 DK専用のhelper APIに依存しない。
- NCS sampleのboard-control helperに依存しない。
- application logicへDK pin番号をhardcodeしない。
- hardware固有処理はZephyr API、Devicetree/Kconfig、または薄いplatform adapterへ寄せる。
