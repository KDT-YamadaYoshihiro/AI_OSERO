#include "renderer.hpp"

#include <DxLib.h>

#include <algorithm>
#include <string>

#include "cardflow/app/text.hpp"
#include "cardflow/board.hpp"

namespace cardflow::gui {

using app::Rect;
using namespace app::layout;

namespace {

constexpr int kStoneRadius = 28;
constexpr int kLogLineHeight = 22;
constexpr int kOpaque = 255;

struct Rgb {
    int r;
    int g;
    int b;
};

unsigned int color(Rgb rgb)
{
    return GetColor(rgb.r, rgb.g, rgb.b);
}

// 配色
constexpr Rgb kBackground{24, 30, 38};
constexpr Rgb kPanel{36, 44, 56};
constexpr Rgb kBoardGreen{34, 120, 78};
constexpr Rgb kGridLine{18, 70, 44};
constexpr Rgb kText{232, 236, 240};
constexpr Rgb kSubText{150, 160, 172};
constexpr Rgb kAccent{255, 210, 80};
constexpr Rgb kBlackStone{22, 22, 26};
constexpr Rgb kWhiteStone{238, 238, 232};
constexpr Rgb kCoverRing{255, 196, 40};
constexpr Rgb kShieldRing{90, 200, 255};
constexpr Rgb kWall{112, 112, 124};
constexpr Rgb kCandidate{255, 230, 100};
constexpr Rgb kChosen{255, 140, 40};
constexpr Rgb kLastAction{240, 70, 70};
constexpr Rgb kButton{70, 110, 170};
constexpr Rgb kButtonHover{96, 140, 205};
constexpr Rgb kButtonDisabled{62, 68, 78};

Rgb categoryColor(CardCategory category)
{
    switch (category) {
    case CardCategory::Green: return {60, 160, 90};
    case CardCategory::Red: return {200, 70, 70};
    case CardCategory::Blue: return {70, 120, 210};
    case CardCategory::Violet: return {150, 90, 200};
    case CardCategory::Orange: return {225, 135, 50};
    case CardCategory::Yellow: return {200, 175, 40};
    }
    return {128, 128, 128};
}

void fillRect(const Rect& rect, Rgb rgb)
{
    DrawBox(rect.x, rect.y, rect.right(), rect.bottom(), color(rgb), TRUE);
}

void frameRect(const Rect& rect, Rgb rgb, int thickness)
{
    DrawBox(rect.x, rect.y, rect.right(), rect.bottom(), color(rgb), FALSE, thickness);
}

// 半透明の塗りつぶし（alpha: 0〜255）
void fillRectAlpha(const Rect& rect, Rgb rgb, int alpha)
{
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    fillRect(rect, rgb);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, kOpaque);
}

bool contains(const std::vector<Position>& list, Position pos)
{
    return std::ranges::find(list, pos) != list.end();
}

int createFont(int size, int thickness)
{
    return CreateFontToHandle("Meiryo", size, thickness, DX_FONTTYPE_ANTIALIASING_4X4, DX_CHARSET_UTF8);
}

} // namespace

Renderer::Renderer()
    : m_fontSmall(createFont(16, 4)),
      m_fontMedium(createFont(20, 5)),
      m_fontLarge(createFont(28, 6)),
      m_fontTitle(createFont(56, 8))
{
}

Renderer::~Renderer()
{
    for (const int font : {m_fontSmall, m_fontMedium, m_fontLarge, m_fontTitle}) {
        if (font != -1) {
            DeleteFontToHandle(font);
        }
    }
}

// ─── 文字とボタン ────────────────────────────────────────────────────

void Renderer::drawText(int x, int y, std::string_view text, unsigned int textColor, int font) const
{
    const std::string buffer{text};
    DrawStringToHandle(x, y, buffer.c_str(), textColor, font);
}

