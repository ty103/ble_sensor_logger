---
name: runbook-router
description: Build/Flash/RTT/troubleshooting の依頼を ai_quickstart と runbooks の正本へルーティングする薄いSkill。
---

# runbook-router

## 目的
Build/Flash/RTTの実行時に、最短で正しいrunbookへ誘導する。

## このSkillの責務
- タスク種別を判定する（build / flash / rtt / troubleshooting）
- 実行前の前提確認を促す（環境、board、shield、build-dir）
- 参照すべきドキュメントを1つに絞って提示する
- 失敗時はtroubleshootingへ誘導する

## このSkillの非責務
- runbook本文の再記述や複製
- 固定コマンドの長文テンプレート管理

## 参照順（正本）
1. firmware/docs/ai_quickstart.md
2. firmware/docs/runbooks/build.md
3. firmware/docs/runbooks/flash.md
4. firmware/docs/runbooks/rtt_debug.md
5. firmware/docs/runbooks/troubleshooting.md

## ルーティング規則
- build系の依頼: firmware/docs/runbooks/build.md
- flash系の依頼: firmware/docs/runbooks/flash.md
- RTT/ログ取得系の依頼: firmware/docs/runbooks/rtt_debug.md
- エラー調査・失敗時: firmware/docs/runbooks/troubleshooting.md

## 出力フォーマット
- 判定タスク: <build|flash|rtt|troubleshooting>
- まず読む: firmware/docs/ai_quickstart.md
- 次に読む: <該当runbook>
- 前提チェック: <不足があれば列挙>
- 実行結果が失敗した場合: firmware/docs/runbooks/troubleshooting.md

## 更新ルール
- 手順変更時はrunbookのみ更新する
- このSkillは「参照先」や「分岐条件」が変わる場合のみ更新する
