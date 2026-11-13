#pragma once

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <string_view>

#include "cardflow/board.hpp"

namespace cardflow::test {

// 文字列から盤面を作るテスト用ヘルパー。
//   '.' 空 / 'B' 黒 / 'W' 白 / '#' 壁 / 'b' 'w' Shield 付きの黒・白
inline Board makeBoard(const std::array<std::string_view, Board::kSize>& rows)
{
    Board board;
    for (int row = 0; row < Board::kSize; ++row) {
        for (int col = 0; col < Board::kSize; ++col) {
            const Position pos{row, col};
            const char ch = rows[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
            bool ok = true;
            switch (ch) {
            case 'B': ok = board.placeStone(pos, Color::Black); break;
            case 'W': ok = board.placeStone(pos, Color::White); break;
            case '#': ok = board.placeWall(pos); break;
            case 'b':
            case 'w': {
                const Color color = ch == 'b' ? Color::Black : Color::White;
                ok = board.placeStone(pos, color) &&
                     board.replaceStone(pos, Stone{.color = color, .shielded = true});
                break;
            }
            default: break;
            }
            EXPECT_TRUE(ok) << "makeBoard failed at " << row << "," << col;
        }
    }
    return board;
}

inline Color colorAt(const Board& board, Position pos)
{
    return board.stoneAt(pos).value().color;
}

} // namespace cardflow::test
