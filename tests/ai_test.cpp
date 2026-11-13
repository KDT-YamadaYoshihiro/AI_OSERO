#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <vector>

#include "board_builder.hpp"
#include "cardflow/ai.hpp"
#include "cardflow/game.hpp"

using cardflow::Board;
using cardflow::CardType;
using cardflow::Color;
using cardflow::Deck;
using cardflow::Game;
using cardflow::ai::Action;
using cardflow::ai::CpuPlayer;
using cardflow::ai::Difficulty;
using cardflow::ai::enumerateActions;
using cardflow::ai::evaluate;
using cardflow::test::makeBoard;

namespace {

// 末尾から 黒 3 枚 → 白 3 枚 の順に配られる山札
Deck scriptedDeck(std::vector<CardType> black, std::vector<CardType> white, std::vector<CardType> rest = {})
{
    std::vector<CardType> pile = rest;
    pile.insert(pile.end(), white.rbegin(), white.rend());
    pile.insert(pile.end(), black.rbegin(), black.rend());
    return Deck(pile, 0);
}

const std::vector<CardType> kFiller{CardType::Block, CardType::Shield, CardType::Restore};

TEST(AiTest, EnumeratesEveryTargetCombination)
{
    const Game game(Board::createInitial(), scriptedDeck({CardType::Place, CardType::Block, CardType::Restore}, kFiller));
    std::map<CardType, int> counts;
    for (const auto& action : enumerateActions(game)) {
        ++counts[action.card];
    }
    EXPECT_EQ(counts[CardType::Place], 56 * 55 / 2);  // 空マス 56 から順序なしの 2 つ
    EXPECT_EQ(counts[CardType::Block], 56);
    EXPECT_EQ(counts[CardType::Restore], 1);
}

TEST(AiTest, EnumeratedActionsAreAllPlayable)
{
    for (const std::uint32_t seed : {11u, 12u, 13u}) {
        Game game(seed);
        for (int turn = 0; turn < 6 && !game.isOver(); ++turn) {
            for (const auto& action : enumerateActions(game)) {
                Game trial = game;
                ASSERT_TRUE(trial.play(action.card, action.targets).ok()) << cardflow::cardInfo(action.card).name;
            }
            game.endTurn();
        }
    }
}

TEST(AiTest, NothingToDoAfterNormalCardWithoutBonus)
{
    Game game(Board::createInitial(), scriptedDeck({CardType::Restore, CardType::Block, CardType::Shield}, kFiller));
    ASSERT_TRUE(game.play(CardType::Restore, {}).ok());
    EXPECT_TRUE(enumerateActions(game).empty());
    CpuPlayer cpu(Difficulty::Easy, 1);
    EXPECT_FALSE(cpu.chooseAction(game).has_value());
}

TEST(AiTest, NormalPrefersPlaceThatGainsStones)
{
    Game game(Board::createInitial(), scriptedDeck({CardType::Place, CardType::Block, CardType::Restore}, kFiller));
    CpuPlayer cpu(Difficulty::Normal, 1);
    const auto action = cpu.chooseAction(game);
    ASSERT_TRUE(action.has_value());
    EXPECT_EQ(action->card, CardType::Place);
    const auto result = game.play(action->card, action->targets);
    ASSERT_TRUE(result.ok());
    EXPECT_GE(result.flipped, 2);
}

TEST(AiTest, NormalTakesImmediateWin)
{
    // Place の 1 石目で最後の白石を挟めば白が全滅する
    const Board board = makeBoard({"BW......", "........", "........", "........",
                                   "........", "........", "........", "B.......",});
    Game game(board, scriptedDeck({CardType::Place, CardType::Block, CardType::Restore}, kFiller));
    CpuPlayer cpu(Difficulty::Normal, 1);
    const auto action = cpu.chooseAction(game);
    ASSERT_TRUE(action.has_value());
    EXPECT_EQ(action->card, CardType::Place);
    ASSERT_TRUE(game.play(action->card, action->targets).ok());
    ASSERT_TRUE(game.isOver());
    EXPECT_EQ(game.result()->winner, Color::Black);
}

TEST(AiTest, NormalEndsTurnWhenNothingImproves)
{
    // 脅威のない局面では、Block / Shield / Cover はいずれも EP を払うだけで良くならない
    const Board quiet = makeBoard({"B.......", "........", "........", "........",
                                   "........", "........", "........", ".......W"});
    const Game game(quiet, scriptedDeck({CardType::Block, CardType::Shield, CardType::Cover}, kFiller));
    CpuPlayer cpu(Difficulty::Normal, 1);
    EXPECT_FALSE(cpu.chooseAction(game).has_value());
}

TEST(AiTest, EasyIsReproducibleAndValid)
{
    const Game game(5);
    CpuPlayer a(Difficulty::Easy, 99);
    CpuPlayer b(Difficulty::Easy, 99);
    const auto first = a.chooseAction(game);
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first, b.chooseAction(game));
    Game trial = game;
    EXPECT_TRUE(trial.play(first->card, first->targets).ok());
}

