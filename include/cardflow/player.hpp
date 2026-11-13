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

    // ─── Reserve（次ターンの Place 追加）────────────────────────────────
    static constexpr int kMaxReservedPlaces = 2;

    // 次のターンに追加される Place の予約数
    [[nodiscard]] int reservedPlaces() const noexcept { return m_reservedPlaces; }
    // このターンだけ使える追加 Place の枚数（手札上限とは別枠）
    [[nodiscard]] int bonusPlaces() const noexcept { return m_bonusPlaces; }
    // 予約を加える（合計は最大 2）
    void reservePlaces(int count) noexcept;
    // ターン開始時: 前ターンの追加分を破棄し、予約分を追加分に移す
    void activateReservedPlaces() noexcept;
    // 追加 Place を 1 枚使う。なければ false
    [[nodiscard]] bool consumeBonusPlace() noexcept;

private:
    Color m_color;
    int m_ep = 0;
    std::vector<CardType> m_hand;
    int m_reservedPlaces = 0;
    int m_bonusPlaces = 0;
};

} // namespace cardflow
