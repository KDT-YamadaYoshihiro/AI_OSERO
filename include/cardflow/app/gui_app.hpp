#pragma once

#include <cstdint>
#include <deque>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "cardflow/ai.hpp"
#include "cardflow/card.hpp"
#include "cardflow/game.hpp"
#include "cardflow/types.hpp"

// GUI の状態と操作（描画ライブラリに依存しない）。
// 描画側は状態を読むだけで、入力は click / cancel / update で渡す。
namespace cardflow::app {

struct GameOptions {
    std::optional<Color> cpuColor;  // std::nullopt なら 2 人対戦
    ai::Difficulty difficulty = ai::Difficulty::Normal;
};

enum class Screen : std::uint8_t {
    Title,        // 2 人対戦 / CPU 対戦 / 終了
    Difficulty,   // CPU の強さ
    ColorSelect,  // あなたの色
    Playing,      // 対戦中（終局後は結果を重ねて表示）
};

// 手札の 1 枠（通常の手札、または Reserve による追加 Place）
struct HandSlot {
    CardType card = CardType::Place;
    bool bonus = false;     // 追加 Place の枠
    int count = 1;          // 追加 Place の枚数
    bool playable = false;  // いま使えるか
};

// 使おうとしているカードと、選んだ対象
struct Selection {
    int slot = 0;
    CardType card = CardType::Place;
    std::vector<Position> targets;
};

class GuiApp {
public:
    // CPU が 1 手ごとに待つ時間（ミリ秒）。自動パスも同じ間隔で行う
    static constexpr int kStepDelayMs = 700;
    static constexpr std::size_t kMaxLogLines = 100;

    explicit GuiApp(std::uint32_t seed);

    // 左クリック（画面座標）
    void click(int x, int y);
    // 右クリック: 選択の取り消し、メニューでは 1 つ戻る
    void cancel();
    // 時間を進める（CPU の手番・自動パス）
    void update(int elapsedMs);

    // 指定の盤面・山札で対戦を始める（テスト用）
    void startGame(Game game, const GameOptions& options);

    [[nodiscard]] Screen screen() const noexcept { return m_screen; }
    [[nodiscard]] bool quitRequested() const noexcept { return m_quit; }
    [[nodiscard]] const GameOptions& options() const noexcept { return m_options; }
    [[nodiscard]] std::uint32_t seed() const noexcept { return m_seed; }

    // 対戦中のみ有効
    [[nodiscard]] const Game* game() const noexcept { return m_game ? &*m_game : nullptr; }
    [[nodiscard]] bool isHumanTurn() const noexcept;
    [[nodiscard]] bool isCpu(Color color) const noexcept;
    [[nodiscard]] std::string playerLabel(Color color) const;  // 例: "黒（あなた）"
    [[nodiscard]] const std::vector<HandSlot>& handSlots() const noexcept { return m_slots; }
    [[nodiscard]] const std::optional<Selection>& selection() const noexcept { return m_selection; }
    [[nodiscard]] const std::vector<Position>& candidates() const noexcept { return m_candidates; }
    [[nodiscard]] const std::vector<Position>& lastActionCells() const noexcept { return m_lastAction; }
    [[nodiscard]] bool canUseSelected() const noexcept;  // 「使う」ボタンを押せるか
    [[nodiscard]] bool canEndTurn() const noexcept;
    [[nodiscard]] std::string hint() const;
    [[nodiscard]] std::string resultText() const;  // 終局時の結果
    [[nodiscard]] const std::deque<std::string>& log() const noexcept { return m_log; }

private:
    void clickMenu(int x, int y);
    void clickPlaying(int x, int y);
    void startNewGame();
    void selectSlot(int slot);
    void chooseTarget(Position pos);
    void play(CardType card, const std::vector<Position>& targets);
    void finishTurn();
    void stepAutomatic();
    void onTurnStarted();
    void clearSelection();
    void refreshCandidates();
    void refreshTurnState();
    void addLog(std::string line);

    Screen m_screen = Screen::Title;
    bool m_quit = false;
    GameOptions m_options;
    std::uint32_t m_seed;      // いまの対戦の seed
    std::uint32_t m_nextSeed;  // 次の対戦の seed
    std::mt19937 m_seedRng;

    std::optional<Game> m_game;
    std::optional<ai::CpuPlayer> m_cpu;
    std::vector<HandSlot> m_slots;
    bool m_autoPending = false;  // CPU の手番、または使えるカードがなく自動で進める
    std::optional<Selection> m_selection;
    std::vector<Position> m_candidates;
    std::vector<Position> m_lastAction;
    std::deque<std::string> m_log;
    int m_waitMs = 0;
};

} // namespace cardflow::app
