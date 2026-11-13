#pragma once

#include <optional>

#include "cardflow/types.hpp"

// GUI の画面配置（1280×720）。描画とクリック判定の両方がここを使う。
namespace cardflow::app {

struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    [[nodiscard]] constexpr bool contains(int px, int py) const noexcept
    {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
    [[nodiscard]] constexpr int right() const noexcept { return x + w; }
    [[nodiscard]] constexpr int bottom() const noexcept { return y + h; }
    [[nodiscard]] constexpr int centerX() const noexcept { return x + w / 2; }
    [[nodiscard]] constexpr int centerY() const noexcept { return y + h / 2; }

    friend constexpr bool operator==(const Rect&, const Rect&) = default;
};

namespace layout {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;

// 盤面（画面中央）
constexpr int kCellSize = 64;
constexpr int kBoardPixels = kCellSize * 8;
constexpr int kBoardLeft = (kWindowWidth - kBoardPixels) / 2;
constexpr int kBoardTop = 112;

// 手札（画面下）
constexpr int kMaxHandSlots = 3;
constexpr int kCardWidth = 200;
constexpr int kCardHeight = 72;
constexpr int kCardGap = 16;
constexpr int kHandTop = kBoardTop + kBoardPixels + 12;

// メニュー（タイトル・強さ・色の選択）のボタン数
constexpr int kMaxMenuButtons = 3;

} // namespace layout

[[nodiscard]] Rect boardRect() noexcept;
[[nodiscard]] Rect cellRect(Position pos) noexcept;
// 画面座標にある盤面のマス（盤外なら std::nullopt）
[[nodiscard]] std::optional<Position> cellAt(int x, int y) noexcept;

[[nodiscard]] Rect hintRect() noexcept;     // いまやること
[[nodiscard]] Rect messageRect() noexcept;  // 直前の出来事
[[nodiscard]] Rect playerPanelRect(Color color) noexcept;  // 黒は左、白は右
[[nodiscard]] Rect handLabelRect() noexcept;
[[nodiscard]] Rect handCardRect(int slot) noexcept;  // slot: 0..kMaxHandSlots-1
[[nodiscard]] Rect passButtonRect() noexcept;

[[nodiscard]] Rect menuButtonRect(int index) noexcept;  // index: 0..kMaxMenuButtons-1
[[nodiscard]] Rect backButtonRect() noexcept;

[[nodiscard]] Rect resultPanelRect() noexcept;
[[nodiscard]] Rect retryButtonRect() noexcept;
[[nodiscard]] Rect toTitleButtonRect() noexcept;

} // namespace cardflow::app
