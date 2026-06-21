# ADR-0006: LSM303AGR accel streamは用途が明確になるまで追加しない

## Status

Accepted

## Context

X-NUCLEO-IKS01A2上のLSM303AGR accelは、2026-06-20のdebug build限定probeでZephyr `st,lis2dh` driver経由のready/sample取得を確認済みである。

一方、現行FirmwareはすでにLSM6DSLの6軸IMU streamを持ち、accel x/y/zは `stream_id=10` の `IMU6_INT16_V1` で取得できる。LSM303AGR accelを `stream_id=11` として通常runtime streamへ追加すると、現時点の主用途ではaccel系streamが重複し、Firmware/PC parser/WebGUI/CSV/tests/docsの更新範囲が増える。

Generic Sensor Monitorとしては、同じ物理量を複数streamで扱えること自体は必要だが、現段階では比較計測や低電力構成などの利用目的がまだ具体化していない。

## Decision

LSM303AGR accel `stream_id=11` は、現時点では通常runtime streamとして追加しない。

`stream_id=11` は将来候補として予約し、以下のいずれかが必要になった時点で実装を再開する。

- LSM6DSL accelとLSM303AGR accelの比較計測が必要になった。
- LSM6DSLを使わない構成でaccel streamが必要になった。
- 低電力構成など、LSM303AGR accelを優先する明確な理由ができた。
- WebGUI/CSVで同一物理量の複数stream表示を検証する代表例が必要になった。

再開時は、既存のLSM303AGR magnetometer実装と同じく、Firmware sensor adapter、Protocol payload format、PC parser、Capability補完metadata、WebGUI表示、CSV列、pytest、実機smoke、handoff更新を一括で扱う。

## Consequences

- 現行runtime stream集合を増やさず、LSM6DSL/HTS221/LPS22HB/LSM303AGR magnetometerの安定化を優先できる。
- Capability schema v1、WebGUI、CSV loggerへの追加同期が不要になる。
- `stream_id=11` は未使用の予約枠として残るため、将来追加時のID再設計は不要である。
- LSM303AGR accelのprobe実績は残るが、現行仕様のacceptance criteriaには含めない。
