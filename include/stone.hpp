#pragma once

#include <cstdint>
#include <string>
#include <optional>

namespace game {

// ─── 石の種類（StoneType）───────────────────────────────────────────────

enum class StoneType {
    Empty,   // 空マス
    Black,   // 黒の石
    White,   // 白の石
    Wall     // 壁（ブロックされたマス）
};

// ─── Stone クラス ──────────────────────────────────────────────────────

/**
 * @brief ゲームボード上の「石」を表すデータ構造体
 *        frozen=True でコンパイル時評価、immutable なオブジェクトとして扱う。
 */
struct Stone {
    StoneType type{};           // 0:Empty, 1:Black, 2:White, 3:Wall
    std::string color;          // "black" or "white"
    int row{0};                 // 行（0～7）
    int col{0};                 // 列（0～7）
    bool covered = false;       // 「Cover」カードで保護されているか
    bool shielded = false;      // 「Shield」カードでシールドがかかっているか

    /**
     * @brief コンストラクタ。デフォルトは空マスとして初期化される。
     */
    [[nodiscard]] static constexpr Stone empty() { return Stone{.type = StoneType::Empty, .row = -1, .col = -1}; }

    /**
     * @brief 黒の石を生成する
     */
    [[nodiscard]] static constexpr Stone make_black(int row, int col) {
        return Stone{
            .type = StoneType::Black,
            .color = "black",
            .row = row,
            .col = col,
            .covered = false,
            .shielded = false
        };
    }

    /**
     * @brief 白の石を生成する
     */
    [[nodiscard]] static constexpr Stone make_white(int row, int col) {
        return Stone{
            .type = StoneType::White,
            .color = "white",
            .row = row,
            .col = col,
            .covered = false,
            .shielded = false
        };
    }

    /**
     * @brief 壁を生成する
     */
    [[nodiscard]] static constexpr Stone make_wall(int row, int col) {
        return Stone{
            .type = StoneType::Wall,
            .color = "",
            .row = row,
            .col = col,
            .covered = false,
            .shielded = false
        };
    }

    /**
     * @brief カバー状態を付与する
     */
    void cover() { covered = true; }

    /**
     * @brief シールド状態を付与する
     */
    void shield() { shielded = true; }

    /**
     * @brief カバーが解除されたか判定
     */
    [[nodiscard]] bool is_covered() const { return covered && type != StoneType::Empty; }

    /**
     * @brief シールドが有効かどうかを判定
     */
    [[nodiscard]] bool has_shield() const { return shielded && type != StoneType::Empty; }

    /**
     * @brief 石が「壁」か判定する
     */
    [[nodiscard]] bool is_wall() const { return type == StoneType::Wall; }

    /**
     * @brief 石が「空マス」か判定する
     */
    [[nodiscard]] bool is_empty() const { return type == StoneType::Empty; }
};

} // namespace game
