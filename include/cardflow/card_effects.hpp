#pragma once

#include <cstdint>
#include <span>

#include "cardflow/board.hpp"
#include "cardflow/card.hpp"
#include "cardflow/player.hpp"
#include "cardflow/types.hpp"

// カード効果と発動手順（仕様書 Clarifications「カード効果の確定」）
//
// 対象（targets）の指定:
//   Place    : 異なる空マス 2 つ
//   Move     : { 自分の石, 移動先 }（直線 1〜3 マス、途中と移動先は空マス）
//   Swap     : { 自分の石, 隣接する敵石 }
//   Block    : 空マス 1 つ
//   Shield   : Shield の付いていない自分の石 1 つ
//   Reversal : 隣接する敵石を持つ自分の石 1 つ
//   Cover / Burst / Reserve / Restore : 対象なし（空）
namespace cardflow {

enum class PlayStatus : std::uint8_t {
    Ok,
    NotInHand,      // 手札（Reserve の追加分を含む）にない
    NotEnoughEp,    // EP 不足
    InvalidTarget,  // 対象の数・位置が不正、または発動条件を満たさない
};

struct PlayResult {
    PlayStatus status = PlayStatus::Ok;
    int flipped = 0;         // 裏返した敵石の数
    int captured = 0;        // 取り除いた石の数（Swap では自分の石の場合もある）
    int blockedByCover = 0;  // Cover で防がれた数
    int shieldsBroken = 0;   // Shield が外れて防がれた数

    [[nodiscard]] constexpr bool ok() const noexcept { return status == PlayStatus::Ok; }
};

// 対象が正しく、効果を発動できるか（盤面・プレイヤーは変更しない）
[[nodiscard]] bool canApplyEffect(CardType type, const Board& board, const Player& actor,
                                  std::span<const Position> targets);

// 効果だけを適用する（手札・EP は扱わない）。不正なら何も変更せず InvalidTarget
PlayResult applyEffect(CardType type, Board& board, Player& actor, std::span<const Position> targets);

// カードを発動する: 手札 → EP → 対象 の順に確認し、すべて満たせば
// EP を支払い、手札から 1 枚除き（追加 Place を優先）、効果を適用する。
// 失敗時は何も変更しない。
PlayResult playCard(CardType type, Board& board, Player& actor, std::span<const Position> targets);

// ターン開始処理: 自分の石の Cover を解除 → EP 回復 → Reserve の予約分を追加
void startTurn(Board& board, Player& player);

} // namespace cardflow
