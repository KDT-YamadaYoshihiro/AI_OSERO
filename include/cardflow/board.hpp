#pragma once

#include <array>
#include <optional>
#include <variant>
#include <vector>

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

    // 初期配置（中央 4 マスに黒白を交互配置）の盤面を作る
    //   d4 = 黒, e4 = 白, d5 = 白, e5 = 黒
    [[nodiscard]] static Board createInitial();

    [[nodiscard]] static constexpr bool isInside(Position pos) noexcept
    {
        return pos.row >= 0 && pos.row < kSize && pos.col >= 0 && pos.col < kSize;
    }

    // ─── 参照 ────────────────────────────────────────────────────────
    [[nodiscard]] bool isEmpty(Position pos) const;
    [[nodiscard]] bool hasStone(Position pos) const;
    [[nodiscard]] bool isWall(Position pos) const;
    [[nodiscard]] std::optional<Stone> stoneAt(Position pos) const;
    [[nodiscard]] std::optional<Wall> wallAt(Position pos) const;

    [[nodiscard]] int countStones(Color color) const;
    [[nodiscard]] int countEmpty() const;

    // 盤面内にある隣接マス（最大 8 個）
    [[nodiscard]] std::vector<Position> neighbors(Position pos) const;

    // ─── 変更（成功時 true）────────────────────────────────────────────
    // 空マスに石を置く
    [[nodiscard]] bool placeStone(Position pos, Color color);
    // 空マスに壁を置く
    [[nodiscard]] bool placeWall(Position pos, std::optional<Color> owner = std::nullopt);
    // 石を取り除く（壁は対象外）
    [[nodiscard]] bool removeStone(Position pos);
    // 石の色を反転する（保護状態は維持）
    [[nodiscard]] bool flipStone(Position pos);
    // 既存の石を差し替える（保護状態の付与・解除に使う）
    [[nodiscard]] bool replaceStone(Position pos, const Stone& stone);
    // 石を空マスへ移す（経路判定はカード側の責務）
    [[nodiscard]] bool moveStone(Position from, Position to);
    // 2 つの石を入れ替える
    [[nodiscard]] bool swapStones(Position a, Position b);

private:
    using Cell = std::variant<std::monostate, Stone, Wall>;

    [[nodiscard]] const Cell& cellAt(Position pos) const;
    [[nodiscard]] Cell& cellAt(Position pos);

    std::array<std::array<Cell, kSize>, kSize> m_cells{};
};

} // namespace cardflow
