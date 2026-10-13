# Contract: UI (DxLib GUI) — observable screen/input behavior

**U-1 Screens**: Title(mode select: 1 CPU battle / 2 hotseat) → Difficulty select (Easy/Normal/Hard, CPU mode only) → Game screen → Result screen. Japanese text throughout (FR-025).

**U-2 Game screen layout**: 8×8 board grid; column hover highlights the landing cell preview; current player's EP gauge, score, turn number, and anchor indicator (highlighted anchor column ±1 markers) visible; own hand shown as card icons with EP costs; opponent hand hidden (count only).

**U-3 Input**: mouse click on a column = drop stone. Keys `1`–`9`, `0` = select hand card slot 1–10; then click target column/cell for targeted cards; `Enter` confirms, `Esc` cancels card selection. Invalid input (full column, out-of-chain column, insufficient EP) → Japanese error message, no state change, re-prompt (FR-025).

**U-4 Feedback**: every flip/cascade animation updates within one frame batch; AI moves appear as normal drops (no special styling beyond a "CPU" label).

**U-5 Help**: in-game `H` key shows rule summary (chain/gravity/cascade/cards) — supports SC-001 (playable in <10 min without external docs).
