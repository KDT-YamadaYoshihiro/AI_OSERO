#include "cardflow/player.hpp"

#include <algorithm>

namespace cardflow {

void Player::recoverEp() noexcept
{
    gainEp(kEpPerTurn);
}

void Player::gainEp(int amount) noexcept
{
    if (amount <= 0) {
        return;
    }
    m_ep = std::min(kMaxEp, m_ep + amount);
}

bool Player::canAfford(int cost) const noexcept
{
    return cost >= 0 && cost <= m_ep;
}

bool Player::spendEp(int cost) noexcept
{
    if (!canAfford(cost)) {
        return false;
    }
    m_ep -= cost;
    return true;
}

bool Player::hasCard(CardType type) const noexcept
{
    return std::ranges::find(m_hand, type) != m_hand.end();
}

bool Player::addCard(CardType type)
{
    if (m_hand.size() >= kHandSize) {
        return false;
    }
    m_hand.push_back(type);
    return true;
}

bool Player::removeCard(CardType type)
{
    const auto it = std::ranges::find(m_hand, type);
    if (it == m_hand.end()) {
        return false;
    }
    m_hand.erase(it);
    return true;
}

} // namespace cardflow
