// T012: 8方向sandwich flip + cascade fixed point (FR-011, R5)
#include "../harness/test.hpp"
#include "../../src/core/rules.hpp"

using namespace gco;

int main() {
    // Test 1: plain drop sandwich → flip (FR-011)
    GameState gs = createInitialState(3);
    // row7: B(0), W(1) pre-placed, Black drops at col2 → W(1) sandwiched B..B
    gs.board.cells[idx(7, 0)].color = BLACK;
    gs.board.cells[idx(7, 1)].color = WHITE;
    Move m; m.kind = Move::DROP; m.col = 2;
    CHECK(applyMove(gs, m));
    CHECK(gs.board.cells[idx(7, 1)].color == BLACK); // flipped
    CHECK(gs.anchor.has_value() && *gs.anchor == 2); // anchor advances even w/o flip (FR-008)

    // Test 2: pure drop with no sandwich → no flip, anchor still advances
    GameState gs2 = createInitialState(4);
    Move m2; m2.kind = Move::DROP; m2.col = 7;
    CHECK(applyMove(gs2, m2));
    CHECK(gs2.board.cells[idx(7, 7)].color == BLACK);
    CHECK(gs2.anchor.has_value() && *gs2.anchor == 7);
    int total = gs2.board.countStones(BLACK) + gs2.board.countStones(WHITE);
    CHECK(total == 5); // no flip: still 2B+2W+1new

    // Test 3: one drop sandwiches a chain of 2 enemy stones → both flip simultaneously (FR-011)
    GameState gs3 = createInitialState(5);
    for (auto& c : gs3.board.cells) c = CellState{}; // empty board for controlled test
    gs3.board.cells[idx(7, 1)].color = WHITE;
    gs3.board.cells[idx(7, 2)].color = WHITE;
    gs3.board.cells[idx(7, 0)].color = BLACK; // anchor side
    Move m3; m3.kind = Move::DROP; m3.col = 3; // Black drops at (7,3)
    CHECK(applyMove(gs3, m3));
    CHECK(gs3.board.cells[idx(7, 1)].color == BLACK);
    CHECK(gs3.board.cells[idx(7, 2)].color == BLACK);

    // Deterministic cascade: gap → settle → landing creates sandwich → auto-flip (FR-012/018)
    GameState gs4 = createInitialState(6);
    for (auto& c : gs4.board.cells) c = CellState{};
    gs4.board.cells[idx(7, 0)].color = BLACK;
    gs4.board.cells[idx(5, 1)].color = WHITE; // floating stone (gap below)
    gs4.board.cells[idx(3, 1)].color = BLACK; // second stone above it
    resolveCascade(gs4, {});
    CHECK(gs4.board.cells[idx(7, 1)].color == WHITE); // gravity: bottom stone lands at row 7
    CHECK(gs4.board.cells[idx(6, 1)].color == BLACK); // stack preserved bottom-to-top
    // now simulate a black stone landing at (7,2): W at (7,1) sandwiched by B(7,0)/B(7,2) → flips
    gs4.board.cells[idx(7, 2)].color = BLACK;
    resolveCascade(gs4, { idx(7, 2) });
    CHECK(gs4.board.cells[idx(7, 1)].color == BLACK); // cascade flip triggered

    // Full cascade in ONE resolution: floating stone falls AND flips a sandwich (R5)
    GameState gs5 = createInitialState(7);
    for (auto& c : gs5.board.cells) c = CellState{};
    gs5.board.cells[idx(7, 1)].color = BLACK;
    gs5.board.cells[idx(7, 2)].color = WHITE;
    gs5.board.cells[idx(5, 3)].color = BLACK; // floating → falls to (7,3), sandwiching W at (7,2)
    resolveCascade(gs5, {}); // settle + flip in one fixed point
    CHECK(gs5.board.cells[idx(7, 3)].color == BLACK); // landed
    CHECK(gs5.board.cells[idx(7, 2)].color == BLACK); // flipped by cascade

    return report();
}
