# ADR-0005: stream単位Configは固定長binary v4を最小実装にする

## Status

Accepted

## Context

現行Config payloadはdevice全体の `sample_interval_ms` を中心にした8 bytes固定長binaryである。

実センサstreamが増え、LSM6DSLは26 Hz、HTS221/LPS22HBは1 Hz、LSM303AGR magnetometerは10 Hzのようにstreamごとのrateが異なる。今後はstreamごとのenable/disableやrate変更が必要になる。

一方で、現時点のCapability schema v1はstream descriptorまでに留めており、field metadataや複雑なConfig descriptorはFirmware payloadへ入れていない。試作段階では、実装とBLE smokeで確認しやすい最小schemaを優先する。

## Decision

次のConfig最小実装は、現行Config v3を破壊的に拡張し、Protocol v4の固定長binary payloadとして設計する。

最小payload案:

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

初期 `op` は `SET_STREAM_INTERVAL` を実装対象にする。2026-06-21時点の最小実装では、Firmwareで実際に変更可能な対象streamは `stream_id=1` の `DUMMY_ACCEL3` のみとし、他streamは固定rateとして扱う。2026-06-23時点では同じ固定長v4 payloadで `stream_id=13` 向けのorientation filter設定 `SET_COMPLEMENTARY_ALPHA`、`SET_MAHONY_KP`、`SET_MAHONY_KI`、`SET_IIR_CUTOFF_MILLIHZ` も実装済みである。`SET_STREAM_ENABLE` はv4 smoke後の次段候補に残す。

現行Characteristic UUIDは維持し、payload `version` でv3 device-wide Configとv4 stream-scoped Configを区別する。試作段階のため、v4実装時にPC backend/Firmwareを同時更新し、v3互換維持は必須にしない。

TLV化、Config Request/Response、複数項目一括更新は、stream単位Configの最小実装と実機確認が終わってから再検討する。

## Consequences

- 既存のControl/Config/Status/Capability構成を保ったまま、stream単位の設定へ進められる。
- 固定長binaryなのでFirmware/Python双方の実装とsmoke確認が軽い。
- `op` と `stream_id` により、将来のenable/rate/range/filter設定を同じCharacteristicに載せやすい。
- v4実装時はFirmware、PC backend、WebGUI/CUI、docs、testsを同時に更新する必要がある。
- TLVより拡張余地は狭い。設定項目が増え、可変長valueやsensor固有parameterが必要になった時点でTLVまたはConfig Request/Responseへ移行する。
