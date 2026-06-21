# Firmware

対象は nRF52840 DK、Zephyr board target は `nrf52840dk/nrf52840`。

nRF Connect SDK v3.2.2 のshellでbuildする。

```bash
west build -b nrf52840dk/nrf52840 firmware
west flash
```

X-NUCLEO-IKS01A2 shieldを有効にしてbuild/flashする。

```bash
west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware --pristine --shield x_nucleo_iks01a2
west flash --build-dir build/firmware
```

VCOMが使えない試作基板向けに、RTT-only loggingでbuild/flashする。

```bash
west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware-rtt --pristine --shield x_nucleo_iks01a2 --extra-conf rtt_debug.conf
west flash --build-dir build/firmware-rtt --dev-id 1050278440
```

X-NUCLEO-IKS01A2の周期I2C probeはbring-up診断時だけ有効にする。

```bash
west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware-probe --pristine --shield x_nucleo_iks01a2 --extra-conf iks01a2_i2c_probe_debug.conf
```

RTT loggingとI2C probeを同時に使う場合は、CMake経由で2つのextra configを渡す。

```bash
west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware-rtt-probe --pristine --shield x_nucleo_iks01a2 -- -DEXTRA_CONF_FILE="rtt_debug.conf;iks01a2_i2c_probe_debug.conf"
```

RTT log取得にはSEGGER toolを使う。`JLinkRTTLogger` を使うと、長時間確認しやすいfile logを残せる。

```text
Device name: NRF52840_XXAA
Target interface: SWD
Interface speed: 4000
RTT Control Block address: <empty for auto>
RTT Channel name or index: 0
Output file: build/rtt_logger_capture.log
```

移植性ルール:

- nRF52840 DK専用のhelper APIに依存しない。
- NCS sampleのboard-control helperに依存しない。
- application logicへDK pin番号をhardcodeしない。
- hardware固有処理はZephyr API、Devicetree/Kconfig、または薄いplatform adapterへ寄せる。
