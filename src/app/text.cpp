#include "cardflow/app/text.hpp"

namespace cardflow::app {

std::string formatPosition(Position pos)
{
    std::string text;
    text += static_cast<char>('a' + pos.col);
    text += static_cast<char>('1' + pos.row);
    return text;
}

std::string_view colorName(Color color) noexcept
{
    return color == Color::Black ? "黒" : "白";
}

} // namespace cardflow::app
