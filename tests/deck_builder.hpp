#pragma once

#include <vector>

#include "cardflow/card.hpp"
#include "cardflow/deck.hpp"

namespace cardflow::test {

// 黒 2 枚 → 白 2 枚 の順に配られ、残り（rest の先頭から順に）が以後の補充に使われる山札。
// 配札は常に黒から 2 枚なので、白に配りたいときは黒の分を 2 枚そろえること
inline Deck scriptedDeck(const std::vector<CardType>& black, const std::vector<CardType>& white,
                         const std::vector<CardType>& rest = {})
{
    std::vector<CardType> pile(rest.rbegin(), rest.rend());
    pile.insert(pile.end(), white.rbegin(), white.rend());
    pile.insert(pile.end(), black.rbegin(), black.rend());
    return Deck(pile);
}

} // namespace cardflow::test