int Renderer::textWidth(std::string_view text, int font) const
{
    const std::string buffer{text};
    return GetDrawStringWidthToHandle(buffer.c_str(), static_cast<int>(buffer.size()), font);
}

void Renderer::drawCentered(const Rect& rect, std::string_view text, unsigned int textColor, int font) const
{
    const int width = textWidth(text, font);
    const int height = GetFontSizeToHandle(font);
    drawText(rect.centerX() - width / 2, rect.centerY() - height / 2 - 2, text, textColor, font);
}

void Renderer::drawButton(const Rect& rect, std::string_view label, bool enabled, bool hovered, int font) const
{
    fillRect(rect, !enabled ? kButtonDisabled : hovered ? kButtonHover : kButton);
    frameRect(rect, enabled ? kText : kSubText, 1);
    drawCentered(rect, label, color(enabled ? kText : kSubText), font);
}

// ─── 画面ごとの描画 ──────────────────────────────────────────────────

void Renderer::draw(const app::GuiApp& app, int mouseX, int mouseY) const
{
    fillRect(Rect{0, 0, kWindowWidth, kWindowHeight}, kBackground);
    switch (app.screen()) {
    case app::Screen::Title:
        drawTitle(mouseX, mouseY);
        break;
    case app::Screen::Difficulty:
        drawDifficulty(mouseX, mouseY);
        break;
    case app::Screen::ColorSelect:
        drawColorSelect(mouseX, mouseY);
        break;
    case app::Screen::Playing:
        drawPlaying(app, mouseX, mouseY);
        break;
    }
}

void Renderer::drawTitle(int mouseX, int mouseY) const
{
    drawCentered(Rect{0, 110, kWindowWidth, 80}, "CardFlow Othello", color(kAccent), m_fontTitle);
    drawCentered(Rect{0, 200, kWindowWidth, 40}, "手札のカードと EP で戦う、これまでにないオセロ", color(kText),
                 m_fontMedium);
    const char* labels[] = {"2人対戦", "CPU対戦", "終了"};
    for (int i = 0; i < 3; ++i) {
        const Rect rect = app::menuButtonRect(i);
        drawButton(rect, labels[i], true, rect.contains(mouseX, mouseY), m_fontLarge);
    }
}

void Renderer::drawDifficulty(int mouseX, int mouseY) const
{
    drawCentered(Rect{0, 180, kWindowWidth, 60}, "CPU の強さを選んでください", color(kText), m_fontLarge);
    const char* labels[] = {"弱い", "普通", "強い"};
    for (int i = 0; i < 3; ++i) {
        const Rect rect = app::menuButtonRect(i);
        drawButton(rect, labels[i], true, rect.contains(mouseX, mouseY), m_fontLarge);
    }
    const Rect back = app::backButtonRect();
    drawButton(back, "戻る", true, back.contains(mouseX, mouseY), m_fontMedium);
}

void Renderer::drawColorSelect(int mouseX, int mouseY) const
{
    drawCentered(Rect{0, 180, kWindowWidth, 60}, "あなたの色を選んでください", color(kText), m_fontLarge);
    const char* labels[] = {"黒（先手）", "白（後手）"};
    for (int i = 0; i < 2; ++i) {
        const Rect rect = app::menuButtonRect(i);
        drawButton(rect, labels[i], true, rect.contains(mouseX, mouseY), m_fontLarge);
    }
    const Rect back = app::backButtonRect();
    drawButton(back, "戻る", true, back.contains(mouseX, mouseY), m_fontMedium);
}

