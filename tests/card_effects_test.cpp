#include <gtest/gtest.h>

#include <initializer_list>
#include <vector>

#include "board_builder.hpp"
#include "cardflow/card_effects.hpp"
#include "cardflow/rules.hpp"

using cardflow::Board;
using cardflow::CardType;
using cardflow::Color;
using cardflow::Player;
using cardflow::PlayStatus;
using cardflow::Position;
using cardflow::Stone;
using cardflow::test::colorAt;
using cardflow::test::makeBoard;

namespace {

using Targets = std::vector<Position>;

Player makePlayer(Color color, int ep, std::initializer_list<CardType> cards)
{
    Player player{color};
    player.gainEp(ep);
    for (const auto card : cards) {
        EXPECT_TRUE(player.addCard(card));
    }
    return player;
}

// ─── 発動手順（playCard）────────────────────────────────────────────

TEST(PlayCardTest, FailsWhenCardNotInHand)
{
    Board board = Board::createInitial();
    Player black = makePlayer(Color::Black, 6, {CardType::Block});
    const auto result = cardflow::playCard(CardType::Place, board, black, Targets{{0, 0}, {0, 1}});
    EXPECT_EQ(result.status, PlayStatus::NotInHand);
    EXPECT_EQ(black.ep(), 6);
    EXPECT_TRUE(board.isEmpty({0, 0}));
}

TEST(PlayCardTest, FailsWhenEpIsShort)
{
    Board board = makeBoard({"BWB.....", "........", "........", "........",
                             "........", "........", "........", "........"});
    Player black = makePlayer(Color::Black, 3, {CardType::Burst});
    const auto result = cardflow::playCard(CardType::Burst, board, black, Targets{});
    EXPECT_EQ(result.status, PlayStatus::NotEnoughEp);
    EXPECT_TRUE(black.hasCard(CardType::Burst));
    EXPECT_TRUE(board.hasStone({0, 1}));
}

TEST(PlayCardTest, InvalidTargetChangesNothing)
{
    Board board = Board::createInitial();
    Player black = makePlayer(Color::Black, 6, {CardType::Block});
    const auto result = cardflow::playCard(CardType::Block, board, black, Targets{{3, 3}});
    EXPECT_EQ(result.status, PlayStatus::InvalidTarget);
    EXPECT_EQ(black.ep(), 6);
    EXPECT_TRUE(black.hasCard(CardType::Block));
}

TEST(PlayCardTest, SuccessPaysEpAndDiscardsCard)
{
    Board board = Board::createInitial();
    Player black = makePlayer(Color::Black, 6, {CardType::Block, CardType::Place});
    const auto result = cardflow::playCard(CardType::Block, board, black, Targets{{0, 0}});
    EXPECT_TRUE(result.ok());
    EXPECT_EQ(black.ep(), 3);
    EXPECT_FALSE(black.hasCard(CardType::Block));
    EXPECT_TRUE(black.hasCard(CardType::Place));
}

// ─── 各カードの効果 ─────────────────────────────────────────────────

TEST(CardEffectTest, PlacePutsTwoStonesAndFlips)
{
    Board board = Board::createInitial();
    Player black = makePlayer(Color::Black, 6, {CardType::Place});
    const auto result = cardflow::playCard(CardType::Place, board, black, Targets{{2, 4}, {5, 3}});
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.flipped, 2);
    EXPECT_EQ(board.countStones(Color::Black), 8);
    EXPECT_EQ(board.countStones(Color::White), 2);
}

TEST(CardEffectTest, PlaceRejectsBadTargets)
{
    const Board board = Board::createInitial();
    const Player black{Color::Black};
    EXPECT_FALSE(cardflow::canApplyEffect(CardType::Place, board, black, Targets{{0, 0}, {0, 0}}));
    EXPECT_FALSE(cardflow::canApplyEffect(CardType::Place, board, black, Targets{{0, 0}, {3, 3}}));
    EXPECT_FALSE(cardflow::canApplyEffect(CardType::Place, board, black, Targets{{0, 0}}));
}

TEST(CardEffectTest, MoveStraightUpToThreeAndFlipsOnLanding)
{
    Board board = makeBoard({"........", "........", "B...WB..", "........",
                             "........", "........", "........", "........"});
    Player black = makePlayer(Color::Black, 3, {CardType::Move});
    const auto result = cardflow::playCard(CardType::Move, board, black, Targets{{2, 0}, {2, 3}});
    ASSERT_TRUE(result.ok());
    EXPECT_TRUE(board.isEmpty({2, 0}));
    EXPECT_EQ(colorAt(board, {2, 3}), Color::Black);
    EXPECT_EQ(result.flipped, 1);
    EXPECT_EQ(colorAt(board, {2, 4}), Color::Black);
}

