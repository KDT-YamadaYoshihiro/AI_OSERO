#include "cardflow/rules.hpp"

namespace cardflow::rules {

namespace {

constexpr int kCells = Board::kSize * Board::kSize;

Position cellAt(int index)
{
    return Position{index / Board::kSize, index % Board::kSize};
}

// dir 方向に、相手の石が 1 つ以上続いた先に自分の石があれば、その相手の石を out に加える
void collectLine(const Board& board, Position pos, Position dir, Color color, std::vector<Position>& out)
{
    const auto start = out.size();
    Position cur{pos.row + dir.row, pos.col + dir.col};
    while (true) {
        const auto stone = board.stoneAt(cur);
        if (!stone.has_value()) {
            break;  // 空マス・壁・盤の端
        }
        if (stone->color == color) {
            return;  // 挟めた
        }
        out.push_back(cur);
        cur = Position{cur.row + dir.row, cur.col + dir.col};
    }
    out.resize(start);  // 挟めなかった
}

} // namespace

std::vector<Position> findFlips(const Board& board, Position pos, Color color)
{
    std::vector<Position> flips;
    if (!board.isEmpty(pos)) {
        return flips;
    }
    for (const auto& dir : Board::kDirections) {
        collectLine(board, pos, dir, color, flips);
    }
    return flips;
}

bool isLegalMove(const Board& board, Position pos, Color color)
{
    if (!board.isEmpty(pos)) {
        return false;
    }
    std::vector<Position> line;
    for (const auto& dir : Board::kDirections) {
        collectLine(board, pos, dir, color, line);
        if (!line.empty()) {
            return true;
        }
    }
    return false;
}

std::vector<Position> legalMoves(const Board& board, Color color)
{
    std::vector<Position> moves;
    for (int i = 0; i < kCells; ++i) {
        if (isLegalMove(board, cellAt(i), color)) {
            moves.push_back(cellAt(i));
        }
    }
    return moves;
}

int countLegalMoves(const Board& board, Color color)
{
    int count = 0;
    for (int i = 0; i < kCells; ++i) {
        if (isLegalMove(board, cellAt(i), color)) {
            ++count;
        }
    }
    return count;
}

bool isFreeCell(const Board& board, Position pos)
{
    return board.isEmpty(pos) && board.hasAdjacentStone(pos);
}

PlaceResult placeAndFlip(Board& board, Position pos, Color color)
{
    PlaceResult result;
    const auto flips = findFlips(board, pos, color);
    if (!board.placeStone(pos, color)) {
        return result;
    }
    for (const auto& target : flips) {
        auto stone = *board.stoneAt(target);
        if (stone.guarded) {
            stone.guarded = false;
            ++result.guardsBroken;
        } else {
            stone.color = color;
            result.flipped.push_back(target);
        }
        (void)board.setStone(target, stone);
    }
    return result;
}

} // namespace cardflow::rules
