#include <gtest/gtest.h>

#include "cardflow/app/text.hpp"

using cardflow::Color;
using cardflow::Position;
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

} // namespace
