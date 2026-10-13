# Contract: Rule Engine (core) — behavior visible to UI/AI/tests

The core engine is a pure state-transition library. UI and AI never mutate the board directly; they only call these operations and read results.

**C-1 `legalDropColumns(state) -> vector<int>`**
- Pre: none. Post: returns exactly the columns satisfying chain rule (anchor±1), not blocked by Anchor card, not full. If anchor unset (first move) → all non-full columns.
- Errors: none (empty vector = pass required).

**C-2 `applyDrop(state, col) -> Resolution`**
- Pre: col ∈ legalDropColumns(state). Post: own stone at lowest empty cell of col; anchor=col; cascade fully resolved (settled + simultaneous flips applied to fixed point); passCounter reset.
- Invalid col → rejected with error code, state unchanged (UI re-prompts).

**C-3 `canPlayCard(state, card) -> bool`** iff EP ≥ cost AND effect preconditions hold (target stone/column exists and is legal per card definition).

**C-4 `applyCard(state, card, params) -> Resolution`**
- Pre: canPlayCard true. Post: EP deducted, card moved to discard, effect applied; if stones moved/created/removed → cascade resolved to fixed point. Shield blocks flips only, never destruction/movement.
- Double: if second drop has no legal column → no state change, no EP spent (card stays in hand).

**C-5 `endTurn(state)`** advances currentPlayer; on next startTurn, EP +3 (cap 6) and draw 1 card if deck non-empty.

**C-6 `isGameOver(state) -> {none | draw | winner}`** true iff passCounter ≥ 2 or all 64 cells occupied; winner = higher stone count.

**C-7 Determinism**: identical seed + identical move sequence → identical game replay (SC-002).

**C-8 Timing contract**: `chooseMove(state, difficulty)` returns a legal move within SC-003 bounds; AI output always passes C-1/C-3 validation.
