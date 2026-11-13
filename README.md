# CardFlow Othello

いつものオセロ（挟んで自分の色にする）に、ここぞで使えるカードを少しだけ加えたゲーム（C++20 / Windows / DxLib）。

- ルールの原典: [.specify/memory/constitution.md](.specify/memory/constitution.md)
- 技術仕様（ルールの細部・CPU・画面）: [.specify/memory/specification.md](.specify/memory/specification.md)
- 開発ルール: [AGENT.md](AGENT.md)

## 遊び方

タイトル画面で「2 人対戦（同じ PC で交互に操作）」か「CPU 対戦（強さ: 弱い / 普通 / 強い、先手・後手を選択）」を選ぶ。

```powershell
.\build\msvc\Release\cardflow.exe        # 山札はランダム
.\build\msvc\Release\cardflow.exe 7      # seed を指定すると同じ展開を再現できる
```

- **基本は通常のオセロ**: 光っているマス（相手の石を挟めるマス）をクリックして石を置く。置けなければパス
- **カード**: 石を置く前に、手札から 1 ターン 1 枚まで使える（使い切り）。開始時 2 枚、自分の番 3 回ごとに 1 枚引く（最大 3 枚）
  - 連打: もう 1 つ石を置ける ／ 守り: 自分の石を 1 回守る ／ 壁: 空マスを壁にする ／ 反転: 相手の石を 1 つ奪う（角は不可） ／ 自由: 石の隣ならどこでも置ける
  - カードをクリック → 光ったマスをクリック（対象のないカードはもう一度クリック）。右クリックで取り消し
- 盤面が埋まる、両者が続けてパスする、どちらかの石が 0 個になったら終了。石の多い方の勝ち
- 盤面の印: 水色の輪 = 守り、灰色の四角 = 壁、赤い点 = 直前に置いた石

## 開発状況

| 範囲 | 状態 |
|---|---|
| 盤面・オセロのルール（守り・壁を含む） | 実装済み・テスト済み |
| カード 5 種、共有の山札（15 枚）、ゲーム進行・パス・終局 | 実装済み・テスト済み |
| CPU 対戦（3 段階） | 実装済み・テスト済み |
| DxLib による GUI（盤面中央・手札は下、マウス操作） | 実装済み（画面の状態と操作はテスト済み） |

`cardflow_othello.py` は初期の試作で、現在の C++ 実装・仕様とは同期していない。

## 構成

```
include/cardflow/   公開ヘッダー
  types / stone / board   基本データ（石・壁・盤面）
  rules        オセロのルール（合法手・裏返し）
  card         カード 5 種の定義と盤面への効果
  deck / game  山札とゲーム進行
  ai           CPU（局面評価・難易度別の手の選択）
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
