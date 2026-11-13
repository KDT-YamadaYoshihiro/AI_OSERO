#include "cardflow/board.hpp"

namespace cardflow {

Board Board::createInitial()
{
    Board board;
    (void)board.placeStone(Position{3, 3}, Color::White);  // d4
    (void)board.placeStone(Position{3, 4}, Color::Black);  // e4
    (void)board.placeStone(Position{4, 3}, Color::Black);  // d5
    (void)board.placeStone(Position{4, 4}, Color::White);  // e5
    return board;
}

const Board::Cell& Board::cellAt(Position pos) const
{
    return m_cells[static_cast<std::size_t>(pos.row)][static_cast<std::size_t>(pos.col)];
}

Board::Cell& Board::cellAt(Position pos)
{
    return m_cells[static_cast<std::size_t>(pos.row)][static_cast<std::size_t>(pos.col)];
}

bool Board::isEmpty(Position pos) const
{
    return isInside(pos) && std::holds_alternative<std::monostate>(cellAt(pos));
}

bool Board::isWall(Position pos) const
{
    return isInside(pos) && std::holds_alternative<Wall>(cellAt(pos));
}

std::optional<Stone> Board::stoneAt(Position pos) const
{
    if (!isInside(pos)) {
        return std::nullopt;
    }
    if (const auto* stone = std::get_if<Stone>(&cellAt(pos))) {
        return *stone;
    }
    return std::nullopt;
}

bool Board::hasAdjacentStone(Position pos) const
{
    for (const auto& dir : kDirections) {
        if (stoneAt(Position{pos.row + dir.row, pos.col + dir.col}).has_value()) {
            return true;
        }
    }
    return false;
}

int Board::countStones(Color color) const
{
    int count = 0;
    for (const auto& row : m_cells) {
        for (const auto& cell : row) {
            if (const auto* stone = std::get_if<Stone>(&cell); stone != nullptr && stone->color == color) {
                ++count;
            }
        }
    }
    return count;
}

int Board::countEmpty() const
{
    int count = 0;
    for (const auto& row : m_cells) {
        for (const auto& cell : row) {
            if (std::holds_alternative<std::monostate>(cell)) {
                ++count;
            }
        }
    }
    return count;
}

bool Board::placeStone(Position pos, Color color)
{
    if (!isEmpty(pos)) {
        return false;
    }
    cellAt(pos) = Stone{color, false};
    return true;
}

bool Board::placeWall(Position pos)
{
    if (!isEmpty(pos)) {
        return false;
    }
    cellAt(pos) = Wall{};
    return true;
}

bool Board::setStone(Position pos, const Stone& stone)
{
    if (!stoneAt(pos).has_value()) {
        return false;
    }
    cellAt(pos) = stone;
    return true;
}

} // namespace cardflow
