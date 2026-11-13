#include "cardflow/board.hpp"

#include <utility>

namespace cardflow {

Board Board::createInitial()
{
    Board board;
    board.m_cells[3][3] = Stone{.color = Color::Black};
    board.m_cells[3][4] = Stone{.color = Color::White};
    board.m_cells[4][3] = Stone{.color = Color::White};
    board.m_cells[4][4] = Stone{.color = Color::Black};
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

bool Board::hasStone(Position pos) const
{
    return isInside(pos) && std::holds_alternative<Stone>(cellAt(pos));
}

bool Board::isWall(Position pos) const
{
    return isInside(pos) && std::holds_alternative<Wall>(cellAt(pos));
}

std::optional<Stone> Board::stoneAt(Position pos) const
{
    if (!hasStone(pos)) {
        return std::nullopt;
    }
    return std::get<Stone>(cellAt(pos));
}

std::optional<Wall> Board::wallAt(Position pos) const
{
    if (!isWall(pos)) {
        return std::nullopt;
    }
    return std::get<Wall>(cellAt(pos));
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

std::vector<Position> Board::neighbors(Position pos) const
{
    std::vector<Position> result;
    if (!isInside(pos)) {
        return result;
    }
    for (const auto& dir : kDirections) {
        const Position next{pos.row + dir.row, pos.col + dir.col};
        if (isInside(next)) {
            result.push_back(next);
        }
    }
    return result;
}

bool Board::placeStone(Position pos, Color color)
{
    if (!isEmpty(pos)) {
        return false;
    }
    cellAt(pos) = Stone{.color = color};
    return true;
}

bool Board::placeWall(Position pos, std::optional<Color> owner)
{
    if (!isEmpty(pos)) {
        return false;
    }
    cellAt(pos) = Wall{.owner = owner};
    return true;
}

bool Board::removeStone(Position pos)
{
    if (!hasStone(pos)) {
        return false;
    }
    cellAt(pos) = std::monostate{};
    return true;
}

bool Board::flipStone(Position pos)
{
    if (!hasStone(pos)) {
        return false;
    }
    auto& stone = std::get<Stone>(cellAt(pos));
    stone.color = opponentOf(stone.color);
    return true;
}

bool Board::replaceStone(Position pos, const Stone& stone)
{
    if (!hasStone(pos)) {
        return false;
    }
    cellAt(pos) = stone;
    return true;
}

bool Board::moveStone(Position from, Position to)
{
    if (!hasStone(from) || !isEmpty(to)) {
        return false;
    }
    cellAt(to) = std::exchange(cellAt(from), std::monostate{});
    return true;
}

bool Board::swapStones(Position a, Position b)
{
    if (!hasStone(a) || !hasStone(b) || a == b) {
        return false;
    }
    std::swap(cellAt(a), cellAt(b));
    return true;
}

} // namespace cardflow
