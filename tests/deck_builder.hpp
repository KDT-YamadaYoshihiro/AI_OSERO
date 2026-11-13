#pragma once

#include <vector>

#include "cardflow/card.hpp"
#include "cardflow/deck.hpp"

namespace cardflow::test {

// 末尾から 黒 3 枚 → 白 3 枚 の順に配られ、残り（rest、先頭側）が補充に使われる山札
inline Deck scriptedDeck(const std::vector<CardType>& black, const std::vector<CardType>& white,
                         const std::vector<CardType>& rest = {})
{
    std::vector<CardType> pile = rest;
    pile.insert(pile.end(), white.rbegin(), white.rend());
    pile.insert(pile.end(), black.rbegin(), black.rend());
    return Deck(pile, 0);
}

} // namespace cardflow::test
