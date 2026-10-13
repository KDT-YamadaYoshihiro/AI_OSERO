# Feature Specification: 重力カスケードオセロ (Gravity Cascade Othello)

**Feature Branch**: `feature/gravity-cascade-othello`

**Created**: 2026-10-13

**Status**: Draft (インタビュー round 1〜6 により全澄清事項解決済み)

**Input**: User description: "これまでにない全く新しいオセロを開発してください。Speckit utilizedで要件化を細部まで行う。スキルカードを利用したゲームにする。C++とDXライブラリを使用。"

## 概念概要

標準オセロの「8方向サンドwich flip」DNA を継承しながら、**3つの未組合の革新**を融合した新ゲーム:

1. **重力 (Gravity)** — 石は盤面任意位置に置かれるのではなく、列(縦軸)へ上から投入され、重力によりその列の最低空cellへ落ちる(四子棋式)。
2. **連鎖 (Chain)** — 毎turn 投入できる列は「前turn dropした列(anchor)の ±1 列」のみ。anchor は毎turn「dropした列」へ更新されるため、ゲームは盤面上必然的に「流れる蛇」のように前進する。
3. **カスケード (Cascade)** — 石の破壊・移動で生じた空隙へ上方石が落下する過程で flip条件を満たせば、auto-flip が連鎖trigger。重力と flip rule の相互作用が动态战术を生む。
4. **スキルカード** — 重力/連鎖 system を manipulation する10種のスキルカード + EP(Energy Point) resource system。毎turn 石drop(必須) + 1枚 card使用(optional)。

---

## User Scenarios & Testing *(mandatory)*

### User Story 1 - 重力連鎖core game vs CPU (Priority: P1)

Player は 8×8 盤面で CPU と対戦する。石は列へdrop → gravityで沈む → standard 8方向 sandwichで flip → cascade auto-flip。anchor ±1 constraint によりplayable列が制限される。両player連続パス or 全64cell fill 後 majority count で勝敗。

**Why this priority**: core rule engine 無しには何も機能しない。MVP 価値の根幹。

**Independent Test**: card system なし(EP 0, hand empty)で rule engine だけで完整 match 完了できる。

**Acceptance Scenarios**:

1. **Given** 初期配置(中央4石が gravityで底へ沈み、列3・4に各2石の塔: 列3 bottom→top B,W / 列4 W,B)、**When** 先手(黒)任意列へdrop, **Then** その列の最低空cellへ石落下, anchor=その列。
2. **Given** anchor=列4, **When** 白が列3 or 列5へdrop, **Then** legal。列7へdrop, **Then** illegal(拒否・再入力)。
3. **Given** dropした石の8方向直線線上に「敵石…敵石…己石」 pattern, **When** drop完成, **Then** 挟まれた敵石すべて flip。
4. **Given** 列5 bottom=B, top=W, **When** 列4へdropで (row7,col4)→(row7,col3)... diagonal sandwich成立, **Then** flip cascade 判定正确動作。
5. **Given** 両playerの anchor±1 列すべてfull, **When** turn開始, **Then** 自動パス。連続パス → game end, majority count, 同数なら draw。

---

### User Story 2 - スキルカードとEP system (Priority: P2)

Player は hand の cardから1枚(EP cost 払って)使用可能。deck(20枚=10種×2)/hand(初期3枚,毎turn 1 draw)/EP(+3/turn, cap 6) が resource economy を形成。Card は gravity/chain system を破・変·控。

**Why this priority**: 用户明示要件「スキルカードを利用したゲーム」。core engine 之上の layer。

**Independent Test**: EP/hand/deck state 独立 test + card effect unit test。

**Acceptance Scenarios**:

1. **Given** EP=3, hand=[Jet(2), Bomb(4)], **When** Jet使用, **Then** EP=1, 任意列へdrop, anchor更新。
2. **Given** EP=1, **When** Bomb(4EP)使用試み, **Then** 拒否・再選択。
3. **Given** 列6 full, **When** Anchor(card)で列6 block, **Then** opponent 次turn 列6へdrop不可。
4. **Given** 列3 top石下方に gap 生成(Bomb), **When** 上方石落下, **Then** 落下完成後 flip判定 → cascade chain reaction 自動trigger。

---

### User Story 3 - 2人ホットシート対戦 (Priority: P3)

同一 machine 上 2 human player が alternating turn で対戦。opponent hand は hidden(公開 information game ではない)。

**Why this priority**: local multiplayer 是 mode requirement だが core engine reuse 故 later slice。

**Independent Test**: CPU AI bypass して 2 human input path 完整 test。

**Acceptance Scenarios**:

1. **Given** hotseat mode, **When** player 2 turn開始, **Then** player 1 hand 盤面UIで非表示。
2. **Given** invalid input(空列・full列・anchor±1外), **When** submit, **Then** error message + re-input prompt, state不变。

---

### User Story 4 - CPU difficulty 3 level (Priority: P4)

