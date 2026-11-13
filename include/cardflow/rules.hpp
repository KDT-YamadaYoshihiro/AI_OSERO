#pragma once

#include <vector>

#include "cardflow/board.hpp"
#include "cardflow/types.hpp"

// オセロのルール（仕様書 1.1）
namespace cardflow::rules {

// pos に color の石を置いたときに挟める相手の石（守りの石も含む）。
// pos が空マスでなければ空を返す。
[[nodiscard]] std::vector<Position> findFlips(const Board& board, Position pos, Color color);

// 合法手か（空マスで、相手の石を 1 つ以上挟める）
[[nodiscard]] bool isLegalMove(const Board& board, Position pos, Color color);

// 合法手の一覧（行優先）
[[nodiscard]] std::vector<Position> legalMoves(const Board& board, Color color);
[[nodiscard]] int countLegalMoves(const Board& board, Color color);

// 「自由」で置けるマスか（空マスで、8 方向のどこかに石がある）
[[nodiscard]] bool isFreeCell(const Board& board, Position pos);

struct PlaceResult {
    std::vector<Position> flipped;  // 裏返った石
    int guardsBroken = 0;           // 守りが外れて裏返らなかった石
};

// 石を置いて、挟んだ石を裏返す（守りの石は裏返らず守りが外れる）。
// 置けるかどうかの判定は呼び出し側の責務。pos が空マスでなければ何もしない。
PlaceResult placeAndFlip(Board& board, Position pos, Color color);

} // namespace cardflow::rules
