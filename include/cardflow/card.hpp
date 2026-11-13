#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "cardflow/board.hpp"
#include "cardflow/types.hpp"

// カード 5 種（仕様書 1.3）
namespace cardflow {

enum class CardType : std::uint8_t {
    Double,  // 連打: このターン、石をもう 1 つ置ける
    Guard,   // 守り: 自分の石 1 つを 1 回だけ裏返らなくする
    Wall,    // 壁: 空マスを壁にする
    Flip,    // 反転: 相手の石 1 つを自分の色にする（角・守りの石は不可）
    Free,    // 自由: 次の 1 手は石の隣の空マスならどこにでも置ける
};

inline constexpr std::size_t kCardTypeCount = 5;

struct CardInfo {
    CardType type;
    std::string_view name;         // 画面に出す名前
    std::string_view description;  // 一行の説明
    bool needsTarget;              // 対象のマスを選ぶか
};

inline constexpr std::array<CardInfo, kCardTypeCount> kCardTable{{
    {CardType::Double, "連打", "もう1つ石を置ける", false},
    {CardType::Guard, "守り", "自分の石を1回守る", true},
    {CardType::Wall, "壁", "空マスを壁にする", true},
    {CardType::Flip, "反転", "相手の石を1つ奪う", true},
    {CardType::Free, "自由", "石の隣ならどこでも置ける", false},
}};

[[nodiscard]] constexpr const CardInfo& cardInfo(CardType type) noexcept
{
    return kCardTable[static_cast<std::size_t>(type)];
}

// 対象のマスとして選べるか（対象のないカードは常に false）
[[nodiscard]] bool isValidCardTarget(CardType type, const Board& board, Color user, Position target);

// 対象を取るカードの盤面への効果を適用する。対象が不正なら何もせず false
[[nodiscard]] bool applyCardTarget(CardType type, Board& board, Color user, Position target);

} // namespace cardflow
