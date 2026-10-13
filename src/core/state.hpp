// GameState: 盤 + player状態 + 連鎖anchor + EP + deck/hand (data-model.md)
#pragma once
#include <algorithm>
#include <array>
#include <optional>
#include <random>
#include <vector>
#include "board.hpp"
#include "../cards/card.hpp"

namespace gco {

struct PlayerState {
    int ep = 0;
    std::vector<CardType> deck;
    std::vector<CardType> hand;
    std::vector<CardType> discard;
};

struct GameState {
    Board board;
    PlayerState players[2];      // 0=BLACK(先手), 1=WHITE
    int current = 0;
    std::optional<int> anchor;   // 連鎖anchor列 (FR-007/008)
    int blockedCol = -1;         // 錨カード封锁列 (FR-024)
    int blockedForTurn = 0;
    int passCounter = 0;
    int turnNumber = 0;
    std::mt19937 rng;
};

// 初期状態: 中心4石が重力でcol3/4へ落下 (FR-002), 确定性seed (SC-002)
inline GameState createInitialState(unsigned seed) {
    GameState gs;
    gs.rng.seed(seed);
    // 標準Othello中心四石 → 重力で列底部へstack
    gs.board.cells[idx(4, 3)].color = WHITE; // 落下後 col3: B(bottom), W(top)
    gs.board.cells[idx(4, 4)].color = BLACK; // col4: W(bottom), B(top)
    gs.board.cells[idx(5, 3)].color = BLACK;
    gs.board.cells[idx(5, 4)].color = WHITE;
    gs.board.settle();

    for (int p = 0; p < 2; ++p) {
        for (int t = 1; t <= 10; ++t) {
            gs.players[p].deck.push_back(static_cast<CardType>(t));
            gs.players[p].deck.push_back(static_cast<CardType>(t)); // 10種×2枚=20
        }
        std::shuffle(gs.players[p].deck.begin(), gs.players[p].deck.end(), gs.rng);
        for (int i = 0; i < 3; ++i) {
            gs.players[p].hand.push_back(gs.players[p].deck.back());
            gs.players[p].deck.pop_back();
        }
    }
    return gs;
}

} // namespace gco
