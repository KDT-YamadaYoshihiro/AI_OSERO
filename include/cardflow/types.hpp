#pragma once

#include <cstdint>

namespace cardflow {

// プレイヤー／石の色
enum class Color : std::uint8_t {
    Black,
    White,
};

// 相手の色を返す
[[nodiscard]] constexpr Color opponentOf(Color color) noexcept
{
    return color == Color::Black ? Color::White : Color::Black;
}

// 盤面上の座標（0 始まり。row = 行、col = 列）
struct Position {
    int row = 0;
    int col = 0;

    friend constexpr bool operator==(const Position&, const Position&) = default;
};

} // namespace cardflow