TEST(CardEffectTest, MoveRejectsInvalidPaths)
{
    const Board board = makeBoard({"BW......", "........", "........", "#.......",
                                   "........", "........", "........", "........"});
    const Player black{Color::Black};
    auto can = [&](Position from, Position to) {
        return cardflow::canApplyEffect(CardType::Move, board, black, Targets{from, to});
    };
    EXPECT_TRUE(can({0, 0}, {2, 2}));   // 斜め 2
    EXPECT_TRUE(can({0, 0}, {2, 0}));   // 縦 2
    EXPECT_FALSE(can({0, 0}, {0, 2}));  // 途中に石
    EXPECT_FALSE(can({0, 0}, {3, 0}));  // 移動先が壁
    EXPECT_FALSE(can({0, 0}, {4, 4}));  // 4 マス
    EXPECT_FALSE(can({0, 0}, {1, 2}));  // 直線でない
    EXPECT_FALSE(can({0, 1}, {1, 1}));  // 相手の石
}

TEST(CardEffectTest, CoverProtectsUntilOwnersNextTurn)
{
    Board board = makeBoard({".BW.....", "........", "........", "........",
                             "........", "........", "........", "........"});
    Player black = makePlayer(Color::Black, 2, {CardType::Cover});
    Player white{Color::White};
    ASSERT_TRUE(cardflow::playCard(CardType::Cover, board, black, Targets{}).ok());
    EXPECT_TRUE(board.stoneAt({0, 1})->covered);

    // 相手のターン: 開始処理では黒の Cover は外れず、裏返しを防ぐ
    cardflow::startTurn(board, white);
    EXPECT_TRUE(board.stoneAt({0, 1})->covered);
    const auto hit = cardflow::rules::placeAndFlip(board, {0, 0}, Color::White);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->blockedByCover, 1);
    EXPECT_EQ(colorAt(board, {0, 1}), Color::Black);

    // 黒の次のターン開始で解除
    cardflow::startTurn(board, black);
    EXPECT_FALSE(board.stoneAt({0, 1})->covered);
}

TEST(CardEffectTest, BurstCapturesSandwichedStones)
{
    Board board = makeBoard({"BWW#....", "........", "........", "........",
                             "........", "........", "........", "........"});
    Player black = makePlayer(Color::Black, 4, {CardType::Burst});
    const auto result = cardflow::playCard(CardType::Burst, board, black, Targets{});
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.captured, 2);
    EXPECT_EQ(board.countStones(Color::White), 0);
}

TEST(CardEffectTest, BurstNeedsSomethingToCapture)
{
    const Board board = makeBoard({"BW......", "........", "........", "........",
                                   "........", "........", "........", "......WB"});
    EXPECT_FALSE(cardflow::canApplyEffect(CardType::Burst, board, Player{Color::Black}, Targets{}));
}

TEST(CardEffectTest, SwapCapturesEnemyThatBecomesSandwiched)
{
    Board board = makeBoard({"BBW.....", "........", "........", "........",
                             "........", "........", "........", "........"});
    Player black = makePlayer(Color::Black, 3, {CardType::Swap});
    const auto result = cardflow::playCard(CardType::Swap, board, black, Targets{{0, 1}, {0, 2}});
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.captured, 1);
    EXPECT_TRUE(board.isEmpty({0, 1}));
    EXPECT_EQ(colorAt(board, {0, 2}), Color::Black);
}

TEST(CardEffectTest, SwapCapturesNothingWhenBothSandwiched)
{
    Board board = makeBoard({"BBWW....", "........", "........", "........",
                             "........", "........", "........", "........"});
    Player black = makePlayer(Color::Black, 3, {CardType::Swap});
    const auto result = cardflow::playCard(CardType::Swap, board, black, Targets{{0, 1}, {0, 2}});
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.captured, 0);
    EXPECT_EQ(colorAt(board, {0, 1}), Color::White);
    EXPECT_EQ(colorAt(board, {0, 2}), Color::Black);
}

TEST(CardEffectTest, SwapCanLoseOwnStone)
{
    Board board = makeBoard({".BWW....", "........", "........", "........",
                             "........", "........", "........", "........"});
    Player black = makePlayer(Color::Black, 3, {CardType::Swap});
    const auto result = cardflow::playCard(CardType::Swap, board, black, Targets{{0, 1}, {0, 2}});
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.captured, 1);
    EXPECT_TRUE(board.isEmpty({0, 2}));  // 自分の石が挟まれて捕獲された
    EXPECT_EQ(board.countStones(Color::Black), 0);
}

TEST(CardEffectTest, SwapRequiresOwnAndAdjacentEnemy)
{
    const Board board = makeBoard({"BW.W....", "B.......", "........", "........",
                                   "........", "........", "........", "........"});
    const Player black{Color::Black};
    EXPECT_TRUE(cardflow::canApplyEffect(CardType::Swap, board, black, Targets{{0, 0}, {0, 1}}));
    EXPECT_TRUE(cardflow::canApplyEffect(CardType::Swap, board, black, Targets{{1, 0}, {0, 1}}));   // 斜め
    EXPECT_FALSE(cardflow::canApplyEffect(CardType::Swap, board, black, Targets{{0, 0}, {0, 3}}));  // 離れている
    EXPECT_FALSE(cardflow::canApplyEffect(CardType::Swap, board, black, Targets{{0, 0}, {1, 0}}));  // 味方同士
    EXPECT_FALSE(cardflow::canApplyEffect(CardType::Swap, board, black, Targets{{0, 1}, {0, 0}}));  // 順序が逆
}

