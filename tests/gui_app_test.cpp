#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "board_builder.hpp"
#include "cardflow/app/gui_app.hpp"
#include "cardflow/app/layout.hpp"
#include "deck_builder.hpp"

using cardflow::Board;
using cardflow::CardType;
using cardflow::Color;
using cardflow::Game;
using cardflow::Position;
using cardflow::ai::Difficulty;
using cardflow::app::GameOptions;
using cardflow::app::GuiApp;
using cardflow::app::Rect;
using cardflow::app::Screen;
using cardflow::test::makeBoard;
using cardflow::test::scriptedDeck;
namespace app = cardflow::app;

namespace {

const std::vector<CardType> kFiller{CardType::Block, CardType::Shield, CardType::Restore};

void clickRect(GuiApp& gui, const Rect& rect)
{
    gui.click(rect.centerX(), rect.centerY());
}

void clickCell(GuiApp& gui, Position pos)
{
    clickRect(gui, app::cellRect(pos));
}

bool logContains(const GuiApp& gui, std::string_view text)
{
    return std::ranges::any_of(gui.log(), [&](const std::string& line) { return line.find(text) != std::string::npos; });
}

// 初期盤面・指定の手札で 2 人対戦を始める
GuiApp startTwoPlayer(const std::vector<CardType>& black, const std::vector<CardType>& white = kFiller,
                      const Board& board = Board::createInitial())
{
    GuiApp gui(1);
    gui.startGame(Game(board, scriptedDeck(black, white, kFiller)), GameOptions{});
    return gui;
}

// ─── メニュー ────────────────────────────────────────────────────────

TEST(GuiAppTest, TitleStartsTwoPlayerGame)
{
    GuiApp gui(42);
    EXPECT_EQ(gui.screen(), Screen::Title);
    EXPECT_EQ(gui.game(), nullptr);
    clickRect(gui, app::menuButtonRect(0));
    ASSERT_EQ(gui.screen(), Screen::Playing);
    ASSERT_NE(gui.game(), nullptr);
    EXPECT_FALSE(gui.options().cpuColor.has_value());
    EXPECT_EQ(gui.seed(), 42u);
    EXPECT_EQ(gui.playerLabel(Color::Black), "黒");
}

TEST(GuiAppTest, CpuMenuChoosesDifficultyAndColor)
{
    GuiApp gui(1);
    clickRect(gui, app::menuButtonRect(1));
    ASSERT_EQ(gui.screen(), Screen::Difficulty);
    clickRect(gui, app::menuButtonRect(2));  // 強い
    ASSERT_EQ(gui.screen(), Screen::ColorSelect);
    clickRect(gui, app::menuButtonRect(1));  // 白（後手）
    ASSERT_EQ(gui.screen(), Screen::Playing);
    EXPECT_EQ(gui.options().cpuColor, Color::Black);
    EXPECT_EQ(gui.options().difficulty, Difficulty::Hard);
    EXPECT_EQ(gui.playerLabel(Color::Black), "黒（CPU・強い）");
    EXPECT_EQ(gui.playerLabel(Color::White), "白（あなた）");
}

TEST(GuiAppTest, BackButtonAndRightClickGoBack)
{
    GuiApp gui(1);
    clickRect(gui, app::menuButtonRect(1));
    clickRect(gui, app::menuButtonRect(0));
    ASSERT_EQ(gui.screen(), Screen::ColorSelect);
    clickRect(gui, app::backButtonRect());
    EXPECT_EQ(gui.screen(), Screen::Difficulty);
    gui.cancel();
    EXPECT_EQ(gui.screen(), Screen::Title);
}

TEST(GuiAppTest, QuitButtonRequestsQuit)
{
    GuiApp gui(1);
    EXPECT_FALSE(gui.quitRequested());
    clickRect(gui, app::menuButtonRect(2));
    EXPECT_TRUE(gui.quitRequested());
}

// ─── カードの使用 ────────────────────────────────────────────────────

TEST(GuiAppTest, PlaceIsPlayedByChoosingTwoCells)
{
    GuiApp gui = startTwoPlayer({CardType::Place, CardType::Block, CardType::Restore});
    ASSERT_EQ(gui.handSlots().size(), 3u);
    clickRect(gui, app::handCardRect(0));
    ASSERT_TRUE(gui.selection().has_value());
    EXPECT_EQ(gui.selection()->card, CardType::Place);
    EXPECT_EQ(gui.candidates().size(), 56u);  // 空マスすべて

    clickCell(gui, Position{0, 0});
    ASSERT_TRUE(gui.selection().has_value());
    EXPECT_EQ(gui.selection()->targets.size(), 1u);
    EXPECT_EQ(gui.candidates().size(), 55u);

    clickCell(gui, Position{7, 7});
    EXPECT_FALSE(gui.selection().has_value());
    const Game& game = *gui.game();
    EXPECT_EQ(game.board().countStones(Color::Black) + game.board().countStones(Color::White), 10);
    EXPECT_TRUE(game.hasPlayedCardThisTurn());
    EXPECT_EQ(gui.lastActionCells(), (std::vector<Position>{{0, 0}, {7, 7}}));
    EXPECT_TRUE(logContains(gui, "黒：Place a1 h8"));
    // 通常のカードは 1 ターン 1 枚
    EXPECT_TRUE(std::ranges::none_of(gui.handSlots(), [](const auto& slot) { return slot.playable; }));
}

TEST(GuiAppTest, NonCandidateCellIsIgnored)
{
    GuiApp gui = startTwoPlayer({CardType::Place, CardType::Block, CardType::Restore});
    clickRect(gui, app::handCardRect(0));
    clickCell(gui, Position{3, 3});  // 石がある
    ASSERT_TRUE(gui.selection().has_value());
    EXPECT_TRUE(gui.selection()->targets.empty());
}

TEST(GuiAppTest, ZeroTargetCardUsesButton)
{
    GuiApp gui = startTwoPlayer({CardType::Restore, CardType::Block, CardType::Shield});
    EXPECT_FALSE(gui.canUseSelected());
    clickRect(gui, app::handCardRect(0));
    ASSERT_TRUE(gui.canUseSelected());
    EXPECT_TRUE(gui.candidates().empty());
    clickRect(gui, app::useButtonRect());
    EXPECT_EQ(gui.game()->player(Color::Black).ep(), 4);  // 3 - 1 + 2
    EXPECT_TRUE(logContains(gui, "Restore"));
}

TEST(GuiAppTest, ClickingSelectedCardOrRightClickCancels)
{
    GuiApp gui = startTwoPlayer({CardType::Block, CardType::Shield, CardType::Restore});
    clickRect(gui, app::handCardRect(0));
    ASSERT_TRUE(gui.selection().has_value());
    EXPECT_EQ(gui.candidates().size(), 56u);
    clickRect(gui, app::handCardRect(0));
    EXPECT_FALSE(gui.selection().has_value());
    EXPECT_TRUE(gui.candidates().empty());

    clickRect(gui, app::handCardRect(1));
    ASSERT_TRUE(gui.selection().has_value());
    EXPECT_EQ(gui.candidates().size(), 4u);  // Shield は自分の石 4 つ
    gui.cancel();
    EXPECT_FALSE(gui.selection().has_value());
}

TEST(GuiAppTest, UnplayableCardShowsReason)
{
    GuiApp gui = startTwoPlayer({CardType::Burst, CardType::Block, CardType::Restore});
    EXPECT_FALSE(gui.handSlots()[0].playable);  // Burst は EP 4、開始時は 3
    clickRect(gui, app::handCardRect(0));
    EXPECT_FALSE(gui.selection().has_value());
    EXPECT_TRUE(logContains(gui, "EP が足りません"));
}

TEST(GuiAppTest, EndTurnButtonPassesTurn)
{
    GuiApp gui = startTwoPlayer({CardType::Block, CardType::Shield, CardType::Restore});
    ASSERT_TRUE(gui.canEndTurn());
    clickRect(gui, app::endTurnButtonRect());
    EXPECT_EQ(gui.game()->currentColor(), Color::White);
    EXPECT_TRUE(logContains(gui, "黒：パス"));
    EXPECT_TRUE(logContains(gui, "白の番"));
}

TEST(GuiAppTest, BonusPlaceAppearsAsExtraSlot)
{
    GuiApp gui = startTwoPlayer({CardType::Reserve, CardType::Block, CardType::Shield});
    clickRect(gui, app::handCardRect(0));
    clickRect(gui, app::useButtonRect());
    EXPECT_NE(gui.hint().find("ターンを終えます"), std::string::npos);  // ほかに使えるカードがない
    clickRect(gui, app::endTurnButtonRect());  // 黒: ターン終了（自動を待たない）
    EXPECT_TRUE(logContains(gui, "黒：ターン終了"));
    clickRect(gui, app::endTurnButtonRect());  // 白: パス
    ASSERT_EQ(gui.game()->currentColor(), Color::Black);
    const auto& slots = gui.handSlots();
    ASSERT_EQ(slots.size(), 4u);
    EXPECT_TRUE(slots[3].bonus);
    EXPECT_EQ(slots[3].card, CardType::Place);
    EXPECT_EQ(slots[3].count, 2);
    EXPECT_TRUE(slots[3].playable);
}

// ─── 自動で進む手番 ──────────────────────────────────────────────────

TEST(GuiAppTest, HumanWithoutPlayableCardsPassesAutomatically)
{
    // どれも EP 4 で、開始時の EP 3 では使えない
    GuiApp gui = startTwoPlayer({CardType::Burst, CardType::Reversal, CardType::Burst});
    EXPECT_TRUE(gui.canEndTurn());  // 待たずにパスしてもよい
    EXPECT_NE(gui.hint().find("パス"), std::string::npos);
    gui.update(GuiApp::kStepDelayMs - 1);
    EXPECT_EQ(gui.game()->currentColor(), Color::Black);
    gui.update(1);
    EXPECT_EQ(gui.game()->currentColor(), Color::White);
    EXPECT_TRUE(logContains(gui, "使えるカードがないためパス"));
}

TEST(GuiAppTest, CpuPlaysItsTurnOverTime)
{
    GuiApp gui(1);
    GameOptions options;
    options.cpuColor = Color::Black;
    options.difficulty = Difficulty::Normal;
    gui.startGame(Game(Board::createInitial(), scriptedDeck({CardType::Place, CardType::Block, CardType::Restore},
                                                            kFiller, kFiller)),
                  options);
    EXPECT_FALSE(gui.isHumanTurn());
    EXPECT_FALSE(gui.canEndTurn());
    clickRect(gui, app::handCardRect(0));  // CPU の手番は操作できない
    EXPECT_FALSE(gui.selection().has_value());
    EXPECT_NE(gui.hint().find("考えています"), std::string::npos);

    for (int step = 0; step < 10 && gui.game()->currentColor() == Color::Black && !gui.game()->isOver(); ++step) {
        gui.update(GuiApp::kStepDelayMs);
    }
    EXPECT_EQ(gui.game()->currentColor(), Color::White);
    EXPECT_TRUE(logContains(gui, "黒（CPU・普通）：Place"));
    EXPECT_TRUE(gui.isHumanTurn());
}

// ─── 終局 ────────────────────────────────────────────────────────────

GuiApp startFinishedGame()
{
    // Place で最後の白石を挟むと白が全滅する
    const Board board = makeBoard({"BW......", "........", "........", "........", "........", "........",
                                   "........", "........"});
    GuiApp gui = startTwoPlayer({CardType::Place, CardType::Block, CardType::Restore}, kFiller, board);
    clickRect(gui, app::handCardRect(0));
    clickCell(gui, Position{0, 2});
    clickCell(gui, Position{7, 7});
    return gui;
}

TEST(GuiAppTest, GameOverShowsResult)
{
    GuiApp gui = startFinishedGame();
    ASSERT_TRUE(gui.game()->isOver());
    EXPECT_EQ(gui.resultText(), "黒 4 対 白 0 … 黒の勝ち！");
    EXPECT_EQ(gui.hint(), gui.resultText());
    EXPECT_TRUE(gui.handSlots().empty());
    EXPECT_FALSE(gui.canEndTurn());
    EXPECT_TRUE(logContains(gui, "ゲーム終了"));
}

TEST(GuiAppTest, RetryStartsNewGameWithNewSeed)
{
    GuiApp gui = startFinishedGame();
    const auto oldSeed = gui.seed();
    clickRect(gui, app::retryButtonRect());
    ASSERT_EQ(gui.screen(), Screen::Playing);
    EXPECT_FALSE(gui.game()->isOver());
    EXPECT_NE(gui.seed(), oldSeed);
    EXPECT_EQ(gui.game()->board().countStones(Color::Black), 4);
}

TEST(GuiAppTest, ToTitleReturnsToMenu)
{
    GuiApp gui = startFinishedGame();
    clickRect(gui, app::toTitleButtonRect());
    EXPECT_EQ(gui.screen(), Screen::Title);
    EXPECT_EQ(gui.game(), nullptr);
}

} // namespace
