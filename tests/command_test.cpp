#include <gtest/gtest.h>

#include <vector>

#include "cardflow/ui/command.hpp"

using cardflow::Position;
using cardflow::ui::Command;
using cardflow::ui::formatPosition;
using cardflow::ui::parseCommand;
using cardflow::ui::parsePosition;

namespace {

TEST(CommandTest, ParsePosition)
{
    EXPECT_EQ(parsePosition("a1"), (Position{0, 0}));
    EXPECT_EQ(parsePosition("e3"), (Position{2, 4}));
    EXPECT_EQ(parsePosition("H8"), (Position{7, 7}));
    EXPECT_FALSE(parsePosition("i1").has_value());
    EXPECT_FALSE(parsePosition("a9").has_value());
    EXPECT_FALSE(parsePosition("a0").has_value());
    EXPECT_FALSE(parsePosition("a10").has_value());
    EXPECT_FALSE(parsePosition("").has_value());
}

TEST(CommandTest, FormatPositionRoundTrips)
{
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            const Position pos{row, col};
            EXPECT_EQ(parsePosition(formatPosition(pos)), pos);
        }
    }
    EXPECT_EQ(formatPosition({2, 4}), "e3");
}

TEST(CommandTest, ParsePlayCard)
{
    const auto command = parseCommand("  1 c4   F5 ");
    ASSERT_TRUE(command.has_value());
    EXPECT_EQ(command->kind, Command::Kind::PlayCard);
    EXPECT_EQ(command->cardNumber, 1);
    const std::vector<Position> expected{{3, 2}, {4, 5}};
    EXPECT_EQ(command->targets, expected);

    const auto noTarget = parseCommand("3");
    ASSERT_TRUE(noTarget.has_value());
    EXPECT_EQ(noTarget->cardNumber, 3);
    EXPECT_TRUE(noTarget->targets.empty());
}

TEST(CommandTest, ParseKeywords)
{
    EXPECT_EQ(parseCommand("p")->kind, Command::Kind::EndTurn);
    EXPECT_EQ(parseCommand("PASS")->kind, Command::Kind::EndTurn);
    EXPECT_EQ(parseCommand("h")->kind, Command::Kind::Help);
    EXPECT_EQ(parseCommand("?")->kind, Command::Kind::Help);
    EXPECT_EQ(parseCommand("q")->kind, Command::Kind::Quit);
}

TEST(CommandTest, ToleratesBomCarriageReturnAndFullWidthSpace)
{
    EXPECT_EQ(parseCommand("\xEF\xBB\xBFq")->kind, Command::Kind::Quit);
    EXPECT_EQ(parseCommand("p\r")->kind, Command::Kind::EndTurn);
    const auto command = parseCommand("2\xE3\x80\x80" "d4");
    ASSERT_TRUE(command.has_value());
    EXPECT_EQ(command->cardNumber, 2);
    EXPECT_EQ(command->targets, (std::vector<Position>{{3, 3}}));
}

TEST(CommandTest, RejectsMalformedInput)
{
    EXPECT_FALSE(parseCommand("").has_value());
    EXPECT_FALSE(parseCommand("   ").has_value());
    EXPECT_FALSE(parseCommand("0").has_value());
    EXPECT_FALSE(parseCommand("x c4").has_value());
    EXPECT_FALSE(parseCommand("1 z9").has_value());
    EXPECT_FALSE(parseCommand("p extra").has_value());
}

} // namespace
