# 個別設計書

作成日: 2026-06-17

このディレクトリでは、現時点の実装に対応する設計書を管理する。汎用化の方針と未決事項は `../generic_sensor_monitor_design_handoff.md`、現行の外部仕様は `../specs/current_implementation_spec.md` を参照する。

## 資料一覧

| 資料 | 役割 | 読むタイミング |
| --- | --- | --- |
| `system_design.md` | Firmware、PC backend、WebGUI/CUIを含む全体設計。BLE/HTTP/WebSocket interface、横断状態、sequenceを定義する。 | 境界仕様変更、全体影響確認、Protocol変更時 |
| `firmware_design.md` | Firmware内部設計。module、App Event Manager event、control、sensor、BLE serviceを定義する。 | Firmware実装・レビュー時 |
| `pc_webapp_design.md` | Python backend、interactive CUI、local WebGUI設計。HTTP API、WebSocket、CSV、UI状態を定義する。 | PC backend/WebApp実装・レビュー時 |

## 更新ルール

- 全体のinterfaceや責務分担が変わる場合は `system_design.md` を更新する。
- Firmware内部のmodule、event、状態管理が変わる場合は `firmware_design.md` を更新する。
- Python backend、CUI、WebGUI、CSV、HTTP/WebSocketが変わる場合は `pc_webapp_design.md` を更新する。
- 正本 `../generic_sensor_monitor_design_handoff.md` と矛盾する場合は、正本側に方針変更を明記する。
- 状態遷移図とシーケンス図はMarkdown内のMermaidで記述する。
- 同じ仕様表を複数資料へ複製しない。必要な場合は参照先を明記する。

