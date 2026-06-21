# docs運用方針

作成日: 2026-06-17  
対象プロジェクト: BLE Sensor Logger / Generic Sensor Monitor

このディレクトリは、開発プロセス、仕様、設計、引き継ぎ、検証記録を管理する。AI駆動で継続開発しやすいよう、資料ごとの責務を分け、作業に不要なコンテキストを読み込まなくてよい構成にする。

## 1. 現状のディレクトリ構成

```text
docs/
  README.md                                  docs全体の入口、資料一覧、運用方針
  development_process.md                     開発プロセスと成果物定義
  generic_sensor_monitor_design_handoff.md   汎用化設計の正本、現在の残課題、設計論点
  specs/
    current_implementation_spec.md           現時点の実装仕様
  design/
    README.md                                個別設計書の入口
    system_design.md                         FW/WebAppを含む全体設計
    firmware_design.md                       Firmware設計
    pc_webapp_design.md                      PC backend/WebApp設計
  adr/
    README.md                                設計判断記録の運用方針
    0001-*.md                                重要な設計判断と理由
  reviews/
    documentation_improvement_review_2026-06-17.md
                                             docs改善とセルフレビュー記録
  records/
    README.md                                検証記録・作業履歴の運用方針
    validation_history_2026-06.md            実機確認・検証履歴
  old/
    README.md                                旧資料の運用方針
    phase1_2_ble_spec_and_plan.md            初期実装の仕様・開発計画
    test_spec.md                             初期実装のテスト仕様・実機試験結果
```

Repository直下の主要ディレクトリは以下の通り。

```text
firmware/   NCS/Zephyr Firmware。BLE Peripheral / GATT Serverとして動作する。
pc_app/     Python backend、interactive CUI、local WebGUI、PC側テスト。
docs/       開発プロセス、仕様、設計、引き継ぎ、検証記録。
build/      Firmware build成果物。通常の設計参照対象ではない。
```

## 2. ドキュメントの役割

| 資料 | 役割 | 主な読者/タイミング | 更新方針 |
| --- | --- | --- | --- |
| `README.md` | プロジェクト利用者向けの起動・操作入口 | セットアップ、実行、テスト時 | コマンドや利用手順が変わったら更新 |
| `docs/README.md` | docs全体の地図 | 作業開始時、資料探索時 | 資料構成を変えたら更新 |
| `docs/development_process.md` | 開発プロセス、成果物、レビューゲート | 仕様化、設計、実装、merge判断時 | プロセス変更時に更新 |
| `docs/generic_sensor_monitor_design_handoff.md` | 汎用化設計の正本 | 設計判断確認、次フェーズ計画時 | 合意済み判断、現在の残課題/次回作業キューが変わったら更新 |
| `docs/specs/current_implementation_spec.md` | 現時点の実装仕様 | 現行挙動の確認、回帰テスト設計時 | 実装の外部仕様が変わったら更新 |
| `docs/design/system_design.md` | FW/WebAppを含む全体設計 | 境界設計、全体影響確認時 | 構成、interface、状態、sequenceが変わったら更新 |
| `docs/design/firmware_design.md` | Firmware内部設計 | Firmware実装・レビュー時 | Firmware module、event、状態が変わったら更新 |
| `docs/design/pc_webapp_design.md` | PC backend/WebApp設計 | PC側実装・レビュー時 | API、UI、logging、状態管理が変わったら更新 |
| `docs/adr/*.md` | 重要な設計判断と理由 | 合意済み判断の背景確認、判断変更時 | 設計判断単位で追加し、handoff本体へ理由の長文を置かない |
| `docs/reviews/*.md` | docs改善や設計レビューの記録 | 改善経緯確認、未解決課題確認時 | レビュー単位で追記または新規作成 |
| `docs/records/*.md` | 実機確認・検証履歴・作業ログ | 過去の確認結果、失敗経緯、コマンド証跡を確認する時 | 検証単位または月単位で追記し、handoff本体へ長文ログを置かない |
| `docs/old/*.md` | 旧仕様・旧テスト結果の一時退避 | 過去経緯を確認する必要がある時だけ | 旧資料は最大1世代を原則とし、必要知見を現行資料へ移したら削除 |

## 3. 正本と優先順位

設計・引き継ぎの正本は `docs/generic_sensor_monitor_design_handoff.md` とする。

優先順位:

1. 現行実装と一致する仕様確認は `docs/specs/current_implementation_spec.md` を優先する。
2. 汎用化の方針、現在の残課題、次フェーズ判断は `docs/generic_sensor_monitor_design_handoff.md` を優先する。
3. 実装作業の流れや承認条件は `docs/development_process.md` を優先する。
4. 詳細設計は `docs/design/` の各資料を優先する。
5. 設計判断の理由は `docs/adr/` を優先する。
6. 旧資料と現行資料が矛盾する場合は、現行資料を優先する。

## 4. AI駆動開発での読み方

作業開始時に常に全資料を読む必要はない。作業テーマに応じて最小限の資料を読む。

| 作業テーマ | 最初に読む資料 |
| --- | --- |
| 汎用化方針、次フェーズ計画 | `docs/generic_sensor_monitor_design_handoff.md` の `現在の残課題 / 次回作業キュー` |
| 新機能の仕様定義 | `docs/development_process.md`, `docs/specs/current_implementation_spec.md` |
| Firmware実装 | `docs/design/firmware_design.md`, `docs/design/system_design.md` のinterface関連 |
| PC backend/WebApp実装 | `docs/design/pc_webapp_design.md`, `docs/design/system_design.md` のHTTP/WebSocket関連 |
| BLE payload変更 | `docs/design/system_design.md`, `docs/specs/current_implementation_spec.md` |
| 合意済み設計判断の理由確認 | `docs/adr/` |
| テスト追加・回帰確認 | `docs/development_process.md`, `docs/specs/current_implementation_spec.md` |
| 実機確認履歴・過去の検証結果確認 | `docs/records/` |
| 旧挙動の確認 | `docs/old/test_spec.md` またはgit履歴 |

## 5. 重複を避けるルール

- `current_implementation_spec.md` は「現在実装されている外部仕様」を書く。将来案は書かない。
- `generic_sensor_monitor_design_handoff.md` は「汎用化の設計判断、現在の残課題 / 次回作業キュー、設計論点バックログ」を書く。現行payloadの詳細表や作業ログは必要最小限に留める。
- `system_design.md` は全体構成、境界、interface、横断状態、横断sequenceを書く。
- `firmware_design.md` はFirmware内部のmodule、event、処理責務を書く。
- `pc_webapp_design.md` はPython backend、CUI、WebGUI、HTTP/WebSocket、CSVを書く。
- 重要な設計判断の背景、選択肢、影響は `docs/adr/` に置く。handoff本体には結論とADRへのリンクだけを残す。
- 実機確認、smoke結果、調査ログ、失敗から得た知見は `docs/records/` に置く。handoff本体には現在判断に必要な短い要約とリンクだけを残す。
- 同じ表を複数資料に複製しない。必要な場合は参照先を明記する。

## 6. 旧資料の扱い

`docs/old/` は再設計期間中の一時退避場所とする。

- 原則として1世代前の設計資料だけを残す。
- 必要な知見は現行資料へ転記する。
- 再設計完了後、旧資料は削除候補とする。
- 履歴確認はgitを使い、docsには今後の意思決定や作業に必要な情報だけを残す。
