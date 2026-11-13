#pragma once

#include <array>
#include <optional>
#include <variant>

#include "cardflow/stone.hpp"
#include "cardflow/types.hpp"

namespace cardflow {

// 8×8 の盤面。各マスは「空」「石」「壁」のいずれか。
// 盤面外の座標を渡した場合、判定系は false / std::nullopt を返し、
// 変更系は何もせず false を返す（例外は投げない）。
class Board {
public:
    static constexpr int kSize = 8;

    // 8 方向（上下左右＋斜め）の相対オフセット
    static constexpr std::array<Position, 8> kDirections{{
        {-1, -1}, {-1, 0}, {-1, 1},
        { 0, -1},          { 0, 1},
        { 1, -1}, { 1, 0}, { 1, 1},
    }};

    // 空の盤面を作る
    Board() = default;

    // 通常のオセロの初期配置: d4 = 白, e4 = 黒, d5 = 黒, e5 = 白
    [[nodiscard]] static Board createInitial();

    [[nodiscard]] static constexpr bool isInside(Position pos) noexcept
    {
        return pos.row >= 0 && pos.row < kSize && pos.col >= 0 && pos.col < kSize;
    }

    [[nodiscard]] static constexpr bool isCorner(Position pos) noexcept
    {
        return (pos.row == 0 || pos.row == kSize - 1) && (pos.col == 0 || pos.col == kSize - 1);
    }

    // ─── 参照 ────────────────────────────────────────────────────────
    [[nodiscard]] bool isEmpty(Position pos) const;
    [[nodiscard]] bool isWall(Position pos) const;
    [[nodiscard]] std::optional<Stone> stoneAt(Position pos) const;
    // 8 方向のどこかに石があるか
    [[nodiscard]] bool hasAdjacentStone(Position pos) const;

    [[nodiscard]] int countStones(Color color) const;
    [[nodiscard]] int countEmpty() const;

    // ─── 変更（成功時 true）────────────────────────────────────────────
    // 空マスに石を置く
    [[nodiscard]] bool placeStone(Position pos, Color color);
    // 空マスを壁にする
    [[nodiscard]] bool placeWall(Position pos);
    // 既存の石を差し替える（色の変更・守りの付け外し）
    [[nodiscard]] bool setStone(Position pos, const Stone& stone);

private:
    using Cell = std::variant<std::monostate, Stone, Wall>;

    [[nodiscard]] const Cell& cellAt(Position pos) const;
    [[nodiscard]] Cell& cellAt(Position pos);

    std::array<std::array<Cell, kSize>, kSize> m_cells{};
};

} // namespace cardflow