Easy(random legal move, card不使用)/Normal(greedy heuristic: flip maximization + basic EP/card use)/Hard(minimax alpha-beta depth 5〜6, card strategy含)。

**Why this priority**: AI strength は対戦 experience 重要だが rule engine 完成後 addition。

**Independent Test**: difficulty level ごとに legal move 生成 + response time 測定。

**Acceptance Scenarios**:

1. **Given** Easy CPU, **When** turn, **Then** anchor±1 内 random legal columnへdrop, 1秒以内。
2. **Given** Hard CPU, **When** complex position, **Then** minimax search で flip-max + cascade-aware move選択, 5秒以内。

---

### Edge Cases

- **full列 drop attempt**: anchor±1 列すべて full → 自動パス。パスは「連続パス counter」へrecord。
- **pure drop(no flip)**: anchor 更新(規則により必ず前进)。
- **Jet 使用時**: chain constraint bypass, anchor=Jetでdropした列へ更新。
- **Shield 保護石 vs Bomb**: Bomb 対象=盤面任意敵石 → Shield 石は「flip不可」だが「destroy 対象」 — destroy可(Shield は flipのみprotect)。UI で明示。
- **Collapse 対象列 bottom=己石 only(1石)**: 列 empty化 → legal。
- **Gust/Magnet 目標列 full**: card effect 不可 → EP refund, card return, turn continue(drop済みならパス扱いなし・effectなし)。
- **Swap で cascade trigger**: swap後 gravity settle + flip resolution 実行。
- **deck exhausted**: draw skip, hand残り枚数で play 継続。
- **EP insufficient for all cards**: card phase skip(石dropは必須故 game flow 継続)。
- **game end 時 board 未fill**: 両player連続パス → end(多数 count)。同数=draw。
- **cascade infinite loop**: settle→flip→settle iteration は fixed point 保証(石数不变, flip单调有限) — engine 側で convergence assert。
- **Anchor block duration**: opponent 1 turn 分。self列へdropは unaffected。

## Requirements *(mandatory)*

### Functional Requirements

**Board & Stones**
- **FR-001**: System MUST maintain an 8×8 grid (constant-defined, 64 cells), each cell state ∈ {Empty, Black, White}.
- **FR-002**: System MUST initialize with the standard 4 center stones placed such that gravity settles them into columns 3 and 4 (0-indexed) as two stacks of 2 (col3: B(bottom)/W(top), col4: W(bottom)/B(top)).
- **FR-003**: Black moves first; turns strictly alternate.

**Gravity & Drop**
- **FR-004**: A drop targets a column; the stone MUST land in the lowest empty cell of that column.
- **FR-005**: A player MUST drop exactly 1 stone per turn unless passing or Double card (2 stones).
- **FR-006**: System MUST auto-detect and enforce: if no legal column exists for the current player, automatic pass with clear UI indication.

**Chain Constraint & Anchor**
- **FR-007**: System MUST maintain an anchor column. Legal drop columns = {anchor-1, anchor+1} only (within 0..7 bounds). The anchor column itself is NOT legal.
- **FR-008**: After every drop (normal or Jet), the anchor MUST update to the column of that drop.
- **FR-009**: At game start, the first player (Black) may drop into any column (no initial anchor).
- **FR-010**: The Anchor card MUST block a named column from drops for exactly one opponent turn; blocked columns are excluded from legal set but do not change the anchor.

**Flipping & Cascade**
- **FR-011**: After any stone placement or movement settles, system MUST apply standard 8-direction sandwich flipping (opponent stones between the moved stone and a same-color own stone in a straight line are flipped).
- **FR-012**: Any card effect that destroys or moves a stone MUST trigger gravity settling of stones above the created gap, then re-run flip detection; this loop MUST repeat until fixed point (no further movement or flips possible).
- **FR-013**: Shield MUST protect exactly one named own stone from flipping until the start of that player's next turn. Destruction effects are not blocked by Shield.

**Cards, Deck, Hand, EP**
- **FR-014**: Each player MUST have an independent deck of 20 cards (10 types × 2 copies) and a hand initialized to 3 cards drawn from it.
- **FR-015**: At the start of each turn, the player MUST draw 1 card if the deck is not empty, and gain +3 EP capped at 6.
- **FR-016**: A player MAY play at most 1 skill card per turn, paying its EP cost; cards are consumed to discard on use.
- **FR-017**: The system MUST implement the following 10 card effects (EP cost in parentheses): Jet(2): drop own stone to any column ignoring chain. Anchor(2): block one column for opponent's next turn. Gust(3): move top stone of a chosen column into an adjacent column (gravity settles). Bomb(4): destroy any one opponent stone (gravity cascade follows). Shield(2): protect one own stone from flipping until own next turn. Magnet(3): move the top stone of an opponent column to the top of another chosen non-full column. Double(4): drop 2 own stones this turn; second drop follows chain rules from first; if second has no legal column the card is not played and EP is not spent. Collapse(3): remove the bottom-most stone of a chosen own column (gravity cascade follows). Swap(3): exchange positions of two adjacent (8-dir) stones of any colors, then resolve gravity/flip. Charge(0): gain +2 EP (capped at 6).
- **FR-018**: Card effects that create gaps MUST trigger the cascade resolution defined in FR-012.

