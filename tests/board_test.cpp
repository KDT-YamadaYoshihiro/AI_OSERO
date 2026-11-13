#include <gtest/gtest.h>

#include "board_builder.hpp"
#include "cardflow/board.hpp"

using cardflow::Board;
using cardflow::Color;
using cardflow::Position;
using cardflow::Stone;
using cardflow::test::makeBoard;

namespace {

TEST(BoardTest, InitialBoardIsStandardOthello)
{
    const Board board = Board::createInitial();
    EXPECT_EQ(board.stoneAt(Position{3, 3})->color, Color::White);  // d4
    EXPECT_EQ(board.stoneAt(Position{3, 4})->color, Color::Black);  // e4
    EXPECT_EQ(board.stoneAt(Position{4, 3})->color, Color::Black);  // d5
    EXPECT_EQ(board.stoneAt(Position{4, 4})->color, Color::White);  // e5
    EXPECT_EQ(board.countStones(Color::Black), 2);
    EXPECT_EQ(board.countStones(Color::White), 2);
    EXPECT_EQ(board.countEmpty(), 60);
}

TEST(BoardTest, OutsideIsNeitherEmptyNorStone)
{
    const Board board;
    EXPECT_FALSE(board.isEmpty(Position{-1, 0}));
    EXPECT_FALSE(board.isWall(Position{8, 0}));
    EXPECT_FALSE(board.stoneAt(Position{0, 8}).has_value());
}

TEST(BoardTest, PlaceOnlyOnEmpty)
{
    Board board;
    EXPECT_TRUE(board.placeStone(Position{0, 0}, Color::Black));
    EXPECT_FALSE(board.placeStone(Position{0, 0}, Color::White));
    EXPECT_FALSE(board.placeWall(Position{0, 0}));
    EXPECT_TRUE(board.placeWall(Position{0, 1}));
    EXPECT_TRUE(board.isWall(Position{0, 1}));
    EXPECT_FALSE(board.placeStone(Position{0, 1}, Color::Black));
    EXPECT_FALSE(board.placeStone(Position{9, 9}, Color::Black));
    EXPECT_EQ(board.countEmpty(), 62);
}

TEST(BoardTest, SetStoneChangesOnlyExistingStones)
{
    Board board;
    EXPECT_FALSE(board.setStone(Position{0, 0}, Stone{Color::White, true}));
    ASSERT_TRUE(board.placeStone(Position{0, 0}, Color::Black));
    EXPECT_TRUE(board.setStone(Position{0, 0}, Stone{Color::White, true}));
    EXPECT_EQ(board.stoneAt(Position{0, 0}), (Stone{Color::White, true}));
}

TEST(BoardTest, AdjacentStoneAndCorner)
{
    const Board board = makeBoard({"B.......", "........", "........", "........", "........", "........",
                                   "........", "#......."});
    EXPECT_TRUE(board.hasAdjacentStone(Position{1, 1}));
    EXPECT_FALSE(board.hasAdjacentStone(Position{2, 2}));
    EXPECT_FALSE(board.hasAdjacentStone(Position{6, 0}));  // 壁は石ではない
    EXPECT_TRUE(Board::isCorner(Position{0, 7}));
    EXPECT_FALSE(Board::isCorner(Position{0, 6}));
}

} // namespace
