#include <gtest/gtest.h>

#include <array>
#include <string_view>
#include <vector>

#include "cardflow/board.hpp"
#include "cardflow/rules.hpp"

using cardflow::Board;
using cardflow::Color;
using cardflow::Position;
using cardflow::Stone;
namespace rules = cardflow::rules;

namespace {

// 文字列から盤面を作る。
//   '.' 空 / 'B' 黒 / 'W' 白 / '#' 壁 / 'b' 'w' 保護（Shield）付きの黒・白
Board makeBoard(const std::array<std::string_view, Board::kSize>& rows)
{
    Board board;
    for (int row = 0; row < Board::kSize; ++row) {
        for (int col = 0; col < Board::kSize; ++col) {
            const Position pos{row, col};
            const char ch = rows[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
            bool ok = true;
            switch (ch) {
            case 'B': ok = board.placeStone(pos, Color::Black); break;
            case 'W': ok = board.placeStone(pos, Color::White); break;
            case '#': ok = board.placeWall(pos); break;
            case 'b':
            case 'w': {
                const Color color = ch == 'b' ? Color::Black : Color::White;
                ok = board.placeStone(pos, color) &&
                     board.replaceStone(pos, Stone{.color = color, .shielded = true});
                break;
            }
            default: break;
            }
            EXPECT_TRUE(ok) << "makeBoard failed at " << row << "," << col;
        }
    }
    return board;
}

Color colorAt(const Board& board, Position pos)
{
    return board.stoneAt(pos).value().color;
}

// ─── findFlips / placeAndFlip（Place の裏返し）────────────────────────

TEST(RulesFlipTest, FlipsLikeOthelloOnInitialBoard)
{
    Board board = Board::createInitial();
    // e3 (2,4) に黒を置くと、e4 (3,4) の白が e5 (4,4) の黒と挟まれる
    const auto flipped = rules::placeAndFlip(board, {2, 4}, Color::Black);
    ASSERT_TRUE(flipped.has_value());
    EXPECT_EQ(*flipped, 1);
    EXPECT_EQ(colorAt(board, {3, 4}), Color::Black);
    EXPECT_EQ(board.countStones(Color::Black), 4);
    EXPECT_EQ(board.countStones(Color::White), 1);
}

TEST(RulesFlipTest, PlacementWithoutFlipIsAllowed)
{
    Board board = Board::createInitial();
    const auto flipped = rules::placeAndFlip(board, {0, 0}, Color::White);
    ASSERT_TRUE(flipped.has_value());
    EXPECT_EQ(*flipped, 0);
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
    EXPECT_EQ(*flipped, 5);
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
    EXPECT_EQ(*flipped, 2);
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

TEST(RulesFlipTest, ProtectedStoneIsSkippedButStillFormsRun)
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
    EXPECT_EQ(*flipped, 1);
    EXPECT_EQ(colorAt(board, {0, 1}), Color::Black);
    EXPECT_EQ(colorAt(board, {0, 2}), Color::White);  // 保護石は裏返らない
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

    EXPECT_EQ(rules::captureAll(board, Color::Black), 3);
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

TEST(RulesCaptureTest, ProtectedStonesAreNotCaptured)
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
    const std::vector<Position> expected{{0, 1}};
    EXPECT_EQ(rules::findCapturable(board, Color::Black), expected);
}

TEST(RulesCaptureTest, InitialBoardHasNothingToCapture)
{
    Board board = Board::createInitial();
    EXPECT_TRUE(rules::findCapturable(board, Color::Black).empty());
    EXPECT_TRUE(rules::findCapturable(board, Color::White).empty());
    EXPECT_EQ(rules::captureAll(board, Color::Black), 0);
}

} // namespace
