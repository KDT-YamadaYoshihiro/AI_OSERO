#pragma once

#include <optional>
#include <vector>

#include "cardflow/board.hpp"
#include "cardflow/types.hpp"

// 挟み・裏返し・捕獲のルール（仕様書 Clarifications Session 2026-11-13）
//
// 「挟み」: 縦・横・斜めの一直線上で、連続する敵石の列の両端が
//           「自分の石」または「壁（設置者を問わない）」であること。
//           盤の端は挟む側にならない。
// 保護（Cover / Shield）された石は列の一部として数えるが、
// 裏返し・捕獲の対象からは除く。
namespace cardflow::rules {

// pos に置かれた（置かれると仮定した）mover の石を起点に、8 方向で挟まれる敵石を返す。
// pos 自体の状態は見ない。返す座標に保護された石は含まない。
[[nodiscard]] std::vector<Position> findFlips(const Board& board, Position pos, Color mover);

// 空マス pos に mover の石を置き、挟んだ敵石を裏返す。
// 置けない（盤外・空でない）場合は std::nullopt、成功時は裏返した枚数を返す。
[[nodiscard]] std::optional<int> placeAndFlip(Board& board, Position pos, Color mover);

// 盤面全体で capturer に挟まれている敵石をすべて返す（重複なし、行優先順）。
// 返す座標に保護された石は含まない。
[[nodiscard]] std::vector<Position> findCapturable(const Board& board, Color capturer);

// findCapturable の石をすべて取り除き、取り除いた枚数を返す。
int captureAll(Board& board, Color capturer);

} // namespace cardflow::rules
