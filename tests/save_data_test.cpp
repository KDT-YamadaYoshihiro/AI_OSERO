#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "board_builder.hpp"
#include "cardflow/app/save_data.hpp"
#include "deck_builder.hpp"
#include "game_builder.hpp"

using cardflow::Board;
using cardflow::CardStatus;
using cardflow::CardType;
using cardflow::Color;
using cardflow::Game;
using cardflow::Position;
using cardflow::ai::Difficulty;
using cardflow::app::GameOptions;
using cardflow::app::SaveData;
using cardflow::test::cardsReady;
using cardflow::test::scriptedDeck;
namespace app = cardflow::app;

namespace {

// 守り・壁・カード使用済み・連打の途中などを含む状態
SaveData sampleData()
{
    Game game = cardsReady(Game(Board::createInitial(),
                                scriptedDeck({CardType::Double, CardType::Wall}, {CardType::Guard, CardType::Free},
                                             {CardType::Flip})));
    EXPECT_TRUE(game.place(Position{2, 3}).has_value());                              // 黒 d3
    EXPECT_EQ(game.useCard(CardType::Guard, Position{4, 4}), CardStatus::Ok);         // 白 e5 に守り
    EXPECT_TRUE(game.place(game.placeableCells().front()).has_value());               // 白
    EXPECT_EQ(game.useCard(CardType::Wall, Position{5, 5}), CardStatus::Ok);          // 黒 f6 を壁に
    GameOptions options;
    options.cpuColor = Color::White;
    options.difficulty = Difficulty::Hard;
    return SaveData{game.state().value(), options, 4'000'000'000u};
}

TEST(SaveDataTest, RoundTripThroughText)
{
    const SaveData data = sampleData();
    const std::string text = app::serializeSave(data);
    const auto parsed = app::parseSave(text);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->seed, data.seed);
    EXPECT_EQ(parsed->options, data.options);
    EXPECT_EQ(app::serializeSave(*parsed), text);  // 盤面・手札・フラグまで同じ

    const auto game = Game::restore(parsed->game);
    ASSERT_TRUE(game.has_value());
    EXPECT_TRUE(game->board().isWall(Position{5, 5}));
    EXPECT_TRUE(game->board().stoneAt(Position{4, 4})->guarded);
    EXPECT_TRUE(game->cardUsedThisTurn());
    EXPECT_EQ(game->currentColor(), Color::Black);
}

TEST(SaveDataTest, TwoPlayerHasNoCpu)
{
    SaveData data = sampleData();
    data.options = GameOptions{};
    const auto parsed = app::parseSave(app::serializeSave(data));
    ASSERT_TRUE(parsed.has_value());
    EXPECT_FALSE(parsed->options.cpuColor.has_value());
}

TEST(SaveDataTest, AcceptsCrLf)
{
    std::string text = app::serializeSave(sampleData());
    std::string crlf;
    for (const char ch : text) {
        if (ch == '\n') {
            crlf += '\r';
        }
        crlf += ch;
    }
    EXPECT_TRUE(app::parseSave(crlf).has_value());
}

TEST(SaveDataTest, RejectsBrokenText)
{
    const std::string text = app::serializeSave(sampleData());
    EXPECT_FALSE(app::parseSave("").has_value());
    EXPECT_FALSE(app::parseSave("hello\n").has_value());

    auto replaceLine = [&](const std::string& key, const std::string& value) {
        std::string copy = text;
        const auto start = copy.find(key + "=");
        const auto end = copy.find('\n', start);
        copy.replace(start, end - start, key + "=" + value);
        return copy;
    };
    EXPECT_FALSE(app::parseSave(replaceLine("board", "XYZ")).has_value());
    EXPECT_FALSE(app::parseSave(replaceLine("current", "red")).has_value());
    EXPECT_FALSE(app::parseSave(replaceLine("deck", "9")).has_value());
    EXPECT_FALSE(app::parseSave(replaceLine("difficulty", "5")).has_value());
    EXPECT_FALSE(app::parseSave(replaceLine("last_placed", "z9")).has_value());
    EXPECT_FALSE(app::parseSave(replaceLine("card_used", "yes")).has_value());

    std::string missing = text;
    missing.erase(missing.find("turn="), missing.find('\n', missing.find("turn=")) - missing.find("turn=") + 1);
    EXPECT_FALSE(app::parseSave(missing).has_value());
}

TEST(SaveDataTest, WritesAndReadsFile)
{
    const auto path = std::filesystem::temp_directory_path() / "cardflow_save_data_test" / "save.txt";
    std::filesystem::remove_all(path.parent_path());
    EXPECT_FALSE(app::readSaveFile(path).has_value());

    const SaveData data = sampleData();
    ASSERT_TRUE(app::writeSaveFile(path, data));  // フォルダも作る
    const auto read = app::readSaveFile(path);
    ASSERT_TRUE(read.has_value());
    EXPECT_EQ(app::serializeSave(*read), app::serializeSave(data));
    std::filesystem::remove_all(path.parent_path());
}

} // namespace
