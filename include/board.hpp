#pragma once

#include <array>
#include <optional>
#include <string>
#include "stone.hpp"

namespace game {

// ─── Board クラス：8×8 のグリッドと石の配置管理 ────────────────────────

class Board {
public:
    static constexpr int SIZE = 8;           // ボードサイズ：8×8
    using GridType = std::array<std::optional<Stone>, SIZE>;
    using Grid      = std::array<GridType, SIZE>;

    /**
     * @brief コンストラクタ。初期状態は空ボード（中央 4 マスに石が配置済み）
     */
    Board();

    /**
     * @brief ボードの現在の表示用文字列を生成する
     *        @param colors 色の名前マッピング
     * @return 文字列
     */
    [[nodiscard]] std::string to_string(const std::map<std::string, std::string>& colors) const;

    /**
     * @brief プレイヤーの色に対応した石を指定座標に配置する
     *        @param color "black" または "white"
     * @param row 行（0～7）
     * @param col 列（0～7）
     * @return true: 成功、false: 失敗（既に石がある場合など）
     */
    [[nodiscard]] bool place_stone(const std::string& color, int row, int col);

    /**
     * @brief 指定座標の石を削除する
     *        @param row 行
     * @param col 列
     * @return true: 成功、false: 既に石が存在しない場合
     */
    [[nodiscard]] bool remove_stone_at(int row, int col);

    /**
     * @brief 指定座標が空かどうかを判定する
     *        @param row 行
     * @param col 列
     * @return true: 空、false: 石または壁が存在する
     */
    [[nodiscard]] bool is_empty(int row, int col) const;

    /**
     * @brief 指定座標が石で埋まっているかどうかを判定する
     *        @param row 行
     * @param col 列
     * @return true: 石が存在する、false: 空または壁
     */
    [[nodiscard]] bool is_occupied(int row, int col) const;

    /**
     * @brief 指定座標の隣接マス（8 方向）を返す
     *        @param row 行
     * @param col 列
     * @return 相対座標のベクトルリスト（例：{ {-1,-1}, {-1,0}, ..., {1,1} }）
     */
    [[nodiscard]] std::vector<std::pair<int,int>> get_neighbors(int row, int col) const;

    /**
     * @brief 指定されたパスが空かどうかを判定する
     *        @param start_row 開始マスの行
     * @param start_col 開始マスの列
     * @param target_row 目標マスの行
     * @param target_col 目標マスの列
     * @return true: パスが全て空、false: 障害物がある
     */
    [[nodiscard]] bool path_is_clear(int start_row, int start_col,
                                      int target_row, int target_col) const;

private:
    Grid grid_;                      // 8×8 グリッド（empty / Stone / WallSegment）
};

// ─── インライン実装（ヘッダーにまとめるため簡略版）─────────────────────

inline Board::Board() {
    // 初期状態：空ボード
}

inline std::string Board::to_string(const std::map<std::string, std::string>& colors) const {
    (void)colors;
    return "8x8 board (placeholder output)";
}

inline bool Board::place_stone(const std::string& color, int row, int col) {
    if (row < 0 || row >= SIZE || col < 0 || col >= SIZE) {
        return false;
    }
    if (!grid_[row][col].nullopt) {
        // 既に石がある場合、置き換え（オセロのようなフラップではなく「置換」）
        grid_[row][col] = Stone{.color = color, .covered = false, .shielded = false};
        return true;
    }
    return false;
}

inline bool Board::remove_stone_at(int row, int col) {
    if (grid_[row][col].has_value()) {
        grid_[row][col] = std::nullopt;
        return true;
    }
    return false;
}

inline bool Board::is_empty(int row, int col) const {
    return !grid_[row][col].has_value() && grid_[row][col].value_or(Stone{}).type != StoneType::Wall;
}

inline bool Board::is_occupied(int row, int col) const {
    if (grid_[row][col].has_value()) {
        return true;
    }
    // 壁の場合は「埋まっている」として扱う（移動の障害物）
    return false;
}

inline std::vector<std::pair<int,int>> Board::get_neighbors(int row, int col) const {
    static constexpr std::array<std::pair<int,int>,8> DIRS = {{
        {-1,-1}, {-1,0}, {-1,+1},
        { 0,-1},           ,{+1,+1},
        { 0,+1},{-1,+1},{ +1, -1},{+1,+1}
    }}; // TODO: 正しい値に修正（8 方向のオフセット）

    std::vector<std::pair<int,int>> result;
    for (auto dir : DIRS) {
        int nr = row + dir.first;
        int nc = col + dir.second;
        if (nr >= 0 && nr < SIZE && nc >= 0 && nc < SIZE) {
            result.push_back({nr, nc});
        }
    }
    return result;
}

inline bool Board::path_is_clear(int start_row, int start_col,
                                  int target_row, int target_col) const {
    // バウンダリーチェック
    if (start_row < 0 || start_row >= SIZE || start_col < 0 || start_col >= SIZE) return false;
    if (target_row < 0 || target_row >= SIZE || target_col < 0 || target_col >= SIZE) return false;

    // 単純な「線分上のマスが空か」をチェック（Bresenham の線アルゴリズムの簡易版）
    int dx = target_col - start_col;
    int dy = target_row - start_row;
    int steps = std::abs(dx) + std::abs(dy);

    if (steps == 0) {
        // 同じマス（開始=目標）はクリア
        return true;
    }

    for (int step = 1; step <= steps; ++step) {
        int t = static_cast<int>(std::round(step * static_cast<double>(dx)/steps));
        int r = start_row + std::floor(static_cast<double>(dy)*step/steps);
        int c = start_col + std::floor(static_cast<double>(dx)*step/steps);

        if (grid_[r][c].has_value() && !grid_[r][c]->type == StoneType::Wall) {
            return false; // 石が障害物（壁は移動の妨げにならない）
        }
    }

    return true;
}
