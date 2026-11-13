#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "board_builder.hpp"
#include "cardflow/board.hpp"
#include "cardflow/rules.hpp"

using cardflow::Board;
using cardflow::Color;
using cardflow::Position;
using cardflow::Stone;
using cardflow::test::colorAt;
using cardflow::test::makeBoard;
namespace rules = cardflow::rules;

namespace {

// ─── findFlips / placeAndFlip（Place の裏返し）────────────────────────

TEST(RulesFlipTest, FlipsLikeOthelloOnInitialBoard)
{
    Board board = Board::createInitial();
    // e3 (2,4) に黒を置くと、e4 (3,4) の白が e5 (4,4) の黒と挟まれる
    const auto flipped = rules::placeAndFlip(board, {2, 4}, Color::Black);
    ASSERT_TRUE(flipped.has_value());
    EXPECT_EQ(flipped->affected, 1);
    EXPECT_EQ(colorAt(board, {3, 4}), Color::Black);
    EXPECT_EQ(board.countStones(Color::Black), 4);
    EXPECT_EQ(board.countStones(Color::White), 1);
}

TEST(RulesFlipTest, PlacementWithoutFlipIsAllowed)
{
    Board board = Board::createInitial();
    const auto flipped = rules::placeAndFlip(board, {0, 0}, Color::White);
    ASSERT_TRUE(flipped.has_value());
    EXPECT_EQ(flipped->affected, 0);
    EXPECT_EQ(colorAt(board, {0, 0}), Color::White);
}

TEST(RulesFlipTest, CannotPlaceOnOccupiedOrOutside)
{
    Board board = Board::createInitial();
    EXPECT_FALSE(rules::placeAndFlip(board, {3, 3}, Color::White).has_value());
    EXPECT_FALSE(rules::placeAndFlip(board, {-1, 0}, Color::White).has_value());
    ASSERT_TRUE(board.placeWall({0, 0}));
    EXPECT_FALSE(rules::placeAndFlip(board, {0, 0}, Color::White).has_value());
}

TEST(RulesFlipTest, FlipsMultipleDirectionsAndLongRuns)
{
    Board board = makeBoard({
        "B.......",
        ".W......",
        "..W.....",
        "....WWB.",
        "...W....",
        "...B....",
        "........",
        "........",
    });
    const auto flipped = rules::placeAndFlip(board, {3, 3}, Color::Black);
    ASSERT_TRUE(flipped.has_value());
    // 右: (3,4)(3,5) → (3,6)黒で挟む / 下: (4,3) → (5,3)黒 / 左上: (2,2)(1,1) → (0,0)黒
    EXPECT_EQ(flipped->affected, 5);
    for (const Position p : {Position{3, 4}, Position{3, 5}, Position{4, 3}, Position{2, 2}, Position{1, 1}}) {
        EXPECT_EQ(colorAt(board, p), Color::Black) << p.row << "," << p.col;
    }
}

TEST(RulesFlipTest, WallActsAsAnchorForBothPlayers)
{
    Board board = makeBoard({
        "........",
        ".WW#....",
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
    });
    const auto flipped = rules::placeAndFlip(board, {1, 0}, Color::Black);
    ASSERT_TRUE(flipped.has_value());
    EXPECT_EQ(flipped->affected, 2);
}

TEST(RulesFlipTest, BoardEdgeAndGapDoNotSandwich)
{
    Board board = makeBoard({
        ".WW.....",  // 右端側は空き → 挟めない
        "......WW",  // 盤端で終わる
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
    });
    EXPECT_TRUE(rules::findFlips(board, {0, 0}, Color::Black).empty());
    EXPECT_TRUE(rules::findFlips(board, {1, 5}, Color::Black).empty());
}

TEST(RulesFlipTest, ShieldBlocksOneFlipThenBreaks)
{
    Board board = makeBoard({
        ".WwB....",
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
    });
    const auto flipped = rules::placeAndFlip(board, {0, 0}, Color::Black);
    ASSERT_TRUE(flipped.has_value());
    EXPECT_EQ(*flipped, (rules::StrikeResult{.affected = 1, .shieldsBroken = 1}));
    EXPECT_EQ(colorAt(board, {0, 1}), Color::Black);
    EXPECT_EQ(colorAt(board, {0, 2}), Color::White);  // Shield で防いだ
    EXPECT_FALSE(board.stoneAt({0, 2})->shielded);     // Shield は外れる

    // 2 回目は防げない
    const std::array<Position, 1> again{Position{0, 2}};
    EXPECT_EQ(rules::strikeFlip(board, again).affected, 1);
    EXPECT_EQ(colorAt(board, {0, 2}), Color::Black);
}

TEST(RulesFlipTest, CoverBlocksWithoutConsumingShield)
{
    Board board = makeBoard({
        ".WWB....",
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
    });
    ASSERT_TRUE(board.replaceStone({0, 2}, Stone{.color = Color::White, .covered = true, .shielded = true}));
    const auto flipped = rules::placeAndFlip(board, {0, 0}, Color::Black);
    ASSERT_TRUE(flipped.has_value());
    EXPECT_EQ(*flipped, (rules::StrikeResult{.affected = 1, .blockedByCover = 1}));
    EXPECT_EQ(colorAt(board, {0, 2}), Color::White);
    EXPECT_TRUE(board.stoneAt({0, 2})->shielded);  // Cover が優先され Shield は残る
}

// ─── findCapturable / captureAll（Burst の捕獲）──────────────────────

TEST(RulesCaptureTest, CapturesRunsSandwichedByStonesOrWalls)
{
    Board board = makeBoard({
        "BWW#....",  // 横: 黒と壁で挟む → 2 個
        "........",
        "....B...",
        "....W...",  // 縦: (2,4)黒 と (4,4)壁
        "....#...",
        "........",
        "W.......",  // 盤端 → 捕獲されない
        "BW......",  // 右が空 → 捕獲されない
    });
    const auto capturable = rules::findCapturable(board, Color::Black);
    const std::vector<Position> expected{{0, 1}, {0, 2}, {3, 4}};
    EXPECT_EQ(capturable, expected);

    EXPECT_EQ(rules::captureAll(board, Color::Black).affected, 3);
    EXPECT_TRUE(board.isEmpty({0, 1}));
    EXPECT_TRUE(board.isEmpty({0, 2}));
    EXPECT_TRUE(board.isEmpty({3, 4}));
    EXPECT_TRUE(board.hasStone({6, 0}));
    EXPECT_TRUE(board.hasStone({7, 1}));
}

TEST(RulesCaptureTest, CapturesDiagonalRuns)
{
    Board board = makeBoard({
        "W.......",
        ".B......",
        "..B.....",
        "...W....",
        "........",
        "......W.",
        ".....B..",
        "....#...",
    });
    // 右下がり: (0,0)白 が (1,1)(2,2)黒 を挟む / 右上がり: (5,6)白 と (7,4)壁 が (6,5)黒 を挟む
    const auto capturable = rules::findCapturable(board, Color::White);
    const std::vector<Position> expected{{1, 1}, {2, 2}, {6, 5}};
    EXPECT_EQ(capturable, expected);
}

TEST(RulesCaptureTest, StoneCountedOnceWhenSandwichedOnTwoAxes)
{
    Board board = makeBoard({
        ".B......",
        "BWB.....",
        ".B......",
        "........",
        "........",
        "........",
        "........",
        "........",
    });
    const auto capturable = rules::findCapturable(board, Color::Black);
    ASSERT_EQ(capturable.size(), 1u);
    EXPECT_EQ(capturable.front(), (Position{1, 1}));
}

TEST(RulesCaptureTest, ShieldedStoneSurvivesCaptureOnce)
{
    Board board = makeBoard({
        "BWwB....",
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
    });
    const std::vector<Position> expected{{0, 1}, {0, 2}};
    EXPECT_EQ(rules::findCapturable(board, Color::Black), expected);  // 保護石も挟みには含む
    EXPECT_EQ(rules::captureAll(board, Color::Black), (rules::StrikeResult{.affected = 1, .shieldsBroken = 1}));
    EXPECT_TRUE(board.isEmpty({0, 1}));
    EXPECT_TRUE(board.hasStone({0, 2}));
}

TEST(RulesCaptureTest, IsSandwiched)
{
    const Board board = makeBoard({
        "BW#.....",
        "BW......",
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
    });
    EXPECT_TRUE(rules::isSandwiched(board, {0, 1}, Color::Black));
    EXPECT_FALSE(rules::isSandwiched(board, {1, 1}, Color::Black));
    EXPECT_FALSE(rules::isSandwiched(board, {0, 0}, Color::Black));  // 自分の石
    EXPECT_FALSE(rules::isSandwiched(board, {5, 5}, Color::Black));  // 空マス
}

TEST(RulesCaptureTest, InitialBoardHasNothingToCapture)
{
    Board board = Board::createInitial();
    EXPECT_TRUE(rules::findCapturable(board, Color::Black).empty());
    EXPECT_TRUE(rules::findCapturable(board, Color::White).empty());
    EXPECT_EQ(rules::captureAll(board, Color::Black).affected, 0);
}

} // namespace
