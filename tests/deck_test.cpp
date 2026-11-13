#include <gtest/gtest.h>

#include <map>

#include "cardflow/deck.hpp"

using cardflow::CardType;
using cardflow::Deck;

namespace {

TEST(DeckTest, StandardDeckHasThreeOfEach)
{
    Deck deck = Deck::createStandard(1);
    EXPECT_EQ(deck.size(), 15u);
    std::map<CardType, int> counts;
    while (const auto card = deck.draw()) {
        ++counts[*card];
    }
    EXPECT_EQ(counts.size(), 5u);
    for (const auto& [card, count] : counts) {
        EXPECT_EQ(count, 3);
    }
    EXPECT_TRUE(deck.empty());
    EXPECT_FALSE(deck.draw().has_value());
}

TEST(DeckTest, SameSeedSameOrder)
{
    Deck a = Deck::createStandard(42);
    Deck b = Deck::createStandard(42);
    for (int i = 0; i < 15; ++i) {
        EXPECT_EQ(a.draw(), b.draw());
    }
}

TEST(DeckTest, DrawsFromBack)
{
    Deck deck({CardType::Wall, CardType::Flip});
    EXPECT_EQ(deck.draw(), CardType::Flip);
    EXPECT_EQ(deck.draw(), CardType::Wall);
}

} // namespace
