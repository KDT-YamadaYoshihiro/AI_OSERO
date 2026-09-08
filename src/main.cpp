#include <windows.h>      // Windows API（dxlib が使っている）
#include <dinput.h>        // DirectInput9
#include <dxguid.h>        // DirectX GUIDs

// dxlib のヘッダーはシステムに同梱されています
// ただし、Windows のバージョンによって場所が異なるため、ここでは簡易的なラッパーを提供します
// 実際の開発では：https://www.dxlib.jp/download/ から dxlib をダウンロードして使います

#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <dx.h>           // DX9 のヘッダー
#endif

namespace game {

struct GameConfig {
    int width = 640;
    int height = 480;
    int fps = 60;
};

class DxGame : public dxlib::DxLib {
public:
    DxGame() {}
    virtual ~DxGame() {}

protected:
    LRESULT OnWmLButtonDown(HWND hwnd, WPARAM wParam, LPARAM lParam) override;
    LRESULT OnWmRButtonDown(HWND hwnd, WPARAM wParam, LPARAM lParam) override;
    void InitSystem();
    void RunLoop();
};

} // namespace game

int main() {
    std::cout << "=== C++20 DxGame (dxlib + Windows API) ===" << std::endl;
    std::cout << "This is a placeholder for dxlib-based game development." << std::endl;
    std::cout << "To run this, you need:" << std::endl;
    std::cout << "  - A compatible C++ compiler (MSVC / ClangCXX / MinGW-w64)" << std::endl;
    std::cout << "  - Windows SDK (for dx.h, dinput.h)" << std::endl;

    return 0;
}
