#include <charconv>
#include <cstdint>
#include <iostream>
#include <random>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#endif

#include "cardflow/ui/console_game.hpp"

// 使い方: cardflow [seed]
//   seed を指定すると山札の並びが固定され、同じ展開を再現できる
int main(int argc, char* argv[])
{
#ifdef _WIN32
    // UTF-8 で入出力する（日本語・記号の文字化け対策）
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::uint32_t seed = std::random_device{}();
    if (argc >= 2) {
        const std::string_view arg{argv[1]};
        std::uint32_t parsed = 0;
        const auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), parsed);
        if (ec != std::errc{} || ptr != arg.data() + arg.size()) {
            std::cerr << "seed には 0 以上の整数を指定してください: " << arg << '\n';
            return 1;
        }
        seed = parsed;
    }

    std::cout << "（seed: " << seed << "）\n";
    cardflow::ui::runConsoleGame(std::cin, std::cout, seed);
    return 0;
}
