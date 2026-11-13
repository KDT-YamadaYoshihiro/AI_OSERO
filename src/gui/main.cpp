#include <DxLib.h>

#include <array>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <random>
#include <string_view>

#include "cardflow/app/gui_app.hpp"
#include "cardflow/app/layout.hpp"
#include "renderer.hpp"

namespace {

constexpr int kColorBits = 32;
constexpr wchar_t kSaveFileName[] = L"cardflow_save.txt";

// 中断データは実行ファイルと同じフォルダに置く
std::filesystem::path savePath()
{
    std::array<wchar_t, 4096> buffer{};
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) {
        return std::filesystem::path{kSaveFileName};
    }
    return std::filesystem::path{buffer.data()}.parent_path() / kSaveFileName;
}

// 使い方: cardflow.exe [seed]
//   seed を指定すると山札の並びが固定され、同じ展開を再現できる
std::uint32_t seedFromCommandLine()
{
    std::uint32_t seed = std::random_device{}();
    if (__argc >= 2 && __argv != nullptr) {
        const std::string_view arg{__argv[1]};
        std::uint32_t parsed = 0;
        const auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), parsed);
        if (ec == std::errc{} && ptr == arg.data() + arg.size()) {
            seed = parsed;
        }
    }
    return seed;
}

void pollMouse(cardflow::app::GuiApp& app)
{
    int button = 0;
    int x = 0;
    int y = 0;
    int type = 0;
    while (GetMouseInputLog2(&button, &x, &y, &type, TRUE) == 0) {
        if (type != MOUSE_INPUT_LOG_DOWN) {
            continue;
        }
        if ((button & MOUSE_INPUT_LEFT) != 0) {
            app.click(x, y);
        } else if ((button & MOUSE_INPUT_RIGHT) != 0) {
            app.cancel();
        }
    }
}

} // namespace

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
    using namespace cardflow::app::layout;

    SetOutApplicationLogValidFlag(FALSE);           // Log.txt を作らない
    SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);  // 文字列は UTF-8
    ChangeWindowMode(TRUE);
    SetGraphMode(kWindowWidth, kWindowHeight, kColorBits);
    SetMainWindowText("CardFlow Othello");
    if (DxLib_Init() == -1) {
        return EXIT_FAILURE;
    }
    SetDrawScreen(DX_SCREEN_BACK);

    {
        const cardflow::gui::Renderer renderer;
        cardflow::app::GuiApp app(seedFromCommandLine(), savePath());
        int last = GetNowCount();
        bool escapeHeld = false;
        while (ProcessMessage() == 0 && !app.quitRequested()) {
            pollMouse(app);
            const bool escape = CheckHitKey(KEY_INPUT_ESCAPE) != 0;
            if (escape && !escapeHeld) {
                app.togglePause();
            }
            escapeHeld = escape;
            const int now = GetNowCount();
            app.update(now - last);
            last = now;

            int mouseX = 0;
            int mouseY = 0;
            GetMousePoint(&mouseX, &mouseY);
            ClearDrawScreen();
            renderer.draw(app, mouseX, mouseY);
            ScreenFlip();
        }
    }

    DxLib_End();
    return EXIT_SUCCESS;
}
