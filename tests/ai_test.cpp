#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "board_builder.hpp"
#include "cardflow/ai.hpp"
#include "cardflow/game.hpp"
#include "deck_builder.hpp"

using cardflow::Board;
using cardflow::CardType;
using cardflow::Color;
using cardflow::Game;
using cardflow::Position;
using cardflow::ai::Action;
using cardflow::ai::CpuPlayer;
using cardflow::ai::Difficulty;
using cardflow::ai::evaluate;
using cardflow::test::makeBoard;
using cardflow::test::scriptedDeck;

namespace {

constexpr Difficulty kSmart[] = {Difficulty::Normal, Difficulty::Hard};

TEST(AiTest, DifficultyNames)
{
    EXPECT_EQ(cardflow::ai::difficultyName(Difficulty::Easy), "弱い");
    EXPECT_EQ(cardflow::ai::difficultyName(Difficulty::Normal), "普通");
    EXPECT_EQ(cardflow::ai::difficultyName(Difficulty::Hard), "強い");
}

TEST(AiTest, CornerIsWorthMore)
{
    const Game corner(makeBoard({"B.......", "........", "........", "...WB...", "...BW...", "........",
                                 "........", "........"}),
                      scriptedDeck({}, {}));
    const Game middle(makeBoard({"........", "........", "..B.....", "...WB...", "...BW...", "........",
                                 "........", "........"}),
                      scriptedDeck({}, {}));
    EXPECT_GT(evaluate(corner, Color::Black), evaluate(middle, Color::Black));
}

TEST(AiTest, CardsInHandAreWorthSomething)
{
    const Game withCard(Board::createInitial(), scriptedDeck({CardType::Wall}, {}));
    const Game without(Board::createInitial(), scriptedDeck({}, {}));
    EXPECT_GT(evaluate(withCard, Color::Black), evaluate(without, Color::Black));
}

TEST(AiTest, SmartCpuTakesCorner)
{
    // a1（角）か c4 のどちらかに置ける
    const Board board = makeBoard({".WB.....", "........", "........", "...WB...", "........", "........",
                                   "........", "........"});
    for (const auto level : kSmart) {
        const Game game(board, scriptedDeck({}, {}));
        CpuPlayer cpu(level, 1);
        const Action action = cpu.chooseAction(game);
        EXPECT_EQ(action.kind, Action::Kind::Place);
        EXPECT_EQ(action.pos, (Position{0, 0}));
    }
}

TEST(AiTest, SmartCpuUsesFlipToWin)
{
    // 黒は置けないが、反転で最後の白石を奪えば勝てる
    const Board board = makeBoard({"B.......", "........", "...W....", "........", "........", "........",
                                   "........", "........"});
    for (const auto level : kSmart) {
        const Game game(board, scriptedDeck({CardType::Flip, CardType::Wall}, {}));
        CpuPlayer cpu(level, 1);
        const Action action = cpu.chooseAction(game);
        EXPECT_EQ(action.kind, Action::Kind::UseCard);
        EXPECT_EQ(action.card, CardType::Flip);
        EXPECT_EQ(action.target, (Position{2, 3}));
    }
}

TEST(AiTest, PassesWhenNothingToDo)
{
    const Board board = makeBoard({"W.......", "........", "........", "........", "........", "........",
                                   "........", ".......B"});
    for (const auto level : {Difficulty::Easy, Difficulty::Normal, Difficulty::Hard}) {
        Game game(board, scriptedDeck({}, {}));
        CpuPlayer cpu(level, 1);
        const Action action = cpu.chooseAction(game);
        EXPECT_EQ(action.kind, Action::Kind::Pass);
        EXPECT_TRUE(cardflow::ai::applyAction(game, action));
    }
}

TEST(AiTest, SmartCpuDoesNotWasteCards)
{
    // 初手では、守りや壁を使っても得をしないので石を置く
    for (const auto level : kSmart) {
        const Game game(Board::createInitial(), scriptedDeck({CardType::Guard, CardType::Wall}, {}));
        CpuPlayer cpu(level, 1);
        EXPECT_EQ(cpu.chooseAction(game).kind, Action::Kind::Place);
    }
}

TEST(AiTest, EasyIsReproducible)
{
    const Game game(5);
    CpuPlayer a(Difficulty::Easy, 99);
    CpuPlayer b(Difficulty::Easy, 99);
    EXPECT_EQ(a.chooseAction(game), b.chooseAction(game));
}

// CPU 同士の対戦が、どの強さの組み合わせでも正しい行動だけで終わること
TEST(AiTest, CpuVersusCpuGamesFinish)
{
    const Difficulty levels[] = {Difficulty::Easy, Difficulty::Normal, Difficulty::Hard};
    std::uint32_t seed = 100;
    for (const auto blackLevel : levels) {
        for (const auto whiteLevel : levels) {
            Game game(seed);
            CpuPlayer black(blackLevel, seed + 1);
            CpuPlayer white(whiteLevel, seed + 2);
            ++seed;
            for (int steps = 0; steps < 300 && !game.isOver(); ++steps) {
                CpuPlayer& cpu = game.currentColor() == Color::Black ? black : white;
                ASSERT_TRUE(cardflow::ai::applyAction(game, cpu.chooseAction(game)));
            }
            EXPECT_TRUE(game.isOver()) << "終わらなかった";
        }
    }
}

} // namespace
