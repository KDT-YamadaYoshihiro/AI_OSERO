#include "cardflow/rules.hpp"

#include <algorithm>
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

Position step(Position pos, Position dir)
{
    return Position{pos.row + dir.row, pos.col + dir.col};
}

// 4 本の軸（横・縦・右下がり斜め・右上がり斜め）の正方向
constexpr std::array<Position, 4> kAxes{{{0, 1}, {1, 0}, {1, 1}, {-1, 1}}};

enum class StrikeKind { Flip, Remove };

StrikeResult strike(Board& board, std::span<const Position> targets, StrikeKind kind)
{
    StrikeResult result;
    for (const auto& pos : targets) {
        const auto stone = board.stoneAt(pos);
        if (!stone.has_value()) {
            continue;
        }
        if (stone->covered) {
            ++result.blockedByCover;
            continue;
        }
        if (stone->shielded) {
            Stone updated = *stone;
            updated.shielded = false;
            if (board.replaceStone(pos, updated)) {
                ++result.shieldsBroken;
            }
            continue;
        }
        const bool done = kind == StrikeKind::Flip ? board.flipStone(pos) : board.removeStone(pos);
        if (done) {
            ++result.affected;
        }
    }
    return result;
}

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
        if (!run.empty() && isAnchor(board, cur, mover)) {
            result.insert(result.end(), run.begin(), run.end());
        }
    }
    return result;
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
            if (marked[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)]) {
                result.push_back(Position{row, col});
            }
        }
    }
    return result;
}

bool isSandwiched(const Board& board, Position pos, Color capturer)
{
    if (!isEnemyStone(board, pos, capturer)) {
        return false;
    }
    const auto capturable = findCapturable(board, capturer);
    return std::ranges::find(capturable, pos) != capturable.end();
}

StrikeResult strikeFlip(Board& board, std::span<const Position> targets)
{
    return strike(board, targets, StrikeKind::Flip);
}

StrikeResult strikeRemove(Board& board, std::span<const Position> targets)
{
    return strike(board, targets, StrikeKind::Remove);
}

std::optional<StrikeResult> placeAndFlip(Board& board, Position pos, Color mover)
{
    if (!board.placeStone(pos, mover)) {
        return std::nullopt;
    }
    return flipFrom(board, pos, mover);
}

StrikeResult flipFrom(Board& board, Position pos, Color mover)
{
    const auto flips = findFlips(board, pos, mover);
    return strikeFlip(board, flips);
}

StrikeResult captureAll(Board& board, Color capturer)
{
    const auto targets = findCapturable(board, capturer);
    return strikeRemove(board, targets);
}

} // namespace cardflow::rules
