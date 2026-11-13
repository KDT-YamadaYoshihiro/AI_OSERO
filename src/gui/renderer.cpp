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

constexpr float kStoneRadius = 26.0f;
constexpr int kCircleSegments = 48;
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
constexpr Rgb kBackground{26, 32, 40};
constexpr Rgb kPanel{40, 48, 60};
constexpr Rgb kBoardGreen{36, 128, 82};
constexpr Rgb kGridLine{20, 78, 48};
constexpr Rgb kText{236, 240, 244};
constexpr Rgb kSubText{150, 160, 172};
constexpr Rgb kAccent{255, 208, 70};
constexpr Rgb kBlackStone{24, 24, 28};
constexpr Rgb kWhiteStone{240, 240, 234};
constexpr Rgb kStoneEdge{90, 90, 90};
constexpr Rgb kStoneShadow{14, 60, 36};
constexpr Rgb kGuardRing{90, 200, 255};
constexpr Rgb kWall{96, 100, 112};
constexpr Rgb kTarget{255, 220, 90};
constexpr Rgb kLastPlaced{235, 60, 60};
constexpr Rgb kButton{70, 110, 170};
constexpr Rgb kButtonHover{96, 140, 205};
constexpr Rgb kAccentButton{214, 120, 40};
constexpr Rgb kAccentButtonHover{238, 148, 64};

Rgb cardColor(CardType type)
{
    switch (type) {
    case CardType::Double: return {225, 135, 50};
    case CardType::Guard: return {70, 140, 220};
    case CardType::Wall: return {120, 124, 140};
    case CardType::Flip: return {210, 70, 70};
    case CardType::Free: return {60, 165, 95};
    }
    return {128, 128, 128};
}

Rgb stoneColor(Color side)
{
    return side == Color::Black ? kBlackStone : kWhiteStone;
}

void fillRect(const Rect& rect, Rgb rgb)
{
    DrawBox(rect.x, rect.y, rect.right(), rect.bottom(), color(rgb), TRUE);
}

void frameRect(const Rect& rect, Rgb rgb, int thickness)
{
    DrawBox(rect.x, rect.y, rect.right(), rect.bottom(), color(rgb), FALSE, thickness);
}

// 半透明で描く（alpha: 0〜255）
template <typename Draw>
void withAlpha(int alpha, Draw draw)
{
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    draw();
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, kOpaque);
}

