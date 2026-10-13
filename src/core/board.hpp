// 盤: 重力落下 (gravity settle) + 補助查询
#pragma once
#include <vector>
#include "types.hpp"

namespace gco {

class Board {
public:
    std::array<CellState, kBoardSize * kBoardSize> cells{};

    int lowestEmpty(int col) const;   // 列の最低空cell (row index), -1 if full
    int topRow(int col) const;        // 列の最上stone row (最小row), -1 if none
    bool columnFull(int col) const { return lowestEmpty(col) < 0; }
    bool boardFull() const;
    int countStones(Cell color) const;

    // 重力落下: 各列のstoneを下方へ凝集。移動したdestination indexを返す (FR-004, R5)
    std::vector<int> settle();
};

} // namespace gco
