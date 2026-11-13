#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "cardflow/game.hpp"
#include "cardflow/ui/console_game.hpp"

using cardflow::CardType;
using cardflow::Game;

namespace {

std::string runWithInput(const std::string& input, std::uint32_t seed = 1)
{
    std::istringstream in(input);
    std::ostringstream out;
    cardflow::ui::runConsoleGame(in, out, seed);
    return out.str();
}

bool contains(const std::string& text, const std::string& part)
{
    return text.find(part) != std::string::npos;
}

TEST(ConsoleGameTest, QuitImmediately)
{
    const auto out = runWithInput("q\n");
    EXPECT_TRUE(contains(out, "ようこそ"));
    EXPECT_TRUE(contains(out, "ラウンド 1"));
    EXPECT_TRUE(contains(out, "中断"));
}

TEST(ConsoleGameTest, TwoPassesEndInDraw)
{
    const auto out = runWithInput("p\np\n");
    EXPECT_TRUE(contains(out, "ゲーム終了"));
    EXPECT_TRUE(contains(out, "引き分け"));
}

TEST(ConsoleGameTest, ReportsBadInput)
{
    const auto out = runWithInput("zzz\n9\nh\nq\n");
    EXPECT_TRUE(contains(out, "解釈できません"));
    EXPECT_TRUE(contains(out, "その番号のカードはありません"));
    EXPECT_TRUE(contains(out, "操作:"));
}

TEST(ConsoleGameTest, EndOfInputStopsGracefully)
{
    const auto out = runWithInput("");
    EXPECT_TRUE(contains(out, "入力が終わった"));
}

TEST(ConsoleGameTest, CardForNumberMapsHand)
{
    const Game game(1);
    const auto& hand = game.currentPlayer().hand();
    ASSERT_EQ(hand.size(), 3u);
    EXPECT_EQ(cardflow::ui::cardForNumber(game, 1), hand[0]);
    EXPECT_EQ(cardflow::ui::cardForNumber(game, 3), hand[2]);
    EXPECT_FALSE(cardflow::ui::cardForNumber(game, 4).has_value());  // 追加 Place なし
    EXPECT_FALSE(cardflow::ui::cardForNumber(game, 0).has_value());
}

TEST(ConsoleGameTest, StatusShowsHandAndCounts)
{
    const Game game(1);
    const auto status = cardflow::ui::renderStatus(game);
    EXPECT_TRUE(contains(status, "黒 ● の番"));
    EXPECT_TRUE(contains(status, "山札 24 枚"));
    EXPECT_TRUE(contains(status, "1) "));
    EXPECT_TRUE(contains(status, "3) "));
}

} // namespace
