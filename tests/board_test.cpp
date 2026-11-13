#include <gtest/gtest.h>

#include <algorithm>

#include "cardflow/board.hpp"

using cardflow::Board;
using cardflow::Color;
using cardflow::Position;
using cardflow::Stone;

namespace {

TEST(BoardTest, DefaultBoardIsEmpty)
{
    const Board board;
    EXPECT_EQ(board.countEmpty(), 64);
    EXPECT_EQ(board.countStones(Color::Black), 0);
    EXPECT_EQ(board.countStones(Color::White), 0);
}

TEST(BoardTest, InitialBoardHasFourStonesEach)
{
    const Board board = Board::createInitial();
    // 中央
    EXPECT_EQ(board.stoneAt({3, 3})->color, Color::Black);
    EXPECT_EQ(board.stoneAt({3, 4})->color, Color::White);
    EXPECT_EQ(board.stoneAt({4, 3})->color, Color::White);
    EXPECT_EQ(board.stoneAt({4, 4})->color, Color::Black);
    // 斜め外側
    EXPECT_EQ(board.stoneAt({2, 2})->color, Color::White);
    EXPECT_EQ(board.stoneAt({2, 5})->color, Color::Black);
    EXPECT_EQ(board.stoneAt({5, 2})->color, Color::Black);
    EXPECT_EQ(board.stoneAt({5, 5})->color, Color::White);
    EXPECT_EQ(board.countStones(Color::Black), 4);
    EXPECT_EQ(board.countStones(Color::White), 4);
    EXPECT_EQ(board.countEmpty(), 56);
}

TEST(BoardTest, IsInsideChecksBounds)
{
    EXPECT_TRUE(Board::isInside({0, 0}));
    EXPECT_TRUE(Board::isInside({7, 7}));
    EXPECT_FALSE(Board::isInside({-1, 0}));
    EXPECT_FALSE(Board::isInside({0, 8}));
    EXPECT_FALSE(Board::isInside({8, 8}));
}

TEST(BoardTest, OutOfRangeQueriesAreSafe)
{
    Board board = Board::createInitial();
    const Position outside{-1, 9};
    EXPECT_FALSE(board.isEmpty(outside));
    EXPECT_FALSE(board.hasStone(outside));
    EXPECT_FALSE(board.isWall(outside));
    EXPECT_FALSE(board.stoneAt(outside).has_value());
    EXPECT_FALSE(board.placeStone(outside, Color::Black));
    EXPECT_FALSE(board.placeWall(outside));
    EXPECT_FALSE(board.removeStone(outside));
    EXPECT_FALSE(board.flipStone(outside));
    EXPECT_TRUE(board.neighbors(outside).empty());
}

TEST(BoardTest, PlaceStoneOnlyOnEmptyCell)
{
    Board board = Board::createInitial();
    EXPECT_TRUE(board.placeStone({0, 0}, Color::White));
    EXPECT_EQ(board.stoneAt({0, 0})->color, Color::White);
    EXPECT_FALSE(board.placeStone({0, 0}, Color::Black));  // 既に石あり
    EXPECT_FALSE(board.placeStone({3, 3}, Color::White));  // 初期石
    EXPECT_EQ(board.stoneAt({3, 3})->color, Color::Black);
}

TEST(BoardTest, WallBlocksPlacementAndIsNotAStone)
{
    Board board;
    EXPECT_TRUE(board.placeWall({2, 2}, Color::Black));
    EXPECT_TRUE(board.isWall({2, 2}));
    EXPECT_FALSE(board.isEmpty({2, 2}));
    EXPECT_FALSE(board.hasStone({2, 2}));
    EXPECT_EQ(board.wallAt({2, 2})->owner, Color::Black);
    EXPECT_FALSE(board.placeStone({2, 2}, Color::White));
    EXPECT_FALSE(board.removeStone({2, 2}));  // 壁は石として除去できない
    EXPECT_FALSE(board.placeWall({2, 2}));
}

TEST(BoardTest, NeutralWallHasNoOwner)
{
    Board board;
    ASSERT_TRUE(board.placeWall({5, 5}));
    EXPECT_FALSE(board.wallAt({5, 5})->owner.has_value());
}

TEST(BoardTest, RemoveStone)
{
    Board board = Board::createInitial();
    EXPECT_TRUE(board.removeStone({3, 3}));
    EXPECT_TRUE(board.isEmpty({3, 3}));
    EXPECT_FALSE(board.removeStone({3, 3}));
}

TEST(BoardTest, FlipStoneKeepsProtection)
{
    Board board;
    ASSERT_TRUE(board.placeStone({1, 1}, Color::Black));
    ASSERT_TRUE(board.replaceStone({1, 1}, Stone{.color = Color::Black, .shielded = true}));
    EXPECT_TRUE(board.flipStone({1, 1}));
    const auto stone = board.stoneAt({1, 1});
    EXPECT_EQ(stone->color, Color::White);
    EXPECT_TRUE(stone->shielded);
    EXPECT_FALSE(board.flipStone({0, 0}));  // 空マス
}

TEST(BoardTest, ReplaceStoneRequiresExistingStone)
{
    Board board;
    EXPECT_FALSE(board.replaceStone({0, 0}, Stone{.color = Color::Black}));
    ASSERT_TRUE(board.placeStone({0, 0}, Color::Black));
    EXPECT_TRUE(board.replaceStone({0, 0}, Stone{.color = Color::Black, .covered = true}));
    EXPECT_TRUE(board.stoneAt({0, 0})->covered);
    EXPECT_TRUE(board.stoneAt({0, 0})->isProtected());
}

TEST(BoardTest, MoveStoneToEmptyCell)
{
    Board board = Board::createInitial();
    EXPECT_TRUE(board.moveStone({3, 3}, {2, 3}));
    EXPECT_TRUE(board.isEmpty({3, 3}));
    EXPECT_EQ(board.stoneAt({2, 3})->color, Color::Black);
    EXPECT_FALSE(board.moveStone({2, 3}, {3, 4}));  // 移動先に石
    EXPECT_FALSE(board.moveStone({0, 0}, {0, 1}));  // 移動元が空
    ASSERT_TRUE(board.placeWall({1, 3}));
    EXPECT_FALSE(board.moveStone({2, 3}, {1, 3}));  // 移動先が壁
}

TEST(BoardTest, SwapStones)
{
    Board board = Board::createInitial();
    EXPECT_TRUE(board.swapStones({3, 3}, {3, 4}));
    EXPECT_EQ(board.stoneAt({3, 3})->color, Color::White);
    EXPECT_EQ(board.stoneAt({3, 4})->color, Color::Black);
    EXPECT_FALSE(board.swapStones({3, 3}, {0, 0}));  // 片方が空
    EXPECT_FALSE(board.swapStones({3, 3}, {3, 3}));  // 同一マス
}

TEST(BoardTest, NeighborsCountDependsOnLocation)
{
    const Board board;
    EXPECT_EQ(board.neighbors({0, 0}).size(), 3u);
    EXPECT_EQ(board.neighbors({0, 3}).size(), 5u);
    EXPECT_EQ(board.neighbors({4, 4}).size(), 8u);

    const auto around = board.neighbors({0, 0});
    EXPECT_NE(std::ranges::find(around, Position{1, 1}), around.end());
    EXPECT_EQ(std::ranges::find(around, Position{0, 0}), around.end());
}

} // namespace
