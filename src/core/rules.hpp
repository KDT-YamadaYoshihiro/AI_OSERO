// 規則引擎: 連鎖/重力/cascade/turn mechanics (contracts/rule-engine.md C-1..C-6)
#pragma once
#include <vector>
#include "state.hpp"

namespace gco {

struct Move {
    enum Kind { DROP, CARD, JET, DOUBLE, PASS } kind = DROP; // CARD = 効果カード + drop; PASS = 連鎖/列全満でパス (FR-019)
    CardType card = CardType::NONE;
    int col = -1;    // drop/Jet列 / Double第一列 / 錨封锁列
    int col2 = -1;   // Double第二列
    int srcCol = -1; // 風/磁石 source
    int dstCol = -1; // 風/磁石 destination
    int r1 = -1, c1 = -1, r2 = -1, c2 = -1; // 爆弾/盾/交換 target cells
};

// C-1: 連鎖anchor±1のみ可 (同列不可), 錨封锁列除外, 満列除外 (FR-007)
std::vector<int> legalDropColumns(const GameState& gs);
bool hasAnyLegalMove(const GameState& gs); // drop or Jet (edge case FR-019)
bool canPlayCard(const GameState& gs, const Move& mv); // effects.cpp (C-3)
bool applyMove(GameState& gs, const Move& mv);         // C-2/C-4: 全turn move (C-4)

enum Result { NONE = 0, DRAW = 1, BLACK_WIN = 2, WHITE_WIN = 3 };
Result gameOver(const GameState& gs); // C-6

void startTurn(GameState& gs); // EP+3 cap6 + draw (FR-015)
void endTurn(GameState& gs);

// internal helpers (implemented in rules.cpp / effects.cpp)
void applyDropOnly(GameState& gs, int col);
void resolveCascade(GameState& gs, const std::vector<int>& touched); // R5 fixed point
void applyCardEffect(GameState& gs, const Move& mv);
} // namespace gco
