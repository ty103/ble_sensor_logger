# ADR-0003: Capability schema v1はstream descriptorまでに留める

## Status

Accepted

## Context

Capabilityは、PC backend/WebGUIがFirmware固有のsensor構成を知るための入口である。

field metadataまでFirmwareから返せるとdevice self-describingになるが、schema v1へ可変長field descriptorを後付けすると、payloadが複雑になり、Firmware/PC双方の実装と互換性管理が重くなる。

現在のWebGUI/CSVに必要なfield label、unit、scale、decimalsは、PC backendが `payload_format` から補完できる。

## Decision

Capability schema v1はstream descriptorまでに留める。

Firmwareはstream_id、stream_type、channel_count、data_type、unit、payload_format、intervalなどのstream metadataを返す。

WebGUI/CSV用のfield metadataは、PC backendが `payload_format` から補完する。

真にdevice self-describingなfield metadataが必要になった時点で、Capability schema v2またはTLV化として再設計する。

## Consequences

- schema v1を小さく保てる。
- Firmware側のCapability実装が固定長binaryで扱いやすい。
- WebGUI/CSVは現行payload_formatに対するPC backend metadataに依存する。
- 新しいpayload_formatを追加する時は、Firmware、PC parser、backend metadata、WebGUI fallback、test/docsを同期する必要がある。
