# Implementation Plan: 重力カスケードオセロ (Gravity Cascade Othello)

**Branch**: `feature/gravity-cascade-othello` | **Date**: 2026-10-13 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/001-gravity-cascade-othello/spec.md`

## Summary

純C++ rule engine（gravity + chain + cascade + cards）を中心に、DxLibによるWindows GUI shell を薄層として分離する。AIは3 tier (Easy random / Normal greedy / Hard minimax alpha-beta depth 5–6)。既存 CardFlow Othello code 一式は完全置換。Testing は dependency-free custom harness で rule engine の deterministic unit tests + e2e self-play。

## Technical Context

**Language/Version**: C++20 (MSVC, `/W4 /permissive-`, 警告ゼロ)
**Primary Dependencies**: DxLib 3.25a (Windows VC++ package), Win32 API (DxLib経由), std library only
**Storage**: N/A (no persistence in v1; RNG seed for determinism)
**Testing**: custom lightweight test harness (header-only assert helpers) + ctest driver
**Target Platform**: Windows 10/11 desktop, MSVC via Visual Studio 2022
**Project Type**: single desktop application (game)
**Performance Goals**: Hard AI move < 5 s; UI 60 fps; Easy AI < 0.1 s
**Constraints**: zero compiler warnings (AGENT.md), offline build, single executable + DxLib.dll
**Scale/Scope**: ~2,000–2,500 LOC core+AI+UI, ~1,500 LOC tests

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- `.specify/memory/constitution.md` は旧 CardFlow Othello の game rules を定義。本 feature は spec 001 によりその game-rule articles (I–VII) を **supersede** する（spec Assumptions 記載）。Engineering principles（quality gates, logic/UI separation, Japanese output）は維持 → **PASS**。
- AGENT.md gates: zero-warning build → `/W4 /permissive-` + warning-as-error policy in plan; branch workflow → branch created; Japanese UI/commits → planned. **PASS**.

## Project Structure

### Documentation (this feature)

```text
specs/001-gravity-cascade-othello/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   ├── rule-engine.md
│   └── ui-contract.md
└── tasks.md             (created by /speckit-tasks)
```

### Source Code (repository root)

```text
third_party/dxlib/          # DxLib.h, DxLib.lib, DxLib.dll (downloaded package)
src/
├── core/                 # pure rule engine — NO DxLib include here
│   ├── board.hpp/.cpp    # Cell/Board, gravity settle, flip resolution
│   ├── state.hpp/.cpp    # GameState: players, EP, deck/hand, anchor, passes
│   ├── rules.hpp/.cpp    # legal moves, cascade resolution, applyMove
│   └── rng.hpp           # seeded std::mt19937 wrapper
├── cards/
│   ├── card.hpp          # CardType enum, EP costs, definitions
│   └── effects.cpp       # 10 pure-effect implementations
├── ai/
│   ├── ai.hpp            # Difficulty enum, chooseMove interface
│   └── ai.cpp            # Easy/Normal/Hard implementations (minimax depth 5–6)
├── ui/
│   ├── render.cpp        # DxLib drawing (board, hand, EP, anchor indicator)
│   ├── input.cpp         # mouse column click + keys 1-9,0
│   └── screens.cpp       # title/mode/difficulty/game/result screens
├── main.cpp              # game loop wiring core ↔ ui
tests/
├── harness/test.hpp      # minimal assert/report helpers
├── unit/
│   ├── board_test.cpp        # gravity settle, flip detection
│   ├── chain_test.cpp        # anchor legality, Jet bypass, Anchor block
│   ├── cascade_test.cpp      # gap → settle → auto-flip fixed point
│   ├── cards_test.cpp        # all 10 card effects incl. edge cases
│   └── ai_test.cpp           # legality + response time bounds
├── e2e/
│   └── game_flow_test.cpp    # seeded self-play Hard vs Easy → valid result
└── CMakeLists.txt
CMakeLists.txt              # root: app + tests targets, DxLib link
```

**Structure Decision**: single application project with strict layering `core → cards/ai → ui`. Core is UI-free and fully unit-testable (AGENT.md §5 MVC separation requirement).

## Complexity Tracking

No constitution violations to justify. Layer count (core/cards/ai/ui) matches existing repo conventions.
