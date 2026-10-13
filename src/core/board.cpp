// 盤実装: gravity settle, stone count
#include "board.hpp"

namespace gco {

int Board::lowestEmpty(int col) const {
    for (int r = kBoardSize - 1; r >= 0; --r) {
        if (cells[idx(r, col)].color == EMPTY) return r;
    }
    return -1;
}

int Board::topRow(int col) const {
    for (int r = 0; r < kBoardSize; ++r) {
        if (cells[idx(r, col)].color != EMPTY) return r;
    }
    return -1;
}

bool Board::boardFull() const {
    for (int i = 0; i < kBoardSize * kBoardSize; ++i) {
        if (cells[i].color == EMPTY) return false;
    }
    return true;
}

int Board::countStones(Cell color) const {
    int n = 0;
    for (int i = 0; i < kBoardSize * kBoardSize; ++i) {
        if (cells[i].color == color) ++n;
    }
    return n;
}

std::vector<int> Board::settle() {
    std::array<CellState, kBoardSize * kBoardSize> before = cells;
    std::vector<int> moved;
    for (int c = 0; c < kBoardSize; ++c) {
        // 下から上へstone収集 (bottom-to-top order preserved)
        std::vector<CellState> stack;
        for (int r = kBoardSize - 1; r >= 0; --r) {
            if (cells[idx(r, c)].color != EMPTY) stack.push_back(cells[idx(r, c)]);
        }
        for (int r = 0; r < kBoardSize; ++r) cells[idx(r, c)] = CellState{};
        int row = kBoardSize - 1;
        for (const CellState& s : stack) {
            cells[idx(row, c)] = s;
            --row;
        }
    }
    // 移動stone = 空cellへ落下したdestination (石はgapへ落ちるためbefore==EMPTY)
    for (int i = 0; i < kBoardSize * kBoardSize; ++i) {
        if (before[i].color == EMPTY && cells[i].color != EMPTY) moved.push_back(i);
    }
    return moved;
}

} // namespace gco
