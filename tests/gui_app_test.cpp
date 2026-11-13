#include <gtest/gtest.h>

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

using Cards = std::vector<CardType>;

void clickRect(GuiApp& gui, const Rect& rect)
{
    gui.click(rect.centerX(), rect.centerY());
}

void clickCell(GuiApp& gui, Position pos)
{
    clickRect(gui, app::cellRect(pos));
}

bool has(std::string_view text, std::string_view part)
{
    return text.find(part) != std::string_view::npos;
}

// 指定の手札で 2 人対戦を始める
GuiApp startTwoPlayer(const Cards& black = {}, const Cards& white = {}, const Board& board = Board::createInitial())
{
    GuiApp gui(1);
    gui.startGame(Game(board, scriptedDeck(black, white)), GameOptions{});
    return gui;
}

// どちらも置けない盤面
const Board kStuck = makeBoard({"W.......", "........", "........", "........", "........", "........",
                                "........", ".......B"});

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
    EXPECT_EQ(gui.handOwner(), Color::White);  // CPU の番でも、下にはあなたの手札
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
    clickRect(gui, app::menuButtonRect(2));
    EXPECT_TRUE(gui.quitRequested());
}

// ─── 石を置く ────────────────────────────────────────────────────────

TEST(GuiAppTest, ClickPlaceableCellPlacesStone)
{
    GuiApp gui = startTwoPlayer();
    EXPECT_EQ(gui.placeHints().size(), 4u);
    EXPECT_TRUE(has(gui.hint(), "光っているマスに石を置いてください"));
    clickCell(gui, Position{0, 0});  // 置けないマスは無視
    EXPECT_EQ(gui.game()->currentColor(), Color::Black);

    clickCell(gui, Position{2, 3});  // d3
    EXPECT_EQ(gui.game()->currentColor(), Color::White);
    EXPECT_EQ(gui.message(), "黒：d3 に置きました（1 個裏返し）");
    EXPECT_EQ(gui.handOwner(), Color::White);  // 2 人対戦では手番の人の手札
    EXPECT_TRUE(has(gui.hint(), "白の番"));
}

// ─── カード ──────────────────────────────────────────────────────────

TEST(GuiAppTest, TargetCardHighlightsAndUsesOnClick)
{
    GuiApp gui = startTwoPlayer({CardType::Flip, CardType::Wall});
    EXPECT_TRUE(gui.isCardUsable(0));
    EXPECT_TRUE(has(gui.hint(), "カードも使えます"));
    clickRect(gui, app::handCardRect(0));
    EXPECT_EQ(gui.selectedSlot(), 0);
    EXPECT_EQ(gui.targetCells(), (std::vector<Position>{{3, 3}, {4, 4}}));
    EXPECT_TRUE(has(gui.hint(), "奪う相手の石"));

    clickCell(gui, Position{3, 3});
    EXPECT_FALSE(gui.selectedSlot().has_value());
    EXPECT_EQ(gui.message(), "黒：「反転」を d4 に使いました");
    EXPECT_EQ(gui.shownHand(), (Cards{CardType::Wall}));
    EXPECT_FALSE(gui.isCardUsable(0));  // 1 ターン 1 枚
    clickRect(gui, app::handCardRect(0));
    EXPECT_EQ(gui.message(), "カードは 1 ターンに 1 枚までです");
    EXPECT_EQ(gui.game()->currentColor(), Color::Black);  // まだ置いていない
}

TEST(GuiAppTest, NoTargetCardUsesOnSecondClick)
{
    GuiApp gui = startTwoPlayer({CardType::Double});
    clickRect(gui, app::handCardRect(0));
    EXPECT_EQ(gui.selectedSlot(), 0);
    EXPECT_TRUE(gui.targetCells().empty());
    EXPECT_TRUE(has(gui.hint(), "もう一度クリックすると「連打」を使います"));
    EXPECT_EQ(gui.game()->placementsLeft(), 1);
    clickRect(gui, app::handCardRect(0));
    EXPECT_EQ(gui.game()->placementsLeft(), 2);
    EXPECT_TRUE(gui.shownHand().empty());

    clickCell(gui, Position{2, 3});
    EXPECT_EQ(gui.game()->currentColor(), Color::Black);
    EXPECT_TRUE(has(gui.hint(), "もう 1 つ置いてください"));
}

