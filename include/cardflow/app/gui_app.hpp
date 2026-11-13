#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "cardflow/ai.hpp"
#include "cardflow/app/options.hpp"
#include "cardflow/card.hpp"
#include "cardflow/game.hpp"
#include "cardflow/types.hpp"

// GUI の状態と操作（描画ライブラリに依存しない）。
// 描画側は状態を読むだけで、入力は click / cancel / togglePause / update で渡す。
namespace cardflow::app {

enum class Screen : std::uint8_t {
    Title,        // 続きから / 2 人対戦 / CPU 対戦 / 終了
    Difficulty,   // CPU の強さ
    ColorSelect,  // あなたの色
    Playing,      // 対戦中（終局後は結果を重ねて表示）
};

// 対戦画面に重ねて表示するパネル
enum class Overlay : std::uint8_t {
    None,
    Paused,         // 一時停止中（再開 / 中断して終わる / 投了）
    ConfirmResign,  // 投了の確認（はい / いいえ）
};

class GuiApp {
public:
    // CPU が 1 歩（カード・石・パス）ごとに待つ時間。自動パスも同じ
    static constexpr int kStepDelayMs = 600;

    // savePath: 中断データの保存先（空なら中断できない）
    explicit GuiApp(std::uint32_t seed, std::filesystem::path savePath = {});

    // 左クリック（画面座標）
    void click(int x, int y);
    // 右クリック: 重ねたパネルを閉じる、カードの選択を取り消す。メニューでは 1 つ戻る
    void cancel();
    // Esc キー: 対戦中の一時停止・再開
    void togglePause();
    // 時間を進める（CPU の手番・自動パス）。一時停止中は進まない
    void update(int elapsedMs);

    // 指定の盤面・山札で対戦を始める（テスト用）
    void startGame(Game game, const GameOptions& options);

    [[nodiscard]] Screen screen() const noexcept { return m_screen; }
    [[nodiscard]] bool quitRequested() const noexcept { return m_quit; }
    [[nodiscard]] const GameOptions& options() const noexcept { return m_options; }
    [[nodiscard]] std::uint32_t seed() const noexcept { return m_seed; }

    // ─── メニュー画面 ────────────────────────────────────────────────
    // いまのメニュー画面のボタン（上から順）
    [[nodiscard]] std::vector<std::string> menuLabels() const;
    [[nodiscard]] std::string menuHeading() const;
    [[nodiscard]] bool showsBackButton() const noexcept;
    [[nodiscard]] bool hasSave() const noexcept { return m_hasSave; }
    // タイトルに出すお知らせ（中断した・読み込めなかった など）
    [[nodiscard]] const std::string& notice() const noexcept { return m_notice; }

    // ─── 対戦中のみ有効 ──────────────────────────────────────────────
    [[nodiscard]] const Game* game() const noexcept { return m_game ? &*m_game : nullptr; }
    [[nodiscard]] bool isHumanTurn() const noexcept;
    [[nodiscard]] bool isCpu(Color color) const noexcept;
    [[nodiscard]] std::string playerLabel(Color color) const;  // 例: "黒（あなた）"

    // メニューバー（対戦中で、終局前のみ）
    [[nodiscard]] bool showsMenuBar() const noexcept;
    [[nodiscard]] static std::vector<std::string> menuBarLabels();
    [[nodiscard]] Overlay overlay() const noexcept { return m_overlay; }
    [[nodiscard]] std::string overlayTitle() const;
    [[nodiscard]] std::vector<std::string> overlayLabels() const;

    // 画面下に出す手札の持ち主（CPU 対戦ではあなた、2 人対戦では手番の人）
    [[nodiscard]] Color handOwner() const noexcept;
    [[nodiscard]] const std::vector<CardType>& shownHand() const;
    // その手札をいま使えるか
    [[nodiscard]] bool isCardUsable(int slot) const noexcept;
    [[nodiscard]] const std::optional<int>& selectedSlot() const noexcept { return m_selected; }
    // 選んだカードの対象として光らせるマス
    [[nodiscard]] const std::vector<Position>& targetCells() const noexcept { return m_targets; }
    // 石を置けるマス（あなたの手番で、対象を取るカードを選んでいないとき）
    [[nodiscard]] const std::vector<Position>& placeHints() const noexcept { return m_placeHints; }
    [[nodiscard]] bool showsPassButton() const noexcept;

    [[nodiscard]] std::string hint() const;                                          // いまやること
    [[nodiscard]] const std::string& message() const noexcept { return m_message; }  // 直前の出来事
    [[nodiscard]] std::string resultText() const;                                    // 終局時の結果

private:
    enum class TitleItem : std::uint8_t { Resume, TwoPlayer, VersusCpu, Quit };

    [[nodiscard]] std::vector<TitleItem> titleItems() const;
    void clickMenu(int x, int y);
    void clickPlaying(int x, int y);
    void clickOverlay(int index);
    void clickCell(Position pos);
    void startNewGame();
    void resumeSaved();
    void suspend();
    void resign();
    void goToTitle(std::string notice);
    void selectSlot(int slot);
    void useCard(CardType card, std::optional<Position> target);
    void placeStone(Position pos);
    void passTurn(bool automatic);
    void stepAutomatic();
    void clearSelection();
    void refresh();
    [[nodiscard]] std::optional<CardType> selectedCard() const;

    Screen m_screen = Screen::Title;
    bool m_quit = false;
    GameOptions m_options;
    std::uint32_t m_seed;      // いまの対戦の seed
    std::uint32_t m_nextSeed;  // 次の対戦の seed
    std::mt19937 m_seedRng;
    std::filesystem::path m_savePath;
    bool m_hasSave = false;
    std::string m_notice;

    std::optional<Game> m_game;
    std::optional<ai::CpuPlayer> m_cpu;
    Overlay m_overlay = Overlay::None;
    std::optional<int> m_selected;
    std::vector<Position> m_targets;
    std::vector<Position> m_placeHints;
    std::vector<bool> m_usable;
    bool m_mustPass = false;
    bool m_autoPending = false;  // CPU の手番、または置けずカードもないため自動で進める
    std::string m_message;
    int m_waitMs = 0;
};

} // namespace cardflow::app