**Game End & Scoring**
- **FR-019**: The game MUST end when both players pass consecutively OR all 64 cells are filled; the side with more stones wins, equal counts is a draw.
- **FR-020**: System MUST display final score and result in Japanese.

**Modes & CPU**
- **FR-021**: System MUST support Human vs CPU and 2-player hotseat modes.
- **FR-022**: System MUST provide CPU difficulty Easy/Normal/Hard as specified in User Story 4.
- **FR-023**: Opponent hand MUST be hidden; EP, score, turn number, anchor indicator, and legal columns MUST be visible to the current player.

**Interface**
- **FR-024**: System MUST render a graphical board where hovering a column previews the landing cell, mouse click drops, and keys 1–9,0 select/confirm skill cards (keyboard shortcuts).
- **FR-025**: All UI text MUST be in Japanese. Invalid input MUST produce an error message and re-prompt without state change.

### Key Entities *(include if feature involves data)*

- **Board / Column**: 8×8 grid; columns are the unit of drop legality.
- **Stone**: Black/White piece with position, shield flag.
- **Player**: color side, EP pool (0–6), hand, deck, discard pile.
- **SkillCard**: 10 types with EP cost and effect definition.
- **Anchor**: current chain column; drives legality.
- **GameSession**: turn state, pass counter, mode, difficulty, history.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A complete Human-vs-CPU match is playable in under 10 minutes for a first-time player without rule questions (rules discoverable via in-game help).
- **SC-002**: The rule engine is deterministic: given the same initial state and RNG seed, identical game replays are produced.
- **SC-003**: CPU response time: Easy < 0.1 s, Normal < 1 s, Hard < 5 s per move on a typical Windows PC.
- **SC-004**: All gravity, chain, flip, cascade and 10 card behaviors are covered by automated unit tests; a scripted self-play (Hard vs Easy, seeded) completes with a valid result in under 60 seconds.
- **SC-005**: The game builds with zero compiler errors and zero warnings on MSVC (per AGENT.md).
- **SC-006**: A test player can correctly explain chain constraint and cascade after one tutorial round ("anchor" indicator makes legality obvious).

## Assumptions

- Windows 10/11 + Visual Studio 2022 (MSVC) available; DxLib will be downloaded/installed during implementation.
- Existing CardFlow Othello code (`src/`, `include/`, `tests/`, `cardflow_othello.py`) is fully replaced (per user decision); the repository keeps Speckit infrastructure (.specify/, specs/) and AGENT.md/.gitignore/README.md.
- Hotseat means both players share one screen; hidden-hand is enforced by UI, not by cryptography.
- No save/load of game state in v1; RNG seed for test determinism is internal.
- The existing `.specify/memory/constitution.md` describes the deprecated CardFlow variant; this spec supersedes its game rules for the new feature.

## Clarifications Resolved (Interview rounds 1–6)

| # | Decision | Outcome |
|---|----------|---------|
| 1 | Core innovation direction | Agent-proposed combination of chain-limited + gravity variants (round 1/2) |
| 2 | Scope | Replace this repo's game entirely (round 1) |
| 3 | Modes | Human vs CPU + 2-player hotseat (round 2) |
| 4 | UI | C++ + DX Library GUI, mouse+keyboard (round 2; user correction: "C++とDXライブラリ") |
| 5 | Board size | 8×8 (round 3) |
| 6 | Initial setup | Standard 4 center stones settle to bottom via gravity (round 3) |
| 7 | No legal chain move | Pass, constraint continues (round 3) |
| 8 | Flip directions | Standard 8-direction (round 3) |
| 9 | Card usage model | Drop required + optional 1 card per turn (round 4) |
| 10 | Resource system | EP (+3/turn, cap 6) retained (round 4) |
| 11 | Card design | Agent-designed 10 gravity/chain-themed cards approved (round 4) |
| 12 | Cascade mechanic | YES — auto-flip on gravity settling (round 4) |
| 13 | Card visibility | Hidden hand (round 4) |
| 14 | Chain definition | anchor ±1 only; same-column re-drop not allowed (round 5) |
| 15 | Anchor update rule | anchor = column dropped this turn, always advances (round 5) |
| 16 | Deck/hand | per-player 20-card deck, initial hand 3, draw 1/turn (round 5) |
| 17 | End condition | consecutive passes or full board → majority count (round 5) |
| 18 | CPU difficulty | Easy/Normal/Hard (minimax depth 5–6 alpha-beta) (round 5) |
| 19 | Initial anchor | Black's first move may drop into any column (round 6) |
