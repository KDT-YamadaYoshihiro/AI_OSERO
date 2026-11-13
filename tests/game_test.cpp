#include <gtest/gtest.h>

#include <vector>

#include "board_builder.hpp"
#include "cardflow/game.hpp"
#include "deck_builder.hpp"

using cardflow::Board;
using cardflow::CardStatus;
using cardflow::CardType;
using cardflow::Color;
using cardflow::Game;
using cardflow::Position;
using cardflow::test::colorAt;
using cardflow::test::makeBoard;
using cardflow::test::scriptedDeck;

namespace {

using Cards = std::vector<CardType>;

Game startInitial(const Cards& black = {}, const Cards& white = {}, const Cards& rest = {})
{
    return Game(Board::createInitial(), scriptedDeck(black, white, rest));
}

// 置けるマスの先頭に置く
void placeFirst(Game& game)
{
    const auto cells = game.placeableCells();
    ASSERT_FALSE(cells.empty());
    ASSERT_TRUE(game.place(cells.front()).has_value());
}

TEST(GameTest, DealsTwoCardsEachAndBlackStarts)
{
    const Game game = startInitial({CardType::Wall, CardType::Flip}, {CardType::Guard, CardType::Free},
                                   {CardType::Double});
    EXPECT_EQ(game.hand(Color::Black), (Cards{CardType::Wall, CardType::Flip}));
    EXPECT_EQ(game.hand(Color::White), (Cards{CardType::Guard, CardType::Free}));
    EXPECT_EQ(game.deck().size(), 1u);
    EXPECT_EQ(game.currentColor(), Color::Black);
    EXPECT_EQ(game.turnNumber(), 1);
    EXPECT_FALSE(game.isOver());
}

TEST(GameTest, StandardDeckGameStarts)
{
    const Game game(7);
    EXPECT_EQ(game.hand(Color::Black).size(), 2u);
    EXPECT_EQ(game.hand(Color::White).size(), 2u);
    EXPECT_EQ(game.deck().size(), 11u);
}

TEST(GameTest, PlacingFlipsAndPassesTurn)
{
    Game game = startInitial();
    EXPECT_EQ(game.placeableCells().size(), 4u);
    EXPECT_FALSE(game.place(Position{0, 0}).has_value());  // 置けないマス
    const auto result = game.place(Position{2, 3});         // d3
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->flipped.size(), 1u);
    EXPECT_EQ(game.lastPlaced(), (Position{2, 3}));
    EXPECT_EQ(game.currentColor(), Color::White);
    EXPECT_EQ(game.turnNumber(), 2);
    EXPECT_EQ(game.board().countStones(Color::Black), 4);
}

TEST(GameTest, OneCardPerTurnBeforePlacing)
{
    Game game = startInitial({CardType::Wall, CardType::Flip});
    EXPECT_TRUE(game.canUseCard(CardType::Wall));
    EXPECT_FALSE(game.canUseCard(CardType::Guard));  // 手札にない
    EXPECT_EQ(game.useCard(CardType::Guard, Position{3, 4}), CardStatus::NotInHand);
    EXPECT_EQ(game.useCard(CardType::Wall), CardStatus::InvalidTarget);                  // 対象が必要
    EXPECT_EQ(game.useCard(CardType::Wall, Position{3, 3}), CardStatus::InvalidTarget);  // 空マスではない
    EXPECT_EQ(game.hand(Color::Black).size(), 2u);                                      // 失敗では減らない

    EXPECT_EQ(game.useCard(CardType::Wall, Position{7, 7}), CardStatus::Ok);
    EXPECT_TRUE(game.board().isWall(Position{7, 7}));
    EXPECT_EQ(game.hand(Color::Black), (Cards{CardType::Flip}));
    EXPECT_TRUE(game.cardUsedThisTurn());
    EXPECT_FALSE(game.canUseCard(CardType::Flip));
    EXPECT_EQ(game.useCard(CardType::Flip, Position{3, 3}), CardStatus::AlreadyUsed);
    EXPECT_EQ(game.currentColor(), Color::Black);  // カードを使ってもまだ自分の番
}

TEST(GameTest, FlipTakesStoneThenStillPlace)
{
    Game game = startInitial({CardType::Flip});
    EXPECT_EQ(game.cardTargets(CardType::Flip), (std::vector<Position>{{3, 3}, {4, 4}}));
    ASSERT_EQ(game.useCard(CardType::Flip, Position{3, 3}), CardStatus::Ok);
    EXPECT_EQ(colorAt(game.board(), Position{3, 3}), Color::Black);
    EXPECT_FALSE(game.placeableCells().empty());
    EXPECT_EQ(game.currentColor(), Color::Black);
}

