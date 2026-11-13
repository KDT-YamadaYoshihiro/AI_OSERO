#include "cardflow/app/gui_app.hpp"

#include <algorithm>
#include <utility>

#include "cardflow/app/layout.hpp"
#include "cardflow/app/text.hpp"

namespace cardflow::app {

namespace {

constexpr std::uint32_t kCpuSeedSalt = 0x9E3779B9u;
const std::vector<CardType> kNoCards;

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

Color GuiApp::handOwner() const noexcept
{
    if (m_options.cpuColor.has_value()) {
        return opponentOf(*m_options.cpuColor);
    }
    return m_game.has_value() ? m_game->currentColor() : Color::Black;
}

const std::vector<CardType>& GuiApp::shownHand() const
{
    return m_game.has_value() ? m_game->hand(handOwner()) : kNoCards;
}

bool GuiApp::isCardUsable(int slot) const noexcept
{
    return slot >= 0 && slot < static_cast<int>(m_usable.size()) && m_usable[static_cast<std::size_t>(slot)];
}

bool GuiApp::showsPassButton() const noexcept
{
    return isHumanTurn() && m_mustPass;
}

std::optional<CardType> GuiApp::selectedCard() const
{
    if (!m_selected.has_value()) {
        return std::nullopt;
    }
    const auto& cards = shownHand();
    const auto index = static_cast<std::size_t>(*m_selected);
    return index < cards.size() ? std::optional<CardType>{cards[index]} : std::nullopt;
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
    const std::string turn = m_options.cpuColor.has_value() ? "" : playerLabel(current) + "の番：";
    if (const auto card = selectedCard()) {
        const auto& info = cardInfo(*card);
        if (!info.needsTarget) {
            return turn + "もう一度クリックすると「" + std::string{info.name} + "」を使います";
        }
        switch (*card) {
        case CardType::Guard: return turn + "守る自分の石を選んでください（右クリックで取り消し）";
        case CardType::Wall: return turn + "壁にする空マスを選んでください（右クリックで取り消し）";
        case CardType::Flip: return turn + "奪う相手の石を選んでください（右クリックで取り消し）";
        case CardType::Double:
        case CardType::Free: break;
        }
    }
    if (m_mustPass) {
        return turn + (m_autoPending ? "置ける場所がないので、パスします" : "置ける場所がありません。カードを使うか、パスしてください");
    }
    if (m_game->placedThisTurn() > 0) {
        return turn + "連打：もう 1 つ置いてください";
    }
    if (m_game->placementsLeft() > 1) {
        return turn + "連打：光っているマスに 2 つ続けて置けます";
    }
    if (m_game->freeMoveActive()) {
        return turn + "自由：石の隣ならどこにでも置けます";
    }
    if (std::ranges::any_of(m_usable, [](bool usable) { return usable; })) {
        return turn + "光っているマスに石を置いてください（置く前にカードも使えます）";
    }
    return turn + "光っているマスに石を置いてください";
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
    for (int slot = 0; slot < static_cast<int>(shownHand().size()); ++slot) {
        if (handCardRect(slot).contains(x, y)) {
            selectSlot(slot);
            return;
        }
    }
    if (showsPassButton() && passButtonRect().contains(x, y)) {
        passTurn(false);
        return;
    }
    if (const auto pos = cellAt(x, y)) {
        clickCell(*pos);
        return;
    }
    clearSelection();
}

void GuiApp::clickCell(Position pos)
{
    if (const auto card = selectedCard()) {
        if (cardInfo(*card).needsTarget && containsPosition(m_targets, pos)) {
            useCard(*card, pos);
        } else {
            clearSelection();
        }
        return;
    }
    if (m_game->canPlace(pos)) {
        placeStone(pos);
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
    m_message = "対戦開始！ 黒が先手です";
    m_waitMs = 0;
    clearSelection();
    refresh();
}

void GuiApp::startNewGame()
{
    m_seed = m_nextSeed;
    startGame(Game(m_seed), m_options);
}

void GuiApp::selectSlot(int slot)
{
    const auto& cards = shownHand();
    const CardType card = cards[static_cast<std::size_t>(slot)];
    if (!isCardUsable(slot)) {
        clearSelection();
        if (m_game->cardUsedThisTurn()) {
            m_message = "カードは 1 ターンに 1 枚までです";
        } else {
            m_message = "「" + std::string{cardInfo(card).name} + "」はいま使える相手がいません";
        }
        return;
    }
    if (m_selected == slot) {
        if (cardInfo(card).needsTarget) {
            clearSelection();
        } else {
            useCard(card, std::nullopt);
        }
        return;
    }
    m_selected = slot;
    m_targets = cardInfo(card).needsTarget ? m_game->cardTargets(card) : std::vector<Position>{};
}

void GuiApp::useCard(CardType card, std::optional<Position> target)
{
    const Color actor = m_game->currentColor();
    clearSelection();
    if (m_game->useCard(card, target) != CardStatus::Ok) {
        return;
    }
    m_message = playerLabel(actor) + "：「" + std::string{cardInfo(card).name} + "」を";
    if (target.has_value()) {
        m_message += " " + formatPosition(*target) + " に";
    }
    m_message += "使いました";
    refresh();
}

void GuiApp::placeStone(Position pos)
{
    const Color actor = m_game->currentColor();
    clearSelection();
    const auto result = m_game->place(pos);
    if (!result.has_value()) {
        return;
    }
    m_message = playerLabel(actor) + "：" + formatPosition(pos) + " に置きました";
    if (!result->flipped.empty()) {
        m_message += "（" + std::to_string(result->flipped.size()) + " 個裏返し）";
    }
    if (result->guardsBroken > 0) {
        m_message += "　守りで " + std::to_string(result->guardsBroken) + " 個防がれました";
    }
    refresh();
}

void GuiApp::passTurn(bool automatic)
{
    const Color actor = m_game->currentColor();
    clearSelection();
    if (!m_game->pass()) {
        return;
    }
    m_message = playerLabel(actor) + (automatic ? "：置ける場所がないのでパス" : "：パス");
    refresh();
}

void GuiApp::stepAutomatic()
{
    const Color current = m_game->currentColor();
    if (isCpu(current) && m_cpu.has_value()) {
        const auto action = m_cpu->chooseAction(*m_game);
        switch (action.kind) {
        case ai::Action::Kind::UseCard:
            useCard(action.card, action.target);
            return;
        case ai::Action::Kind::Place:
            placeStone(action.pos);
            return;
        case ai::Action::Kind::Pass:
            passTurn(true);
            return;
        }
        return;
    }
    passTurn(true);
}

void GuiApp::clearSelection()
{
    m_selected.reset();
    m_targets.clear();
}

// 置けるマス・使えるカード・自動で進めるかを計算し直す（盤面や手番が変わったとき）
void GuiApp::refresh()
{
    m_placeHints.clear();
    m_usable.clear();
    m_mustPass = false;
    m_autoPending = false;
    if (!m_game.has_value() || m_game->isOver()) {
        return;
    }
    const bool myTurn = isHumanTurn() && handOwner() == m_game->currentColor();
    for (const auto card : shownHand()) {
        m_usable.push_back(myTurn && m_game->canUseCard(card));
    }
    m_mustPass = m_game->mustPass();
    if (isHumanTurn()) {
        m_placeHints = m_game->placeableCells();
    }
    const bool anyUsable = std::ranges::any_of(m_usable, [](bool usable) { return usable; });
    m_autoPending = isCpu(m_game->currentColor()) || (m_mustPass && !anyUsable);
}

} // namespace cardflow::app
