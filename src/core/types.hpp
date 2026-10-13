// 重力カスケードオセロ: 基本型定義 (core types)
#pragma once
#include <array>
#include <cstdint>

namespace gco {

enum Cell : int { EMPTY = 0, BLACK = 1, WHITE = 2 };

// AGENT.md §6: 盤サイズ拡張可能性 — 定数のみ変更で対応
constexpr int kBoardSize = 8;

inline int idx(int r, int c) { return r * kBoardSize + c; }
inline bool inBounds(int r, int c) { return r >= 0 && r < kBoardSize && c >= 0 && c < kBoardSize; }
inline Cell opposite(Cell c) { return c == BLACK ? WHITE : BLACK; }

struct CellState {
    Cell color = EMPTY;
    int shieldUntil = 0; // 盾: turnNumber < shieldUntil の間裏返されない (FR-018)
};

} // namespace gco
