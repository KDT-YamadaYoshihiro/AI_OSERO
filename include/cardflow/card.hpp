#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace cardflow {

// カードの種類（効果の実装は今後追加）
enum class CardType : std::uint8_t {
    Place,
    Move,
    Cover,
    Burst,
    Swap,
    Block,
    Shield,
    Reversal,
    Reserve,
    Restore,
};

// カードのカテゴリ
enum class CardCategory : std::uint8_t {
    Green,   // 基礎操作
    Red,     // 攻撃・展開
    Blue,    // 防御・ブロック
    Violet,  // 特殊操作
    Orange,  // 補助・強化
    Yellow,  // 調整・回復
};

// カードの静的情報
struct CardInfo {
    CardType type;
    std::string_view name;
    CardCategory category;
    int cost;  // 発動に必要な EP
};

inline constexpr std::size_t kCardTypeCount = 10;
inline constexpr std::size_t kHandSize = 3;  // 手札の最大枚数

// CardType の宣言順に並べたカード表（仕様書 §4 / 憲章 Article II.B）
inline constexpr std::array<CardInfo, kCardTypeCount> kCardTable{{
    {CardType::Place,    "Place",    CardCategory::Green,  2},
    {CardType::Move,     "Move",     CardCategory::Green,  3},
    {CardType::Cover,    "Cover",    CardCategory::Green,  2},
    {CardType::Burst,    "Burst",    CardCategory::Red,    4},
    {CardType::Swap,     "Swap",     CardCategory::Red,    3},
    {CardType::Block,    "Block",    CardCategory::Blue,   3},
    {CardType::Shield,   "Shield",   CardCategory::Blue,   2},
    {CardType::Reversal, "Reversal", CardCategory::Violet, 4},
    {CardType::Reserve,  "Reserve",  CardCategory::Orange, 1},
    {CardType::Restore,  "Restore",  CardCategory::Yellow, 1},
}};

[[nodiscard]] constexpr const CardInfo& cardInfo(CardType type) noexcept
{
    return kCardTable[static_cast<std::size_t>(type)];
}

// 手札 1 組あたりのカテゴリ上限（仕様書 Clarifications Q2）
[[nodiscard]] constexpr int maxCardsPerCategory(CardCategory category) noexcept
{
    switch (category) {
    case CardCategory::Green:
    case CardCategory::Red:
    case CardCategory::Blue:
        return 2;
    case CardCategory::Violet:
    case CardCategory::Orange:
    case CardCategory::Yellow:
        return 1;
    }
    return 0;
}

// 手札が枚数上限とカテゴリ上限を満たしているか
[[nodiscard]] bool isBalancedHand(std::span<const CardType> hand) noexcept;

} // namespace cardflow
