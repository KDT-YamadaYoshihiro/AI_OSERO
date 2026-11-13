#include "cardflow/deck.hpp"

#include <algorithm>
#include <random>

namespace cardflow {

Deck Deck::createStandard(std::uint32_t seed)
{
    std::vector<CardType> pile;
    for (std::size_t i = 0; i < kCardTypeCount; ++i) {
        for (int copy = 0; copy < kCopiesPerCard; ++copy) {
            pile.push_back(static_cast<CardType>(i));
        }
    }
    std::mt19937 rng(seed);
    std::ranges::shuffle(pile, rng);
    return Deck(std::move(pile));
}

std::optional<CardType> Deck::draw()
{
    if (m_pile.empty()) {
        return std::nullopt;
    }
    const CardType card = m_pile.back();
    m_pile.pop_back();
    return card;
}

} // namespace cardflow
