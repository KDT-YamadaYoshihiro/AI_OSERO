#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "cardflow/card.hpp"

namespace cardflow {

// 両プレイヤー共有の山札（使い切り。捨て札は持たない）
class Deck {
public:
    static constexpr int kCopiesPerCard = 3;  // 5 種 × 3 枚 = 15 枚

    // 標準構成の山札をシャッフルして作る（seed で再現可能）
    [[nodiscard]] static Deck createStandard(std::uint32_t seed);

    // 任意の並びの山札を作る（テスト・中断からの再開用）。末尾から引く
    explicit Deck(std::vector<CardType> pile) : m_pile(std::move(pile)) {}

    // 残りの札（末尾が次に引く札）
    [[nodiscard]] const std::vector<CardType>& cards() const noexcept { return m_pile; }
    [[nodiscard]] std::size_t size() const noexcept { return m_pile.size(); }
    [[nodiscard]] bool empty() const noexcept { return m_pile.empty(); }

    // 1 枚引く。空なら std::nullopt
    [[nodiscard]] std::optional<CardType> draw();

private:
    std::vector<CardType> m_pile;
};

} // namespace cardflow
