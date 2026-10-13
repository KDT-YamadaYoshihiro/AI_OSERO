# Tasks: 重力カスケードオセロ (Gravity Cascade Othello)

**Input**: Design documents from `specs/001-gravity-cascade-othello/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md

**Status (2026-10-13)**: 31/34 tasks done — ctest 7 suites 100% PASS, GUI exe runs. Deviations: UI implemented in single `src/main.cpp` (no separate ui/ files); Hard tier fixed depth 5 (no iterative deepening); T010/T019/T026 skipped as covered by existing ai/selfplay tests.

**Tests**: INCLUDED — repo convention is TDD (AGENT.md §3 zero-defect gate; SC-004 requires automated coverage). Tests written first, failing, then implemented.

## Phase 1: Setup (Shared Infrastructure)

- [x] T001 Remove legacy CardFlow Othello code: delete `src/`, `include/`, `tests/`, `cardflow_othello.py` (full replacement per user decision)
- [x] T002 Extract DxLib VC++ package into `third_party/dxlib/` (DxLib.h, DxLib.lib, DxLib.dll) from downloaded zip
- [x] T003 [P] Create root `CMakeLists.txt`: MSVC flags `/W4 /permissive- /utf-8`, targets `gravitycascade` (app) + test executables, link DxLib.lib, copy DxLib.dll next to exe
- [x] T004 [P] Create test harness `tests/harness/test.hpp` (CHECK macro, suite runner, exit code) per research R4
- [x] T005 Update `.gitignore`: add `build/`, `third_party/dxlib/*.zip`, `.vs/`

## Phase 2: Foundational (Blocking Prerequisites)

**CRITICAL**: No user story work can begin until this phase is complete.

- [x] T006 Create `src/core/types.hpp`: Cell enum {Empty,Black,White}, kBoardSize=8 constant, index helpers
- [x] T007 [P] Implement `src/core/board.hpp/.cpp`: 64-cell array, lowestEmpty(col), topIndex(col), settle() returning moved cells (FR-004, data-model invariants)
- [x] T008 [P] Implement `src/core/state.hpp/.cpp`: GameState (players EP 0..6, deck 20 cards, hand 3, anchor optional<int>, blockedColumn+expiry, passCounter, turnNumber), seeded RNG wrapper `src/core/rng.hpp` (SC-002 determinism)
- [x] T009 Implement initial setup: standard 4 center stones settled by gravity into columns 3/4 as stacks col3 B(bottom)/W(top), col4 W(bottom)/B(top) (FR-002)
- [ ] T010 Wire a console smoke-test executable `tests/unit/foundation_smoke.cpp` that constructs initial state and prints ASCII board

**Checkpoint**: engine skeleton + test/build infrastructure functional (app may not yet run in GUI).

## Phase 3: User Story 1 — 重力連鎖 core game vs CPU (Priority: P1) MVP

**Goal**: Human can play a complete gravity-chain match vs Easy CPU to a valid result.
**Independent Test**: run app, play a full match with cards disabled (empty hand), verify win/draw detection.

### Tests for User Story 1 (write first, must fail)
- [x] T011 [P] [US1] Chain/anchor tests in `tests/unit/chain_test.cpp`: first move any column (FR-009); legal set = anchor±1 only, same column illegal (FR-007); anchor updates to dropped column every turn (FR-008)
- [x] T012 [P] [US1] Flip/cascade tests in `tests/unit/flip_test.cpp`: 8-direction sandwich detection (FR-011); pure drop with no sandwich → no flip, anchor still advances; simultaneous application per step + fixed-point convergence (R5)
- [x] T013 [P] [US1] End-condition tests in `tests/unit/end_test.cpp`: consecutive passes → game over + majority count (FR-019); full board → game over; draw on equal counts

### Implementation for User Story 1
- [x] T014 [US1] Implement `src/core/rules.hpp/.cpp`: legalDropColumns(), applyDrop() with cascade resolution to fixed point, pass handling, isGameOver (contracts C-1, C-2, C-6)
- [x] T015 [US1] Implement `src/ai/ai.hpp/.cpp` Easy tier: uniform random legal drop via seeded RNG (FR-022, contract C-8)
- [x] T016 [US1] Implement `src/ui/render.cpp`: DxLib window 1024×768, board grid, stones as circles, landing preview on column hover, anchor ±1 indicator, EP/score/turn HUD (contracts U-1, U-2)
- [x] T017 [US1] Implement `src/ui/input.cpp` + `src/ui/screens.cpp`: title → difficulty select → game loop → result screen; mouse click = drop; invalid input → Japanese error message, state unchanged (FR-025, U-1, U-3)
- [x] T018 [US1] Implement `src/main.cpp`: wire core ↔ ui ↔ ai for CPU mode; hand starts empty so US1 runs without cards
- [ ] T019 [US1] Verify US1 acceptance scenarios 1–5 via scripted e2e test `tests/e2e/game_flow_test.cpp` (seeded, engine-only, no GUI)

**Checkpoint**: playable MVP vs Easy CPU; all US1 tests green.

## Phase 4: User Story 2 — スキルカードとEP system (Priority: P2)

**Goal**: Full card economy: EP +3/turn cap 6, deck/hand/draw, 10 effects incl. cascade triggers.
**Independent Test**: unit-test every card effect against hand-built board states; play a match using cards.

### Tests for User Story 2 (write first, must fail)
- [x] T020 [P] [US2] Card-effect tests `tests/unit/cards_test.cpp`: Jet bypasses chain & updates anchor (FR-017); Anchor blocks column one opponent turn; Gust top-stone move + cascade; Bomb destroys any enemy stone incl. shielded (edge case); Shield blocks flips until own next turn but not destruction; Magnet moves opponent top stone; Double refunds EP when second drop illegal; Collapse removes chosen own bottom stone → cascade; Swap exchanges adjacent stones then resolves; Charge +2 capped at 6; EP validation rejects unaffordable cards (FR-016); deck exhaustion skips draw (edge case)

### Implementation for User Story 2
- [x] T021 [US2] Implement `src/cards/card.hpp`: CardType enum, EP costs table, card definitions (data-model table verbatim)
- [x] T022 [US2] Implement `src/cards/effects.cpp`: canPlayCard + applyCard pure functions per contract C-3/C-4, cascade via rules.resolve
- [x] T023 [US2] Turn economy: startTurn EP +3 cap 6, draw 1 from deck if non-empty (FR-015) in `src/core/rules.cpp`
- [x] T024 [US2] Card UI: hand icons with EP cost, keys 1–9/0 select, target selection for targeted cards, discard tracking, opponent hand hidden (count only) (U-2, U-3, FR-023)

**Checkpoint**: US1 + US2 independently functional; full rule set active.

## Phase 5: User Story 3 — 2人ホットシート (Priority: P3)

**Goal**: Two humans share one machine with hidden hands.
**Independent Test**: hotseat mode, both sides human input, hand visibility toggles per turn.

- [x] T025 [P] [US3] Hotseat flow in `src/ui/screens.cpp`: mode 2 at title sets both players human; hide non-current player's hand each turn (FR-021, FR-023)
- [ ] T026 [US3] Hotseat e2e scripted test in `tests/e2e/hotseat_test.cpp` with two scripted move sequences verifying pass/turn logic

**Checkpoint**: both modes work.

## Phase 6: User Story 4 — CPU difficulty 3 level (Priority: P4)

**Goal**: Normal and Hard tiers meet SC-003 timing and legality.
**Independent Test**: run AI on fixed positions; assert legality + time bounds.

- [x] T027 [P] [US4] Normal tier in `src/ai/ai.cpp`: 1-ply lookahead maximizing stone delta after full cascade; card use when immediate gain exceeds EP-adjusted threshold (FR-022, R6)
- [x] T028 [US4] Hard tier in `src/ai/ai.cpp`: alpha-beta minimax depth 5–6 with iterative deepening and 5 s cap; move set = legal drops ∪ top-4 card combos; eval = stone diff + mobility + EP (R6)
- [x] T029 [US4] AI tests in `tests/unit/ai_test.cpp`: every returned move passes contract C-1/C-3 validation; Easy < 0.1 s, Normal < 1 s, Hard < 5 s on seeded mid-game positions (SC-003)

**Checkpoint**: all four user stories functional.

## Phase 7: Polish & Cross-Cutting Concerns

- [x] T030 [P] In-game help screen `H` key: rules summary in Japanese (SC-001, U-5)
- [x] T031 [P] Seeded self-play stress test `tests/e2e/selfplay_test.cpp`: Hard vs Easy, 3 seeds, valid result < 60 s each (SC-004)
- [x] T032 Zero-warning verification: build with `/W4 /permissive-` and fix all warnings (SC-005, AGENT.md gate)
- [x] T033 Update `README.md`: new game rules summary, build/run/test instructions from quickstart.md
- [x] T034 Run quickstart.md validation end-to-end (build → ctest → launch exe manually via screenshot check if possible)

## Dependencies & Execution Order

- Phase 1 → Phase 2 (blocking) → US1 → {US2, US3, US4 in priority order; US3/US4 may overlap after US2's state fields exist}
- Within stories: tests first (RED), then implementation (GREEN).
- Parallel: T003‖T004‖T005; T007‖T008; T011‖T012‖T013; T020 standalone.

## Implementation Strategy

MVP = Phases 1–3 (playable core match). Then cards (P2), hotseat (P3), AI tiers (P4), polish. Commit per phase on `feature/gravity-cascade-othello`; PR after Phase 7 (AGENT.md §2).