TEST(GuiAppTest, OtherClickOrRightClickCancelsSelection)
{
    GuiApp gui = startTwoPlayer({CardType::Flip, CardType::Guard});
    clickRect(gui, app::handCardRect(0));
    clickCell(gui, Position{0, 0});  // 対象ではないマス
    EXPECT_FALSE(gui.selectedSlot().has_value());
    EXPECT_EQ(gui.shownHand().size(), 2u);

    clickRect(gui, app::handCardRect(1));
    EXPECT_EQ(gui.targetCells().size(), 2u);  // 守り: 自分の石 2 つ
    gui.cancel();
    EXPECT_FALSE(gui.selectedSlot().has_value());
    EXPECT_TRUE(gui.targetCells().empty());
}

// ─── パス ────────────────────────────────────────────────────────────

TEST(GuiAppTest, PassButtonWhenStuckWithCards)
{
    GuiApp gui = startTwoPlayer({CardType::Free}, {}, kStuck);
    EXPECT_TRUE(gui.showsPassButton());
    EXPECT_TRUE(has(gui.hint(), "カードを使うか、パスしてください"));
    gui.update(5000);  // カードがあるので自動ではパスしない
    EXPECT_EQ(gui.game()->currentColor(), Color::Black);
    clickRect(gui, app::passButtonRect());
    EXPECT_EQ(gui.game()->currentColor(), Color::White);
    EXPECT_EQ(gui.message(), "黒：パス");
}

TEST(GuiAppTest, AutoPassWhenStuckWithoutCards)
{
    GuiApp gui = startTwoPlayer({}, {}, kStuck);
    EXPECT_TRUE(has(gui.hint(), "パスします"));
    gui.update(GuiApp::kStepDelayMs - 1);
    EXPECT_EQ(gui.game()->currentColor(), Color::Black);
    gui.update(1);
    EXPECT_EQ(gui.game()->currentColor(), Color::White);
    EXPECT_EQ(gui.message(), "黒：置ける場所がないのでパス");
    gui.update(GuiApp::kStepDelayMs);  // 白も置けない → 終局
    EXPECT_TRUE(gui.game()->isOver());
}

// ─── CPU ─────────────────────────────────────────────────────────────

TEST(GuiAppTest, CpuPlaysItsTurnOverTime)
{
    GuiApp gui(1);
    GameOptions options;
    options.cpuColor = Color::Black;
    options.difficulty = Difficulty::Normal;
    gui.startGame(Game(Board::createInitial(), scriptedDeck({}, {})), options);
    EXPECT_FALSE(gui.isHumanTurn());
    EXPECT_TRUE(has(gui.hint(), "考えています"));
    EXPECT_TRUE(gui.placeHints().empty());
    clickCell(gui, Position{2, 3});  // CPU の番は操作できない
    EXPECT_EQ(gui.game()->currentColor(), Color::Black);

    gui.update(GuiApp::kStepDelayMs);
    EXPECT_EQ(gui.game()->currentColor(), Color::White);
    EXPECT_TRUE(has(gui.message(), "黒（CPU・普通）："));
    EXPECT_TRUE(gui.isHumanTurn());
    EXPECT_FALSE(gui.placeHints().empty());
}

// ─── 終局 ────────────────────────────────────────────────────────────

GuiApp startFinishedGame()
{
    const Board board = makeBoard({"BW......", "........", "........", "........", "........", "........",
                                   "........", "........"});
    GuiApp gui = startTwoPlayer({}, {}, board);
    clickCell(gui, Position{0, 2});  // 白が全滅
    return gui;
}

TEST(GuiAppTest, GameOverShowsResult)
{
    GuiApp gui = startFinishedGame();
    ASSERT_TRUE(gui.game()->isOver());
    EXPECT_EQ(gui.resultText(), "黒 3 対 白 0 … 黒の勝ち！");
    EXPECT_EQ(gui.hint(), gui.resultText());
    EXPECT_FALSE(gui.showsPassButton());
}

TEST(GuiAppTest, RetryStartsNewGameWithNewSeed)
{
    GuiApp gui = startFinishedGame();
    const auto oldSeed = gui.seed();
    clickRect(gui, app::retryButtonRect());
    ASSERT_EQ(gui.screen(), Screen::Playing);
    EXPECT_FALSE(gui.game()->isOver());
    EXPECT_NE(gui.seed(), oldSeed);
    EXPECT_EQ(gui.game()->board().countStones(Color::Black), 2);
}

TEST(GuiAppTest, ToTitleReturnsToMenu)
{
    GuiApp gui = startFinishedGame();
    clickRect(gui, app::toTitleButtonRect());
    EXPECT_EQ(gui.screen(), Screen::Title);
    EXPECT_EQ(gui.game(), nullptr);
}

} // namespace
