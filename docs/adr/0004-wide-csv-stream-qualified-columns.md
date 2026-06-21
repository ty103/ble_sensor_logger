# ADR-0004: CSVは単一wide CSV + stream-qualified列にする

## Status

Accepted

## Context

LSM303AGR magnetometer追加後、複数streamが同時に通知されるようになった。

複数streamには同じfield名が存在し得る。例えばmixed streamとLSM6DSL IMU6 streamはどちらも `accel_z_mg` を持つ。

stream別CSVへ分割する案もあるが、WebGUIからのdownloadと短時間の試作ログでは、1つのCSVにまとまっている方が扱いやすい。

## Decision

CSVはstream別ファイルに分割せず、1つのwide CSVとしてdownloadする。

値列は `/api/capability` の `streams[].fields[]` から生成し、列名を `s<stream_id>_<field>` にする。

sample行と一致しないstreamの値列は空欄にする。

## Consequences

- 同名fieldを持つstream同士を列で安全に分離できる。
- 1つのdownload fileで複数streamを扱える。
- 非該当stream列は空欄が増える。
- 長時間loggingや解析tool要件が明確になった場合は、backend保存、streaming download、stream別CSV分割を再検討する。
