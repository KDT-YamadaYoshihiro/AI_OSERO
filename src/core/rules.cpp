// 規則引擎実装: 連鎖 legality, drop, cascade resolution, game over
#include "rules.hpp"
#include <algorithm>
#include <cstdlib>

namespace gco {

std::vector<int> legalDropColumns(const GameState& gs) {
    std::vector<int> res;
    for (int c = 0; c < kBoardSize; ++c) {
        if (gs.board.columnFull(c)) continue;
        if (gs.blockedCol == c && gs.turnNumber == gs.blockedForTurn) continue; // 錨 (FR-024)
        if (!gs.anchor.has_value()) { res.push_back(c); continue; } // 初手任意列 (FR-009)
        int a = *gs.anchor;
        if (c != a && std::abs(c - a) <= 1) res.push_back(c); // anchor±1のみ (FR-007)
    }
    return res;
}

bool hasAnyLegalMove(const GameState& gs) {
    if (!legalDropColumns(gs).empty()) return true;
    // Jetで連鎖 bypass可能か (edge case: 全列 illegal → pass)
    const PlayerState& p = gs.players[gs.current];
    for (CardType c : p.hand) {
        if (c == CardType::JET && p.ep >= cardCost(CardType::JET)) {
            for (int col = 0; col < kBoardSize; ++col) {
                if (!gs.board.columnFull(col) &&
                    !(gs.blockedCol == col && gs.turnNumber == gs.blockedForTurn)) return true;
            }
        }
    }
    return false;
}

// R5: settle → 同時flip → fixed point (flipは石を動かさないため1回で收敛)
void resolveCascade(GameState& gs, const std::vector<int>& touched) {
    static const int DR[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    static const int DC[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };

    std::vector<int> moved = gs.board.settle();
    std::vector<int> active = touched;
    active.insert(active.end(), moved.begin(), moved.end());

    bool flipSet[kBoardSize * kBoardSize] = {};
    for (int pos : active) {
        Cell m = gs.board.cells[pos].color;
        if (m == EMPTY) continue;
        int r = pos / kBoardSize, c = pos % kBoardSize;
        for (int d = 0; d < 8; ++d) {
            int nr = r + DR[d], nc = c + DC[d];
            std::vector<int> chain;
            while (inBounds(nr, nc) && gs.board.cells[idx(nr, nc)].color == opposite(m)) {
                chain.push_back(idx(nr, nc));
                nr += DR[d]; nc += DC[d];
            }
            if (!chain.empty() && inBounds(nr, nc) && gs.board.cells[idx(nr, nc)].color == m) {
                for (int f : chain) flipSet[f] = true; // 同時apply (FR-011)
            }
        }
    }
    for (int i = 0; i < kBoardSize * kBoardSize; ++i) {
        if (flipSet[i] && gs.board.cells[i].shieldUntil <= gs.turnNumber) {
            gs.board.cells[i].color = opposite(gs.board.cells[i].color); // 盾は裏返防止 (FR-018)
        }
    }
}

void applyDropOnly(GameState& gs, int col) {
    int r = gs.board.lowestEmpty(col);
    if (r < 0) return;
    gs.board.cells[idx(r, col)].color = gs.current == 0 ? BLACK : WHITE;
    gs.anchor = col;       // anchor = そのturn dropした列へ更新 (FR-008)
    gs.passCounter = 0;
    resolveCascade(gs, { idx(r, col) });
}

bool applyMove(GameState& gs, const Move& mv) {
    GameState copy = gs; // 検証はcopy上で → 失敗時state unchanged (C-2/C-4)
    if (mv.card != CardType::NONE) {
        if (!canPlayCard(copy, mv)) return false;
        copy.players[copy.current].ep -= cardCost(mv.card);
        auto& hand = copy.players[copy.current].hand;
        auto it = std::find(hand.begin(), hand.end(), mv.card);
        if (it == hand.end()) return false;
        hand.erase(it);
        copy.players[copy.current].discard.push_back(mv.card); // card → discard
        applyCardEffect(copy, mv); // JET/DOUBLEはdrop込み効果; CARD kindsは純効果
    }
    if (mv.kind == Move::DROP || mv.kind == Move::CARD) {
        auto legal = legalDropColumns(copy);
        if (std::find(legal.begin(), legal.end(), mv.col) == legal.end()) return false;
        applyDropOnly(copy, mv.col);
    }
    gs = std::move(copy);
    return true;
}

void startTurn(GameState& gs) {
    ++gs.turnNumber;
    PlayerState& p = gs.players[gs.current];
    p.ep = std::min(6, p.ep + 3); // EP +3/turn cap 6 (FR-015)
    if (!p.deck.empty() && p.hand.size() < 10) {
        p.hand.push_back(p.deck.back());
        p.deck.pop_back();
    }
}

void endTurn(GameState& gs) { gs.current ^= 1; }

Result gameOver(const GameState& gs) {
    if (gs.passCounter >= 2 || gs.board.boardFull()) { // FR-019
        int b = gs.board.countStones(BLACK);
        int w = gs.board.countStones(WHITE);
        if (b > w) return BLACK_WIN;
        if (w > b) return WHITE_WIN;
        return DRAW;
    }
    return NONE;
}

} // namespace gco
