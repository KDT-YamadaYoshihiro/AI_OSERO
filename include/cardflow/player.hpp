#pragma once

#include <vector>

#include "cardflow/card.hpp"
#include "cardflow/types.hpp"

namespace cardflow {

// プレイヤー（色・EP・手札）
class Player {
public:
    static constexpr int kEpPerTurn = 3;  // ターン開始時の EP 回復量
    static constexpr int kMaxEp = 6;      // EP の上限

    explicit Player(Color color) noexcept : m_color(color) {}

    [[nodiscard]] Color color() const noexcept { return m_color; }
    [[nodiscard]] int ep() const noexcept { return m_ep; }
    [[nodiscard]] const std::vector<CardType>& hand() const noexcept { return m_hand; }

    // ─── EP ──────────────────────────────────────────────────────────
    // ターン開始時の回復（+3、上限 6）
    void recoverEp() noexcept;
    // 任意量の回復（Restore 等）。負の値は無視、上限 6
    void gainEp(int amount) noexcept;
    [[nodiscard]] bool canAfford(int cost) const noexcept;
    // EP を支払う。不足・負のコストなら何もせず false
    [[nodiscard]] bool spendEp(int cost) noexcept;

    // ─── 手札 ────────────────────────────────────────────────────────
    [[nodiscard]] bool hasCard(CardType type) const noexcept;
    // 手札に加える。上限（3 枚）に達していれば false
    [[nodiscard]] bool addCard(CardType type);
    // 該当カードを 1 枚取り除く。持っていなければ false
    [[nodiscard]] bool removeCard(CardType type);

private:
    Color m_color;
    int m_ep = 0;
    std::vector<CardType> m_hand;
};

} // namespace cardflow
