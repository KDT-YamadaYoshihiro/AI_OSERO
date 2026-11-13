#include "cardflow/rules.hpp"

#include <array>
#include <cstddef>

namespace cardflow::rules {

namespace {

// 挟む側（自分の石 or 壁）か
bool isAnchor(const Board& board, Position pos, Color owner)
{
    if (board.isWall(pos)) {
        return true;
    }
    const auto stone = board.stoneAt(pos);
    return stone.has_value() && stone->color == owner;
}

bool isEnemyStone(const Board& board, Position pos, Color owner)
{
    const auto stone = board.stoneAt(pos);
    return stone.has_value() && stone->color != owner;
}

bool isProtectedStone(const Board& board, Position pos)
{
    const auto stone = board.stoneAt(pos);
    return stone.has_value() && stone->isProtected();
}

Position step(Position pos, Position dir)
{
    return Position{pos.row + dir.row, pos.col + dir.col};
}

// 4 本の軸（横・縦・右下がり斜め・右上がり斜め）の正方向
constexpr std::array<Position, 4> kAxes{{{0, 1}, {1, 0}, {1, 1}, {-1, 1}}};

} // namespace

std::vector<Position> findFlips(const Board& board, Position pos, Color mover)
{
    std::vector<Position> result;
    if (!Board::isInside(pos)) {
        return result;
    }
    for (const auto& dir : Board::kDirections) {
        std::vector<Position> run;
        Position cur = step(pos, dir);
        while (isEnemyStone(board, cur, mover)) {
            run.push_back(cur);
            cur = step(cur, dir);
        }
        if (run.empty() || !isAnchor(board, cur, mover)) {
            continue;
        }
        for (const auto& p : run) {
            if (!isProtectedStone(board, p)) {
                result.push_back(p);
            }
        }
    }
    return result;
}

std::optional<int> placeAndFlip(Board& board, Position pos, Color mover)
{
    if (!board.isEmpty(pos)) {
        return std::nullopt;
    }
    const auto flips = findFlips(board, pos, mover);
    if (!board.placeStone(pos, mover)) {
        return std::nullopt;
    }
    int flipped = 0;
    for (const auto& p : flips) {
        if (board.flipStone(p)) {
            ++flipped;
        }
    }
    return flipped;
}

std::vector<Position> findCapturable(const Board& board, Color capturer)
{
    std::array<std::array<bool, Board::kSize>, Board::kSize> marked{};

    for (const auto& axis : kAxes) {
        const Position back{-axis.row, -axis.col};
        for (int row = 0; row < Board::kSize; ++row) {
            for (int col = 0; col < Board::kSize; ++col) {
                const Position start{row, col};
                // 列の先頭（軸の逆方向が敵石でない）からのみ走査する
                if (!isEnemyStone(board, start, capturer) || isEnemyStone(board, step(start, back), capturer)) {
                    continue;
                }
                std::vector<Position> run;
                Position cur = start;
                while (isEnemyStone(board, cur, capturer)) {
                    run.push_back(cur);
                    cur = step(cur, axis);
                }
                if (!isAnchor(board, step(start, back), capturer) || !isAnchor(board, cur, capturer)) {
                    continue;
                }
                for (const auto& p : run) {
                    marked[static_cast<std::size_t>(p.row)][static_cast<std::size_t>(p.col)] = true;
                }
            }
        }
    }

    std::vector<Position> result;
    for (int row = 0; row < Board::kSize; ++row) {
        for (int col = 0; col < Board::kSize; ++col) {
            const Position pos{row, col};
            if (marked[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] && !isProtectedStone(board, pos)) {
                result.push_back(pos);
            }
        }
    }
    return result;
}

int captureAll(Board& board, Color capturer)
{
    int captured = 0;
    for (const auto& pos : findCapturable(board, capturer)) {
        if (board.removeStone(pos)) {
            ++captured;
        }
    }
    return captured;
}

} // namespace cardflow::rules
