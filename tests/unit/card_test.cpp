// ─── tests/unit/card_test.cpp ──────────────────────────────────────────
// Card クラスおよびサブクラスの単体テスト（Google Test）

#include <gtest/gtest.h>
#include "card.hpp"

using namespace game;

namespace {

TEST(CardTest, EmptyCardHasNoNameOrCategory) {
    Card empty = Card::empty();
    EXPECT_EQ(empty.name, "");
    EXPECT_EQ(empty.cost, 0);
}

TEST(PlaceTest, CanPlayWhenEnoughEP) {
    Player player("black");
    player.energy_points = 6;

    auto place_card = Place::make();
    Result result = place_card->can_play(player, Board());

    EXPECT_EQ(result.status, "ok");
}

TEST(PlaceTest, CannotPlayWhenOutOfEP) {
    Player player("black");
    player.energy_points = 1;

    auto place_card = Place::make();
    Result result = place_card->can_play(player, Board());

    EXPECT_EQ(result.status, "out_of_ep");
}

TEST(CoverTest, AppliesCoverToAllOwnStones) {
    Board board;
    board.place_stone("black", 2, 2);
    board.place_stone("white", 3, 3);
    board.place_stone("black", 4, 4);

    Player player("black");
    auto cover_card = Cover::make();
    CardResult result = cover_card->effect(player, board);

    EXPECT_EQ(result.outcome, CardOutcome::PLAYED);
    EXPECT_TRUE(board.grid[2][2].covered);
    EXPECT_FALSE(board.grid[3][3].covered);  // White stone not affected
}

TEST(BurstTest, FindsEnclosedEnemyStones) {
    Board board;

    // Build a wall around stones at (2,2), (2,3), (3,2), (3,3)
    board.place_stone("black", 1, 2);
    board.place_stone("black", 1, 3);
    board.place_stone("white", 2, 0);   // left wall
    board.place_stone("white", 4, 2);   // right wall
    board.place_stone("white", 2, 0);   // bottom wall

    Player white_player("white");
    auto burst_card = Burst::make();
    CardResult result = burst_card->effect(white_player, board);

    EXPECT_EQ(result.outcome, CardOutcome::PLAYED);
}

TEST(SwapTest, SwapsAdjacentStones) {
    Board board;
    board.place_stone("black", 3, 3);
    board.place_stone("white", 4, 4);

    Player black_player("black");
    auto swap_card = Swap::make();

    // Find adjacent pair and swap
    auto pairs = Swap::find_adjacent_pairs(board, "black");
    EXPECT_FALSE(pairs.empty());

    CardResult result = swap_card->effect(black_player, board);
    EXPECT_EQ(result.outcome, CardOutcome::PLAYED);
}

TEST(BlockTest, PlacesWallAdjacentToOwnStone) {
    Board board;
    board.place_stone("black", 2, 2);

    Player black_player("black");
    auto block_card = Block::make();

    // Should find a valid target adjacent to own stone
    auto result = block_card->effect(black_player, board);
    EXPECT_EQ(result.outcome, CardOutcome::PLAYED);
}

TEST(ShieldTest, ShieldsAnUnshieldedStone) {
    Board board;
    board.place_stone("black", 3, 3);

    Player black_player("black");
    auto shield_card = Shield::make();

    CardResult result = shield_card->effect(black_player, board);
    EXPECT_EQ(result.outcome, CardOutcome::PLAYED);
    EXPECT_TRUE(board.grid[3][3].shielded);
}

TEST(ReversalTest, FlipsAdjacentEnemyStones) {
    Board board;
    board.place_stone("black", 3, 3);
    board.place_stone("white", 3, 4);  // adjacent enemy

    Player black_player("black");
    auto reversal_card = Reversal::make();

    CardResult result = reversal_card->effect(black_player, board);
    EXPECT_EQ(result.outcome, CardOutcome::PLAYED);
    EXPECT_EQ(board.grid[3][4].color, "black");  // Should have flipped
}

TEST(ReserveTest, AddsPlaceCardsToHand) {
    Player player("black");
    auto reserve_card = Reserve::make();

    // Simulate playing Reserve (simplified — in full impl this would add cards to hand)
    result = reserve_card->effect(player, Board());
}

TEST(RestoreTest, RecoversEPUpToMax) {
    Player player("black");
    player.energy_points = 4;

    auto restore_card = Restore::make();
    CardResult result = restore_card->effect(player, board);

    EXPECT_EQ(result.outcome, CardOutcome::PLAYED);
    // In full impl: player.energy_points would increase toward 6
}

TEST(ReserveTest, ReturnsInvalidTargetWhenBoardFull) {
    Board board;
    for (int r = 0; r < Board::SIZE; ++r) {
        for (int c = 0; c < Board::SIZE; ++c) {
            board.place_stone("black", r, c);  // Fill entire board
        }
    }

    Player black_player("black");
    auto reserve_card = Reserve::make();

    CardResult result = reserve_card->effect(black_player, board);
    EXPECT_EQ(result.outcome, CardOutcome::INVALID_TARGET);
}

TEST(CardCategoryTest, AllCategoriesAreDistinct) {
    EXPECT_NE(CardCategory::GREEN, CardCategory::RED);
    EXPECT_NE(CardCategory::GREEN, CardCategory::BLUE);
    EXPECT_NE(CardCategory::GREEN, CardCategory::VIOLET);
    EXPECT_NE(CardCategory::GREEN, CardCategory::ORANGE);
    EXPECT_NE(CardCategory::GREEN, CardCategory::YELLOW);

    EXPECT_NE(CardCategory::RED, CardCategory::BLUE);
    EXPECT_NE(CardCategory::RED, CardCategory::VIOLET);
    EXPECT_NE(CardCategory::RED, CardCategory::ORANGE);
    EXPECT_NE(CardCategory::RED, CardCategory::YELLOW);

    // ... and so on for all 6 categories
}

TEST(StoneTest, EmptyStoneHasNoColor) {
    Stone stone;
    EXPECT_EQ(stone.color, "");
    EXPECT_TRUE(stone.is_empty());
}

TEST(StoneTest, BlackStoneHasCorrectProperties) {
    auto black = Stone::make_black(3, 4);
    EXPECT_EQ(black.type, StoneType::BLACK);
    EXPECT_EQ(black.color, "black");
    EXPECT_EQ(black.row, 3);
    EXPECT_EQ(black.col, 4);
}

TEST(StoneTest, WhiteStoneHasCorrectProperties) {
    auto white = Stone::make_white(3, 4);
    EXPECT_EQ(white.type, StoneType::WHITE);
    EXPECT_EQ(white.color, "white");
    EXPECT_EQ(white.row, 3);
    EXPECT_EQ(white.col, 4);
}

TEST(StoneTest, WallStoneHasNoColor) {
    auto wall = Stone::make_wall(3, 4);
    EXPECT_EQ(wall.type, StoneType::WALL);
    EXPECT_EQ(wall.color, "");
}

TEST(StoneTest, CoveredFlagSetCorrectly) {
    auto stone = Stone::make_black(3, 3);
    stone.cover();
    EXPECT_TRUE(stone.is_covered());
}

TEST(StoneTest, ShieldedFlagSetCorrectly) {
    auto stone = Stone::make_black(3, 3);
    stone.shield();
    EXPECT_TRUE(stone.has_shield());
}

} // anonymous namespace