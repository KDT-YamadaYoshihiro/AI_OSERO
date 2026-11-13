#include <gtest/gtest.h>

#include <string>

#include "cardflow/board.hpp"
#include "cardflow/ui/board_view.hpp"

using cardflow::Board;
using cardflow::Color;
using cardflow::ui::renderBoard;

namespace {

TEST(BoardViewTest, RendersInitialBoard)
{
    const std::string expected =
        "  a b c d e f g h\n"
        "1 ・・・・・・・・\n"
        "2 ・・・・・・・・\n"
        "3 ・・・・・・・・\n"
        "4 ・・・●○・・・\n"
        "5 ・・・○●・・・\n"
        "6 ・・・・・・・・\n"
        "7 ・・・・・・・・\n"
        "8 ・・・・・・・・\n";
    EXPECT_EQ(renderBoard(Board::createInitial()), expected);
}

TEST(BoardViewTest, RendersWall)
{
    Board board;
    ASSERT_TRUE(board.placeWall({0, 7}, Color::White));
    EXPECT_NE(renderBoard(board).find("1 ・・・・・・・■\n"), std::string::npos);
}

} // namespace
