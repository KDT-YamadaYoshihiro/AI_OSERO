#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "board_builder.hpp"
#include "deck_builder.hpp"
#include "cardflow/game.hpp"

using cardflow::Board;
using cardflow::CardType;
using cardflow::Color;
using cardflow::Deck;
using cardflow::Game;
using cardflow::PlayStatus;
using cardflow::Position;
using cardflow::test::makeBoard;
using cardflow::test::scriptedDeck;

namespace {

using Targets = std::vector<Position>;

std::size_t totalCards(const Game& game)
{
    return game.player(Color::Black).hand().size() + game.player(Color::White).hand().size() +
           game.deck().drawPileSize() + game.deck().discardPileSize();
}

TEST(GameTest, StartsWithDealtHandsAndBlackToMove)
{
    const Game game(123);
    EXPECT_EQ(game.currentColor(), Color::Black);
    EXPECT_EQ(game.round(), 1);
    EXPECT_FALSE(game.isOver());
    EXPECT_EQ(game.player(Color::Black).hand().size(), 3u);
    EXPECT_EQ(game.player(Color::White).hand().size(), 3u);
    EXPECT_EQ(game.player(Color::Black).ep(), 3);
    EXPECT_EQ(game.player(Color::White).ep(), 0);
    EXPECT_EQ(game.deck().drawPileSize(), 24u);
    EXPECT_EQ(game.board().countStones(Color::Black), 4);
}

TEST(GameTest, WipeoutEndsGameImmediately)
{
    const Board board = makeBoard({"BW......", "........", "........", "........",
                                   "........", "........", "........", "........"});
    Game game(board, scriptedDeck({CardType::Place, CardType::Block, CardType::Restore},
                                  {CardType::Block, CardType::Shield, CardType::Restore}, {}));
    ASSERT_TRUE(game.play(CardType::Place, Targets{{0, 2}, {7, 7}}).ok());
    ASSERT_TRUE(game.isOver());
    EXPECT_EQ(game.result()->winner, Color::Black);
    EXPECT_EQ(game.result()->whiteStones, 0);
}

TEST(GameTest, OneNormalCardPerTurnAndRefillAtTurnEnd)
{
    Game game(Board::createInitial(),
              scriptedDeck({CardType::Block, CardType::Shield, CardType::Restore},
                           {CardType::Block, CardType::Shield, CardType::Restore}, {CardType::Burst}));
    ASSERT_TRUE(game.play(CardType::Block, Targets{{0, 0}}).ok());
    EXPECT_TRUE(game.hasPlayedCardThisTurn());
    EXPECT_EQ(game.play(CardType::Restore, Targets{}).status, PlayStatus::CardLimitReached);
    EXPECT_EQ(game.player(Color::Black).hand().size(), 2u);
    EXPECT_EQ(game.deck().discardPileSize(), 1u);

    game.endTurn();
    EXPECT_EQ(game.player(Color::Black).hand().size(), 3u);
    EXPECT_TRUE(game.player(Color::Black).hasCard(CardType::Burst));
    EXPECT_EQ(game.currentColor(), Color::White);
    EXPECT_EQ(game.player(Color::White).ep(), 3);
    EXPECT_EQ(totalCards(game), 7u);
}

TEST(GameTest, CanPlayAnythingReflectsEpAndLimit)
{
    Game game(Board::createInitial(),
              scriptedDeck({CardType::Block, CardType::Burst, CardType::Reversal},
                           {CardType::Restore, CardType::Place, CardType::Shield}, {}));
    EXPECT_TRUE(game.canPlay(CardType::Block));
    EXPECT_FALSE(game.canPlay(CardType::Burst));     // EP 不足（4）
    EXPECT_FALSE(game.canPlay(CardType::Restore));   // 手札にない
    EXPECT_TRUE(game.canPlayAnything());
    ASSERT_TRUE(game.play(CardType::Block, Targets{{0, 0}}).ok());
    EXPECT_FALSE(game.canPlayAnything());            // 通常カード使用済み
}

TEST(GameTest, BonusPlaceIsOutsideOneCardLimit)
{
    Game game(Board::createInitial(),
              scriptedDeck({CardType::Reserve, CardType::Block, CardType::Shield},
                           {CardType::Block, CardType::Shield, CardType::Restore},
                           {CardType::Burst, CardType::Burst}));
    ASSERT_TRUE(game.play(CardType::Reserve, Targets{}).ok());  // EP 3 → 2
    game.endTurn();
    game.endTurn();  // 白はパス

    // 黒の 2 ターン目: EP 2 + 3 = 5、追加 Place 2 枚
    EXPECT_EQ(game.player(Color::Black).bonusPlaces(), 2);
    ASSERT_TRUE(game.play(CardType::Block, Targets{{7, 7}}).ok());  // 通常カード（EP 5 → 2）
    const auto bonus = game.play(CardType::Place, Targets{{0, 0}, {0, 1}});
    EXPECT_TRUE(bonus.ok());  // 制限の外
    EXPECT_EQ(game.player(Color::Black).ep(), 0);
    EXPECT_EQ(game.player(Color::Black).bonusPlaces(), 1);
    EXPECT_EQ(game.deck().discardPileSize(), 2u);  // Reserve と Block のみ（追加 Place は山札外）
}

TEST(GameTest, TwoConsecutivePassesEndTheGame)
{
    Game game(77);
    game.endTurn();  // 黒パス
    EXPECT_FALSE(game.isOver());
    EXPECT_TRUE(game.lastTurnWasPass());
    game.endTurn();  // 白パス
    ASSERT_TRUE(game.isOver());
    EXPECT_FALSE(game.result()->winner.has_value());  // 4 対 4 で引き分け
    EXPECT_EQ(game.play(CardType::Place, Targets{{0, 0}, {0, 1}}).status, PlayStatus::GameOver);
}

TEST(GameTest, PassThenActionResetsPassStreak)
{
    Game game(Board::createInitial(),
              scriptedDeck({CardType::Block, CardType::Shield, CardType::Restore},
                           {CardType::Block, CardType::Shield, CardType::Restore},
                           {CardType::Burst, CardType::Burst, CardType::Burst}));
    game.endTurn();  // 黒パス
    ASSERT_TRUE(game.play(CardType::Restore, Targets{}).ok());
    game.endTurn();  // 白は行動
    EXPECT_EQ(game.round(), 2);
    game.endTurn();  // 黒パス
    EXPECT_FALSE(game.isOver());
    game.endTurn();  // 白パス → 連続 2 パス
    EXPECT_TRUE(game.isOver());
}

TEST(GameTest, WinnerHasMoreStones)
{
    const Board board = makeBoard({"BBB.....", "........", "........", "...W....",
                                   "........", "........", "........", "........"});
    Game game(board, Deck::createStandard(5));
    game.endTurn();
    game.endTurn();
    ASSERT_TRUE(game.isOver());
    EXPECT_EQ(game.result()->winner, Color::Black);
    EXPECT_EQ(game.result()->blackStones, 3);
    EXPECT_EQ(game.result()->whiteStones, 1);
}

TEST(GameTest, CardsAreConservedOverManyTurns)
{
    Game game(2026);
    for (int turn = 0; turn < 40 && !game.isOver(); ++turn) {
        // 手札の先頭から、使えるカードを適当な対象で試す
        for (const auto card : game.currentPlayer().hand()) {
            if (game.canPlay(card) && cardflow::targetCount(card) == 0) {
                (void)game.play(card, Targets{});
                break;
            }
        }
        game.endTurn();
        EXPECT_EQ(totalCards(game), 30u);
    }
}

// 有効な対象の組を総当たりで 1 つ探す
std::optional<Targets> findTargets(const Game& game, CardType card)
{
    const int count = cardflow::targetCount(card);
    const auto& board = game.board();
    const auto& actor = game.currentPlayer();
    if (count == 0) {
        return cardflow::canApplyEffect(card, board, actor, Targets{}) ? std::optional<Targets>{Targets{}}
                                                                        : std::nullopt;
    }
    for (int a = 0; a < 64; ++a) {
        const Position first{a / 8, a % 8};
        if (count == 1) {
            if (cardflow::canApplyEffect(card, board, actor, Targets{first})) {
                return Targets{first};
            }
            continue;
        }
        for (int b = 0; b < 64; ++b) {
            const Targets targets{first, Position{b / 8, b % 8}};
            if (cardflow::canApplyEffect(card, board, actor, targets)) {
                return targets;
            }
        }
    }
    return std::nullopt;
}

// 使えるカードを使い続ける自動対戦で、例外や不変条件の破れが起きないこと
TEST(GameTest, AutomatedPlaythroughKeepsInvariants)
{
    for (const std::uint32_t seed : {1u, 2u, 3u}) {
        Game game(seed);
        for (int turn = 0; turn < 120 && !game.isOver(); ++turn) {
            while (game.canPlayAnything()) {
                bool played = false;
                std::vector<CardType> options = game.currentPlayer().hand();
                if (game.currentPlayer().bonusPlaces() > 0) {
                    options.push_back(CardType::Place);
                }
                for (const auto card : options) {
                    if (!game.canPlay(card)) {
                        continue;
                    }
                    const auto targets = findTargets(game, card);
                    ASSERT_TRUE(targets.has_value()) << "canPlay なのに対象がない";
                    ASSERT_TRUE(game.play(card, *targets).ok());
                    played = true;
                    break;
                }
                ASSERT_TRUE(played);
            }
            game.endTurn();

            ASSERT_EQ(totalCards(game), 30u) << "seed " << seed << " turn " << turn;
            for (const auto color : {Color::Black, Color::White}) {
                const auto& p = game.player(color);
                ASSERT_GE(p.ep(), 0);
                ASSERT_LE(p.ep(), cardflow::Player::kMaxEp);
                ASSERT_LE(p.hand().size(), cardflow::kHandSize);
                ASSERT_TRUE(cardflow::isBalancedHand(p.hand()));
            }
            const auto& board = game.board();
            int walls = 0;
            for (int i = 0; i < 64; ++i) {
                walls += board.isWall(Position{i / 8, i % 8}) ? 1 : 0;
            }
            const int cells = board.countStones(Color::Black) + board.countStones(Color::White) +
                              board.countEmpty() + walls;
            ASSERT_EQ(cells, 64);
        }
    }
}

} // namespace
