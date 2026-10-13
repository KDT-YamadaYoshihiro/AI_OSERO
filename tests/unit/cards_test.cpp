// T020: all 10 card effects + edge cases (FR-016..024)
#include "../harness/test.hpp"
#include "../../src/core/rules.hpp"

using namespace gco;

static GameState fresh(unsigned seed, int ep, std::vector<CardType> hand) {
    GameState gs = createInitialState(seed);
    startTurn(gs);
    gs.players[0].ep = ep;
    gs.players[0].hand = hand;
    return gs;
}

static void emptyBoard(GameState& gs) {
    for (auto& c : gs.board.cells) c = CellState{};
}

int main() {
    // FR-016: insufficient EP → rejected
    GameState gs = fresh(10, 1, {CardType::BOMB});
    Move m; m.kind = Move::CARD; m.card = CardType::BOMB; m.col = 0; m.r1 = 4; m.c1 = 4;
    CHECK(!canPlayCard(gs, m)); // ep 1 < cost 4
    CHECK(!applyMove(gs, m));
    CHECK(gs.players[0].ep == 1); // state unchanged on failure

    // BOMB: destroys enemy stone, triggers cascade settle+flip (FR-018)
    gs = fresh(11, 6, {CardType::BOMB});
    emptyBoard(gs);
    gs.board.cells[idx(7, 0)].color = BLACK;
    gs.board.cells[idx(7, 1)].color = WHITE;
    gs.board.cells[idx(6, 1)].color = WHITE; // stacked above → falls when bottom removed
    gs.anchor = 1; // legal {0,2}
    Move bm; bm.kind = Move::CARD; bm.card = CardType::BOMB; bm.col = 2; bm.r1 = 7; bm.c1 = 1;
    CHECK(canPlayCard(gs, bm));
    CHECK(applyMove(gs, bm));
    CHECK(gs.board.cells[idx(7, 1)].color == BLACK); // upper W fell to (7,1), then sandwiched by B(0)/B(2) → flipped
    CHECK(gs.board.cells[idx(6, 1)].color == EMPTY);
    CHECK(gs.players[0].ep == 2); // 6-4

    // BOMB destroys SHIELDED stone (edge case: shield blocks flips not destruction)
    gs = fresh(12, 6, {CardType::BOMB});
    emptyBoard(gs);
    gs.board.cells[idx(7, 3)].color = WHITE;
    gs.board.cells[idx(7, 3)].shieldUntil = 99;
    gs.anchor = 3; // legal {2,4} → col4 drop OK
    Move bms; bms.kind = Move::CARD; bms.card = CardType::BOMB; bms.col = 4; bms.r1 = 7; bms.c1 = 3;
    CHECK(applyMove(gs, bms));
    CHECK(gs.board.cells[idx(7, 3)].color == EMPTY);

    // SHIELD: blocks flip until own next turn (FR-018)
    gs = fresh(13, 6, {CardType::SHIELD});
    emptyBoard(gs);
    gs.board.cells[idx(7, 0)].color = WHITE;
    gs.board.cells[idx(7, 1)].color = BLACK; // own stone to protect
    Move sh; sh.kind = Move::CARD; sh.card = CardType::SHIELD; sh.col = 2; sh.r1 = 7; sh.c1 = 1;
    CHECK(applyMove(gs, sh)); // Black drops col2 (also creates no sandwich here)
    CHECK(gs.board.cells[idx(7, 1)].shieldUntil == gs.turnNumber + 2);
    // White's turn: drop col3 → sandwich W(0),B(1),W(3) → B protected
    endTurn(gs); startTurn(gs);
    Move w; w.kind = Move::DROP; w.col = 3;
    CHECK(applyMove(gs, w));
    CHECK(gs.board.cells[idx(7, 1)].color == BLACK); // shielded, not flipped
    // Black's next turn: shield expired → same sandwich flips it
    gs.turnNumber += 2; // simulate reaching the expiry turn
    gs.board.cells[idx(7, 2)].color = WHITE; // synthetic sandwich around (7,1)
    resolveCascade(gs, { idx(7, 2) });
    CHECK(gs.board.cells[idx(7, 1)].color == WHITE); // now flips

    // GUST: move top stone sideways + cascade (FR-018)
    gs = fresh(14, 6, {CardType::GUST});
    emptyBoard(gs);
    gs.board.cells[idx(7, 2)].color = BLACK;
    gs.board.cells[idx(6, 2)].color = WHITE; // top of col2
    gs.anchor = 2; // legal {1,3}
    Move gu; gu.kind = Move::CARD; gu.card = CardType::GUST; gu.col = 3; gu.srcCol = 2; gu.dstCol = 1;
    CHECK(applyMove(gs, gu));
    CHECK(gs.board.cells[idx(6, 2)].color == EMPTY);
    CHECK(gs.board.cells[idx(7, 1)].color == WHITE); // moved W fell to bottom of col1

    // MAGNET: move opponent top stone (FR-018)
    gs = fresh(15, 6, {CardType::MAGNET});
    emptyBoard(gs);
    gs.board.cells[idx(7, 4)].color = WHITE;
    gs.board.cells[idx(6, 4)].color = WHITE; // top = (6,4)
    gs.anchor = 3; // legal {2,4}... col4 has stones but not full
    Move mg; mg.kind = Move::CARD; mg.card = CardType::MAGNET; mg.col = 2; mg.srcCol = 4; mg.dstCol = 6;
    CHECK(applyMove(gs, mg));
    CHECK(gs.board.cells[idx(7, 6)].color == WHITE); // moved stone fell to col6 bottom

    // DOUBLE: two drops (FR-017)
    gs = fresh(16, 6, {CardType::DOUBLE});
    emptyBoard(gs);
    Move dbl; dbl.kind = Move::DOUBLE; dbl.card = CardType::DOUBLE; dbl.col = 3; dbl.col2 = 4;
    CHECK(applyMove(gs, dbl));
    CHECK(gs.board.cells[idx(7, 3)].color == BLACK);
    CHECK(gs.board.cells[idx(7, 4)].color == BLACK);
    CHECK(gs.players[0].ep == 2);

    // DOUBLE edge: second drop illegal → NOT played, EP refunded (FR-017)
    gs = fresh(17, 6, {CardType::DOUBLE});
    emptyBoard(gs);
    for (int r = 0; r < kBoardSize; ++r) gs.board.cells[idx(r, 4)].color = WHITE; // col4 full
    Move dbl2; dbl2.kind = Move::DOUBLE; dbl2.card = CardType::DOUBLE; dbl2.col = 3; dbl2.col2 = 4;
    CHECK(!canPlayCard(gs, dbl2));
    CHECK(!applyMove(gs, dbl2));
    CHECK(gs.players[0].ep == 6); // refund: no EP spent

    // COLLAPSE: remove own bottom stone → cascade (FR-018)
    gs = fresh(18, 6, {CardType::COLLAPSE});
    emptyBoard(gs);
    gs.board.cells[idx(7, 5)].color = BLACK;
    gs.board.cells[idx(6, 5)].color = WHITE;
    gs.anchor = 4; // legal {3,5}
    Move co; co.kind = Move::CARD; co.card = CardType::COLLAPSE; co.col = 3; co.srcCol = 5;
    CHECK(canPlayCard(gs, co));
    CHECK(applyMove(gs, co));
    CHECK(gs.board.cells[idx(7, 5)].color == WHITE); // B removed, W fell down

    // SWAP: exchange vertically adjacent stones (FR-018)
    gs = fresh(19, 6, {CardType::SWAP});
    emptyBoard(gs);
    gs.board.cells[idx(7, 1)].color = BLACK;
    gs.board.cells[idx(6, 1)].color = WHITE;
    gs.anchor = 1; // legal {0,2}
    Move sw; sw.kind = Move::CARD; sw.card = CardType::SWAP; sw.col = 0; sw.r1 = 7; sw.c1 = 1; sw.r2 = 6; sw.c2 = 1;
    CHECK(applyMove(gs, sw));
    CHECK(gs.board.cells[idx(7, 1)].color == WHITE);
    CHECK(gs.board.cells[idx(6, 1)].color == BLACK);

    // CHARGE: +2 EP capped at 6 (FR-016)
    gs = fresh(20, 5, {CardType::CHARGE});
    Move ch; ch.kind = Move::CARD; ch.card = CardType::CHARGE; ch.col = 0;
    CHECK(applyMove(gs, ch));
    CHECK(gs.players[0].ep == 6); // 5+2 capped

    // Deck exhaustion: no draw when deck empty (edge case FR-014)
    gs = fresh(21, 3, {});
    gs.players[0].deck.clear();
    gs.players[1].deck.clear();
    endTurn(gs); startTurn(gs);
    CHECK(gs.players[1].hand.size() == 3); // no draw from empty deck

    return report();
}