TEST(GameTest, DoublePlacesTwice)
{
    // 黒は a2・a7 に置ける（h4 の白は残る）
    const Board board = makeBoard({"........", ".WB.....", "........", ".......W", "........", "........",
                                   ".WB.....", "........"});
    Game game(board, scriptedDeck({CardType::Double}, {}));
    ASSERT_EQ(game.useCard(CardType::Double), CardStatus::Ok);
    EXPECT_EQ(game.placementsLeft(), 2);
    ASSERT_TRUE(game.place(Position{1, 0}).has_value());
    EXPECT_EQ(game.currentColor(), Color::Black);  // もう 1 つ置ける
    EXPECT_EQ(game.placedThisTurn(), 1);
    EXPECT_FALSE(game.mustPass());
    ASSERT_TRUE(game.place(Position{6, 0}).has_value());
    EXPECT_FALSE(game.isOver());
    EXPECT_EQ(game.currentColor(), Color::White);
}

TEST(GameTest, DoubleEndsTurnWhenNoSecondMove)
{
    const Board board = makeBoard({"BW......", "........", "........", "........", "........", "........",
                                   "........", "......W."});
    Game game(board, scriptedDeck({CardType::Double}, {}));
    ASSERT_EQ(game.useCard(CardType::Double), CardStatus::Ok);
    ASSERT_TRUE(game.place(Position{0, 2}).has_value());  // c1: b1 を挟む
    EXPECT_EQ(game.currentColor(), Color::White);          // 2 つ目は置けないので終わる
}

TEST(GameTest, FreeAllowsCellNextToStone)
{
    Game game = startInitial({CardType::Free});
    EXPECT_FALSE(game.canPlace(Position{2, 2}));  // c3 は通常は置けない
    ASSERT_EQ(game.useCard(CardType::Free), CardStatus::Ok);
    EXPECT_TRUE(game.freeMoveActive());
    EXPECT_TRUE(game.canPlace(Position{2, 2}));
    EXPECT_FALSE(game.canPlace(Position{0, 0}));  // 石の隣ではない
    const auto result = game.place(Position{2, 2});
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->flipped.empty());
    EXPECT_EQ(game.currentColor(), Color::White);
    EXPECT_FALSE(game.freeMoveActive());
}

TEST(GameTest, GuardTargetsOwnStone)
{
    Game game = startInitial({CardType::Wall, CardType::Wall}, {CardType::Guard});
    ASSERT_TRUE(game.place(Position{2, 3}).has_value());                                  // 黒 d3（d4 を裏返す）
    EXPECT_EQ(game.useCard(CardType::Guard, Position{3, 3}), CardStatus::InvalidTarget);  // d4 はもう黒
    ASSERT_EQ(game.useCard(CardType::Guard, Position{4, 4}), CardStatus::Ok);             // 白 e5
    EXPECT_TRUE(game.board().stoneAt(Position{4, 4})->guarded);
}

TEST(GameTest, PassOnlyWhenNoMove)
{
    Game initial = startInitial();
    EXPECT_FALSE(initial.mustPass());
    EXPECT_FALSE(initial.pass());

    const Board board = makeBoard({"W.......", "........", "........", "........", "........", "........",
                                   "........", ".......B"});
    Game game(board, scriptedDeck({}, {}));
    EXPECT_TRUE(game.mustPass());
    ASSERT_TRUE(game.pass());
    EXPECT_EQ(game.currentColor(), Color::White);
    EXPECT_TRUE(game.lastTurnWasPass());
    ASSERT_TRUE(game.pass());  // 両者が続けてパス
    ASSERT_TRUE(game.isOver());
    EXPECT_FALSE(game.result()->winner.has_value());  // 1 対 1 で引き分け
}

TEST(GameTest, WipeoutEndsGame)
{
    const Board board = makeBoard({"BW......", "........", "........", "........", "........", "........",
                                   "........", "........"});
    Game game(board, scriptedDeck({}, {}));
    ASSERT_TRUE(game.place(Position{0, 2}).has_value());
    ASSERT_TRUE(game.isOver());
    EXPECT_EQ(game.result()->winner, Color::Black);
    EXPECT_EQ(game.result()->blackStones, 3);
    EXPECT_EQ(game.result()->whiteStones, 0);
    EXPECT_FALSE(game.place(Position{0, 3}).has_value());
    EXPECT_EQ(game.useCard(CardType::Wall, Position{5, 5}), CardStatus::GameOver);
}

TEST(GameTest, FullBoardEndsGame)
{
    const Board board = makeBoard({".WB#####", "W#######", "########", "########", "########", "########",
                                   "########", "########"});
    Game game(board, scriptedDeck({}, {}));
    ASSERT_TRUE(game.place(Position{0, 0}).has_value());
    ASSERT_TRUE(game.isOver());
    EXPECT_EQ(game.result()->winner, Color::Black);
    EXPECT_EQ(game.result()->whiteStones, 1);
}

