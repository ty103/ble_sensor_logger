# ADR-0002: BrowserはPython backend経由でBLEを扱う

## Status

Accepted

## Context

WebGUIは、scan/connect、start/stop、status表示、realtime graph、CSV loggingを扱う。

BrowserからWeb Bluetooth APIでBLEを直接扱う案もあるが、ブラウザやOSごとの差分が大きく、自動テストやCSV logging、再接続処理もBrowser側へ寄りやすい。

## Decision

BrowserはBLEを直接扱わない。

BLE通信はPython backendに閉じ込め、BrowserはHTTP/WebSocketでPython backendと通信する。

## Consequences

- CUI、WebGUI、自動テストが同じPC backend APIとprotocol parserを共有できる。
- CSV logging、再接続、protocol変換をbackend側で一元管理できる。
- Browser側はBLE権限やWeb Bluetooth実装差分の影響を受けにくい。
- WebGUI利用時はPython backendの起動が前提になる。
