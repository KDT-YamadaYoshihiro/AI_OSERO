#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "board_builder.hpp"
#include "cardflow/app/gui_app.hpp"
#include "cardflow/app/layout.hpp"
#include "deck_builder.hpp"
#include "game_builder.hpp"

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
using cardflow::test::cardsReady;
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
    gui.startGame(cardsReady(Game(board, scriptedDeck(black, white))), GameOptions{});
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

TEST(GuiAppTest, CardsAreLockedAtFirst)
{
    GuiApp gui(1);
    gui.startGame(Game(Board::createInitial(), scriptedDeck({CardType::Flip, CardType::Wall}, {})), GameOptions{});
    EXPECT_EQ(gui.lockedTurnIndex(), 1);
    EXPECT_FALSE(gui.isCardUsable(0));
    EXPECT_TRUE(has(gui.hint(), "（カードは 4 回目の番から）"));
    clickRect(gui, app::handCardRect(0));
    EXPECT_FALSE(gui.selectedSlot().has_value());
    EXPECT_EQ(gui.message(), "カードは自分の番の 4 回目から使えます（いまは 1 回目）");
}

TEST(GuiAppTest, LockNoteDuringCpuTurnShowsNextTurn)
{
    GuiApp gui(1);
    GameOptions options;
    options.cpuColor = Color::Black;
    gui.startGame(Game(Board::createInitial(), scriptedDeck({}, {})), options);
    EXPECT_EQ(gui.lockedTurnIndex(), 1);  // あなた（白）の次の番は 1 回目
    EXPECT_EQ(gui.cardLockNote(), "カードは自分の番の 4 回目から使えます（次が 1 回目）");
}

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

// ─── メニューバー・一時停止・投了 ────────────────────────────────────

GuiApp startCpuGame(Color cpuColor, const std::filesystem::path& savePath = {})
{
    GuiApp gui(1, savePath);
    GameOptions options;
    options.cpuColor = cpuColor;
    gui.startGame(Game(Board::createInitial(), scriptedDeck({}, {})), options);
    return gui;
}

TEST(GuiAppTest, MenuBarShownOnlyWhilePlaying)
{
    GuiApp title(1);
    EXPECT_FALSE(title.showsMenuBar());
    GuiApp gui = startTwoPlayer();
    EXPECT_TRUE(gui.showsMenuBar());
    EXPECT_EQ(GuiApp::menuBarLabels().size(), static_cast<std::size_t>(app::layout::kMenuBarButtons));
    gui = startFinishedGame();
    EXPECT_FALSE(gui.showsMenuBar());
}

TEST(GuiAppTest, PauseStopsCpuAndBlocksBoard)
{
    GuiApp gui = startCpuGame(Color::Black);
    clickRect(gui, app::menuBarButtonRect(0));  // 一時停止
    ASSERT_EQ(gui.overlay(), app::Overlay::Paused);
    EXPECT_EQ(gui.overlayTitle(), "一時停止中");
    gui.update(GuiApp::kStepDelayMs * 5);
    EXPECT_EQ(gui.game()->currentColor(), Color::Black);  // CPU は止まっている

    clickRect(gui, app::overlayButtonRect(0));  // 再開
    EXPECT_EQ(gui.overlay(), app::Overlay::None);
    gui.update(GuiApp::kStepDelayMs);
    EXPECT_EQ(gui.game()->currentColor(), Color::White);
}

TEST(GuiAppTest, EscapeTogglesPauseAndRightClickResumes)
{
    GuiApp gui = startTwoPlayer();
    gui.togglePause();
    EXPECT_EQ(gui.overlay(), app::Overlay::Paused);
    clickCell(gui, Position{2, 3});  // 一時停止中は置けない
    EXPECT_EQ(gui.game()->currentColor(), Color::Black);
    EXPECT_FALSE(gui.showsPassButton());
    gui.togglePause();
    EXPECT_EQ(gui.overlay(), app::Overlay::None);
    gui.togglePause();
    gui.cancel();
    EXPECT_EQ(gui.overlay(), app::Overlay::None);
}