TEST(GameTest, DrawsEveryThirdOwnTurn)
{
    Game game = startInitial({CardType::Wall, CardType::Flip}, {CardType::Guard, CardType::Free},
                             {CardType::Double, CardType::Free});
    for (int i = 0; i < 4; ++i) {  // 黒 2 回・白 2 回
        placeFirst(game);
    }
    // 黒の 3 回目の番の始めに 1 枚引く
    ASSERT_EQ(game.currentColor(), Color::Black);
    EXPECT_EQ(game.hand(Color::Black), (Cards{CardType::Wall, CardType::Flip, CardType::Double}));
    placeFirst(game);
    EXPECT_EQ(game.hand(Color::White), (Cards{CardType::Guard, CardType::Free, CardType::Free}));
    EXPECT_TRUE(game.deck().empty());
}

TEST(GameTest, NoDrawWhenHandIsFull)
{
    Game game = startInitial({CardType::Wall, CardType::Flip}, {CardType::Guard, CardType::Free},
                             {CardType::Double, CardType::Wall, CardType::Free});
    for (int i = 0; i < 10; ++i) {  // 黒の 6 回目の番まで進める
        placeFirst(game);
    }
    ASSERT_EQ(game.currentColor(), Color::Black);
    EXPECT_EQ(game.hand(Color::Black).size(), 3u);  // 3 回目で 3 枚、6 回目は上限なので引かない
    EXPECT_EQ(game.deck().size(), 1u);
}

// ─── 投了 ────────────────────────────────────────────────────────────

TEST(GameTest, ResignEndsGame)
{
    Game game = startInitial();
    ASSERT_TRUE(game.resign(Color::Black));
    ASSERT_TRUE(game.isOver());
    EXPECT_EQ(game.result()->winner, Color::White);
    EXPECT_EQ(game.result()->resignedBy, Color::Black);
    EXPECT_EQ(game.result()->blackStones, 2);
    EXPECT_FALSE(game.resign(Color::White));  // 終局後はできない
}

// ─── 中断・再開 ──────────────────────────────────────────────────────

TEST(GameTest, StateRoundTripKeepsEverything)
{
    Game game = startInitial({CardType::Double, CardType::Flip}, {CardType::Guard, CardType::Free},
                             {CardType::Wall, CardType::Free});
    placeFirst(game);  // 黒
    ASSERT_EQ(game.useCard(CardType::Guard, Position{4, 4}), CardStatus::Ok);  // 白 e5 を守る
    const auto state = game.state();
    ASSERT_TRUE(state.has_value());

    auto restored = Game::restore(*state);
    ASSERT_TRUE(restored.has_value());
    EXPECT_EQ(restored->currentColor(), Color::White);
    EXPECT_EQ(restored->turnNumber(), 2);
    EXPECT_TRUE(restored->cardUsedThisTurn());
    EXPECT_EQ(restored->hand(Color::Black), game.hand(Color::Black));
    EXPECT_EQ(restored->hand(Color::White), game.hand(Color::White));
    EXPECT_EQ(restored->deck().cards(), game.deck().cards());
    EXPECT_EQ(restored->lastPlaced(), game.lastPlaced());
    EXPECT_TRUE(restored->board().stoneAt(Position{4, 4})->guarded);
    EXPECT_EQ(restored->placeableCells(), game.placeableCells());

    // 再開後も同じように進む
    placeFirst(game);
    placeFirst(*restored);
    EXPECT_EQ(restored->state()->board.countStones(Color::Black), game.board().countStones(Color::Black));
    EXPECT_EQ(restored->currentColor(), game.currentColor());
}

TEST(GameTest, StateIsEmptyAfterGameOver)
{
    Game game = startInitial();
    ASSERT_TRUE(game.resign(Color::White));
    EXPECT_FALSE(game.state().has_value());
}

TEST(GameTest, RestoreRejectsInconsistentState)
{
    const auto base = startInitial().state().value();

    auto placedWithoutCard = base;
    placedWithoutCard.placed = 1;  // カードなしで置いた後に同じ番は続かない
    EXPECT_FALSE(Game::restore(placedWithoutCard).has_value());

    auto tooManyCards = base;
    tooManyCards.hands[0] = {CardType::Wall, CardType::Wall, CardType::Wall, CardType::Wall};
    EXPECT_FALSE(Game::restore(tooManyCards).has_value());

    auto lastOnEmpty = base;
    lastOnEmpty.lastPlaced = Position{0, 0};  // 石のないマス
    EXPECT_FALSE(Game::restore(lastOnEmpty).has_value());

    auto finished = base;
    finished.board = makeBoard({"B.......", "........", "........", "........", "........", "........",
                                "........", "........"});  // 白が 0 個
    EXPECT_FALSE(Game::restore(finished).has_value());

    EXPECT_TRUE(Game::restore(base).has_value());
}

} // namespace