TEST(CardEffectTest, BlockPlacesOwnedWall)
{
    Board board = Board::createInitial();
    Player white = makePlayer(Color::White, 3, {CardType::Block});
    ASSERT_TRUE(cardflow::playCard(CardType::Block, board, white, Targets{{0, 0}}).ok());
    EXPECT_EQ(board.wallAt({0, 0})->owner, Color::White);
}

TEST(CardEffectTest, ShieldTargetsOwnUnshieldedStone)
{
    Board board = Board::createInitial();
    Player black = makePlayer(Color::Black, 4, {CardType::Shield, CardType::Shield});
    EXPECT_EQ(cardflow::playCard(CardType::Shield, board, black, Targets{{3, 4}}).status,
              PlayStatus::InvalidTarget);  // 相手の石
    ASSERT_TRUE(cardflow::playCard(CardType::Shield, board, black, Targets{{3, 3}}).ok());
    EXPECT_TRUE(board.stoneAt({3, 3})->shielded);
    EXPECT_EQ(cardflow::playCard(CardType::Shield, board, black, Targets{{3, 3}}).status,
              PlayStatus::InvalidTarget);  // すでに Shield
}

TEST(CardEffectTest, ReversalFlipsAdjacentEnemies)
{
    Board board = makeBoard({"........", "........", "..WW....", "...Bw...",
                             "....W...", "........", "........", "......W."});
    Player black = makePlayer(Color::Black, 4, {CardType::Reversal});
    const auto result = cardflow::playCard(CardType::Reversal, board, black, Targets{{3, 3}});
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.flipped, 3);
    EXPECT_EQ(result.shieldsBroken, 1);
    EXPECT_EQ(colorAt(board, {3, 4}), Color::White);
    EXPECT_EQ(colorAt(board, {7, 6}), Color::White);  // 隣接していない
    EXPECT_EQ(board.countStones(Color::Black), 4);
}

TEST(CardEffectTest, ReversalNeedsAdjacentEnemy)
{
    const Board board = makeBoard({"B.......", "..W.....", "........", "........",
                                   "........", "........", "........", "........"});
    EXPECT_FALSE(cardflow::canApplyEffect(CardType::Reversal, board, Player{Color::Black}, Targets{{0, 0}}));
}

TEST(CardEffectTest, ReserveGrantsTwoBonusPlacesNextTurnOnly)
{
    Board board = Board::createInitial();
    Player black = makePlayer(Color::Black, 1, {CardType::Reserve, CardType::Block, CardType::Shield});
    ASSERT_TRUE(cardflow::playCard(CardType::Reserve, board, black, Targets{}).ok());
    EXPECT_EQ(black.reservedPlaces(), 2);
    EXPECT_EQ(black.bonusPlaces(), 0);

    cardflow::startTurn(board, black);  // 次のターン
    EXPECT_EQ(black.reservedPlaces(), 0);
    EXPECT_EQ(black.bonusPlaces(), 2);
    EXPECT_FALSE(black.hasCard(CardType::Place));

    // 手札に Place がなくても追加分で発動でき、手札は減らない
    ASSERT_TRUE(cardflow::playCard(CardType::Place, board, black, Targets{{0, 0}, {0, 7}}).ok());
    EXPECT_EQ(black.bonusPlaces(), 1);
    EXPECT_EQ(black.hand().size(), 2u);

    cardflow::startTurn(board, black);  // 使わなかった分は消える
    EXPECT_EQ(black.bonusPlaces(), 0);
}

TEST(CardEffectTest, ReserveCannotExceedLimit)
{
    const Board board;
    Player black{Color::Black};
    black.reservePlaces(2);
    EXPECT_FALSE(cardflow::canApplyEffect(CardType::Reserve, board, black, Targets{}));
}

TEST(CardEffectTest, RestorePaysThenGainsTwo)
{
    Board board;
    Player full = makePlayer(Color::Black, 6, {CardType::Restore});
    ASSERT_TRUE(cardflow::playCard(CardType::Restore, board, full, Targets{}).ok());
    EXPECT_EQ(full.ep(), 6);  // 6 - 1 + 2 → 上限 6

    Player low = makePlayer(Color::White, 2, {CardType::Restore});
    ASSERT_TRUE(cardflow::playCard(CardType::Restore, board, low, Targets{}).ok());
    EXPECT_EQ(low.ep(), 3);
}

TEST(CardEffectTest, TargetlessCardsRejectTargets)
{
    const Board board = Board::createInitial();
    const Player black{Color::Black};
    EXPECT_FALSE(cardflow::canApplyEffect(CardType::Restore, board, black, Targets{{0, 0}}));
    EXPECT_FALSE(cardflow::canApplyEffect(CardType::Cover, board, black, Targets{{3, 3}}));
}

TEST(StartTurnTest, RecoversEp)
{
    Board board;
    Player black{Color::Black};
    cardflow::startTurn(board, black);
    EXPECT_EQ(black.ep(), 3);
}

} // namespace
