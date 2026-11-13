#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <vector>

#include "cardflow/deck.hpp"

using cardflow::CardType;
using cardflow::Color;
using cardflow::Deck;
using cardflow::Player;

namespace {

std::vector<CardType> drawAll(Deck& deck)
{
    std::vector<CardType> cards;
    while (deck.drawPileSize() > 0) {
        cards.push_back(*deck.draw());
    }
    return cards;
}

TEST(DeckTest, StandardDeckHasThreeOfEachCard)
{
    Deck deck = Deck::createStandard(1);
    EXPECT_EQ(deck.drawPileSize(), 30u);
    std::array<int, cardflow::kCardTypeCount> counts{};
    for (const auto card : drawAll(deck)) {
        ++counts[static_cast<std::size_t>(card)];
    }
    for (const int count : counts) {
        EXPECT_EQ(count, Deck::kCopiesPerCard);
    }
}

TEST(DeckTest, SameSeedGivesSameOrder)
{
    Deck a = Deck::createStandard(42);
    Deck b = Deck::createStandard(42);
    Deck c = Deck::createStandard(43);
    const auto orderA = drawAll(a);
    EXPECT_EQ(orderA, drawAll(b));
    EXPECT_NE(orderA, drawAll(c));
}

TEST(DeckTest, DrawsFromBackOfGivenPile)
{
    Deck deck({CardType::Place, CardType::Burst}, 0);
    EXPECT_EQ(deck.draw(), CardType::Burst);
    EXPECT_EQ(deck.draw(), CardType::Place);
}

TEST(DeckTest, ReshufflesDiscardWhenEmpty)
{
    Deck deck({CardType::Place}, 0);
    EXPECT_EQ(deck.draw(), CardType::Place);
    EXPECT_FALSE(deck.draw().has_value());  // 山札・捨て札とも空

    deck.discard(CardType::Swap);
    EXPECT_EQ(deck.discardPileSize(), 1u);
    EXPECT_EQ(deck.draw(), CardType::Swap);
    EXPECT_EQ(deck.discardPileSize(), 0u);
}

TEST(DeckTest, RefillHandUpToThree)
{
    Deck deck = Deck::createStandard(7);
    Player player{Color::Black};
    EXPECT_EQ(deck.refillHand(player), 3);
    EXPECT_EQ(player.hand().size(), 3u);
    EXPECT_TRUE(cardflow::isBalancedHand(player.hand()));
    EXPECT_EQ(deck.drawPileSize(), 27u);
    EXPECT_EQ(deck.refillHand(player), 0);  // すでに 3 枚
}

TEST(DeckTest, RefillSkipsCardsBreakingCategoryLimit)
{
    // 末尾から Place, Move（Green 2 枚）→ Cover（Green 3 枚目は不可）→ Burst
    Deck deck({CardType::Burst, CardType::Cover, CardType::Move, CardType::Place}, 0);
    Player player{Color::Black};
    EXPECT_EQ(deck.refillHand(player), 3);
    const std::vector expected{CardType::Place, CardType::Move, CardType::Burst};
    EXPECT_EQ(player.hand(), expected);
    EXPECT_EQ(deck.drawPileSize(), 1u);  // Cover は山札に戻る
    EXPECT_EQ(deck.draw(), CardType::Cover);
}

TEST(DeckTest, RefillStopsWhenCardsRunOut)
{
    Deck deck({CardType::Place}, 0);
    Player player{Color::White};
    EXPECT_EQ(deck.refillHand(player), 1);
    EXPECT_EQ(player.hand().size(), 1u);
}

} // namespace
