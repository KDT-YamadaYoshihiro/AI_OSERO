#pragma once

#include <string_view>

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
    void drawText(int x, int y, std::string_view text, unsigned int color, int font) const;
    void drawCentered(const app::Rect& rect, std::string_view text, unsigned int color, int font) const;
    [[nodiscard]] int textWidth(std::string_view text, int font) const;

    int m_fontSmall;
    int m_fontMedium;
    int m_fontLarge;
    int m_fontHuge;
    int m_fontTitle;
};

} // namespace cardflow::gui
