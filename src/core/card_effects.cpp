#include "cardflow/card_effects.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <vector>

#include "cardflow/rules.hpp"

namespace cardflow {

namespace {

constexpr int kPlaceStones = 2;
constexpr int kMaxMoveDistance = 3;
constexpr int kRestoreAmount = 2;
constexpr int kReservePlaces = 2;

bool isOwnStone(const Board& board, Position pos, Color color)
{
    const auto stone = board.stoneAt(pos);
    return stone.has_value() && stone->color == color;
}

bool isEnemyStone(const Board& board, Position pos, Color color)
{
    const auto stone = board.stoneAt(pos);
    return stone.has_value() && stone->color != color;
}

int chebyshevDistance(Position a, Position b)
{
    return std::max(std::abs(a.row - b.row), std::abs(a.col - b.col));
}

bool hasAdjacentEnemy(const Board& board, Position pos, Color color)
{
    for (const auto& n : board.neighbors(pos)) {
        if (isEnemyStone(board, n, color)) {
            return true;
        }
    }
    return false;
}

// Move: 8 方向の直線で 1〜3 マス、途中と移動先が空マス
bool isValidMove(const Board& board, Color color, Position from, Position to)
{
    if (!isOwnStone(board, from, color) || !board.isEmpty(to)) {
        return false;
    }
    const int dr = to.row - from.row;
    const int dc = to.col - from.col;
    const bool straight = dr == 0 || dc == 0 || std::abs(dr) == std::abs(dc);
    const int distance = chebyshevDistance(from, to);
    if (!straight || distance < 1 || distance > kMaxMoveDistance) {
        return false;
    }
    const Position dir{(dr > 0) - (dr < 0), (dc > 0) - (dc < 0)};
    for (int i = 1; i < distance; ++i) {
        if (!board.isEmpty(Position{from.row + dir.row * i, from.col + dir.col * i})) {
            return false;
        }
    }
    return true;
}

void accumulateFlips(PlayResult& result, const rules::StrikeResult& strike)
{
    result.flipped += strike.affected;
    result.blockedByCover += strike.blockedByCover;
    result.shieldsBroken += strike.shieldsBroken;
}

void accumulateCaptures(PlayResult& result, const rules::StrikeResult& strike)
{
    result.captured += strike.affected;
    result.blockedByCover += strike.blockedByCover;
    result.shieldsBroken += strike.shieldsBroken;
}

bool hasCardToPlay(const Player& player, CardType type)
{
    return player.hasCard(type) || (type == CardType::Place && player.bonusPlaces() > 0);
}

} // namespace

bool canApplyEffect(CardType type, const Board& board, const Player& actor, std::span<const Position> targets)
{
    const Color me = actor.color();
    switch (type) {
    case CardType::Place:
        return targets.size() == kPlaceStones && targets[0] != targets[1] && board.isEmpty(targets[0]) &&
               board.isEmpty(targets[1]);
    case CardType::Move:
        return targets.size() == 2 && isValidMove(board, me, targets[0], targets[1]);
    case CardType::Cover:
        return targets.empty() && board.countStones(me) > 0;
    case CardType::Burst:
        return targets.empty() && !rules::findCapturable(board, me).empty();
    case CardType::Swap:
        return targets.size() == 2 && isOwnStone(board, targets[0], me) && isEnemyStone(board, targets[1], me) &&
               chebyshevDistance(targets[0], targets[1]) == 1;
    case CardType::Block:
        return targets.size() == 1 && board.isEmpty(targets[0]);
    case CardType::Shield:
        return targets.size() == 1 && isOwnStone(board, targets[0], me) && !board.stoneAt(targets[0])->shielded;
    case CardType::Reversal:
        return targets.size() == 1 && isOwnStone(board, targets[0], me) && hasAdjacentEnemy(board, targets[0], me);
    case CardType::Reserve:
        return targets.empty() && actor.reservedPlaces() < Player::kMaxReservedPlaces;
    case CardType::Restore:
        return targets.empty();
    }
    return false;
}

bool hasValidTarget(CardType type, const Board& board, const Player& actor)
{
    const int count = targetCount(type);
    if (count == 0) {
        return canApplyEffect(type, board, actor, {});
    }
    for (int a = 0; a < Board::kSize * Board::kSize; ++a) {
        const Position first{a / Board::kSize, a % Board::kSize};
        if (count == 1) {
            const std::array<Position, 1> targets{first};
            if (canApplyEffect(type, board, actor, targets)) {
                return true;
            }
            continue;
        }
        for (int b = 0; b < Board::kSize * Board::kSize; ++b) {
            const std::array<Position, 2> targets{first, Position{b / Board::kSize, b % Board::kSize}};
            if (canApplyEffect(type, board, actor, targets)) {
                return true;
            }
        }
    }
    return false;
}

PlayResult applyEffect(CardType type, Board& board, Player& actor, std::span<const Position> targets)
{
    PlayResult result;
    if (!canApplyEffect(type, board, actor, targets)) {
        result.status = PlayStatus::InvalidTarget;
        return result;
    }

    const Color me = actor.color();
    switch (type) {
    case CardType::Place:
        for (const auto& pos : targets) {
            if (const auto strike = rules::placeAndFlip(board, pos, me)) {
                accumulateFlips(result, *strike);
            }
        }
        break;
    case CardType::Move:
        if (board.moveStone(targets[0], targets[1])) {
            accumulateFlips(result, rules::flipFrom(board, targets[1], me));
        }
        break;
    case CardType::Cover:
        board.setCoverAll(me, true);
        break;
    case CardType::Burst:
        accumulateCaptures(result, rules::captureAll(board, me));
        break;
    case CardType::Swap: {
        const Position ownPos = targets[0];
        const Position enemyPos = targets[1];
        if (!board.swapStones(ownPos, enemyPos)) {
            break;
        }
        // 入れ替え後: 敵石は ownPos に、自分の石は enemyPos にある
        const bool enemyHit = rules::isSandwiched(board, ownPos, me);
        const bool ownHit = rules::isSandwiched(board, enemyPos, opponentOf(me));
        if (enemyHit != ownHit) {
            const std::array<Position, 1> victim{enemyHit ? ownPos : enemyPos};
            accumulateCaptures(result, rules::strikeRemove(board, victim));
        }
        break;
    }
    case CardType::Block:
        (void)board.placeWall(targets[0], me);
        break;
    case CardType::Shield: {
        Stone stone = *board.stoneAt(targets[0]);
        stone.shielded = true;
        (void)board.replaceStone(targets[0], stone);
        break;
    }
    case CardType::Reversal: {
        std::vector<Position> enemies;
        for (const auto& n : board.neighbors(targets[0])) {
            if (isEnemyStone(board, n, me)) {
                enemies.push_back(n);
            }
        }
        accumulateFlips(result, rules::strikeFlip(board, enemies));
        break;
    }
    case CardType::Reserve:
        actor.reservePlaces(kReservePlaces);
        break;
    case CardType::Restore:
        actor.gainEp(kRestoreAmount);
        break;
    }
    return result;
}

PlayResult playCard(CardType type, Board& board, Player& actor, std::span<const Position> targets)
{
    PlayResult result;
    if (!hasCardToPlay(actor, type)) {
        result.status = PlayStatus::NotInHand;
        return result;
    }
    const int cost = cardInfo(type).cost;
    if (!actor.canAfford(cost)) {
        result.status = PlayStatus::NotEnoughEp;
        return result;
    }
    if (!canApplyEffect(type, board, actor, targets)) {
        result.status = PlayStatus::InvalidTarget;
        return result;
    }

    // ここから先は失敗しない（上で確認済み）
    (void)actor.spendEp(cost);
    if (!(type == CardType::Place && actor.consumeBonusPlace())) {
        (void)actor.removeCard(type);
    }
    return applyEffect(type, board, actor, targets);
}

void startTurn(Board& board, Player& player)
{
    board.setCoverAll(player.color(), false);
    player.recoverEp();
    player.activateReservedPlaces();
}

} // namespace cardflow
