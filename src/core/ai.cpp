#include "cardflow/ai.hpp"

#include <algorithm>
#include <array>
#include <limits>

#include "cardflow/card_effects.hpp"
#include "cardflow/rules.hpp"

namespace cardflow::ai {

namespace {

constexpr int kWinScore = 1'000'000;
constexpr int kStoneWeight = 10;
constexpr int kEpWeight = 1;
constexpr int kReservedPlaceWeight = 3;
constexpr int kCornerWeight = 6;
constexpr int kEdgeWeight = 1;
constexpr int kProtectedWeight = 2;
constexpr int kCapturableThreatWeight = 6;
constexpr int kFlipThreatWeight = 4;
constexpr int kCaptureChanceWeight = 3;
constexpr int kFlipChanceWeight = 2;
constexpr int kMaxMoveDistance = 3;

constexpr int kCells = Board::kSize * Board::kSize;

Position cellAt(int index)
{
    return Position{index / Board::kSize, index % Board::kSize};
}

bool isOwnStone(const Board& board, Position pos, Color color)
{
    const auto stone = board.stoneAt(pos);
    return stone.has_value() && stone->color == color;
}

void addIfValid(std::vector<Action>& out, const Game& game, CardType card, std::vector<Position> targets)
{
    if (canApplyEffect(card, game.board(), game.currentPlayer(), targets)) {
        out.push_back(Action{card, std::move(targets)});
    }
}

void enumerateFor(std::vector<Action>& out, const Game& game, CardType card)
{
    const Board& board = game.board();
    const Color me = game.currentColor();
    switch (card) {
    case CardType::Place:
        for (int a = 0; a < kCells; ++a) {
            if (!board.isEmpty(cellAt(a))) {
                continue;
            }
            for (int b = a + 1; b < kCells; ++b) {
                addIfValid(out, game, card, {cellAt(a), cellAt(b)});
            }
        }
        break;
    case CardType::Move:
        for (int a = 0; a < kCells; ++a) {
            const Position from = cellAt(a);
            if (!isOwnStone(board, from, me)) {
                continue;
            }
            for (const auto& dir : Board::kDirections) {
                for (int dist = 1; dist <= kMaxMoveDistance; ++dist) {
                    addIfValid(out, game, card, {from, Position{from.row + dir.row * dist, from.col + dir.col * dist}});
                }
            }
        }
        break;
    case CardType::Swap:
        for (int a = 0; a < kCells; ++a) {
            const Position own = cellAt(a);
            if (!isOwnStone(board, own, me)) {
                continue;
            }
            for (const auto& n : board.neighbors(own)) {
                addIfValid(out, game, card, {own, n});
            }
        }
        break;
    case CardType::Block:
    case CardType::Shield:
    case CardType::Reversal:
        for (int a = 0; a < kCells; ++a) {
            addIfValid(out, game, card, {cellAt(a)});
        }
        break;
    case CardType::Cover:
    case CardType::Burst:
    case CardType::Reserve:
    case CardType::Restore:
        addIfValid(out, game, card, {});
        break;
    }
}

bool isCorner(Position pos)
{
    const bool edgeRow = pos.row == 0 || pos.row == Board::kSize - 1;
    const bool edgeCol = pos.col == 0 || pos.col == Board::kSize - 1;
    return edgeRow && edgeCol;
}

bool isEdge(Position pos)
{
    return pos.row == 0 || pos.row == Board::kSize - 1 || pos.col == 0 || pos.col == Board::kSize - 1;
}

bool isUnprotectedStone(const Board& board, Position pos)
{
    const auto stone = board.stoneAt(pos);
    return stone.has_value() && !stone->isProtected();
}

// attacker の Burst で取られうる相手の石の数（保護中の石は除く）
int capturableCount(const Board& board, Color attacker)
{
    int capturable = 0;
    for (const auto& pos : rules::findCapturable(board, attacker)) {
        if (isUnprotectedStone(board, pos)) {
            ++capturable;
        }
    }
    return capturable;
}

// attacker が 1 石置いたときに裏返せる相手の石の最大数（保護中の石は除く）
int maxFlipCount(const Board& board, Color attacker)
{
    auto unprotected = [&](Position pos) { return isUnprotectedStone(board, pos); };
    int maxFlips = 0;
    for (int a = 0; a < kCells; ++a) {
        const Position pos = cellAt(a);
        if (!board.isEmpty(pos)) {
            continue;
        }
        const auto flips = rules::findFlips(board, pos, attacker);
        maxFlips = std::max(maxFlips, static_cast<int>(std::ranges::count_if(flips, unprotected)));
    }
    return maxFlips;
}

} // namespace

std::vector<Action> enumerateActions(const Game& game)
{
    std::vector<Action> actions;
    std::array<bool, kCardTypeCount> seen{};
    std::vector<CardType> candidates = game.currentPlayer().hand();
    if (game.currentPlayer().bonusPlaces() > 0) {
        candidates.push_back(CardType::Place);
    }
    for (const auto card : candidates) {
        auto& done = seen[static_cast<std::size_t>(card)];
        if (done || !game.canPlay(card)) {
            continue;
        }
        done = true;
        enumerateFor(actions, game, card);
    }
    return actions;
}

std::string_view difficultyName(Difficulty difficulty) noexcept
{
    switch (difficulty) {
    case Difficulty::Easy: return "弱い";
    case Difficulty::Normal: return "普通";
    case Difficulty::Hard: return "強い";
    }
    return "";
}

int evaluate(const Game& game, Color me, Difficulty difficulty)
{
    const Color opponent = opponentOf(me);
    if (const auto& result = game.result()) {
        if (!result->winner.has_value()) {
            return 0;
        }
        return *result->winner == me ? kWinScore : -kWinScore;
    }

    const Board& board = game.board();
    const Player& self = game.player(me);
    int score = (board.countStones(me) - board.countStones(opponent)) * kStoneWeight;
    score += self.ep() * kEpWeight;
    score += (self.reservedPlaces() + self.bonusPlaces()) * kReservedPlaceWeight;
    // 相手の次の番の脅威（Burst で取られる・Place で裏返される）
    score -= capturableCount(board, opponent) * kCapturableThreatWeight;
    score -= maxFlipCount(board, opponent) * kFlipThreatWeight;

    if (difficulty != Difficulty::Hard) {
        return score;
    }

    // 自分の次の番の好機
    score += capturableCount(board, me) * kCaptureChanceWeight;
    score += maxFlipCount(board, me) * kFlipChanceWeight;

    for (int a = 0; a < kCells; ++a) {
        const Position pos = cellAt(a);
        const auto stone = board.stoneAt(pos);
        if (!stone.has_value()) {
            continue;
        }
        const int sign = stone->color == me ? 1 : -1;
        if (isCorner(pos)) {
            score += sign * kCornerWeight;
        } else if (isEdge(pos)) {
            score += sign * kEdgeWeight;
        }
        if (stone->color == me && stone->isProtected()) {
            score += kProtectedWeight;
        }
    }
    return score;
}

CpuPlayer::CpuPlayer(Difficulty difficulty, std::uint32_t seed) : m_difficulty(difficulty), m_rng(seed) {}

std::optional<Action> CpuPlayer::chooseAction(const Game& game)
{
    if (game.isOver()) {
        return std::nullopt;
    }
    const auto actions = enumerateActions(game);
    if (actions.empty()) {
        return std::nullopt;
    }

    if (m_difficulty == Difficulty::Easy) {
        std::uniform_int_distribution<std::size_t> pick(0, actions.size() - 1);
        return actions[pick(m_rng)];
    }

    const Color me = game.currentColor();
    const int baseline = evaluate(game, me, m_difficulty);
    int bestScore = std::numeric_limits<int>::min();
    std::optional<Action> best;
    int ties = 0;
    for (const auto& action : actions) {
        Game trial = game;
        if (!trial.play(action.card, action.targets).ok()) {
            continue;
        }
        const int score = evaluate(trial, me, m_difficulty);
        if (score > bestScore) {
            bestScore = score;
            best = action;
            ties = 1;
        } else if (score == bestScore) {
            // 同点は等確率で選ぶ（リザーバサンプリング）
            ++ties;
            std::uniform_int_distribution<int> pick(1, ties);
            if (pick(m_rng) == 1) {
                best = action;
            }
        }
    }
    if (!best.has_value() || bestScore <= baseline) {
        return std::nullopt;  // 良くなる手がなければターンを終える
    }
    return best;
}

} // namespace cardflow::ai