void Renderer::drawPlaying(const app::GuiApp& app, int mouseX, int mouseY) const
{
    const Game& game = *app.game();
    std::string heading = "ラウンド " + std::to_string(game.round()) + " ／ " + app.playerLabel(game.currentColor()) + "の番";
    if (game.isOver()) {
        heading = "ゲーム終了";
    }
    drawText(kBoardLeft, 24, heading, color(kText), m_fontLarge);
    const std::string seed = "seed " + std::to_string(app.seed());
    drawText(kWindowWidth - 24 - textWidth(seed, m_fontSmall), 32, seed, color(kSubText), m_fontSmall);

    drawBoard(app, mouseX, mouseY);
    drawStatus(app);
    drawHand(app, mouseX, mouseY);
    drawControls(app, mouseX, mouseY);
    drawLog(app);
    if (game.isOver()) {
        drawResult(app, mouseX, mouseY);
    }
}

void Renderer::drawBoard(const app::GuiApp& app, int mouseX, int mouseY) const
{
    const Game& game = *app.game();
    const Board& board = game.board();
    const Rect area = app::boardRect();
    fillRect(area, kBoardGreen);

    // 座標の見出し（a〜h、1〜8）
    for (int i = 0; i < Board::kSize; ++i) {
        const std::string col(1, static_cast<char>('a' + i));
        const std::string row(1, static_cast<char>('1' + i));
        const Rect top = app::cellRect(Position{0, i});
        const Rect left = app::cellRect(Position{i, 0});
        drawCentered(Rect{top.x, area.y - 26, top.w, 24}, col, color(kSubText), m_fontSmall);
        drawCentered(Rect{area.x - 28, left.y, 24, left.h}, row, color(kSubText), m_fontSmall);
    }

    const auto& selection = app.selection();
    const auto hovered = app::cellAt(mouseX, mouseY);
    for (int row = 0; row < Board::kSize; ++row) {
        for (int col = 0; col < Board::kSize; ++col) {
            const Position pos{row, col};
            const Rect cell = app::cellRect(pos);
            if (contains(app.candidates(), pos)) {
                const bool hover = hovered.has_value() && *hovered == pos;
                fillRectAlpha(cell, kCandidate, hover ? 170 : 90);
            }
            if (selection.has_value()) {
                const auto& targets = selection->targets;
                if (const auto it = std::ranges::find(targets, pos); it != targets.end()) {
                    fillRectAlpha(cell, kChosen, 170);
                }
            }
            frameRect(cell, kGridLine, 1);
            if (contains(app.lastActionCells(), pos)) {
                frameRect(Rect{cell.x + 2, cell.y + 2, cell.w - 4, cell.h - 4}, kLastAction, 3);
            }

            const int cx = cell.centerX();
            const int cy = cell.centerY();
            if (const auto wall = board.wallAt(pos)) {
                fillRect(Rect{cell.x + 8, cell.y + 8, cell.w - 16, cell.h - 16}, kWall);
                DrawLine(cell.x + 8, cell.y + 8, cell.right() - 8, cell.bottom() - 8, color(kBackground), 2);
                DrawLine(cell.right() - 8, cell.y + 8, cell.x + 8, cell.bottom() - 8, color(kBackground), 2);
                if (wall->owner.has_value()) {
                    const Rgb owner = *wall->owner == Color::Black ? kBlackStone : kWhiteStone;
                    DrawCircleAA(static_cast<float>(cell.right() - 16), static_cast<float>(cell.y + 16), 7.0f, 24,
                                 color(owner), TRUE);
                }
            } else if (const auto stone = board.stoneAt(pos)) {
                const bool black = stone->color == Color::Black;
                const auto fx = static_cast<float>(cx);
                const auto fy = static_cast<float>(cy);
                DrawCircleAA(fx + 2.0f, fy + 3.0f, kStoneRadius, 48, GetColor(10, 50, 30), TRUE);
                DrawCircleAA(fx, fy, kStoneRadius, 48, color(black ? kBlackStone : kWhiteStone), TRUE);
                DrawCircleAA(fx, fy, kStoneRadius, 48, GetColor(90, 90, 90), FALSE, 1.5f);
                if (stone->covered) {
                    DrawCircleAA(fx, fy, kStoneRadius - 6.0f, 48, color(kCoverRing), FALSE, 4.0f);
                }
                if (stone->shielded) {
                    DrawCircleAA(fx, fy, kStoneRadius + 3.0f, 48, color(kShieldRing), FALSE, 4.0f);
                }
            }

            if (selection.has_value()) {
                const auto& targets = selection->targets;
                if (const auto it = std::ranges::find(targets, pos); it != targets.end()) {
                    const std::string number = std::to_string(it - targets.begin() + 1);
                    drawText(cell.x + 4, cell.y + 2, number, color(kAccent), m_fontSmall);
                }
            }
        }
    }
    frameRect(area, kGridLine, 3);

    // 記号の説明
    drawText(area.x, area.bottom() + 10, "金の輪 = Cover（次の自分の番まで保護）  水色の輪 = Shield（1 回防ぐ）  ■ = 壁",
             color(kSubText), m_fontSmall);
}

