# ドキュメント改善セルフレビュー

作成日: 2026-06-17  
対象: docs構成、開発プロセス、現行仕様書、設計書

## 1. 目的

開発プロセス定義とドキュメント整備を段階的に進めた結果、資料の責務が重なり始めていたため、現状のディレクトリ構成、全ドキュメントの名称・役割・運用方針を整理した。

レビュー観点:

- 各ドキュメントに重複や矛盾がないか。
- 継続的に改良開発を進める前提になっているか。
- AI駆動で開発する際に、各作業に不要なコンテキストが混じりにくい構成か。

## 2. 改善サマリ

追加・更新した主な資料:

| 資料 | 変更内容 |
| --- | --- |
| `docs/README.md` | docs全体の入口、ディレクトリ構成、資料の役割、読むタイミング、重複回避ルールを整理 |
| `docs/development_process.md` | 成果物体系、標準フロー、検証、merge条件を実運用向けに再定義 |
| `docs/specs/current_implementation_spec.md` | 現行実装のBLE/Protocol/HTTP/WebSocket/UI仕様を書き起こし |
| `docs/design/system_design.md` | FW/WebAppを含む全体設計、interface、状態遷移図、sequence図を作成 |
| `docs/design/firmware_design.md` | Firmware module、App Event Manager event、状態、Control/Config/Sensor/BLE設計を作成 |
| `docs/design/pc_webapp_design.md` | Python backend、CUI、WebGUI、HTTP/WebSocket、CSV設計を作成 |
| `docs/generic_sensor_monitor_design_handoff.md` | 新しい資料体系を反映し、handoffを汎用化判断と未決事項の正本へ整理 |

## 3. セルフレビュー 1回目

観点: 資料責務の分離、重複排除

確認結果:

| 確認項目 | 結果 | 対応 |
| --- | --- | --- |
| 正本と現行仕様が混ざっていないか | 一部混ざりやすい構造だった | 現行仕様を `docs/specs/current_implementation_spec.md` に分離 |
| 設計書作成予定が実体とずれていないか | `architecture.md` など旧案が残っていた | 3分割の `system_design.md` / `firmware_design.md` / `pc_webapp_design.md` に更新 |
| AIが作業ごとに読む資料を選べるか | 入口資料が不足していた | `docs/README.md` に作業テーマ別の読み方を追加 |

判断:

- handoff資料は将来方針の正本として維持する。
- 現行実装の詳細は仕様書・設計書へ分離し、handoffの肥大化を避ける。

## 4. セルフレビュー 2回目

観点: 継続開発プロセスとして使えるか

確認結果:

| 確認項目 | 結果 | 対応 |
| --- | --- | --- |
| 仕様、設計、実装、検証、docs更新の流れが明確か | 旧資料では提案段階の表現が多かった | `development_process.md` をプロセス定義として再構成 |
| 成果物と更新タイミングが明確か | 一覧が不足していた | 成果物体系表を追加 |
| merge条件が明確か | handoff更新の必要性はあったが条件が散らばっていた | フェーズ終了条件とmerge条件を追加 |
| uv利用が明記されているか | 一部資料で明確でなかった | `development_process.md` と実行例で `uv` を明記 |

判断:

- 開発プロセスは「毎回すべての資料を更新する」ではなく、「外部仕様、全体境界、FW内部、PC/WebApp内部」の変更に応じて更新対象を選ぶ運用にする。
- 小さな修正でも、外部仕様が変わる場合は現行仕様書を更新する。

## 5. セルフレビュー 3回目

観点: 現行実装との整合、将来方針との分離

確認結果:

| 確認項目 | 結果 | 対応 |
| --- | --- | --- |
| BLE UUID、Characteristic、payload sizeが実装と一致するか | `protocol.h`, `protocol.py`, `ble_service.c` と照合済み | 現行仕様書へ反映 |
| HTTP/WebSocket routeが実装と一致するか | `web_api.py` と照合済み | 現行仕様書とPC/WebApp設計書へ反映 |
| 状態遷移が実装と一致するか | Device、BLE、backend、WebGUIの状態を現行実装に合わせた | 全体設計書と個別設計書へMermaidで反映 |
| 将来案が現行仕様書に混入していないか | 制約・非対象範囲として明示する必要あり | `非対象範囲` と `既知の制約` に分離 |
| 実装上の気づきを設計課題として残したか | `connection_count` と `notify_interval_ms` の整理余地あり | 各設計書の既知課題に記録 |

判断:

- 現行仕様書は「現在実装されていること」を主語にする。
- 将来追加したいCapability、Status Notify、stream_id、NUSは、非対象範囲または改善候補として明示し、現行仕様と混同しない。

## 6. 残課題

| 課題 | 次の扱い |
| --- | --- |
| Capability payload形式 | 次フェーズの仕様定義で決める |
| Sensor Data frame形式 | stream_id、timestamp、sequence幅を含めて再設計する |
| Status Notify payload | Control/Config結果通知と合わせて設計する |
| CSVの複数stream表現 | Capability driven WebGUI設計時に決める |
| `connection_count`の責務整理 | Firmware実装修正時にBLE層/control層のどちらを正にするか決める |
| 旧資料削除 | 再設計完了後、必要知見を現行資料へ移したうえで削除する |

## 7. 結論

現行docsは、以下の役割分担に整理した。

- `docs/README.md`: docsの地図。
- `docs/development_process.md`: 開発プロセスと成果物定義。
- `docs/generic_sensor_monitor_design_handoff.md`: 汎用化方針、未決事項、次作業の正本。
- `docs/specs/current_implementation_spec.md`: 現行実装の外部仕様。
- `docs/design/`: 実装を理解・変更するための設計書。
- `docs/reviews/`: 改善とレビューの経緯。
- `docs/old/`: 旧資料の一時退避。

この構成により、Firmware作業、PC/WebApp作業、Protocol変更、汎用化設計で参照する資料を分けられるため、AI駆動開発時に不要なコンテキストを読み込みにくい。

