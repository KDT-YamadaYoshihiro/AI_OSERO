#pragma once

#include <string_view>
#include <vector>

#include "cardflow/app/gui_app.hpp"
#include "cardflow/app/layout.hpp"

// DxLib による描画。DxLib_Init の後に作り、DxLib_End の前に破棄する。
namespace cardflow::gui {

class Renderer {
public:
    Renderer();
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // 画面全体を描く（mouseX / mouseY はホバー表示用）
    void draw(const app::GuiApp& app, int mouseX, int mouseY) const;

private:
    void drawMenuScreen(const app::GuiApp& app, int mouseX, int mouseY) const;
    void drawPlaying(const app::GuiApp& app, int mouseX, int mouseY) const;
    void drawMenuBar(const app::GuiApp& app, int mouseX, int mouseY) const;
    void drawOverlay(const app::GuiApp& app, int mouseX, int mouseY) const;
    void drawBoard(const app::GuiApp& app, int mouseX, int mouseY) const;
    void drawPlayerPanel(const app::GuiApp& app, Color color) const;
    void drawHand(const app::GuiApp& app, int mouseX, int mouseY) const;
    void drawResult(const app::GuiApp& app, int mouseX, int mouseY) const;

    void drawButton(const app::Rect& rect, std::string_view label, bool hovered, int font, bool accent = false) const;
    // maxWidth > 0 なら、その幅に収まるまで文字を小さくする（縦の中心は保つ）
    void drawText(int x, int y, std::string_view text, unsigned int color, int font, int maxWidth = 0) const;
    // 枠の中央に描く。枠の幅に収まらなければ文字を小さくする
    void drawCentered(const app::Rect& rect, std::string_view text, unsigned int color, int font) const;
    [[nodiscard]] int textWidth(std::string_view text, int font) const;
    // font から順に小さいフォントを試し、maxWidth に収まる最初のものを返す
    [[nodiscard]] int fitFont(std::string_view text, int font, int maxWidth) const;
    [[nodiscard]] int fontOfSize(int size) const;

    struct Font {
        int size;
        int handle;
    };
    std::vector<Font> m_fonts;  // 大きい順

    int m_fontSmall = -1;
    int m_fontMedium = -1;
    int m_fontLarge = -1;
    int m_fontHuge = -1;
    int m_fontTitle = -1;
};

} // namespace cardflow::gui