TEST(AiTest, NormalPenalizesCapturableStones)
{
    // 白から見て、挟まれている（Burst で取られうる）石がある局面は不利
    const Board threatened = makeBoard({"BWB.....", "........", "........", "........",
                                        "........", "........", "........", "........"});
    const Board safe = makeBoard({"BW.B....", "........", "........", "........",
                                  "........", "........", "........", "........"});
    const Game a(threatened, scriptedDeck(kFiller, kFiller));
    const Game b(safe, scriptedDeck(kFiller, kFiller));
    EXPECT_LT(evaluate(a, Color::White, Difficulty::Normal), evaluate(b, Color::White, Difficulty::Normal));
}

TEST(AiTest, NormalPenalizesFlippableStones)
{
    // 白から見て、次の Place で裏返されうる石（A）は、裏返されない石（B）より危険
    const Board exposed = makeBoard({"BW......", "........", "........", "........",
                                     "........", "........", "........", ".......B"});
    const Board safe = makeBoard({"B.W.....", "........", "........", "........",
                                  "........", "........", "........", ".......B"});
    const Game a(exposed, scriptedDeck(kFiller, kFiller));
    const Game b(safe, scriptedDeck(kFiller, kFiller));
    EXPECT_LT(evaluate(a, Color::White, Difficulty::Normal), evaluate(b, Color::White, Difficulty::Normal));
}

TEST(AiTest, HardAddsOwnOpportunities)
{
    // 黒から見て、Burst で取れる敵石がある局面（A）は、強いでは普通より高く評価される
    const Board chance = makeBoard({"BWB.....", "........", "........", "........",
                                    "........", "........", "........", "........"});
    const Board none = makeBoard({"BW.B....", "........", "........", "........",
                                  "........", "........", "........", "........"});
    const Game a(chance, scriptedDeck(kFiller, kFiller));
    const Game b(none, scriptedDeck(kFiller, kFiller));
    const int bonusA = evaluate(a, Color::Black, Difficulty::Hard) - evaluate(a, Color::Black, Difficulty::Normal);
    const int bonusB = evaluate(b, Color::Black, Difficulty::Hard) - evaluate(b, Color::Black, Difficulty::Normal);
    EXPECT_GT(bonusA, bonusB);
}

TEST(AiTest, EvaluateFinishedGame)
{
    const Board board = makeBoard({"BW......", "........", "........", "........",
                                   "........", "........", "........", "........"});
    Game game(board, scriptedDeck({CardType::Place, CardType::Block, CardType::Restore}, kFiller));
    ASSERT_TRUE(game.play(CardType::Place, std::vector<cardflow::Position>{{0, 2}, {7, 7}}).ok());
    ASSERT_TRUE(game.isOver());
    EXPECT_GT(evaluate(game, Color::Black, Difficulty::Normal), 100000);
    EXPECT_LT(evaluate(game, Color::White, Difficulty::Hard), -100000);
}

// CPU 同士の対戦が、どの強さの組み合わせでも正常に終わること
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
            int turns = 0;
            for (; turns < 400 && !game.isOver(); ++turns) {
                CpuPlayer& cpu = game.currentColor() == Color::Black ? black : white;
                while (const auto action = cpu.chooseAction(game)) {
                    ASSERT_TRUE(game.play(action->card, action->targets).ok());
                    if (game.isOver()) {
                        break;
                    }
                }
                game.endTurn();
            }
            EXPECT_LT(turns, 400) << "終わらなかった";
        }
    }
}

} // namespace
