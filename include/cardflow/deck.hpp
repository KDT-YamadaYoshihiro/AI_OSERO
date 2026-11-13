#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <vector>

#include "cardflow/card.hpp"
#include "cardflow/player.hpp"

namespace cardflow {

// 両プレイヤー共有の山札と捨て札（仕様書「ゲーム進行の確定」）
class Deck {
public:
    static constexpr int kCopiesPerCard = 3;  // 10 種 × 3 枚 = 30 枚

    // 標準構成の山札をシャッフルして作る（seed で再現可能）
    [[nodiscard]] static Deck createStandard(std::uint32_t seed);

    // 任意の並びの山札を作る（テスト用）。drawPile の末尾から引く
    Deck(std::vector<CardType> drawPile, std::uint32_t seed);

    [[nodiscard]] std::size_t drawPileSize() const noexcept { return m_drawPile.size(); }
    [[nodiscard]] std::size_t discardPileSize() const noexcept { return m_discardPile.size(); }

    // 1 枚引く。山札が空なら捨て札をシャッフルして戻す。両方空なら std::nullopt
    [[nodiscard]] std::optional<CardType> draw();

    // 使ったカードを捨て札に置く
    void discard(CardType type);

    // 手札が上限（3 枚）になるまで引く。カテゴリ上限を超える札は脇に置いて引き直し、
    // 最後に山札へ戻してシャッフルする。引いた枚数を返す
    int refillHand(Player& player);

private:
    void shuffleDrawPile();

    std::vector<CardType> m_drawPile;
    std::vector<CardType> m_discardPile;
    std::mt19937 m_rng;
};

} // namespace cardflow