void Renderer::drawStatus(const app::GuiApp& app) const
{
    const Game& game = *app.game();
    const Board& board = game.board();
    const Rect area = app::statusRect();
    fillRect(area, kPanel);

    int y = area.y + 10;
    for (const Color side : {Color::Black, Color::White}) {
        const Player& player = game.player(side);
        const bool current = !game.isOver() && game.currentColor() == side;
        if (current) {
            fillRect(Rect{area.x, y - 4, 6, 30}, kAccent);
        }
        const auto stone = side == Color::Black ? kBlackStone : kWhiteStone;
        DrawCircleAA(static_cast<float>(area.x + 28), static_cast<float>(y + 11), 11.0f, 32, color(stone), TRUE);
        DrawCircleAA(static_cast<float>(area.x + 28), static_cast<float>(y + 11), 11.0f, 32, GetColor(120, 120, 120),
                     FALSE);
        drawText(area.x + 48, y, app.playerLabel(side), color(current ? kAccent : kText), m_fontMedium);
        drawText(area.x + 270, y, "石 " + std::to_string(board.countStones(side)), color(kText), m_fontMedium);

        // EP（上限 6 のゲージ）
        drawText(area.x + 360, y, "EP", color(kText), m_fontMedium);
        for (int i = 0; i < Player::kMaxEp; ++i) {
            const Rect pip{area.x + 396 + i * 26, y + 3, 20, 18};
            if (i < player.ep()) {
                fillRect(pip, kAccent);
            } else {
                frameRect(pip, kSubText, 1);
            }
        }
        y += 32;
    }

    const Player& me = game.currentPlayer();
    std::string deck = "山札 " + std::to_string(game.deck().drawPileSize()) + " 枚 ／ 捨て札 " +
                       std::to_string(game.deck().discardPileSize()) + " 枚";
    if (!game.isOver() && me.reservedPlaces() > 0) {
        deck += " ／ 次の番に追加 Place " + std::to_string(me.reservedPlaces()) + " 枚";
    }
    drawText(area.x + 16, y + 2, deck, color(kSubText), m_fontSmall);
}

