#include "cardflow/card.hpp"

namespace cardflow {

namespace {

// 表の並びと enum の値が一致していること
constexpr bool tableMatchesEnum()
{
    for (std::size_t i = 0; i < kCardTypeCount; ++i) {
        if (static_cast<std::size_t>(kCardTable[i].type) != i) {
            return false;
        }
    }
    return true;
}
static_assert(tableMatchesEnum(), "kCardTable の並びは CardType の定義順に合わせる");

} // namespace

bool isValidCardTarget(CardType type, const Board& board, Color user, Position target)
{
    const auto stone = board.stoneAt(target);
    switch (type) {
    case CardType::Guard:
        return stone.has_value() && stone->color == user && !stone->guarded;
    case CardType::Wall:
        // 石の隣（8 方向のどこかに石がある空マス）にだけ置ける
        return board.isEmpty(target) && board.hasAdjacentStone(target);
    case CardType::Flip:
        return stone.has_value() && stone->color != user && !stone->guarded && !Board::isCorner(target);
    case CardType::Double:
    case CardType::Free:
        return false;
    }
    return false;
}

bool applyCardTarget(CardType type, Board& board, Color user, Position target)
{
    if (!isValidCardTarget(type, board, user, target)) {
        return false;
    }
    switch (type) {
    case CardType::Guard:
        return board.setStone(target, Stone{user, true});
    case CardType::Wall:
        return board.placeWall(target);
    case CardType::Flip:
        return board.setStone(target, Stone{user, false});
    case CardType::Double:
    case CardType::Free:
        return false;
    }
    return false;
}

} // namespace cardflow
