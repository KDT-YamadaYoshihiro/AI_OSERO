#include "cardflow/ai.hpp"

#include <algorithm>
#include <cstdlib>
#include <array>
#include <limits>
#include <vector>

#include "cardflow/rules.hpp"

namespace cardflow::ai {

namespace {

constexpr int kWinScore = 1'000'000;
constexpr int kMobilityWeight = 5;
constexpr int kGuardWeight = 10;
constexpr int kCardWeight = 15;
constexpr int kEndgameEmpties = 12;
constexpr int kEndgameStoneWeight = 10;
constexpr int kSafeNearCornerWeight = 5;
constexpr int kCells = Board::kSize * Board::kSize;

// マスの重み（角が最も高く、角の隣は危険）
constexpr std::array<std::array<int, Board::kSize>, Board::kSize> kSquareWeights{{
    {100, -20, 10, 5, 5, 10, -20, 100},
    {-20, -50, -2, -2, -2, -2, -50, -20},
    {10, -2, 1, 1, 1, 1, -2, 10},
    {5, -2, 1, 0, 0, 1, -2, 5},
    {5, -2, 1, 0, 0, 1, -2, 5},
    {10, -2, 1, 1, 1, 1, -2, 10},
    {-20, -50, -2, -2, -2, -2, -50, -20},
    {100, -20, 10, 5, 5, 10, -20, 100},
}};

Position cellAt(int index)
{
    return Position{index / Board::kSize, index % Board::kSize};
}

// 角の隣のマスは、その角が埋まっていれば危険ではない
int squareWeight(const Board& board, Position pos)
{
    const int weight = kSquareWeights[static_cast<std::size_t>(pos.row)][static_cast<std::size_t>(pos.col)];
    if (weight >= 0) {
        return weight;
    }
    const Position corner{pos.row < Board::kSize / 2 ? 0 : Board::kSize - 1,
                          pos.col < Board::kSize / 2 ? 0 : Board::kSize - 1};
    const bool nearCorner = std::abs(pos.row - corner.row) <= 1 && std::abs(pos.col - corner.col) <= 1;
    if (nearCorner && !board.isEmpty(corner)) {
        return kSafeNearCornerWeight;
    }
    return weight;
}

// カードの使い方の候補（card が空なら使わない）
struct CardOption {
    std::optional<CardType> card;
    std::optional<Position> target;
};

std::vector<CardOption> cardOptions(const Game& game)
{
    std::vector<CardOption> options{CardOption{}};
    std::array<bool, kCardTypeCount> seen{};
    for (const auto card : game.hand(game.currentColor())) {
        auto& done = seen[static_cast<std::size_t>(card)];
        if (done || !game.canUseCard(card)) {
            continue;
        }
        done = true;
        if (cardInfo(card).needsTarget) {
            for (const auto& target : game.cardTargets(card)) {
                options.push_back(CardOption{card, target});
            }
        } else {
            options.push_back(CardOption{card, std::nullopt});
        }
    }
    return options;
}

// 自分の番を終えた局面の点数（強いは相手の最善の応手まで読む）
int scoreAfterTurn(const Game& game, Color me, Difficulty difficulty)
{
    if (difficulty != Difficulty::Hard || game.isOver() || game.currentColor() == me) {
        return evaluate(game, me);
    }
    const auto replies = game.placeableCells();
    if (replies.empty()) {
        Game passed = game;
        (void)passed.pass();
        return evaluate(passed, me);
    }
    int worst = std::numeric_limits<int>::max();
    for (const auto& pos : replies) {
        Game reply = game;
        (void)reply.place(pos);
        worst = std::min(worst, evaluate(reply, me));
    }
    return worst;
}

// pos に置いたときの点数（連打で続けて置けるなら、続きの最善手まで含める）
int scorePlacement(const Game& game, Position pos, Color me, Difficulty difficulty)
{
    Game next = game;
    (void)next.place(pos);
    if (!next.isOver() && next.currentColor() == me && next.placedThisTurn() > 0) {
        int best = std::numeric_limits<int>::min();
        for (const auto& second : next.placeableCells()) {
            best = std::max(best, scorePlacement(next, second, me, difficulty));
        }
        return best;
    }
    return scoreAfterTurn(next, me, difficulty);
}

} // namespace

std::string_view difficultyName(Difficulty difficulty) noexcept
{
    switch (difficulty) {
    case Difficulty::Easy: return "弱い";
    case Difficulty::Normal: return "普通";
    case Difficulty::Hard: return "強い";
    }
    return "";
}

int evaluate(const Game& game, Color me)
{
    const Board& board = game.board();
    const Color opponent = opponentOf(me);
    const int stoneDiff = board.countStones(me) - board.countStones(opponent);
    if (const auto& result = game.result()) {
        if (!result->winner.has_value()) {
            return 0;
        }
        return *result->winner == me ? kWinScore + stoneDiff : -kWinScore + stoneDiff;
    }

    int score = 0;
    for (int i = 0; i < kCells; ++i) {
        const Position pos = cellAt(i);
        const auto stone = board.stoneAt(pos);
        if (!stone.has_value()) {
            continue;
        }
        const int sign = stone->color == me ? 1 : -1;
        score += sign * squareWeight(board, pos);
        if (stone->guarded) {
            score += sign * kGuardWeight;
        }
    }
    score += (rules::countLegalMoves(board, me) - rules::countLegalMoves(board, opponent)) * kMobilityWeight;
    score += (static_cast<int>(game.hand(me).size()) - static_cast<int>(game.hand(opponent).size())) * kCardWeight;
    if (board.countEmpty() <= kEndgameEmpties) {
        score += stoneDiff * kEndgameStoneWeight;
    }
    return score;
}

CpuPlayer::CpuPlayer(Difficulty difficulty, std::uint32_t seed) : m_difficulty(difficulty), m_rng(seed) {}

Action CpuPlayer::chooseAction(const Game& game)
{
    if (game.isOver()) {
        return Action{};
    }
    return m_difficulty == Difficulty::Easy ? chooseRandom(game) : chooseBest(game);
}

Action CpuPlayer::chooseRandom(const Game& game)
{
    const auto options = cardOptions(game);
    const auto cells = game.placeableCells();
    // 置けないときは半々、置けるときは 4 回に 1 回カードを使う
    std::uniform_int_distribution<int> percent(0, 99);
    const int chance = cells.empty() ? 50 : 25;
    if (options.size() > 1 && percent(m_rng) < chance) {
        std::uniform_int_distribution<std::size_t> pick(1, options.size() - 1);
        const auto& option = options[pick(m_rng)];
        return Action{Action::Kind::UseCard, *option.card, option.target, {}};
    }
    if (cells.empty()) {
        return Action{};
    }
    std::uniform_int_distribution<std::size_t> pick(0, cells.size() - 1);
    return Action{Action::Kind::Place, CardType::Double, std::nullopt, cells[pick(m_rng)]};
}

Action CpuPlayer::chooseBest(const Game& game)
{
    const Color me = game.currentColor();
    int bestScore = std::numeric_limits<int>::min();
    Action best{};
    int ties = 0;
    auto consider = [&](int score, const Action& action) {
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
    };

    for (const auto& option : cardOptions(game)) {
        Game trial = game;
        if (option.card.has_value() && trial.useCard(*option.card, option.target) != CardStatus::Ok) {
            continue;
        }
        // 最初の一歩: カードを使うならカード、使わないなら置く・パス
        const Action cardStep{Action::Kind::UseCard, option.card.value_or(CardType::Double), option.target, {}};
        if (trial.isOver()) {
            consider(evaluate(trial, me), cardStep);
            continue;
        }
        const auto cells = trial.placeableCells();
        if (cells.empty()) {
            Game passed = trial;
            (void)passed.pass();
            consider(scoreAfterTurn(passed, me, m_difficulty), option.card.has_value() ? cardStep : Action{});
            continue;
        }
        for (const auto& pos : cells) {
            const int score = scorePlacement(trial, pos, me, m_difficulty);
            consider(score, option.card.has_value() ? cardStep
                                                    : Action{Action::Kind::Place, CardType::Double, std::nullopt, pos});
        }
    }
    return best;
}

bool applyAction(Game& game, const Action& action)
{
    switch (action.kind) {
    case Action::Kind::UseCard:
        return game.useCard(action.card, action.target) == CardStatus::Ok;
    case Action::Kind::Place:
        return game.place(action.pos).has_value();
    case Action::Kind::Pass:
        return game.pass();
    }
    return false;
}

} // namespace cardflow::ai