void Renderer::drawHand(const app::GuiApp& app, int mouseX, int mouseY) const
{
    const Game& game = *app.game();
    if (game.isOver()) {
        return;
    }
    const bool hidden = app.isCpu(game.currentColor());
    std::string label = hidden ? "CPU の手札（伏せています）" : app.playerLabel(game.currentColor()) + "の手札";
    if (game.hasPlayedCardThisTurn()) {
        label += "　このターンは使用済み";
    }
    drawText(kPanelLeft, kHandTop - 24, label, color(kSubText), m_fontSmall);

    const auto& slots = app.handSlots();
    for (int i = 0; i < static_cast<int>(slots.size()) && i < kMaxHandSlots; ++i) {
        const auto& slot = slots[static_cast<std::size_t>(i)];
        const Rect card = app::handCardRect(i);
        if (hidden) {
            fillRect(card, kPanel);
            frameRect(card, kSubText, 1);
            drawCentered(card, "？", color(kSubText), m_fontLarge);
            continue;
        }
        const auto& info = cardInfo(slot.card);
        const Rgb band = categoryColor(info.category);
        fillRect(card, kPanel);
        fillRect(Rect{card.x, card.y, card.w, 36}, band);
        drawCentered(Rect{card.x, card.y, card.w, 36}, info.name, color(kText), m_fontMedium);

        const std::string category = std::string{app::categoryName(info.category)} + "　EP " + std::to_string(info.cost);
        drawText(card.x + 10, card.y + 46, category, color(kAccent), m_fontMedium);
        const auto lines = app::cardDescription(slot.card);
        drawText(card.x + 10, card.y + 84, lines[0], color(kText), m_fontSmall);
        drawText(card.x + 10, card.y + 106, lines[1], color(kText), m_fontSmall);
        if (slot.bonus) {
            drawText(card.x + 10, card.y + 146, "追加 ×" + std::to_string(slot.count) + "（別枠）", color(kAccent),
                     m_fontSmall);
        }

        const bool selected = app.selection().has_value() && app.selection()->slot == i;
        if (!slot.playable) {
            fillRectAlpha(card, kBackground, 150);
        }
        if (selected) {
            frameRect(card, kAccent, 4);
        } else if (slot.playable && app.isHumanTurn() && card.contains(mouseX, mouseY)) {
            frameRect(card, kText, 2);
        } else {
            frameRect(card, band, 1);
        }
    }
}

void Renderer::drawControls(const app::GuiApp& app, int mouseX, int mouseY) const
{
    const Game& game = *app.game();
    if (!game.isOver()) {
        const Rect use = app::useButtonRect();
        const Rect cancel = app::cancelButtonRect();
        const Rect end = app::endTurnButtonRect();
        drawButton(use, "使う", app.canUseSelected(), use.contains(mouseX, mouseY), m_fontMedium);
        drawButton(cancel, "取り消し", app.selection().has_value(), cancel.contains(mouseX, mouseY), m_fontMedium);
        drawButton(end, game.hasActedThisTurn() ? "ターン終了" : "パス", app.canEndTurn(), end.contains(mouseX, mouseY),
                   m_fontMedium);
    }
    const Rect hint = app::hintRect();
    drawText(hint.x, hint.y + 4, app.hint(), color(kAccent), m_fontMedium);
}

void Renderer::drawLog(const app::GuiApp& app) const
{
    const Rect area = app::logRect();
    fillRect(area, kPanel);
    const auto& log = app.log();
    const int lines = std::max(0, (area.h - 12) / kLogLineHeight);
    const std::size_t count = std::min(log.size(), static_cast<std::size_t>(lines));
    int y = area.y + 8;
    for (std::size_t i = log.size() - count; i < log.size(); ++i) {
        drawText(area.x + 12, y, log[i], color(i + 1 == log.size() ? kText : kSubText), m_fontSmall);
        y += kLogLineHeight;
    }
}

void Renderer::drawResult(const app::GuiApp& app, int mouseX, int mouseY) const
{
    fillRectAlpha(Rect{0, 0, kWindowWidth, kWindowHeight}, kBackground, 150);
    const Rect panel = app::resultPanelRect();
    fillRect(panel, kPanel);
    frameRect(panel, kAccent, 2);
    drawCentered(Rect{panel.x, panel.y + 30, panel.w, 50}, "ゲーム終了", color(kAccent), m_fontTitle);
    drawCentered(Rect{panel.x, panel.y + 110, panel.w, 40}, app.resultText(), color(kText), m_fontLarge);
    const Rect retry = app::retryButtonRect();
    const Rect title = app::toTitleButtonRect();
    drawButton(retry, "もう一度", true, retry.contains(mouseX, mouseY), m_fontMedium);
    drawButton(title, "タイトルへ", true, title.contains(mouseX, mouseY), m_fontMedium);
}

} // namespace cardflow::gui
