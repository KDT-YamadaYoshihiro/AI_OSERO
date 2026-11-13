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
    void drawTitle(int mouseX, int mouseY) const;
    void drawDifficulty(int mouseX, int mouseY) const;
    void drawColorSelect(int mouseX, int mouseY) const;
    void drawPlaying(const app::GuiApp& app, int mouseX, int mouseY) const;
    void drawBoard(const app::GuiApp& app, int mouseX, int mouseY) const;
    void drawStatus(const app::GuiApp& app) const;
    void drawHand(const app::GuiApp& app, int mouseX, int mouseY) const;
    void drawControls(const app::GuiApp& app, int mouseX, int mouseY) const;
    void drawLog(const app::GuiApp& app) const;
    void drawResult(const app::GuiApp& app, int mouseX, int mouseY) const;

    void drawButton(const app::Rect& rect, std::string_view label, bool enabled, bool hovered, int font) const;
    void drawText(int x, int y, std::string_view text, unsigned int color, int font) const;
    void drawCentered(const app::Rect& rect, std::string_view text, unsigned int color, int font) const;
    [[nodiscard]] int textWidth(std::string_view text, int font) const;

    int m_fontSmall;
    int m_fontMedium;
    int m_fontLarge;
    int m_fontTitle;
};

} // namespace cardflow::gui
