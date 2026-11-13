#include <gtest/gtest.h>

#include <vector>

#include "cardflow/card.hpp"

using cardflow::CardCategory;
using cardflow::cardInfo;
using cardflow::CardType;
using cardflow::isBalancedHand;

namespace {

TEST(CardTest, CostsMatchSpecification)
{
    EXPECT_EQ(cardInfo(CardType::Place).cost, 2);
    EXPECT_EQ(cardInfo(CardType::Move).cost, 3);
    EXPECT_EQ(cardInfo(CardType::Cover).cost, 2);
    EXPECT_EQ(cardInfo(CardType::Burst).cost, 4);
    EXPECT_EQ(cardInfo(CardType::Swap).cost, 3);
    EXPECT_EQ(cardInfo(CardType::Block).cost, 3);
    EXPECT_EQ(cardInfo(CardType::Shield).cost, 2);
    EXPECT_EQ(cardInfo(CardType::Reversal).cost, 4);
    EXPECT_EQ(cardInfo(CardType::Reserve).cost, 1);
    EXPECT_EQ(cardInfo(CardType::Restore).cost, 1);
}

TEST(CardTest, CategoriesMatchConstitution)
{
    EXPECT_EQ(cardInfo(CardType::Place).category, CardCategory::Green);
    EXPECT_EQ(cardInfo(CardType::Move).category, CardCategory::Green);
    EXPECT_EQ(cardInfo(CardType::Cover).category, CardCategory::Green);
    EXPECT_EQ(cardInfo(CardType::Burst).category, CardCategory::Red);
    EXPECT_EQ(cardInfo(CardType::Swap).category, CardCategory::Red);
    EXPECT_EQ(cardInfo(CardType::Block).category, CardCategory::Blue);
    EXPECT_EQ(cardInfo(CardType::Shield).category, CardCategory::Blue);
    EXPECT_EQ(cardInfo(CardType::Reversal).category, CardCategory::Violet);
    EXPECT_EQ(cardInfo(CardType::Reserve).category, CardCategory::Orange);
    EXPECT_EQ(cardInfo(CardType::Restore).category, CardCategory::Yellow);
}

TEST(CardTest, NameLookup)
{
    EXPECT_EQ(cardInfo(CardType::Reversal).name, "Reversal");
    EXPECT_EQ(cardInfo(CardType::Reversal).type, CardType::Reversal);
}

TEST(CardTest, BalancedHandAcceptsMixedCategories)
{
    const std::vector hand{CardType::Place, CardType::Move, CardType::Burst};
    EXPECT_TRUE(isBalancedHand(hand));
    EXPECT_TRUE(isBalancedHand(std::vector<CardType>{}));
}

TEST(CardTest, BalancedHandRejectsThreeGreen)
{
    const std::vector hand{CardType::Place, CardType::Move, CardType::Cover};
    EXPECT_FALSE(isBalancedHand(hand));
}

TEST(CardTest, BalancedHandRejectsTwoOfSingleLimitCategory)
{
    const std::vector hand{CardType::Reversal, CardType::Reversal};
    EXPECT_FALSE(isBalancedHand(hand));
}

TEST(CardTest, BalancedHandRejectsTooManyCards)
{
    const std::vector hand{CardType::Place, CardType::Burst, CardType::Block, CardType::Restore};
    EXPECT_FALSE(isBalancedHand(hand));
}

} // namespace
