# CardFlow Othello

手札のカードと EP（エネルギーポイント）を使って石を置く・動かす・守る・奪う、カード駆動型のオセロ派生ゲーム（C++20 / Windows コンソール）。

- ルールの原典: [.specify/memory/constitution.md](.specify/memory/constitution.md)
- 技術仕様: [.specify/memory/specification.md](.specify/memory/specification.md)
- 開発ルール: [AGENT.md](AGENT.md)

## 開発状況

| 範囲 | 状態 |
|---|---|
| 盤面（石・壁・初期配置・移動/入替/反転などの基本操作） | 実装済み・テスト済み |
| プレイヤー（EP 回復 +3／上限 6、支払い、手札 3 枚） | 実装済み・テスト済み |
| カード定義（10 種のコスト・カテゴリ、手札のカテゴリ上限判定） | 実装済み・テスト済み |
| コンソール描画 | 盤面表示のみ |
| カード効果・捕獲ルール | 未実装（「壁で囲んで捕獲」の定義が仕様で未確定） |
| デッキ・配札、ゲームループ、勝敗判定、入力 | 未実装 |

`cardflow_othello.py` は初期の試作で、現在の C++ 実装・仕様とは同期していない。

## 構成

```
include/cardflow/   公開ヘッダー（types / stone / board / card / player, ui/board_view）
src/core/           ゲームロジック（UI 非依存）→ cardflow_core
src/ui/             コンソール描画 → cardflow_ui
src/main.cpp        実行ファイル cardflow
tests/              GoogleTest（初回構成時に v1.15.2 を自動取得）
```

## ビルドとテスト

Visual Studio 2026（「C++ によるデスクトップ開発」）が必要。IDE ではフォルダーを開けば `CMakePresets.json` が読み込まれる。コマンドラインでは「Developer PowerShell for VS」から:

```powershell
cmake --preset msvc          # 構成（MSBuild / x64）
cmake --build --preset debug # ビルド（/W4 /WX：警告はエラー扱い）
ctest --preset debug         # テスト実行
.\build\msvc\Debug\cardflow.exe
```
