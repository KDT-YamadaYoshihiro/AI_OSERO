#include <gtest/gtest.h>

#include "cardflow/player.hpp"

using cardflow::CardType;
using cardflow::Color;
using cardflow::Player;

namespace {

TEST(PlayerTest, StartsWithZeroEpAndEmptyHand)
{
    const Player player{Color::White};
    EXPECT_EQ(player.color(), Color::White);
    EXPECT_EQ(player.ep(), 0);
    EXPECT_TRUE(player.hand().empty());
}

TEST(PlayerTest, RecoverEpAddsThreeUpToSix)
{
    Player player{Color::Black};
    player.recoverEp();
    EXPECT_EQ(player.ep(), 3);
    player.recoverEp();
    EXPECT_EQ(player.ep(), 6);
    player.recoverEp();
    EXPECT_EQ(player.ep(), 6);
}

TEST(PlayerTest, GainEpIsCappedAndIgnoresNegative)
{
    Player player{Color::Black};
    player.gainEp(5);
    EXPECT_EQ(player.ep(), 5);
    player.gainEp(2);
    EXPECT_EQ(player.ep(), 6);
    player.gainEp(-3);
    EXPECT_EQ(player.ep(), 6);
}

TEST(PlayerTest, SpendEpExactlyEqualToCost)
{
    Player player{Color::Black};
    player.recoverEp();
    EXPECT_TRUE(player.canAfford(3));
    EXPECT_TRUE(player.spendEp(3));
    EXPECT_EQ(player.ep(), 0);
}

TEST(PlayerTest, SpendEpFailsWhenInsufficient)
{
    Player player{Color::Black};
    player.recoverEp();
    EXPECT_FALSE(player.canAfford(4));
    EXPECT_FALSE(player.spendEp(4));
    EXPECT_EQ(player.ep(), 3);
    EXPECT_FALSE(player.spendEp(-1));
    EXPECT_EQ(player.ep(), 3);
}

TEST(PlayerTest, HandHoldsAtMostThreeCards)
{
    Player player{Color::Black};
    EXPECT_TRUE(player.addCard(CardType::Place));
    EXPECT_TRUE(player.addCard(CardType::Burst));
    EXPECT_TRUE(player.addCard(CardType::Restore));
    EXPECT_FALSE(player.addCard(CardType::Block));
    EXPECT_EQ(player.hand().size(), 3u);
}

TEST(PlayerTest, RemoveCardRemovesOneCopy)
{
    Player player{Color::Black};
    ASSERT_TRUE(player.addCard(CardType::Place));
    ASSERT_TRUE(player.addCard(CardType::Place));
    EXPECT_TRUE(player.removeCard(CardType::Place));
    EXPECT_TRUE(player.hasCard(CardType::Place));
    EXPECT_TRUE(player.removeCard(CardType::Place));
    EXPECT_FALSE(player.hasCard(CardType::Place));
    EXPECT_FALSE(player.removeCard(CardType::Place));
}

} // namespace
