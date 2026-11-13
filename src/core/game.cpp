#include "cardflow/game.hpp"

#include <algorithm>
#include <utility>

namespace cardflow {

namespace {

constexpr int kCells = Board::kSize * Board::kSize;

std::size_t indexOf(Color color) noexcept
{
    return color == Color::Black ? 0 : 1;
}

Position cellAt(int index)
{
    return Position{index / Board::kSize, index % Board::kSize};
}

} // namespace

Game::Game(std::uint32_t seed) : Game(Board::createInitial(), Deck::createStandard(seed)) {}

Game::Game(Board board, Deck deck) : m_board(std::move(board)), m_deck(std::move(deck))
{
    for (const Color color : {Color::Black, Color::White}) {
        for (int i = 0; i < kStartingHand; ++i) {
            if (const auto card = m_deck.draw()) {
                mutableHand(color).push_back(*card);
            }
        }
    }
    startTurn();
}

const std::vector<CardType>& Game::hand(Color color) const noexcept
{
    return m_hands[indexOf(color)];
}

std::vector<CardType>& Game::mutableHand(Color color) noexcept
{
    return m_hands[indexOf(color)];
}

// ─── カード ──────────────────────────────────────────────────────────

bool Game::canUseCard(CardType type) const
{
    // カードは石を置く前だけ。置いた後に同じターンが続くのは連打（カード使用済み）のときだけ
    if (isOver() || m_cardUsed || std::ranges::find(hand(m_current), type) == hand(m_current).end()) {
        return false;
    }
    return !cardInfo(type).needsTarget || !cardTargets(type).empty();
}

std::vector<Position> Game::cardTargets(CardType type) const
{
    std::vector<Position> targets;
    if (!cardInfo(type).needsTarget) {
        return targets;
    }
    for (int i = 0; i < kCells; ++i) {
        if (isValidCardTarget(type, m_board, m_current, cellAt(i))) {
            targets.push_back(cellAt(i));
        }
    }
    return targets;
}

CardStatus Game::useCard(CardType type, std::optional<Position> target)
{
    if (isOver()) {
        return CardStatus::GameOver;
    }
    auto& cards = mutableHand(m_current);
    const auto it = std::ranges::find(cards, type);
    if (it == cards.end()) {
        return CardStatus::NotInHand;
    }
    if (m_cardUsed) {
        return CardStatus::AlreadyUsed;
    }
    if (cardInfo(type).needsTarget != target.has_value()) {
        return CardStatus::InvalidTarget;
    }

    switch (type) {
    case CardType::Double:
        m_placementsLeft = 2;
        break;
    case CardType::Free:
        m_freeMove = true;
        break;
    case CardType::Guard:
    case CardType::Wall:
    case CardType::Flip:
        if (!applyCardTarget(type, m_board, m_current, *target)) {
            return CardStatus::InvalidTarget;
        }
        break;
    }
    cards.erase(it);
    m_cardUsed = true;
    // 反転で相手の石が 0 個、壁で空マスが 0 になれば終局
    (void)checkFinished();
    return CardStatus::Ok;
}

// ─── 石を置く ────────────────────────────────────────────────────────

bool Game::canPlace(Position pos) const
{
    if (isOver() || m_placementsLeft <= 0) {
        return false;
    }
    return rules::isLegalMove(m_board, pos, m_current) || (m_freeMove && rules::isFreeCell(m_board, pos));
}

std::vector<Position> Game::placeableCells() const
{
    std::vector<Position> cells;
    for (int i = 0; i < kCells; ++i) {
        if (canPlace(cellAt(i))) {
            cells.push_back(cellAt(i));
        }
    }
    return cells;
}

std::optional<rules::PlaceResult> Game::place(Position pos)
{
    if (!canPlace(pos)) {
        return std::nullopt;
    }
    auto result = rules::placeAndFlip(m_board, pos, m_current);
    m_lastPlaced = pos;
    ++m_placed;
    --m_placementsLeft;
    m_freeMove = false;
    if (checkFinished()) {
        return result;
    }
    if (m_placementsLeft > 0 && !placeableCells().empty()) {
        return result;  // 連打: もう 1 つ置く
    }
    nextTurn(false);
    return result;
}

bool Game::mustPass() const
{
    return !isOver() && m_placed == 0 && placeableCells().empty();
}

bool Game::pass()
{
    if (!mustPass()) {
        return false;
    }
    if (m_previousPassed) {
        finish();  // 両者が続けてパス
        return true;
    }
    nextTurn(true);
    return true;
}

// ─── 進行 ────────────────────────────────────────────────────────────

void Game::startTurn()
{
    m_cardUsed = false;
    m_placed = 0;
    m_placementsLeft = 1;
    m_freeMove = false;
    int& turns = m_ownTurns[indexOf(m_current)];
    ++turns;
    auto& cards = mutableHand(m_current);
    if (turns % kDrawInterval == 0 && static_cast<int>(cards.size()) < kMaxHand) {
        if (const auto card = m_deck.draw()) {
            cards.push_back(*card);
        }
    }
}

void Game::nextTurn(bool passed)
{
    m_previousPassed = passed;
    m_current = opponentOf(m_current);
    ++m_turnNumber;
    startTurn();
}

// 空マスがない、またはどちらかの石が 0 個なら終局
bool Game::checkFinished()
{
    if (m_board.countEmpty() == 0 || m_board.countStones(Color::Black) == 0 ||
        m_board.countStones(Color::White) == 0) {
        finish();
        return true;
    }
    return false;
}

void Game::finish()
{
    GameResult result;
    result.blackStones = m_board.countStones(Color::Black);
    result.whiteStones = m_board.countStones(Color::White);
    if (result.blackStones != result.whiteStones) {
        result.winner = result.blackStones > result.whiteStones ? Color::Black : Color::White;
    }
    m_result = result;
}

} // namespace cardflow