void drawStone(float x, float y, float radius, Color side)
{
    DrawCircleAA(x + 2.0f, y + 3.0f, radius, kCircleSegments, color(kStoneShadow), TRUE);
    DrawCircleAA(x, y, radius, kCircleSegments, color(stoneColor(side)), TRUE);
    DrawCircleAA(x, y, radius, kCircleSegments, color(kStoneEdge), FALSE, 1.5f);
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
      m_fontLarge(createFont(26, 6)),
      m_fontHuge(createFont(56, 8)),
      m_fontTitle(createFont(60, 8))
{
}

Renderer::~Renderer()
{
    for (const int font : {m_fontSmall, m_fontMedium, m_fontLarge, m_fontHuge, m_fontTitle}) {
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

void Renderer::drawButton(const Rect& rect, std::string_view label, bool hovered, int font, bool accent) const
{
    const Rgb base = accent ? kAccentButton : kButton;
    const Rgb hover = accent ? kAccentButtonHover : kButtonHover;
    fillRect(rect, hovered ? hover : base);
    frameRect(rect, kText, 1);
    drawCentered(rect, label, color(kText), font);
}

// ─── 画面ごとの描画 ──────────────────────────────────────────────────

void Renderer::draw(const app::GuiApp& app, int mouseX, int mouseY) const
{
    fillRect(Rect{0, 0, kWindowWidth, kWindowHeight}, kBackground);
    switch (app.screen()) {
    case app::Screen::Title:
        drawTitle(mouseX, mouseY);
        break;
    case app::Screen::Difficulty: {
        const char* const labels[] = {"弱い", "普通", "強い"};
        drawMenu("CPU の強さを選んでください", labels, 3, true, mouseX, mouseY);
        break;
    }
    case app::Screen::ColorSelect: {
        const char* const labels[] = {"黒（先手）", "白（後手）"};
        drawMenu("あなたの色を選んでください", labels, 2, true, mouseX, mouseY);
        break;
    }
    case app::Screen::Playing:
        drawPlaying(app, mouseX, mouseY);
        break;
    }
}

void Renderer::drawTitle(int mouseX, int mouseY) const
{
    drawCentered(Rect{0, 100, kWindowWidth, 80}, "CardFlow Othello", color(kAccent), m_fontTitle);
    drawCentered(Rect{0, 196, kWindowWidth, 36}, "いつものオセロに、ここぞで使えるカードを 1 枚", color(kText),
                 m_fontMedium);
    drawCentered(Rect{0, 232, kWindowWidth, 30}, "石を置く前に、手札のカードを 1 枚使えます", color(kSubText),
                 m_fontSmall);
    const char* const labels[] = {"2人対戦", "CPU対戦", "終了"};
    for (int i = 0; i < 3; ++i) {
        const Rect rect = app::menuButtonRect(i);
        drawButton(rect, labels[i], rect.contains(mouseX, mouseY), m_fontLarge);
    }
}

void Renderer::drawMenu(std::string_view heading, const char* const* labels, int count, bool withBack, int mouseX,
                        int mouseY) const
{
    drawCentered(Rect{0, 180, kWindowWidth, 60}, heading, color(kText), m_fontLarge);
    for (int i = 0; i < count; ++i) {
        const Rect rect = app::menuButtonRect(i);
        drawButton(rect, labels[i], rect.contains(mouseX, mouseY), m_fontLarge);
    }
    if (withBack) {
        const Rect back = app::backButtonRect();
        drawButton(back, "戻る", back.contains(mouseX, mouseY), m_fontMedium);
    }
}

void Renderer::drawPlaying(const app::GuiApp& app, int mouseX, int mouseY) const
{
    const Game& game = *app.game();
    drawCentered(app::hintRect(), app.hint(), color(kAccent), m_fontLarge);
    drawCentered(app::messageRect(), app.message(), color(kSubText), m_fontMedium);
    const std::string seed = "seed " + std::to_string(app.seed());
    drawText(kWindowWidth - 16 - textWidth(seed, m_fontSmall), kWindowHeight - 22, seed, color(kSubText),
             m_fontSmall);

    drawBoard(app, mouseX, mouseY);
    drawPlayerPanel(app, Color::Black);
    drawPlayerPanel(app, Color::White);
    drawHand(app, mouseX, mouseY);
    if (app.showsPassButton()) {
        const Rect pass = app::passButtonRect();
        drawButton(pass, "パス", pass.contains(mouseX, mouseY), m_fontLarge, true);
    }
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
        const Rect top = app::cellRect(Position{0, i});
        const Rect left = app::cellRect(Position{i, 0});
        drawCentered(Rect{top.x, area.y - 24, top.w, 22}, std::string(1, static_cast<char>('a' + i)), color(kSubText),
                     m_fontSmall);
        drawCentered(Rect{area.x - 26, left.y, 22, left.h}, std::string(1, static_cast<char>('1' + i)),
                     color(kSubText), m_fontSmall);
    }

    const auto hovered = app::cellAt(mouseX, mouseY);
    const bool choosingTarget = !app.targetCells().empty();
    const Color current = game.currentColor();

    for (int row = 0; row < Board::kSize; ++row) {
        for (int col = 0; col < Board::kSize; ++col) {
            const Position pos{row, col};
            const Rect cell = app::cellRect(pos);
            const bool hover = hovered.has_value() && *hovered == pos;
            const auto cx = static_cast<float>(cell.centerX());
            const auto cy = static_cast<float>(cell.centerY());

            if (choosingTarget && contains(app.targetCells(), pos)) {
                withAlpha(hover ? 190 : 110, [&] { fillRect(cell, kTarget); });
            }
            frameRect(cell, kGridLine, 1);

            if (board.isWall(pos)) {
                const Rect block{cell.x + 6, cell.y + 6, cell.w - 12, cell.h - 12};
                fillRect(block, kWall);
                frameRect(block, kBackground, 2);
            } else if (const auto stone = board.stoneAt(pos)) {
                drawStone(cx, cy, kStoneRadius, stone->color);
                if (stone->guarded) {
                    DrawCircleAA(cx, cy, kStoneRadius + 2.0f, kCircleSegments, color(kGuardRing), FALSE, 4.0f);
                }
                if (game.lastPlaced() == pos) {
                    DrawCircleAA(cx, cy, 5.0f, 24, color(kLastPlaced), TRUE);
                }
            } else if (!choosingTarget && !app.selectedSlot().has_value() && contains(app.placeHints(), pos)) {
                // 置けるマス: 薄く光らせて手番の色の点。マウスを乗せると半透明の石
                withAlpha(hover ? 90 : 55, [&] { fillRect(cell, kTarget); });
                if (hover) {
                    withAlpha(160, [&] { drawStone(cx, cy, kStoneRadius, current); });
                } else {
                    DrawCircleAA(cx, cy, 8.0f, 24, color(stoneColor(current)), TRUE);
                    DrawCircleAA(cx, cy, 8.0f, 24, color(kStoneEdge), FALSE, 1.0f);
                }
            }
        }
    }
    frameRect(area, kGridLine, 3);
}

void Renderer::drawPlayerPanel(const app::GuiApp& app, Color side) const
{
    const Game& game = *app.game();
    const Rect area = app::playerPanelRect(side);
    const bool current = !game.isOver() && game.currentColor() == side;
    fillRect(area, kPanel);
    frameRect(area, current ? kAccent : kPanel, current ? 3 : 1);

    drawStone(static_cast<float>(area.centerX()), static_cast<float>(area.y + 60), 34.0f, side);
    drawCentered(Rect{area.x, area.y + 104, area.w, 30}, app.playerLabel(side), color(current ? kAccent : kText),
                 m_fontMedium);
    drawCentered(Rect{area.x, area.y + 140, area.w, 70}, std::to_string(game.board().countStones(side)),
                 color(kText), m_fontHuge);
    drawCentered(Rect{area.x, area.y + 214, area.w, 24}, "石の数", color(kSubText), m_fontSmall);
    drawCentered(Rect{area.x, area.y + 248, area.w, 26},
                 "カード " + std::to_string(game.hand(side).size()) + " 枚", color(kText), m_fontMedium);
    if (current) {
        drawCentered(Rect{area.x, area.bottom() + 12, area.w, 30}, "▶ 手番", color(kAccent), m_fontLarge);
    }
}

void Renderer::drawHand(const app::GuiApp& app, int mouseX, int mouseY) const
{
    const Game& game = *app.game();
    if (game.isOver()) {
        return;
    }
    const Color owner = app.handOwner();
    const std::string label = app.options().cpuColor.has_value() ? "あなたの手札" : app.playerLabel(owner) + "の手札";
    const Rect labelArea = app::handLabelRect();
    drawText(labelArea.x, labelArea.y + 8, label, color(kText), m_fontMedium);
    drawText(labelArea.x, labelArea.y + 38, "クリックで使う", color(kSubText), m_fontSmall);

    const auto& cards = app.shownHand();
    if (cards.empty()) {
        drawText(app::handCardRect(0).x, labelArea.y + 24, "（カードはありません。3 手ごとに 1 枚引きます）",
                 color(kSubText), m_fontSmall);
        return;
    }
    for (int i = 0; i < static_cast<int>(cards.size()) && i < kMaxHandSlots; ++i) {
        const CardType type = cards[static_cast<std::size_t>(i)];
        const auto& info = cardInfo(type);
        const bool usable = app.isCardUsable(i);
        const bool selected = app.selectedSlot() == i;
        Rect card = app::handCardRect(i);
        if (selected) {
            card.y -= 10;
        }

        fillRect(card, kPanel);
        fillRect(Rect{card.x, card.y, 10, card.h}, cardColor(type));
        drawText(card.x + 22, card.y + 8, info.name, color(kText), m_fontLarge);
        drawText(card.x + 22, card.y + 44, info.description, color(kSubText), m_fontSmall);

        if (!usable) {
            withAlpha(150, [&] { fillRect(card, kBackground); });
        }
        if (selected) {
            frameRect(card, kAccent, 3);
        } else if (usable && card.contains(mouseX, mouseY)) {
            frameRect(card, kText, 2);
        } else {
            frameRect(card, cardColor(type), 1);
        }
    }
}

void Renderer::drawResult(const app::GuiApp& app, int mouseX, int mouseY) const
{
    withAlpha(150, [&] { fillRect(Rect{0, 0, kWindowWidth, kWindowHeight}, kBackground); });
    const Rect panel = app::resultPanelRect();
    fillRect(panel, kPanel);
    frameRect(panel, kAccent, 2);
    drawCentered(Rect{panel.x, panel.y + 30, panel.w, 60}, "ゲーム終了", color(kAccent), m_fontTitle);
    drawCentered(Rect{panel.x, panel.y + 110, panel.w, 40}, app.resultText(), color(kText), m_fontLarge);
    const Rect retry = app::retryButtonRect();
    const Rect title = app::toTitleButtonRect();
    drawButton(retry, "もう一度", retry.contains(mouseX, mouseY), m_fontMedium);
    drawButton(title, "タイトルへ", title.contains(mouseX, mouseY), m_fontMedium);
}

} // namespace cardflow::gui
