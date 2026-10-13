# 重力カスケードオセロ (Gravity Cascade Othello)

重力+連鎖制約+スキルカードで完全リニューアルした8×8オセロ。Speckit仕様: `specs/001-gravity-cascade-othello/`

## 規則概要
- **重力drop**: 石を列上から投入 → 重力で最低空cellへ落下 (四子棋式)
- **連鎖制約**: 投入列は前turn dropした列(anchor)の ±1 のみ。同列drop不可 — 盤必ず前進
- **cascade固定点**: 裏返/落下を連鎖自動処理 (8方向挟まねば裏返)
- **EP system**: +3/turn, max 6。毎turn石drop(必須)+1枚カード使用(optional)
- **スキルカード10種**: Jet(2EP) 任意列drop / Anchor(2EP) 列封锁 / Gust(3EP) 頂石横移 / Bomb(4EP) 敵石破壊 / Shield(2EP) 裏返保護 / Magnet(3EP) 敵頂石移動 / Double(4EP) 二石drop / Collapse(3EP) 自底石除去→cascade / Swap(3EP) 隣接石交換 / Charge(0EP) +2EP
- **end**: 両player連続pass → 多数決/引き分け

## ビルド・テスト (検証済み)
```bash
# prereq: MSVC 2022, CMake, Ninja, DxLib 3.25a VCを third_party/dxlib/ に解凍
winget install Kitware.CMake Ninja-build.Ninja
curl -L -o third_party/dxlib/DxLib_VC3_25a.zip https://dxlib.xsrv.jp/DxLib/DxLib_VC3_25a.zip
# 解凍後 headers/libs を third_party/dxlib/dxlib_vc/ にコピー (ASCII path)

cmake -B build-ninja -G Ninja
cmake --build build-ninja
ctest --test-dir build-ninja --output-on-failure   # 7 suites, 100% PASS
```
注意: CMake 4.xでは`enable_testing()`必須 (CMakeLists.txtに追加済み)。VS generatorでもビル可。

## プレイ
```bash
build-ninja\gravitycascade.exe
```
操作: 1(CPU battle)/2(hotseat)→难度選択→数字キー1-9でカード選択→列クリックでdrop。H=ヘルプ, R=restart, ESC=cancel

## 構造
- `src/core/` 規則引擎 (types/board/state/rules) — UI/AIはここ経由のみ
- `src/cards/` card.hpp + effects.cpp (10種純関数)
- `src/ai/` Easy(随机)/Normal(1-ply greedy)/Hard(alpha-beta minimax)
- `src/main.cpp` DxLib GUI
- `tests/` 自前CHECK harness: board/chain/flip/end/cards/ai + seeded self-play e2e
