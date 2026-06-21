# 開発プロセス定義

作成日: 2026-06-17  
対象プロジェクト: BLE Sensor Logger / Generic Sensor Monitor

## 1. この資料の位置づけ

本資料は、本プロジェクトで仕様定義、設計、実装、検証、ドキュメント更新、git操作をどの順序で進めるかを定める。

設計判断と引き継ぎの正本は `docs/generic_sensor_monitor_design_handoff.md` とする。本資料はプロセスと成果物の定義を扱い、個別の技術判断は必要に応じて正本または `docs/design/` へ反映する。

## 2. 基本方針

- 仕様、設計、実装、検証、ドキュメント更新を分けて扱う。
- 変更の大きさに応じて、ユーザーレビュー/承認のタイミングを置く。
- 実機が必要な作業は、実機接続前に可能な限り仕様、設計、自動テスト、Firmware build確認を済ませる。
- `master` は常に動く・参照可能な正本branchとして扱う。
- 未完成または実験中の作業は作業branchに残し、正本資料へ未mergeであることと次回作業を記録する。
- PC側の依存管理とテスト実行は `pip` ではなく `uv` を使う。
- AI駆動で効率よく作業するため、作業テーマごとに読む資料を最小化する。

## 3. 成果物体系

| 成果物 | パス | 内容 | 作成/更新タイミング |
| --- | --- | --- | --- |
| プロジェクト入口 | `README.md` | セットアップ、実行、テスト、利用手順 | コマンドや利用方法が変わった時 |
| docs入口 | `docs/README.md` | ディレクトリ構成、資料の役割、運用方針 | docs構成を変えた時 |
| 開発プロセス | `docs/development_process.md` | 標準フロー、成果物、レビュー観点、merge条件 | プロセス変更時 |
| 汎用化handoff | `docs/generic_sensor_monitor_design_handoff.md` | 合意済み設計判断、未決事項、次作業 | 設計判断や計画が変わった時 |
| 現行仕様書 | `docs/specs/current_implementation_spec.md` | 現時点の外部仕様、操作、payload、API | 実装仕様が変わった時 |
| 全体設計書 | `docs/design/system_design.md` | システム構成、architecture、interface、状態、sequence | 境界や全体設計が変わった時 |
| Firmware設計書 | `docs/design/firmware_design.md` | Firmware module、event、状態、処理sequence | Firmware内部設計が変わった時 |
| PC/WebApp設計書 | `docs/design/pc_webapp_design.md` | Python backend、CUI、WebGUI、HTTP/WebSocket、CSV | PC/WebApp内部設計が変わった時 |
| レビュー記録 | `docs/reviews/*.md` | セルフレビュー、設計レビュー、改善ログ | レビュー単位 |
| 旧資料 | `docs/old/*.md` | 旧仕様・旧テスト結果 | 必要知見を現行資料へ移すまで |

## 4. 標準フロー

### 4.1 要求整理

ユーザーの目的、背景、期待動作、制約、優先度を整理する。Codexは必要に応じて確認質問を行うが、合理的に判断できる小さな実装詳細は既存方針に沿って進める。

成果物:

- 対象範囲と非対象範囲
- 受け入れ条件
- 未決事項
- 影響する資料一覧

出口条件:

- 変更目的が明確である。
- 保留する未決事項と、今回決める事項が分かれている。

### 4.2 仕様書作成/更新

仕様書には、少なくとも以下を含める。

- 目的
- 対象範囲
- 非対象範囲
- ユーザー操作
- Firmwareに期待する動作
- PC backendに期待する動作
- WebGUI/CUIに期待する動作
- BLE interfaceまたはApplication protocolへの影響
- 正常系
- 異常系
- 受け入れ条件
- 実機確認項目

小さな変更は `docs/specs/current_implementation_spec.md` の該当節を更新する。大きな新機能や実験機能は、必要に応じて `docs/specs/` に個別仕様書を追加する。

### 4.3 設計書作成/更新

承認済み仕様または明確な改善要求に基づき、影響する設計書を更新する。

設計書には、少なくとも以下の観点を含める。

- 現状architectureへの影響
- Firmware module構成
- PC backend module構成
- WebGUI/CUI構成
- BLE GATT Service/Characteristic仕様
- Application protocol payload仕様
- HTTP/WebSocket interface仕様
- 状態遷移
- シーケンス
- エラー処理
- 互換性
- テスト方針
- 移行方針

出口条件:

- `docs/generic_sensor_monitor_design_handoff.md` の合意済み設計判断と矛盾していない。
- 矛盾または方針変更がある場合、正本資料が更新されている。
- 作業者が実装に必要な資料だけを読める粒度になっている。

### 4.4 実装前チェック

実装前に以下を確認する。