TEST(GuiAppTest, ResignAsksForConfirmation)
{
    GuiApp gui = startCpuGame(Color::White);
    clickRect(gui, app::menuBarButtonRect(2));  // 投了
    ASSERT_EQ(gui.overlay(), app::Overlay::ConfirmResign);
    clickRect(gui, app::overlayButtonRect(1));  // いいえ
    EXPECT_EQ(gui.overlay(), app::Overlay::None);
    EXPECT_FALSE(gui.game()->isOver());

    clickRect(gui, app::menuBarButtonRect(2));
    clickRect(gui, app::overlayButtonRect(0));  // はい
    ASSERT_TRUE(gui.game()->isOver());
    EXPECT_EQ(gui.game()->result()->resignedBy, Color::Black);
    EXPECT_EQ(gui.resultText(), "黒（あなた）が投了 … 白（CPU・普通）の勝ち！");
    EXPECT_EQ(gui.message(), "黒（あなた）が投了しました");
}

TEST(GuiAppTest, ResignDuringCpuTurnIsYours)
{
    GuiApp gui = startCpuGame(Color::Black);  // CPU が先手
    clickRect(gui, app::menuBarButtonRect(0));
    clickRect(gui, app::overlayButtonRect(2));  // 一時停止 → 投了
    ASSERT_EQ(gui.overlay(), app::Overlay::ConfirmResign);
    clickRect(gui, app::overlayButtonRect(0));
    EXPECT_EQ(gui.game()->result()->resignedBy, Color::White);
}

// ─── 中断・再開 ──────────────────────────────────────────────────────

class GuiAppSaveTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_path = std::filesystem::temp_directory_path() / "cardflow_gui_app_test" / "save.txt";
        std::filesystem::remove_all(m_path.parent_path());
    }
    void TearDown() override { std::filesystem::remove_all(m_path.parent_path()); }

    std::filesystem::path m_path;
};

TEST_F(GuiAppSaveTest, SuspendSavesAndResumeContinues)
{
    GuiApp gui = startCpuGame(Color::White, m_path);
    clickCell(gui, Position{2, 3});  // 黒 d3
    ASSERT_EQ(gui.game()->currentColor(), Color::White);
    const auto stonesBefore = gui.game()->board().countStones(Color::Black);

    clickRect(gui, app::menuBarButtonRect(1));  // 中断して終わる
    ASSERT_EQ(gui.screen(), Screen::Title);
    EXPECT_TRUE(std::filesystem::exists(m_path));
    EXPECT_TRUE(gui.hasSave());
    EXPECT_TRUE(has(gui.notice(), "中断しました"));
    ASSERT_EQ(gui.menuLabels().front(), "続きから");

    // アプリを起動し直しても、タイトルに「続きから」が出る
    GuiApp next(99, m_path);
    ASSERT_TRUE(next.hasSave());
    clickRect(next, app::menuButtonRect(0));
    ASSERT_EQ(next.screen(), Screen::Playing);
    EXPECT_EQ(next.message(), "中断した対戦を再開しました");
    EXPECT_EQ(next.options().cpuColor, Color::White);
    EXPECT_EQ(next.game()->currentColor(), Color::White);
    EXPECT_EQ(next.game()->board().countStones(Color::Black), stonesBefore);
    EXPECT_EQ(next.seed(), 1u);
    EXPECT_FALSE(next.hasSave());  // 再開したデータは消える
    EXPECT_FALSE(std::filesystem::exists(m_path));

    next.update(GuiApp::kStepDelayMs);  // CPU の番から続く
    EXPECT_EQ(next.game()->currentColor(), Color::Black);
}

TEST_F(GuiAppSaveTest, PauseMenuCanSuspend)
{
    GuiApp gui = startCpuGame(Color::Black, m_path);
    gui.togglePause();
    clickRect(gui, app::overlayButtonRect(1));  // 中断して終わる
    EXPECT_EQ(gui.screen(), Screen::Title);
    EXPECT_TRUE(std::filesystem::exists(m_path));
}

TEST_F(GuiAppSaveTest, BrokenSaveShowsNotice)
{
    std::filesystem::create_directories(m_path.parent_path());
    std::ofstream(m_path) << "broken";
    GuiApp gui(1, m_path);
    ASSERT_TRUE(gui.hasSave());
    clickRect(gui, app::menuButtonRect(0));  // 続きから
    EXPECT_EQ(gui.screen(), Screen::Title);
    EXPECT_EQ(gui.notice(), "中断データを読み込めませんでした");
    EXPECT_FALSE(gui.hasSave());
    EXPECT_EQ(gui.menuLabels().front(), "2人対戦");
}

TEST(GuiAppTest, SuspendWithoutSavePathFails)
{
    GuiApp gui = startTwoPlayer();
    clickRect(gui, app::menuBarButtonRect(1));
    EXPECT_EQ(gui.screen(), Screen::Playing);
    EXPECT_EQ(gui.message(), "中断データを保存できませんでした");
}

} // namespace
