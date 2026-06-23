# AGENTS.md

## 基本方針

- 日本語で回答すること。
- ドキュメントは日本語で作成すること。ただし専門用語は英語のままで良い（例: BLEのAdvertisingは広告と訳さずAdvertisingのままで良い）。
- pipでなくuvを使う。
- Build/Flash/RTTの初動では、まず `firmware/docs/ai_quickstart.md` を参照し、タスクに応じて `firmware/docs/runbooks/` の対応ファイルを参照する（build.md / flash.md / rtt_debug.md / troubleshooting.md）。

## 設計・引き継ぎ資料の扱い

- 本プロジェクトの設計・引き継ぎの正本は `docs/generic_sensor_monitor_design_handoff.md` とする。
- 作業開始時は正本を確認し、設計判断や未決事項に反する変更をしない。
- チャットで設計判断が更新された場合は、必要に応じて正本を更新する。
- 旧設計資料は `docs/old/` に最大1世代だけ残す。
- 旧資料の内容が正本と矛盾する場合は、正本を優先する。
- 旧資料は再設計完了後、必要な知見を正本へ移したうえで削除する。

## git運用

- `master`を常に動く・参照可能な正本branchとして扱う。
- 作業branchはチャット単位ではなく作業テーマ単位で作る。
- 作業branch名は原則として `codex/<theme>` とする。
- チャット末尾では、必要に応じて `docs/generic_sensor_monitor_design_handoff.md` を更新する。
- `master`へmergeする条件に、handoff資料が最新であることを含める。
- 未完成・実験中の作業は作業branchに残し、handoff資料に未mergeであることと次回作業を記録する。
- commit、merge、pushなどのgit操作は、作業内容と検証結果を確認してから行う。
- 作業開始時は、可能であれば作業テーマが分かる短いチャット名を設定する。環境上チャット名を変更できない場合は、開始メッセージで推奨チャット名を宣言する。
- 引き継ぎ資料に基づいて作業を開始する場合は、実装前に「今回どこまで進めたらcommitするか」と「どこまで確認できたらmasterへmergeするか」を宣言する。
