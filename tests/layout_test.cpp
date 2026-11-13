#include <gtest/gtest.h>

#include <vector>

#include "cardflow/app/layout.hpp"
#include "cardflow/board.hpp"

using cardflow::Board;
using cardflow::Position;
using cardflow::app::Rect;
namespace app = cardflow::app;
namespace layout = cardflow::app::layout;

namespace {

bool overlaps(const Rect& a, const Rect& b)
{
    return a.x < b.right() && b.x < a.right() && a.y < b.bottom() && b.y < a.bottom();
}

bool insideWindow(const Rect& rect)
{
    return rect.x >= 0 && rect.y >= 0 && rect.right() <= layout::kWindowWidth &&
           rect.bottom() <= layout::kWindowHeight;
}

TEST(LayoutTest, CellCenterMapsBackToCell)
{
    for (int row = 0; row < Board::kSize; ++row) {
        for (int col = 0; col < Board::kSize; ++col) {
            const Position pos{row, col};
            const Rect cell = app::cellRect(pos);
            EXPECT_EQ(app::cellAt(cell.centerX(), cell.centerY()), pos);
            EXPECT_EQ(app::cellAt(cell.x, cell.y), pos);
            EXPECT_EQ(app::cellAt(cell.right() - 1, cell.bottom() - 1), pos);
        }
    }
}

TEST(LayoutTest, OutsideBoardIsNoCell)
{
    const Rect board = app::boardRect();
    EXPECT_FALSE(app::cellAt(board.x - 1, board.y).has_value());
    EXPECT_FALSE(app::cellAt(board.x, board.y - 1).has_value());
    EXPECT_FALSE(app::cellAt(board.right(), board.y).has_value());
    EXPECT_FALSE(app::cellAt(board.x, board.bottom()).has_value());
}

TEST(LayoutTest, PlayingAreasFitAndDoNotOverlap)
{
    std::vector<Rect> areas{app::boardRect(),      app::statusRect(),       app::useButtonRect(),
                            app::cancelButtonRect(), app::endTurnButtonRect(), app::hintRect(),
                            app::logRect()};
    for (int slot = 0; slot < layout::kMaxHandSlots; ++slot) {
        areas.push_back(app::handCardRect(slot));
    }
    for (std::size_t i = 0; i < areas.size(); ++i) {
        EXPECT_TRUE(insideWindow(areas[i])) << i;
        for (std::size_t j = i + 1; j < areas.size(); ++j) {
            EXPECT_FALSE(overlaps(areas[i], areas[j])) << i << " と " << j;
        }
    }
}

TEST(LayoutTest, MenuButtonsFitAndDoNotOverlap)
{
    std::vector<Rect> buttons{app::backButtonRect()};
    for (int i = 0; i < layout::kMaxMenuButtons; ++i) {
        buttons.push_back(app::menuButtonRect(i));
    }
    for (std::size_t i = 0; i < buttons.size(); ++i) {
        EXPECT_TRUE(insideWindow(buttons[i])) << i;
        for (std::size_t j = i + 1; j < buttons.size(); ++j) {
            EXPECT_FALSE(overlaps(buttons[i], buttons[j])) << i << " と " << j;
        }
    }
}

TEST(LayoutTest, ResultButtonsAreInsidePanel)
{
    const Rect panel = app::resultPanelRect();
    for (const Rect& button : {app::retryButtonRect(), app::toTitleButtonRect()}) {
        EXPECT_TRUE(panel.contains(button.x, button.y));
        EXPECT_TRUE(panel.contains(button.right() - 1, button.bottom() - 1));
    }
    EXPECT_FALSE(overlaps(app::retryButtonRect(), app::toTitleButtonRect()));
}

} // namespace
