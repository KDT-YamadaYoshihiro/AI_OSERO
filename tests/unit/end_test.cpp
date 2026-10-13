// T013: end conditions (FR-019): consecutive passes / full board → majority count, draw
#include "../harness/test.hpp"
#include "../../src/core/rules.hpp"

using namespace gco;

int main() {
    // Pass-based game over: no legal columns (chain ±1 all full), no Jet in hand
    GameState gs = createInitialState(2);
    for (auto& c : gs.board.cells) c = CellState{};
    for (int r = 0; r < kBoardSize; ++r) {
        gs.board.cells[idx(r, 1)].color = BLACK; // fill col1 fully
        gs.board.cells[idx(r, 7)].color = WHITE; // fill col7 fully
    }
    gs.anchor = 0; // only col1 adjacent → full → no legal move
    CHECK(legalDropColumns(gs).empty());
    CHECK(!hasAnyLegalMove(gs));

    // simulate two consecutive passes → game over with majority count
    gs.passCounter = 1;
    endTurn(gs); startTurn(gs);
    gs.players[gs.current].hand.clear(); // no Jet rescue → pure pass path (FR-019)
    CHECK(!hasAnyLegalMove(gs));
    gs.passCounter = 2;
    Result r = gameOver(gs);
    CHECK(r == DRAW); // 8B vs 8W equal → draw (FR-019)
    // majority counting: remove one black stone → white wins
    gs.board.cells[idx(0, 1)] = CellState{};
    r = gameOver(gs);
    CHECK(r == WHITE_WIN);

    // Full board → game over
    GameState gsf = createInitialState(8);
    for (auto& c : gsf.board.cells) c.color = BLACK;
    CHECK(gameOver(gsf) == BLACK_WIN);

    // Jet prevents pass when available (edge case: chain dead but Jet possible)
    GameState gs2 = createInitialState(9);
    for (auto& c : gs2.board.cells) c = CellState{};
    for (int rr = 0; rr < kBoardSize; ++rr) gs2.board.cells[idx(rr, 1)].color = BLACK;
    gs2.anchor = 0;
    gs2.players[0].hand.push_back(CardType::JET);
    gs2.players[0].ep = 2;
    CHECK(hasAnyLegalMove(gs2)); // Jet rescue available

    return report();
}
