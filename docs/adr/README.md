# ADR運用方針

ADRはArchitecture Decision Recordの略で、後から迷い直しやすい設計判断を1件1ファイルで残す。

## 役割

- handoff本体から判断理由の長文を分離する。
- 決定済みの設計判断について、背景、決定、影響を短く残す。
- 仕様書や作業ログではなく、設計判断の理由を追う入口にする。

## 更新ルール

- 新しい設計判断、または既存判断の変更があった時だけ追加・更新する。
- ファイル名は `0001-short-title.md` のように連番にする。
- Statusは `Accepted`、`Superseded`、`Proposed` のいずれかを使う。
- handoff本体にはADRへのリンクと現在の結論だけを書く。

## ADR一覧

| ADR | Status | 決定 |
| --- | --- | --- |
| `0001-custom-gatt-primary-transport.md` | Accepted | 主軸transportは独自GATTにする |
| `0002-browser-through-python-backend.md` | Accepted | BrowserはBLEを直接扱わずPython backend経由にする |
| `0003-capability-schema-v1-stream-descriptors-only.md` | Accepted | Capability schema v1はstream descriptorまでに留める |
| `0004-wide-csv-stream-qualified-columns.md` | Accepted | CSVは単一wide CSV + stream-qualified列にする |
| `0005-stream-scoped-config-v4.md` | Proposed | stream単位Configは固定長binary v4を最小実装にする |
| `0006-defer-lsm303agr-accel-stream.md` | Accepted | LSM303AGR accel streamは用途が明確になるまで追加しない |
