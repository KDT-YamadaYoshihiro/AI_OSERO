#include <gtest/gtest.h>

#include <vector>

#include "board_builder.hpp"
#include "cardflow/rules.hpp"

using cardflow::Board;
using cardflow::Color;
using cardflow::Position;
using cardflow::test::colorAt;
using cardflow::test::makeBoard;
namespace rules = cardflow::rules;

namespace {

TEST(RulesTest, InitialLegalMovesAreStandard)
{
    const Board board = Board::createInitial();
    // 黒の初手: d3, c4, f5, e6
    const std::vector<Position> expected{{2, 3}, {3, 2}, {4, 5}, {5, 4}};
    EXPECT_EQ(rules::legalMoves(board, Color::Black), expected);
    EXPECT_EQ(rules::countLegalMoves(board, Color::White), 4);
}

TEST(RulesTest, PlaceFlipsSandwichedStones)
{
    Board board = Board::createInitial();
    const auto result = rules::placeAndFlip(board, Position{2, 3}, Color::Black);  // d3
    EXPECT_EQ(result.flipped, (std::vector<Position>{{3, 3}}));
    EXPECT_EQ(colorAt(board, Position{3, 3}), Color::Black);
    EXPECT_EQ(board.countStones(Color::Black), 4);
    EXPECT_EQ(board.countStones(Color::White), 1);
}

TEST(RulesTest, FlipsInSeveralDirections)
{
    // 中央の空マスを 8 方向から白が囲み、その外側に黒がいる
    Board cross = makeBoard({"........", ".B.B.B..", "..WWW...", ".BW.WB..", "..WWW...", ".B.B.B..",
                             "........", "........"});
    const auto result = rules::placeAndFlip(cross, Position{3, 3}, Color::Black);
    EXPECT_EQ(result.flipped.size(), 8u);
    EXPECT_EQ(cross.countStones(Color::White), 0);
}

TEST(RulesTest, WallAndEdgeStopTheLine)
{
    const Board board = makeBoard({"WW......", "........", "........", "........", "........", "........",
                                   "........", "B#W....."});
    // c1 から左は白が盤の端まで続くので挟めない
    EXPECT_TRUE(rules::findFlips(board, Position{0, 2}, Color::Black).empty());
    // 壁は挟みの端にならない
    EXPECT_FALSE(rules::isLegalMove(board, Position{7, 3}, Color::Black));
}

TEST(RulesTest, GuardedStoneDoesNotFlip)
{
    Board board = makeBoard({"BwW.....", "........", "........", "........", "........", "........",
                             "........", "........"});
    ASSERT_TRUE(rules::isLegalMove(board, Position{0, 3}, Color::Black));
    const auto result = rules::placeAndFlip(board, Position{0, 3}, Color::Black);
    EXPECT_EQ(result.flipped, (std::vector<Position>{{0, 2}}));
    EXPECT_EQ(result.guardsBroken, 1);
    EXPECT_EQ(colorAt(board, Position{0, 1}), Color::White);
    EXPECT_FALSE(board.stoneAt(Position{0, 1})->guarded);  // 守りは外れる
}

TEST(RulesTest, FreeCellNeedsAdjacentStone)
{
    const Board board = makeBoard({"B.......", "........", "........", "........", "........", "........",
                                   "........", "........"});
    EXPECT_TRUE(rules::isFreeCell(board, Position{1, 1}));
    EXPECT_FALSE(rules::isFreeCell(board, Position{2, 2}));
    EXPECT_FALSE(rules::isFreeCell(board, Position{0, 0}));  // 空マスではない
}

} // namespace
