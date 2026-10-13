// T007+T009: gravity settle (FR-005) + initial setup (FR-006) + deck/hand (FR-014)
#include "../harness/test.hpp"
#include "../../src/core/rules.hpp"

using namespace gco;

int main() {
    // FR-006: initial 4 stones settle to bottom of columns 3/4
    GameState gs = createInitialState(42);
    CHECK(gs.board.cells[idx(7, 3)].color == BLACK);
    CHECK(gs.board.cells[idx(6, 3)].color == WHITE);
    CHECK(gs.board.cells[idx(7, 4)].color == WHITE);
    CHECK(gs.board.cells[idx(6, 4)].color == BLACK);
    CHECK(gs.board.lowestEmpty(3) == 5);
    CHECK(gs.board.topRow(3) == 6);
    CHECK(!gs.board.columnFull(3));
    CHECK(!gs.board.boardFull());
    CHECK(gs.board.countStones(BLACK) == 2);
    CHECK(gs.board.countStones(WHITE) == 2);

    // FR-014: per-player deck 20 (10 types x2), initial hand 3
    CHECK(gs.players[0].deck.size() + gs.players[0].hand.size() == 20);
    CHECK(gs.players[0].hand.size() == 3);
    CHECK(gs.players[1].deck.size() + gs.players[1].hand.size() == 20);

    // Deterministic settle with gaps (FR-005)
    GameState gs2 = createInitialState(1);
    for (auto& c : gs2.board.cells) c = CellState{};
    gs2.board.cells[idx(4, 2)].color = BLACK; // floating stones in col2
    gs2.board.cells[idx(2, 2)].color = WHITE;
    auto moved = gs2.board.settle();
    CHECK(moved.size() == 2);
    CHECK(gs2.board.cells[idx(7, 2)].color == BLACK); // stack order preserved bottom-to-top
    CHECK(gs2.board.cells[idx(6, 2)].color == WHITE);
    CHECK(gs2.board.cells[idx(4, 2)].color == EMPTY);
    CHECK(gs2.board.settle().empty()); // idempotent

    // Reproducibility: same seed → identical state (FR-025)
    GameState a = createInitialState(777), b = createInitialState(777);
    for (int i = 0; i < kBoardSize * kBoardSize; ++i) CHECK(a.board.cells[i].color == b.board.cells[i].color);
    CHECK(a.players[0].hand == b.players[0].hand);

    return report();
}
