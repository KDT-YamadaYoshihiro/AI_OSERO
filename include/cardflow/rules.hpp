#pragma once

#include <optional>
#include <span>
#include <vector>

#include "cardflow/board.hpp"
#include "cardflow/types.hpp"

// 挟み・裏返し・捕獲のルール（仕様書 Clarifications Session 2026-11-13）
//
// 「挟み」: 縦・横・斜めの一直線上で、連続する敵石の列の両端が
//           「自分の石」または「壁（設置者を問わない）」であること。
//           盤の端は挟む側にならない。
// 保護（Q7）: 保護された石も列の一部として数える。裏返し・捕獲を受けたとき、
//           Cover の石は効果を受けず、Shield の石は Shield が外れるだけで済む。
namespace cardflow::rules {

// 裏返し・捕獲を実際に適用した結果
struct StrikeResult {
    int affected = 0;        // 実際に裏返した／取り除いた石の数
    int blockedByCover = 0;  // Cover で防がれた数
    int shieldsBroken = 0;   // Shield が外れて防がれた数

    friend constexpr bool operator==(const StrikeResult&, const StrikeResult&) = default;
};

// pos に置かれた（置かれると仮定した）mover の石を起点に、8 方向で挟まれる敵石を返す。
// pos 自体の状態は見ない。保護された石も含む。
[[nodiscard]] std::vector<Position> findFlips(const Board& board, Position pos, Color mover);

// 盤面全体で capturer に挟まれている敵石をすべて返す（重複なし、行優先順）。保護された石も含む。
[[nodiscard]] std::vector<Position> findCapturable(const Board& board, Color capturer);

// pos の敵石が capturer に挟まれているか
[[nodiscard]] bool isSandwiched(const Board& board, Position pos, Color capturer);

// 指定の石を、保護を考慮して裏返す／取り除く
StrikeResult strikeFlip(Board& board, std::span<const Position> targets);
StrikeResult strikeRemove(Board& board, std::span<const Position> targets);

// 空マス pos に mover の石を置き、挟んだ敵石を裏返す。置けなければ std::nullopt。
[[nodiscard]] std::optional<StrikeResult> placeAndFlip(Board& board, Position pos, Color mover);

// mover の石が pos にある前提で、そこを起点に挟んだ敵石を裏返す（Move の着地用）
StrikeResult flipFrom(Board& board, Position pos, Color mover);

// capturer に挟まれている敵石をすべて捕獲する
StrikeResult captureAll(Board& board, Color capturer);

} // namespace cardflow::rules
