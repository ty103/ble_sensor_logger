# ADR-0001: 主軸transportは独自GATTにする

## Status

Accepted

## Context

Generic Sensor Monitorは、開発用センサーデバイスのsample、control、config、status、capabilityを扱う。

NUSのようなbyte streamを主軸にすると、GATT上の意味はTX/RXに閉じ、payload内に独自frame protocolを多重化する必要がある。一方で独自GATTなら、BLE discovery時点でCharacteristicの役割が見える。

## Decision

主軸transportは独自GATTにする。

Characteristicは、Sensor Data、Control、Config、Status、Capabilityを基本構成とする。

NUSやSerial UARTは主軸にはしない。ただし、Application protocolは将来transport adapterで差し替えられる構造を保つ。

## Consequences

- BLE toolから見たときに、Characteristicの用途が分かりやすい。
- WebGUI/backend側で役割ごとの処理を分けやすい。
- Capability discoveryや複数sensor streamへ拡張しやすい。
- NUS/Serial対応が必要になった場合は、transport adapterとframe boundary設計を追加する必要がある。
