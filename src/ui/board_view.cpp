#include "cardflow/ui/board_view.hpp"

#include <string_view>

namespace cardflow::ui {

namespace {

std::string_view cellGlyph(const Board& board, Position pos)
{
    if (board.isWall(pos)) {
        return "■";
    }
    if (const auto stone = board.stoneAt(pos)) {
        if (stone->isProtected()) {
            return stone->color == Color::Black ? "◉" : "◎";
        }
        return stone->color == Color::Black ? "●" : "○";
    }
    return "・";
}

} // namespace

std::string renderBoard(const Board& board)
{
    std::string out = "  a b c d e f g h\n";
    for (int row = 0; row < Board::kSize; ++row) {
        out += static_cast<char>('1' + row);
        out += ' ';
        for (int col = 0; col < Board::kSize; ++col) {
            out += cellGlyph(board, Position{row, col});
        }
        out += '\n';
    }
    return out;
}

} // namespace cardflow::ui
