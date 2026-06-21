# records運用方針

`docs/records/` は、実機確認、smoke test、調査ログ、失敗から得た知見など、過去の証跡を残す場所である。

## 役割

- handoff本体を現在の判断と残課題に集中させる。
- 長い検証ログ、作業履歴、コマンド結果、実機確認の時系列を保存する。
- 後から「いつ、どのbranchで、何を確認したか」を追えるようにする。

## 更新ルール

- 現在の優先順位や次回作業キューは `docs/generic_sensor_monitor_design_handoff.md` に書く。
- 実機確認や調査の詳細は `docs/records/` に追記する。
- handoff本体へは、現在判断に必要な短い要約とrecordsへのリンクだけを残す。
- 月単位またはテーマ単位でファイルを分ける。

## 現在の記録

| 資料 | 内容 |
| --- | --- |
| `validation_history_2026-06.md` | 2026-06のBLE/Firmware/WebGUI実機確認・検証履歴 |
