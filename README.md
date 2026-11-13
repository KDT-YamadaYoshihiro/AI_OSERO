# CardFlow Othello

手札のカードと EP（エネルギーポイント）を使って石を置く・動かす・守る・奪う、カード駆動型のオセロ派生ゲーム（C++20 / Windows / DxLib）。

- ルールの原典: [.specify/memory/constitution.md](.specify/memory/constitution.md)
- 技術仕様（確定事項は末尾の Clarifications）: [.specify/memory/specification.md](.specify/memory/specification.md)
- 開発ルール: [AGENT.md](AGENT.md)

## 遊び方

タイトル画面で「2 人対戦（同じ PC で交互に操作）」か「CPU 対戦（強さ: 弱い / 普通 / 強い、先手・後手を選択）」を選ぶ。

```powershell
.\build\msvc\Release\cardflow.exe        # 山札はランダム
.\build\msvc\Release\cardflow.exe 7      # seed を指定すると同じ展開を再現できる
```

- 初期配置は各 4 個（中央 4 個＋斜め外側 4 個）
- 毎ターン EP +3（上限 6）。手札（3 枚）からカードを **1 ターン 1 枚** 使える（Reserve の追加 Place は別枠）
- 操作はマウス: 手札のカードをクリック → 光っているマスを順にクリック。対象のないカードは「使う」で発動。右クリックで取り消し
- 「パス／ターン終了」で手番を終える。使えるカードがなければ自動で進む
- ターン終了時に手札を 3 枚まで補充。両者が続けてパスすると終了し、石の多い方の勝ち。どちらかの石が 0 個になったら即終了
- 盤面: 金の輪 = Cover、水色の輪 = Shield、灰色の四角 = 壁、赤枠 = 直前に使われたマス

## 開発状況

| 範囲 | 状態 |
|---|---|
| 盤面・石・壁、プレイヤー（EP・手札）、カード定義 | 実装済み・テスト済み |
| 挟み・裏返し・捕獲ルール（Cover / Shield の保護を含む） | 実装済み・テスト済み |
| カード 10 種の効果、発動手順、ターン開始処理 | 実装済み・テスト済み |
| 共有デッキ（10 種 × 3 枚）・配札・補充、ゲーム進行、パスと終局判定 | 実装済み・テスト済み |
| CPU 対戦（3 段階）、全滅による終局 | 実装済み・テスト済み |
| DxLib による GUI（マウス操作、2 人対戦・CPU 対戦） | 実装済み（画面の状態と操作はテスト済み） |

`cardflow_othello.py` は初期の試作で、現在の C++ 実装・仕様とは同期していない。

## 構成

```
include/cardflow/   公開ヘッダー
  types / stone / board / card / player   基本データ
  rules        挟み・裏返し・捕獲
  card_effects カード効果と発動手順
  deck / game  山札とゲーム進行
  ai           CPU（合法手の列挙・局面評価・難易度別の手の選択）
  app/         GUI の状態と操作・画面配置・文言（描画ライブラリ非依存）
src/core/           ゲームロジック（UI 非依存）→ cardflow_core
src/app/            GUI の状態と操作 → cardflow_app
src/gui/            DxLib による描画と WinMain → 実行ファイル cardflow
third_party/dxlib/  DxLib（各自で配置、git 管理外）
tests/              GoogleTest（初回構成時に v1.15.2 を自動取得）
```

## ビルドとテスト

Visual Studio 2026（「C++ によるデスクトップ開発」）が必要。

### DxLib の配置（初回のみ）

1. [DxLib のダウンロードページ](https://dxlib.xsrv.jp/dxdload.html) から「VisualStudio( C++ )用」（`DxLib_VC3_25a.zip`、約 207 MB）を取得して展開する
2. 展開した `DxLib_VC\プロジェクトに追加すべきファイル_VC用` の**中身**を `third_party\dxlib\` にコピーする（`third_party\dxlib\DxLib.h` があればよい）

DxLib は約 660 MB あるため git には含めない。見つからない場合、GUI（`cardflow`）はビルドされず、ロジックとテストだけがビルドされる。

### ビルド

IDE ではフォルダーを開けば `CMakePresets.json` が読み込まれる。コマンドラインでは「Developer PowerShell for VS」から:

```powershell
cmake --preset msvc            # 構成（MSBuild / x64）
cmake --build --preset debug   # ビルド（/W4 /WX：警告はエラー扱い）
cmake --build --preset release
ctest --preset debug           # テスト実行
```
