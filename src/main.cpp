#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

#include "cardflow/board.hpp"
#include "cardflow/player.hpp"
#include "cardflow/ui/board_view.hpp"

int main()
{
#ifdef _WIN32
    // UTF-8 で出力する（日本語・記号の文字化け対策）
    SetConsoleOutputCP(CP_UTF8);
#endif

    using cardflow::Board;
    using cardflow::Color;
    using cardflow::Player;

    const Board board = Board::createInitial();
    Player black{Color::Black};
    Player white{Color::White};
    black.recoverEp();

    std::cout << "CardFlow Othello（開発中：ゲームループ未実装）\n\n"
              << cardflow::ui::renderBoard(board) << '\n'
              << "黒 ● 石 " << board.countStones(Color::Black) << " / EP " << black.ep() << '\n'
              << "白 ○ 石 " << board.countStones(Color::White) << " / EP " << white.ep() << '\n';
    return 0;
}
