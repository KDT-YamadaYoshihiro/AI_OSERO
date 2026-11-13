#include "cardflow/deck.hpp"

#include <algorithm>
#include <utility>

namespace cardflow {

Deck Deck::createStandard(std::uint32_t seed)
{
    std::vector<CardType> cards;
    cards.reserve(kCardTypeCount * kCopiesPerCard);
    for (const auto& info : kCardTable) {
        for (int i = 0; i < kCopiesPerCard; ++i) {
            cards.push_back(info.type);
        }
    }
    Deck deck(std::move(cards), seed);
    deck.shuffleDrawPile();
    return deck;
}

Deck::Deck(std::vector<CardType> drawPile, std::uint32_t seed)
    : m_drawPile(std::move(drawPile)), m_rng(seed)
{
}

void Deck::shuffleDrawPile()
{
    std::ranges::shuffle(m_drawPile, m_rng);
}

std::optional<CardType> Deck::draw()
{
    if (m_drawPile.empty()) {
        if (m_discardPile.empty()) {
            return std::nullopt;
        }
        m_drawPile = std::exchange(m_discardPile, {});
        shuffleDrawPile();
    }
    const CardType card = m_drawPile.back();
    m_drawPile.pop_back();
    return card;
}

void Deck::discard(CardType type)
{
    m_discardPile.push_back(type);
}

int Deck::refillHand(Player& player)
{
    std::vector<CardType> setAside;
    int drawn = 0;
    while (player.hand().size() < kHandSize) {
        const auto card = draw();
        if (!card.has_value()) {
            break;
        }
        std::vector<CardType> candidate = player.hand();
        candidate.push_back(*card);
        if (!isBalancedHand(candidate)) {
            setAside.push_back(*card);
            continue;
        }
        if (player.addCard(*card)) {
            ++drawn;
        }
    }
    if (!setAside.empty()) {
        m_drawPile.insert(m_drawPile.end(), setAside.begin(), setAside.end());
        shuffleDrawPile();
    }
    return drawn;
}

} // namespace cardflow
