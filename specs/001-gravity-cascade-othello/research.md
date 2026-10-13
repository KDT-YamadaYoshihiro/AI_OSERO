# Phase 0 Research: 重力カスケードオセロ

All NEEDS CLARIFICATION items resolved; game-rule decisions are already fixed by the interview (see spec.md Clarifications table). Remaining unknowns were technical.

## R1. Build system & toolchain
- **Decision**: CMake ≥3.15 with MSVC generator (`cmake -B build -G "Visual Studio 17 2022"`), link `DxLib.lib`, ship `DxLib.dll` beside the executable. Compile flags `/W4 /permissive- /utf-8`.
- **Rationale**: VS2022 is installed; CMake already exists in repo; DxLib VC++ package ships prebuilt lib/dll/header.
- **Alternatives**: raw .vcxproj (rejected — existing project uses CMake); MinGW (rejected — AGENT.md targets MSVC/MSBuild).

## R2. DxLib acquisition
- **Decision**: download official VC++ package `https://dxlib.xsrv.jp/DxLib/DxLib_VC3_25a.zip` (~206 MB), extract into `third_party/dxlib/`.
- **Rationale**: official current version 3.25a; verified reachable from this machine.
- **Alternatives**: older mirrors (vector.co.jp) — rejected, older versions.

## R3. Japanese text rendering in DxLib
- **Decision**: use `CreateFontToHandle("Meiryo", 16, -1)` + `DrawFormatStringToHandle` for all UI text; source files UTF-8 with BOM and `/utf-8` flag.
- **Rationale**: DxLib supports font handles and string drawing (verified in official reference); Meiryo is standard on Windows 10/11.
- **Risk & fallback**: if glyph rendering garbles under MSVC UTF-8, switch string literals to Shift-JIS (AGENT.md allows CP932). Validation step added to quickstart.

## R4. Test framework
- **Decision**: dependency-free header-only test harness (`tests/harness/test.hpp`) with `CHECK(cond)` macros, per-suite main(), driven by CTest.
- **Rationale**: gtest/catch2 via pkg-config is impractical on MSVC; FetchContent needs network at build time; zero-warning requirement is easier to guarantee with a ~100-line harness. Rule engine is pure functions → trivial to test.
- **Alternatives considered**: Google Test (repo precedent via pkg-config — not available on Windows), Catch2 single-header (adds compile weight, marginal benefit).

## R5. Cascade resolution semantics (core novelty)
- **Decision**: after any placement/movement/removal: (1) gravity-settle each column compacting stones downward; (2) detect all sandwich flips simultaneously using the pre-flip board; (3) apply flips atomically; (4) repeat only if a settle moved stones. Termination is guaranteed: settles strictly decrease "gap count" and flips are finite per step.
- **Rationale**: simultaneous application avoids order-dependent ambiguity (spec Edge Case "cascade infinite loop").
- **Alternatives**: sequential per-stone flipping — rejected as order-sensitive and harder to reason about.

## R6. AI design
- **Decision**: Easy = uniform random legal drop, no cards. Normal = 1-ply lookahead maximizing stone delta after full cascade resolution; uses cards only when immediate gain > EP-adjusted threshold. Hard = alpha-beta minimax depth 5–6 over moves {legal drops} ∪ {top-4 card+drop combos}, evaluation = stone diff + mobility + EP, iterative deepening with 5 s time cap.
- **Rationale**: branching factor ≈ 2 drops × ≤5 card options ≈ 10; depth 6 is tractable on a modern PC within the SC-003 budget.
- **Alternatives**: MCTS (rejected — overkill for 8×8 with deterministic rules).

## R7. Determinism & RNG
- **Decision**: `GameState` owns a `std::mt19937` seeded from a seed passed at construction; all draws/AI randomness route through it.
- **Rationale**: SC-002 (reproducible replays) and testability.

## R8. Board representation
- **Decision**: `std::array<uint8_t, 64>` with per-cell shield flag in a parallel bitmask; constants `kBoardSize = 8` (AGENT.md §6 extensibility).
- **Rationale**: cache-friendly, trivially serializable for tests.

## Constitution Re-check (post-design)
No violations introduced: core/UI separation honored; zero-warning build strategy defined; Japanese UI confirmed. PASS.
