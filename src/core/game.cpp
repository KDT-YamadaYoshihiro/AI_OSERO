#include "cardflow/game.hpp"

#include <utility>

namespace cardflow {

Game::Game(std::uint32_t seed) : Game(Board::createInitial(), Deck::createStandard(seed)) {}

Game::Game(Board board, Deck deck) : m_board(std::move(board)), m_deck(std::move(deck))
{
    m_deck.refillHand(m_black);
    m_deck.refillHand(m_white);
    startTurn(m_board, m_black);
}

const Player& Game::player(Color color) const noexcept
{
    return color == Color::Black ? m_black : m_white;
}

Player& Game::mutablePlayer(Color color) noexcept
{
    return color == Color::Black ? m_black : m_white;
}

bool Game::usesBonusPlace(CardType type) const noexcept
{
    return type == CardType::Place && currentPlayer().bonusPlaces() > 0;
}

bool Game::canPlay(CardType type) const
{
    if (isOver()) {
        return false;
    }
    const Player& actor = currentPlayer();
    if (!usesBonusPlace(type) && (m_playedNormalCard || !actor.hasCard(type))) {
        return false;
    }
    return actor.canAfford(cardInfo(type).cost) && hasValidTarget(type, m_board, actor);
}

bool Game::canPlayAnything() const
{
    for (const auto& info : kCardTable) {
        if (canPlay(info.type)) {
            return true;
        }
    }
    return false;
}

PlayResult Game::play(CardType type, std::span<const Position> targets)
{
    PlayResult result;
    if (isOver()) {
        result.status = PlayStatus::GameOver;
        return result;
    }
    const bool bonus = usesBonusPlace(type);
    if (!bonus && m_playedNormalCard) {
        result.status = PlayStatus::CardLimitReached;
        return result;
    }

    result = playCard(type, m_board, mutablePlayer(m_current), targets);
    if (!result.ok()) {
        return result;
    }
    m_actedThisTurn = true;
    if (!bonus) {
        m_playedNormalCard = true;
        m_deck.discard(type);
    }
    return result;
}

void Game::endTurn()
{
    if (isOver()) {
        return;
    }
    m_deck.refillHand(mutablePlayer(m_current));

    const bool passed = !m_actedThisTurn;
    if (passed && m_previousTurnPassed) {
        finish();
        return;
    }
    m_previousTurnPassed = passed;

    m_current = opponentOf(m_current);
    if (m_current == Color::Black) {
        ++m_round;
    }
    m_playedNormalCard = false;
    m_actedThisTurn = false;
    startTurn(m_board, mutablePlayer(m_current));
}

void Game::finish()
{
    GameResult result;
    result.blackStones = m_board.countStones(Color::Black);
    result.whiteStones = m_board.countStones(Color::White);
    if (result.blackStones > result.whiteStones) {
        result.winner = Color::Black;
    } else if (result.whiteStones > result.blackStones) {
        result.winner = Color::White;
    }
    m_result = result;
}

} // namespace cardflow
