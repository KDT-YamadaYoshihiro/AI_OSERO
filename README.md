# CardFlow Othello

手札のカードと EP（エネルギーポイント）を使って石を置く・動かす・守る・奪う、カード駆動型のオセロ派生ゲーム（C++20 / Windows コンソール）。

- ルールの原典: [.specify/memory/constitution.md](.specify/memory/constitution.md)
- 技術仕様（確定事項は末尾の Clarifications）: [.specify/memory/specification.md](.specify/memory/specification.md)
- 開発ルール: [AGENT.md](AGENT.md)

## 遊び方

同じ端末で 2 人が交互に入力して対戦する。

```powershell
.\build\msvc\Release\cardflow.exe        # 山札はランダム
.\build\msvc\Release\cardflow.exe 7      # seed を指定すると同じ展開を再現できる
```

- 毎ターン EP +3（上限 6）。手札（3 枚）からカードを **1 ターン 1 枚** 使える（Reserve の追加 Place は別枠）
- 入力: `<番号> [マス ...]`（例: `1 c4 f5`）、`p` ターン終了／パス、`h` ヘルプ、`q` 終了
- ターン終了時に手札を 3 枚まで補充。両者が続けてパスすると終了し、石の多い方の勝ち
- 盤面記号: `●○` 石、`◉◎` 保護中（Cover / Shield）の石、`■` 壁

## 開発状況

| 範囲 | 状態 |
|---|---|
| 盤面・石・壁、プレイヤー（EP・手札）、カード定義 | 実装済み・テスト済み |
| 挟み・裏返し・捕獲ルール（Cover / Shield の保護を含む） | 実装済み・テスト済み |
| カード 10 種の効果、発動手順、ターン開始処理 | 実装済み・テスト済み |
| 共有デッキ（10 種 × 3 枚）・配札・補充、ゲーム進行、パスと終局判定 | 実装済み・テスト済み |
| コンソール対戦（2 人で交互に入力） | 実装済み・テスト済み |
| CPU 対戦、DxLib などの GUI | 未着手 |

`cardflow_othello.py` は初期の試作で、現在の C++ 実装・仕様とは同期していない。

## 構成

```
include/cardflow/   公開ヘッダー
  types / stone / board / card / player   基本データ
  rules        挟み・裏返し・捕獲
  card_effects カード効果と発動手順
  deck / game  山札とゲーム進行
  ui/          盤面描画・コマンド解析・コンソール対戦
src/core/           ゲームロジック（UI 非依存）→ cardflow_core
src/ui/             コンソール UI → cardflow_ui
src/main.cpp        実行ファイル cardflow
tests/              GoogleTest（初回構成時に v1.15.2 を自動取得）
```

## ビルドとテスト

Visual Studio 2026（「C++ によるデスクトップ開発」）が必要。IDE ではフォルダーを開けば `CMakePresets.json` が読み込まれる。コマンドラインでは「Developer PowerShell for VS」から:

```powershell
cmake --preset msvc            # 構成（MSBuild / x64）
cmake --build --preset debug   # ビルド（/W4 /WX：警告はエラー扱い）
cmake --build --preset release
ctest --preset debug           # テスト実行
```
