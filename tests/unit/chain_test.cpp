// T011: 連鎖anchor legality (FR-007/008/009) + Jet bypass + Anchor block
#include "../harness/test.hpp"
#include "../../src/core/rules.hpp"

using namespace gco;

int main() {
    // FR-014: initial hand 3 cards, +1 draw per turn
    GameState gs = createInitialState(1);
    CHECK(gs.players[0].hand.size() == 3);
    startTurn(gs);
    CHECK(gs.turnNumber == 1);
    CHECK(gs.players[0].ep == 3);            // EP +3/turn (FR-015)
    CHECK(gs.players[0].hand.size() == 4);   // turn draw
    auto legal = legalDropColumns(gs);
    CHECK(legal.size() == 8);

    // FR-007: after drop col3 → only {2,4} legal; same column illegal
    Move m; m.kind = Move::DROP; m.col = 3;
    CHECK(applyMove(gs, m));
    CHECK(gs.anchor.has_value() && *gs.anchor == 3); // FR-008
    auto legal2 = legalDropColumns(gs);
    CHECK(legal2.size() == 2 && legal2[0] == 2 && legal2[1] == 4);
    Move same; same.kind = Move::DROP; same.col = 3;
    CHECK(!applyMove(gs, same)); // same column illegal (FR-007)

    // FR-008: anchor advances every turn even without flips
    endTurn(gs); startTurn(gs);
    Move m2; m2.kind = Move::DROP; m2.col = 2;
    CHECK(applyMove(gs, m2));
    CHECK(gs.anchor.has_value() && *gs.anchor == 2);

    // Jet bypasses chain and updates anchor (FR-017) — Black's turn now
    endTurn(gs); startTurn(gs);
    gs.players[0].hand.push_back(CardType::JET);
    gs.players[0].ep = 6;
    Move jet; jet.kind = Move::JET; jet.card = CardType::JET; jet.col = 7;
    CHECK(canPlayCard(gs, jet));
    CHECK(applyMove(gs, jet));
    CHECK(gs.anchor.has_value() && *gs.anchor == 7);
    CHECK(gs.players[0].ep == 4); // EP deducted by 2

    // Anchor card: blocks column for opponent's next turn only (FR-024)
    endTurn(gs); startTurn(gs); // White's turn, anchor=7 → legal {6}
    gs.players[1].hand.push_back(CardType::ANCHOR);
    gs.players[1].ep = 6;
    Move acard; acard.kind = Move::CARD; acard.card = CardType::ANCHOR;
    acard.col = 6; acard.srcCol = 5; // drop col6, block col5
    CHECK(applyMove(gs, acard));
    // opponent (Black) next turn: anchor=6 → {5,7}; col5 blocked
    endTurn(gs); startTurn(gs);
    auto legal3 = legalDropColumns(gs);
    CHECK(!legal3.empty());
    for (int c : legal3) CHECK(c != 5);
    CHECK(gs.blockedCol == 5 && gs.turnNumber == gs.blockedForTurn);
    // block expires naturally on the following turn (flag check)
    Move d2; d2.kind = Move::DROP; d2.col = 7;
    CHECK(applyMove(gs, d2)); // Black drops col7 → anchor=7
    endTurn(gs); startTurn(gs);
    CHECK(!(gs.blockedCol == 5 && gs.turnNumber == gs.blockedForTurn)); // expired

    return report();
}
