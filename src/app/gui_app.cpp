#include "cardflow/app/gui_app.hpp"

#include <algorithm>
#include <utility>

#include "cardflow/app/layout.hpp"
#include "cardflow/app/text.hpp"
#include "cardflow/board.hpp"
#include "cardflow/card_effects.hpp"

namespace cardflow::app {

namespace {

constexpr std::uint32_t kCpuSeedSalt = 0x9E3779B9u;
constexpr int kCells = Board::kSize * Board::kSize;

Position cellAtIndex(int index)
{
    return Position{index / Board::kSize, index % Board::kSize};
}

bool containsPosition(const std::vector<Position>& list, Position pos)
{
    return std::ranges::find(list, pos) != list.end();
}

// メニュー画面でクリックされたボタン（なければ -1）
int menuIndexAt(int x, int y, int buttons)
{
    for (int i = 0; i < buttons; ++i) {
        if (menuButtonRect(i).contains(x, y)) {
            return i;
        }
    }
    return -1;
}

} // namespace

GuiApp::GuiApp(std::uint32_t seed) : m_seed(seed), m_nextSeed(seed), m_seedRng(seed) {}

// ─── 状態の参照 ──────────────────────────────────────────────────────

bool GuiApp::isCpu(Color color) const noexcept
{
    return m_options.cpuColor.has_value() && *m_options.cpuColor == color;
}

bool GuiApp::isHumanTurn() const noexcept
{
    return m_game.has_value() && !m_game->isOver() && !isCpu(m_game->currentColor());
}

std::string GuiApp::playerLabel(Color color) const
{
    std::string label{colorName(color)};
    if (m_options.cpuColor.has_value()) {
        if (isCpu(color)) {
            label += "（CPU・";
            label += ai::difficultyName(m_options.difficulty);
            label += "）";
        } else {
            label += "（あなた）";
        }
    }
    return label;
}

bool GuiApp::canUseSelected() const noexcept
{
    return isHumanTurn() && m_selection.has_value() && targetCount(m_selection->card) == 0;
}

bool GuiApp::canEndTurn() const noexcept
{
    // 使えるカードがなく自動で進む場合も、待たずに終えられる
    return m_screen == Screen::Playing && isHumanTurn();
}

std::string GuiApp::hint() const
{
    if (!m_game.has_value()) {
        return "";
    }
    if (m_game->isOver()) {
        return resultText();
    }
    const Color current = m_game->currentColor();
    if (isCpu(current)) {
        return playerLabel(current) + "が考えています…";
    }
    if (m_autoPending) {
        return m_game->hasActedThisTurn() ? "これ以上使えるカードがないため、ターンを終えます"
                                          : "使えるカードがないため、パスします";
    }
    if (m_selection.has_value()) {
        return std::string{targetPrompt(m_selection->card, static_cast<int>(m_selection->targets.size()))};
    }
    return "使うカードを選んでください（右クリックで取り消し）";
}

std::string GuiApp::resultText() const
{
    if (!m_game.has_value() || !m_game->result().has_value()) {
        return "";
    }
    const auto& result = *m_game->result();
    std::string text = "黒 " + std::to_string(result.blackStones) + " 対 白 " + std::to_string(result.whiteStones) + " … ";
    if (result.winner.has_value()) {
        text += playerLabel(*result.winner) + "の勝ち！";
    } else {
        text += "引き分け";
    }
    return text;
}

// ─── 入力 ────────────────────────────────────────────────────────────

void GuiApp::click(int x, int y)
{
    if (m_screen == Screen::Playing) {
        clickPlaying(x, y);
    } else {
        clickMenu(x, y);
    }
}

void GuiApp::cancel()
{
    switch (m_screen) {
    case Screen::Title:
        break;
    case Screen::Difficulty:
        m_screen = Screen::Title;
        break;
    case Screen::ColorSelect:
        m_screen = Screen::Difficulty;
        break;
    case Screen::Playing:
        clearSelection();
        break;
    }
}

void GuiApp::update(int elapsedMs)
{
    if (m_screen != Screen::Playing || !m_game.has_value() || m_game->isOver() || !m_autoPending) {
        m_waitMs = 0;
        return;
    }
    m_waitMs += elapsedMs;
    if (m_waitMs >= kStepDelayMs) {
        m_waitMs = 0;
        stepAutomatic();
    }
}

void GuiApp::clickMenu(int x, int y)
{
    switch (m_screen) {
    case Screen::Title:
        switch (menuIndexAt(x, y, 3)) {
        case 0:
            m_options = GameOptions{};
            startNewGame();
            break;
        case 1:
            m_screen = Screen::Difficulty;
            break;
        case 2:
            m_quit = true;
            break;
        default:
            break;
        }
        break;
    case Screen::Difficulty:
        if (const int index = menuIndexAt(x, y, 3); index >= 0) {
            m_options.difficulty = static_cast<ai::Difficulty>(index);
            m_screen = Screen::ColorSelect;
        } else if (backButtonRect().contains(x, y)) {
            m_screen = Screen::Title;
        }
        break;
    case Screen::ColorSelect:
        if (const int index = menuIndexAt(x, y, 2); index >= 0) {
            // あなたが黒（先手）なら CPU は白
            m_options.cpuColor = index == 0 ? Color::White : Color::Black;
            startNewGame();
        } else if (backButtonRect().contains(x, y)) {
            m_screen = Screen::Difficulty;
        }
        break;
    case Screen::Playing:
        break;
    }
}

void GuiApp::clickPlaying(int x, int y)
{
    if (!m_game.has_value()) {
        return;
    }
    if (m_game->isOver()) {
        if (retryButtonRect().contains(x, y)) {
            startNewGame();
        } else if (toTitleButtonRect().contains(x, y)) {
            m_game.reset();
            m_screen = Screen::Title;
        }
        return;
    }
    if (!isHumanTurn()) {
        return;
    }

    for (int slot = 0; slot < static_cast<int>(m_slots.size()); ++slot) {
        if (handCardRect(slot).contains(x, y)) {
            selectSlot(slot);
            return;
        }
    }
    if (useButtonRect().contains(x, y)) {
        if (canUseSelected()) {
            play(m_selection->card, {});
        }
        return;
    }
    if (cancelButtonRect().contains(x, y)) {
        clearSelection();
        return;
    }
    if (endTurnButtonRect().contains(x, y)) {
        const Color current = m_game->currentColor();
        addLog(playerLabel(current) + (m_game->hasActedThisTurn() ? "：ターン終了" : "：パス"));
        m_game->endTurn();
        finishTurn();
        return;
    }
    if (const auto pos = cellAt(x, y); pos.has_value() && m_selection.has_value()) {
        chooseTarget(*pos);
    }
}

// ─── 対戦の進行 ──────────────────────────────────────────────────────

void GuiApp::startGame(Game game, const GameOptions& options)
{
    m_options = options;
    m_nextSeed = m_seedRng();  // 「もう一度」では別の山札にする
    m_game.emplace(std::move(game));
    m_cpu.reset();
    if (m_options.cpuColor.has_value()) {
        m_cpu.emplace(m_options.difficulty, m_seed ^ kCpuSeedSalt);
    }
    m_screen = Screen::Playing;
    m_log.clear();
    m_lastAction.clear();
    addLog("対戦開始（seed " + std::to_string(m_seed) + "）");
    onTurnStarted();
}

void GuiApp::startNewGame()
{
    m_seed = m_nextSeed;
    startGame(Game(m_seed), m_options);
}

void GuiApp::selectSlot(int slot)
{
    if (m_selection.has_value() && m_selection->slot == slot) {
        clearSelection();
        return;
    }
    const HandSlot& target = m_slots[static_cast<std::size_t>(slot)];
    if (!target.playable) {
        clearSelection();
        PlayStatus status = PlayStatus::CardLimitReached;
        if (target.bonus || !m_game->hasPlayedCardThisTurn()) {
            // 対象なしで試すと、手札 → EP → 対象 の順に理由がわかる
            Game trial = *m_game;
            status = trial.play(target.card, {}).status;
            if (status == PlayStatus::Ok) {
                status = PlayStatus::InvalidTarget;
            }
        }
        addLog(std::string{cardInfo(target.card).name} + "：" + std::string{statusMessage(status)});
        return;
    }
    m_selection = Selection{slot, target.card, {}};
    refreshCandidates();
}

void GuiApp::chooseTarget(Position pos)
{
    if (!containsPosition(m_candidates, pos)) {
        return;
    }
    m_selection->targets.push_back(pos);
    if (static_cast<int>(m_selection->targets.size()) == targetCount(m_selection->card)) {
        const Selection chosen = *m_selection;
        play(chosen.card, chosen.targets);
        return;
    }
    refreshCandidates();
}

void GuiApp::play(CardType card, const std::vector<Position>& targets)
{
    const Color actor = m_game->currentColor();
    clearSelection();
    const auto result = m_game->play(card, targets);
    if (!result.ok()) {
        addLog(std::string{cardInfo(card).name} + "：" + std::string{statusMessage(result.status)});
        return;
    }
    std::string line = playerLabel(actor) + "：" + std::string{cardInfo(card).name};
    for (const auto& pos : targets) {
        line += ' ';
        line += formatPosition(pos);
    }
    line += describeResult(result);
    addLog(std::move(line));
    m_lastAction = targets;
    if (m_game->isOver()) {
        addLog("ゲーム終了：" + resultText());
    }
    refreshTurnState();
}

// endTurn の後始末（終局の記録、または次の手番の開始）
void GuiApp::finishTurn()
{
    if (m_game->isOver()) {
        clearSelection();
        addLog("ゲーム終了：" + resultText());
        refreshTurnState();
        return;
    }
    onTurnStarted();
}

void GuiApp::stepAutomatic()
{
    const Color current = m_game->currentColor();
    if (isCpu(current) && m_cpu.has_value()) {
        if (const auto action = m_cpu->chooseAction(*m_game)) {
            play(action->card, action->targets);
            return;
        }
        addLog(playerLabel(current) + (m_game->hasActedThisTurn() ? "：ターン終了" : "：パス"));
    } else {
        addLog(playerLabel(current) + (m_game->hasActedThisTurn() ? "：使えるカードがないためターン終了"
                                                                  : "：使えるカードがないためパス"));
    }
    m_game->endTurn();
    finishTurn();
}

void GuiApp::onTurnStarted()
{
    clearSelection();
    m_waitMs = 0;
    addLog("― ラウンド " + std::to_string(m_game->round()) + "：" + playerLabel(m_game->currentColor()) + "の番 ―");
    refreshTurnState();
}

void GuiApp::clearSelection()
{
    m_selection.reset();
    m_candidates.clear();
}

// 次に選べるマス: そのマスを加えて、対象をすべて正しく埋められるもの
void GuiApp::refreshCandidates()
{
    m_candidates.clear();
    if (!m_selection.has_value() || !m_game.has_value()) {
        return;
    }
    const Board& board = m_game->board();
    const Player& actor = m_game->currentPlayer();
    const CardType card = m_selection->card;
    const int remaining = targetCount(card) - static_cast<int>(m_selection->targets.size());

    std::vector<Position> trial = m_selection->targets;
    for (int a = 0; a < kCells; ++a) {
        const Position pos = cellAtIndex(a);
        if (containsPosition(m_selection->targets, pos)) {
            continue;
        }
        trial.push_back(pos);
        bool ok = false;
        if (remaining == 1) {
            ok = canApplyEffect(card, board, actor, trial);
        } else if (remaining == 2) {
            for (int b = 0; b < kCells && !ok; ++b) {
                trial.push_back(cellAtIndex(b));
                ok = b != a && canApplyEffect(card, board, actor, trial);
                trial.pop_back();
            }
        }
        trial.pop_back();
        if (ok) {
            m_candidates.push_back(pos);
        }
    }
}

// 手札の枠と、自動で進めるかどうかを計算し直す（盤面や手番が変わったとき）
void GuiApp::refreshTurnState()
{
    m_slots.clear();
    m_autoPending = false;
    if (!m_game.has_value() || m_game->isOver()) {
        return;
    }
    const Player& player = m_game->currentPlayer();
    const bool limitReached = m_game->hasPlayedCardThisTurn();
    for (const auto card : player.hand()) {
        m_slots.push_back(HandSlot{card, false, 1, !limitReached && m_game->canPlay(card)});
    }
    if (player.bonusPlaces() > 0) {
        m_slots.push_back(HandSlot{CardType::Place, true, player.bonusPlaces(), m_game->canPlay(CardType::Place)});
    }
    m_autoPending = isCpu(m_game->currentColor()) || !m_game->canPlayAnything();
}

void GuiApp::addLog(std::string line)
{
    m_log.push_back(std::move(line));
    while (m_log.size() > kMaxLogLines) {
        m_log.pop_front();
    }
}

} // namespace cardflow::app
