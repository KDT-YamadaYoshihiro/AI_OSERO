// T029: AI legality, determinism, no-floating invariant across difficulties (FR-021/022)
#include "../harness/test.hpp"
#include "../../src/core/rules.hpp"
#include "../../src/ai/ai.hpp"

using namespace gco;

static bool noFloating(const GameState& gs) {
    for (int c = 0; c < kBoardSize; ++c) {
        int count = 0;
        for (int r = 7; r >= 0; --r) if (gs.board.cells[idx(r, c)].color != EMPTY) ++count;
        for (int r = 7; r > 7 - count; --r) if (gs.board.cells[idx(r, c)].color == EMPTY) return false;
        for (int r = 7 - count; r >= 0; --r) if (gs.board.cells[idx(r, c)].color != EMPTY) return false;
    }
    return true;
}

int main() {
    for (int diff = 0; diff <= 2; ++diff) {
        GameState gs = createInitialState(100 + diff);
        int moves = 0, passes = 0;
        while (gameOver(gs) == NONE && moves + passes < 80) {
            startTurn(gs);
            Move mv = chooseMove(gs, static_cast<Difficulty>(diff));
            if (mv.kind == Move::PASS) { ++passes; gs.passCounter++; }
            else { CHECK(applyMove(gs, mv)); ++moves; gs.passCounter = 0; }
            CHECK(noFloating(gs)); // FR-018 invariant: no floating stones after any move
            endTurn(gs);
        }
        CHECK(moves > 0);
        CHECK(gameOver(gs) != NONE || moves + passes >= 80);
    }

    // Determinism: same seed + same difficulty → same first move (FR-025 spirit)
    GameState g1 = createInitialState(55), g2 = createInitialState(55);
    startTurn(g1); startTurn(g2);
    Move m1 = chooseMove(g1, HARD), m2 = chooseMove(g2, HARD);
    CHECK(m1.kind == m2.kind && m1.col == m2.col);

    return report();
}
