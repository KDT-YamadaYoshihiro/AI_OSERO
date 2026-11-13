#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>

#include "cardflow/app/text.hpp"

using cardflow::CardType;
using cardflow::Color;
using cardflow::kCardTypeCount;
using cardflow::PlayResult;
using cardflow::Position;
using cardflow::targetCount;
namespace app = cardflow::app;

namespace {

TEST(TextTest, FormatPosition)
{
    EXPECT_EQ(app::formatPosition(Position{0, 0}), "a1");
    EXPECT_EQ(app::formatPosition(Position{2, 4}), "e3");
    EXPECT_EQ(app::formatPosition(Position{7, 7}), "h8");
}

TEST(TextTest, ColorName)
{
    EXPECT_EQ(app::colorName(Color::Black), "黒");
    EXPECT_EQ(app::colorName(Color::White), "白");
}

TEST(TextTest, EveryCardHasDescriptionAndPrompts)
{
    for (std::size_t i = 0; i < kCardTypeCount; ++i) {
        const auto type = static_cast<CardType>(i);
        EXPECT_FALSE(app::cardDescription(type)[0].empty()) << i;
        for (int step = 0; step < std::max(1, targetCount(type)); ++step) {
            EXPECT_FALSE(app::targetPrompt(type, step).empty()) << i << " step " << step;
        }
    }
}

TEST(TextTest, DescribeResultListsOnlyNonZero)
{
    EXPECT_EQ(app::describeResult(PlayResult{}), "");
    PlayResult result;
    result.flipped = 2;
    result.shieldsBroken = 1;
    EXPECT_EQ(app::describeResult(result), " 裏返し2 Shield消費1");
}

} // namespace
