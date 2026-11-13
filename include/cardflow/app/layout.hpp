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

// 盤面
constexpr int kCellSize = 72;
constexpr int kBoardLeft = 56;
constexpr int kBoardTop = 104;
constexpr int kBoardPixels = kCellSize * 8;

// 右側のパネル
constexpr int kPanelLeft = 680;
constexpr int kPanelWidth = kWindowWidth - kPanelLeft - 24;

// 手札（通常 3 枚＋追加 Place 1 枚まで）
constexpr int kMaxHandSlots = 4;
constexpr int kCardWidth = 132;
constexpr int kCardHeight = 180;
constexpr int kCardGap = 12;
constexpr int kHandTop = 236;

// メニュー（タイトル・強さ・色の選択）のボタン数
constexpr int kMaxMenuButtons = 3;

} // namespace layout

[[nodiscard]] Rect boardRect() noexcept;
[[nodiscard]] Rect cellRect(Position pos) noexcept;
// 画面座標にある盤面のマス（盤外なら std::nullopt）
[[nodiscard]] std::optional<Position> cellAt(int x, int y) noexcept;

[[nodiscard]] Rect statusRect() noexcept;           // 石数・EP・山札
[[nodiscard]] Rect handCardRect(int slot) noexcept;  // slot: 0..kMaxHandSlots-1
[[nodiscard]] Rect useButtonRect() noexcept;
[[nodiscard]] Rect cancelButtonRect() noexcept;
[[nodiscard]] Rect endTurnButtonRect() noexcept;
[[nodiscard]] Rect hintRect() noexcept;
[[nodiscard]] Rect logRect() noexcept;

[[nodiscard]] Rect menuButtonRect(int index) noexcept;  // index: 0..kMaxMenuButtons-1
[[nodiscard]] Rect backButtonRect() noexcept;

[[nodiscard]] Rect resultPanelRect() noexcept;
[[nodiscard]] Rect retryButtonRect() noexcept;
[[nodiscard]] Rect toTitleButtonRect() noexcept;

} // namespace cardflow::app
