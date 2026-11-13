#include <gtest/gtest.h>

#include <cstddef>

#include "board_builder.hpp"
#include "cardflow/card.hpp"

using cardflow::Board;
using cardflow::CardType;
using cardflow::Color;
using cardflow::kCardTypeCount;
using cardflow::Position;
using cardflow::Stone;
using cardflow::test::makeBoard;
namespace cf = cardflow;

namespace {

const Board kSample = makeBoard({"WB......", ".Bw.....", "..W.....", "........", "........", "........",
                                 "........", "........"});

TEST(CardTest, TableHasNamesAndDescriptions)
{
    for (std::size_t i = 0; i < kCardTypeCount; ++i) {
        const auto& info = cf::cardInfo(static_cast<CardType>(i));
        EXPECT_FALSE(info.name.empty());
        EXPECT_FALSE(info.description.empty());
    }
    EXPECT_FALSE(cf::cardInfo(CardType::Double).needsTarget);
    EXPECT_FALSE(cf::cardInfo(CardType::Free).needsTarget);
    EXPECT_TRUE(cf::cardInfo(CardType::Flip).needsTarget);
}

TEST(CardTest, GuardTargetsOwnUnguardedStone)
{
    EXPECT_TRUE(cf::isValidCardTarget(CardType::Guard, kSample, Color::Black, Position{0, 1}));
    EXPECT_FALSE(cf::isValidCardTarget(CardType::Guard, kSample, Color::Black, Position{0, 0}));  // 相手の石
    EXPECT_FALSE(cf::isValidCardTarget(CardType::Guard, kSample, Color::White, Position{1, 2}));  // 守り済み
    Board board = kSample;
    ASSERT_TRUE(cf::applyCardTarget(CardType::Guard, board, Color::Black, Position{0, 1}));
    EXPECT_EQ(board.stoneAt(Position{0, 1}), (Stone{Color::Black, true}));
}

TEST(CardTest, WallTargetsEmptyCell)
{
    EXPECT_TRUE(cf::isValidCardTarget(CardType::Wall, kSample, Color::Black, Position{7, 7}));
    EXPECT_FALSE(cf::isValidCardTarget(CardType::Wall, kSample, Color::Black, Position{0, 0}));
    Board board = kSample;
    ASSERT_TRUE(cf::applyCardTarget(CardType::Wall, board, Color::Black, Position{7, 7}));
    EXPECT_TRUE(board.isWall(Position{7, 7}));
}

TEST(CardTest, FlipTargetsOpponentStoneExceptCornerAndGuarded)
{
    EXPECT_TRUE(cf::isValidCardTarget(CardType::Flip, kSample, Color::Black, Position{2, 2}));
    EXPECT_FALSE(cf::isValidCardTarget(CardType::Flip, kSample, Color::Black, Position{0, 0}));  // 角
    EXPECT_FALSE(cf::isValidCardTarget(CardType::Flip, kSample, Color::Black, Position{1, 2}));  // 守り
    EXPECT_FALSE(cf::isValidCardTarget(CardType::Flip, kSample, Color::Black, Position{0, 1}));  // 自分の石
    Board board = kSample;
    ASSERT_TRUE(cf::applyCardTarget(CardType::Flip, board, Color::Black, Position{2, 2}));
    EXPECT_EQ(board.stoneAt(Position{2, 2})->color, Color::Black);
}

TEST(CardTest, NoTargetCardsRejectTargets)
{
    Board board = kSample;
    EXPECT_FALSE(cf::isValidCardTarget(CardType::Double, board, Color::Black, Position{7, 7}));
    EXPECT_FALSE(cf::applyCardTarget(CardType::Free, board, Color::Black, Position{7, 7}));
    EXPECT_FALSE(cf::applyCardTarget(CardType::Flip, board, Color::Black, Position{0, 0}));
    EXPECT_EQ(board.stoneAt(Position{0, 0})->color, Color::White);  // 失敗時は変更なし
}

} // namespace
