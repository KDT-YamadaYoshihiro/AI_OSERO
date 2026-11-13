#include "cardflow/app/layout.hpp"

#include "cardflow/board.hpp"

namespace cardflow::app {

using namespace layout;

namespace {

constexpr int kSideMargin = 48;
constexpr int kPanelGap = 48;
constexpr int kHandWidth = kMaxHandSlots * kCardWidth + (kMaxHandSlots - 1) * kCardGap;
constexpr int kHandLeft = (kWindowWidth - kHandWidth) / 2;
constexpr int kMenuButtonWidth = 360;
constexpr int kMenuButtonHeight = 60;
constexpr int kMenuTop = 290;
constexpr int kMenuGap = 18;
constexpr int kMenuBarButtonWidth = 150;

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

Rect menuBarRect() noexcept
{
    return Rect{0, 0, kWindowWidth, kMenuBarHeight};
}

Rect menuBarButtonRect(int index) noexcept
{
    return Rect{12 + index * (kMenuBarButtonWidth + 8), 4, kMenuBarButtonWidth, kMenuBarHeight - 8};
}

Rect hintRect() noexcept
{
    return Rect{kSideMargin, kMenuBarHeight + 6, kWindowWidth - kSideMargin * 2, 34};
}

Rect messageRect() noexcept
{
    return Rect{kSideMargin, hintRect().bottom(), kWindowWidth - kSideMargin * 2, 24};
}

Rect playerPanelRect(Color color) noexcept
{
    const int width = kBoardLeft - kSideMargin - kPanelGap;
    const int x = color == Color::Black ? kSideMargin : kBoardLeft + kBoardPixels + kPanelGap;
    return Rect{x, kBoardTop, width, 300};
}

Rect handLabelRect() noexcept
{
    return Rect{kSideMargin, kHandTop, kHandLeft - kSideMargin - kCardGap, kCardHeight};
}

Rect handCardRect(int slot) noexcept
{
    return Rect{kHandLeft + slot * (kCardWidth + kCardGap), kHandTop, kCardWidth, kCardHeight};
}

Rect passButtonRect() noexcept
{
    const int width = 200;
    return Rect{kWindowWidth - kSideMargin - width, kHandTop + 2, width, kCardHeight - 4};
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

Rect overlayPanelRect() noexcept
{
    return Rect{(kWindowWidth - 480) / 2, 180, 480, 360};
}

Rect overlayButtonRect(int index) noexcept
{
    const Rect panel = overlayPanelRect();
    return Rect{panel.x + 90, panel.y + 110 + index * 76, panel.w - 180, 56};
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
