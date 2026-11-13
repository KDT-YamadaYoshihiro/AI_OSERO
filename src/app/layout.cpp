#include "cardflow/app/layout.hpp"

#include "cardflow/board.hpp"

namespace cardflow::app {

using namespace layout;

namespace {

constexpr int kButtonTop = kHandTop + kCardHeight + 20;
constexpr int kButtonHeight = 48;
constexpr int kMenuButtonWidth = 360;
constexpr int kMenuButtonHeight = 64;
constexpr int kMenuTop = 300;
constexpr int kMenuGap = 24;

} // namespace

Rect boardRect() noexcept
{
    return Rect{kBoardLeft, kBoardTop, kBoardPixels, kBoardPixels};
}

Rect cellRect(Position pos) noexcept
{
    return Rect{kBoardLeft + pos.col * kCellSize, kBoardTop + pos.row * kCellSize, kCellSize, kCellSize};
}

std::optional<Position> cellAt(int x, int y) noexcept
{
    if (!boardRect().contains(x, y)) {
        return std::nullopt;
    }
    const Position pos{(y - kBoardTop) / kCellSize, (x - kBoardLeft) / kCellSize};
    if (!Board::isInside(pos)) {
        return std::nullopt;
    }
    return pos;
}

Rect statusRect() noexcept
{
    return Rect{kPanelLeft, kBoardTop, kPanelWidth, 100};
}

Rect handCardRect(int slot) noexcept
{
    return Rect{kPanelLeft + slot * (kCardWidth + kCardGap), kHandTop, kCardWidth, kCardHeight};
}

Rect useButtonRect() noexcept
{
    return Rect{kPanelLeft, kButtonTop, 160, kButtonHeight};
}

Rect cancelButtonRect() noexcept
{
    return Rect{kPanelLeft + 176, kButtonTop, 160, kButtonHeight};
}

Rect endTurnButtonRect() noexcept
{
    return Rect{kPanelLeft + kPanelWidth - 200, kButtonTop, 200, kButtonHeight};
}

Rect hintRect() noexcept
{
    return Rect{kPanelLeft, kButtonTop + kButtonHeight + 12, kPanelWidth, 32};
}

Rect logRect() noexcept
{
    const int top = hintRect().bottom() + 8;
    return Rect{kPanelLeft, top, kPanelWidth, kBoardTop + kBoardPixels - top};
}

Rect menuButtonRect(int index) noexcept
{
    return Rect{(kWindowWidth - kMenuButtonWidth) / 2, kMenuTop + index * (kMenuButtonHeight + kMenuGap),
                kMenuButtonWidth, kMenuButtonHeight};
}

Rect backButtonRect() noexcept
{
    const Rect last = menuButtonRect(kMaxMenuButtons);
    return Rect{(kWindowWidth - 200) / 2, last.y + 8, 200, 48};
}

Rect resultPanelRect() noexcept
{
    return Rect{(kWindowWidth - 560) / 2, 200, 560, 300};
}

Rect retryButtonRect() noexcept
{
    const Rect panel = resultPanelRect();
    return Rect{panel.x + 40, panel.bottom() - 88, 220, 56};
}

Rect toTitleButtonRect() noexcept
{
    const Rect panel = resultPanelRect();
    return Rect{panel.right() - 260, panel.bottom() - 88, 220, 56};
}

} // namespace cardflow::app
