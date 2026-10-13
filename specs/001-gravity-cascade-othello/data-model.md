# Phase 1 Data Model: 重力カスケードオセロ

## Entities

### Cell / Board
- `Cell`: `color ∈ {Empty, Black, White}`; shield tracked separately as a bitmask per board.
- `Board`: fixed `kBoardSize × kBoardSize` array (8×8), row-major. Derived operations:
  - `lowestEmpty(col)` → row index or none (column full)
  - `topIndex(col)` → highest occupied row in column
  - `settle()` → compact stones downward per column, return moved-cell set
- **Invariants**: stones never float below an empty cell after any resolution step; exactly one stone per cell.

### Stone
- Identity = position + color + shield flag (transient). No persistent IDs needed.

### Player
- `side`: Black(0)/White(1)
- `ep: int` 0..6 (+3 at turn start, cap 6; Charge adds +2)
- `deck: vector<CardType>` (20 cards, 10 types × 2, shuffled by seeded RNG)
- `hand: vector<CardType>` (initial 3 drawn from deck)
- `discard: vector<CardType>`

### GameState
- board, shieldMask, currentPlayer, anchor (`optional<int>` — none before first move), blockedColumn + expiryTurn (from Anchor card), passCounter, turnNumber, rng, mode (vsCPU/hotseat), difficulty.
- **State transitions**:
  - `startTurn(p)`: ep += 3 (cap); draw 1 if deck non-empty.
  - `legalDropColumns()`: anchor==null → all columns; else {anchor±1} ∩ [0,7], minus blocked columns, minus full columns.
  - `applyDrop(col)`: place stone at lowestEmpty(col); update anchor=col; run cascade resolution.
  - `applyCard(card, params)`: validate EP ≥ cost and effect legality; deduct EP; mutate board/state; run cascade if movement/creation occurred; card → discard.
  - `endTurn()`: if no legal drop even via Jet → pass++, check end condition; else pass=0 was already reset on move.
  - `isGameOver()`: passCounter ≥ 2 or all cells filled.

### Move
- `{kind: Drop | CardDrop(Jet) | CardEffect, column, targetColumn, targetCell}` — produced by rules engine; consumed by UI/AI.

### CardType (10 types)
| Type | EP | Effect (pure state transition) |
|---|---|---|
| Jet | 2 | drop own stone to any non-full column (chain bypass), anchor=that column |
| Anchor | 2 | block named column for opponent's next turn |
| Gust | 3 | move top stone of chosen column to adjacent column (settle+flip) |
| Bomb | 4 | destroy any one opponent stone (settle+flip cascade) |
| Shield | 2 | mark own stone un-flippable until own next turn start |
| Magnet | 3 | move top stone of an opponent column to top of another non-full column |
| Double | 4 | two drops this turn; if second drop illegal → card not played, EP refunded |
| Collapse | 3 | remove bottom stone of chosen own column (settle+flip cascade) |
| Swap | 3 | exchange two 8-adjacent stones (any colors), then settle+flip |
| Charge | 0 | ep += 2 (cap 6) |

### Cascade Resolution (rules)
`resolve(board, movedCells)`: loop { settle; if any stone moved → detect simultaneous sandwiches from all moved stones against pre-flip board; apply flips atomically; } until no movement. Shielded stones are excluded from flipping but not from destruction/movement.

## Validation Rules (from FR IDs)
- FR-004/007: drop legality enforced in `legalDropColumns`.
- FR-016: card playable iff EP ≥ cost AND effect preconditions met (target exists, column not full).
- FR-019: end detection checked after every turn resolution.
- FR-012/R5: convergence guaranteed by monotonic gap reduction.
