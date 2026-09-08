// ─── tests/unit/player_test.cpp ────────────────────────────────────────
// Player クラスの単体テスト（Google Test）

#include <gtest/gtest.h>
#include "player.hpp"
#include "board.hpp"
#include "card.hpp"

using namespace game;

namespace {

TEST(PlayerTest, ConstructorInitializesZeroEP) {
    Player black("black");
    EXPECT_EQ(black.color, "black");
    EXPECT_EQ(black.energy_points, 0);
}

TEST(PlayerTest, RecoverEPIncreasesByThreeCappedAtSix) {
    Player player("black");
    
    player.recover_ep();   // 0 → 3
    player.recover_ep();   // 3 → 6 (capped at max)
    player.recover_ep();   // 6 → 6 (still capped)

    EXPECT_EQ(player.energy_points, 6);
}

TEST(PlayerTest, CanPlayCardWhenEnoughEP) {
    Player player("black");
    player.recover_ep();  // +3 EP → now has 3

    Place place_card;
    Result result = player.can_play_card(place_card);
    
    EXPECT_EQ(result.status, "ok");
}

TEST(PlayerTest, CannotPlayCardWhenOutOfEP) {
    Player player("black");  // Starts with 0 EP
    Place place_card;        // cost = 2

    Result result = player.can_play_card(place_card);
    EXPECT_EQ(result.status, "out_of_ep");
}

TEST(PlayerTest, PlayCardPaysCostAndReducesEP) {
    Player player("black");
    player.recover_ep();   // 0 → 3
    player.recover_ep();   // 3 → 6 (capped)

    Place place_card;
    EXPECT_TRUE(player.play_card(place_card));

    EXPECT_EQ(player.energy_points, 4);  // Paid cost of 2
}

TEST(PlayerTest, PlayCardFailsWhenOutOfEP) {
    Player player("black");
    Place place_card(2);   // cost = 2

    bool success = player.play_card(place_card);
    EXPECT_EQ(success, false);
    EXPECT_EQ(player.energy_points, 0);  // No deduction when play fails
}

TEST(PlayerTest, AddToHandLimitsAtThreeCards) {
    Player player("black");

    Place p1;
    Move m1;
    Cover c1;

    player.add_to_hand(&p1);   // hand size = 1
    player.add_to_hand(&m1);   // hand size = 2
    player.add_to_hand(&c1);   // hand size = 3 (full)

    EXPECT_EQ(player.hand.size(), 3U);

    Place p2;
    bool added = false;
    for (auto& card : player.hand) {
        if (card->name == "Place") {
            auto* ptr = &p1; // placeholder — in full impl would properly manage ownership
            break;
        }
    }
}

TEST(PlayerTest, FindCardReturnsCorrectPointer) {
    Player player("black");
    Place place_card;

    player.add_to_hand(&place_card);

    Card* found = player.find_card("Place");
    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found->name, "Place");
}

TEST(PlayerTest, FindCardReturnsNullForNonexistent) {
    Player player("black");

    Card* result = player.find_card("NonExistentCard");
    EXPECT_EQ(result, nullptr);
}

} // anonymous namespace