- 作業branch名
- チャット名または推奨チャット名
- 変更対象ファイル
- 実装範囲
- 実装しない範囲
- 今回のcommit予定境界
- `master` へのmerge予定条件
- 自動テストで確認する項目
- Firmware buildで確認する項目
- 実機で確認する項目
- 実機が接続されていない場合に保留する項目

作業branchは原則として `codex/<theme>` とする。

作業開始時は、作業テーマが分かる短いチャット名を設定する。推奨形式は `BLE Sensor Logger: <作業テーマ>` とする。チャット名を変更できない環境では、開始メッセージで `推奨チャット名: ...` を宣言し、ユーザーが手動で変更できるようにする。

引き継ぎ資料に基づく継続作業では、実装へ入る前に、Codexは以下を短く宣言する。

- 推奨チャット名、または設定済みチャット名。
- どの成果または検証まで進めたらcommitするか。
- どの追加確認まで完了したら `master` へmergeするか。
- 今回の作業範囲外として次commitまたは次branchへ回す内容。

この宣言は開始時点の見込みであり、実装中にblockerや設計変更が見つかった場合は、handoff資料へ理由と次回作業を残して更新する。

### 4.5 実装

承認済み仕様・設計に従って実装する。実装中に仕様または設計とのずれが見つかった場合は、差分を説明し、必要に応じて仕様書または設計書を更新する。

### 4.6 セルフコードレビュー

実装後に以下を確認する。

- 仕様書の受け入れ条件を満たしているか。
- 設計書と矛盾していないか。
- BLE payloadの互換性を意図せず壊していないか。
- Firmware callback内で重い処理をしていないか。
- Firmwareのprotocol層とtransport層が過度に結合していないか。
- PC backendのtransport層、protocol層、app core、UI層が分離されているか。
- WebGUI/CUIの表示が仕様に沿っているか。
- エラー処理とtimeoutがあるか。
- ログや例外が調査可能な粒度になっているか。
- ドキュメント更新が必要な変更か。

## 5. 検証

### 5.1 PC自動テスト

PC側は `uv` を使ってテストを実行する。

```bash
cd pc_app
uv run --extra dev pytest
```

### 5.2 Firmware build

Firmware側はNCS環境でbuildする。

```bash
west build -b nrf52840dk/nrf52840 firmware --build-dir build/firmware
```

### 5.3 実機テスト

実機テストでは以下を記録する。

- 実施日
- 使用branch
- Firmware build対象
- Device識別情報
- 実行した手順
- 期待結果
- 実結果
- 未確認項目
- 既知の不具合

ユーザー実機確認で見つかった差分は、以下に分類する。

- 仕様通りだが分かりにくい挙動
- 仕様漏れ
- 設計漏れ
- 実装不具合
- 実機環境依存
- 今回範囲外の改善要望

## 6. ドキュメント更新ルール

- 実装仕様が変わったら `docs/specs/current_implementation_spec.md` を更新する。
- 全体構成、BLE/HTTP/WebSocket interface、横断状態、sequenceが変わったら `docs/design/system_design.md` を更新する。
- Firmware内部のmodule、event、状態、エラー処理が変わったら `docs/design/firmware_design.md` を更新する。
- PC backend、CUI、WebGUI、CSV、APIが変わったら `docs/design/pc_webapp_design.md` を更新する。
- 汎用化方針、未決事項、次作業が変わったら `docs/generic_sensor_monitor_design_handoff.md` を更新する。
- 利用手順やコマンドが変わったら `README.md` を更新する。
- docs構成を変えたら `docs/README.md` と `docs/design/README.md` を更新する。

## 7. フェーズ終了条件

フェーズ終了前に以下を確認する。

- 仕様書が最新である。
- 設計書が最新である。
- `docs/generic_sensor_monitor_design_handoff.md` が最新である。
- 作業開始時に宣言したcommit予定境界に対して、実際にどこまで到達したかが説明できる。
- README更新要否を確認済みである。
- PC自動テスト結果、Firmware build結果、実機確認結果、または未検証理由が記録されている。
- 未解決事項が正本資料またはレビュー記録に残っている。
- `git status` で意図しない差分がない。

上記を満たした後、必要に応じてcommit、`master`へのmergeを行う。

## 8. merge条件

`master` へmergeする条件:

- 作業目的が完了している。
- 自動テストまたは未実施理由が明確である。
- Firmware変更を含む場合、buildまたは未実施理由が明確である。
- 実機依存の確認項目が完了または保留理由つきで記録されている。
- handoff資料と関連設計書が最新である。
- 作業開始時に宣言したmerge予定条件を満たしている、または条件変更の理由がhandoff資料に記録されている。
- 未完成・実験中の内容が作業branchまたは未決事項に明記されている。

チャットが終わっただけでは、未完成の変更を無理に `master` へmergeしない。
