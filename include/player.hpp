#pragma once

#include "board.hpp"
#include <string>
#include <vector>

namespace game {

// ─── Player クラス ──────────────────────────────────────────────────────

class Player {
public:
    std::string color;          // "black" or "white"
    int energy_points = 0;      // EP：初期は 0、毎ターン +3（上限 6）
    std::vector<Card*> hand{};  // 手札（最大 3 枚、使い切ったら元に戻る）

    /**
     * @brief コンストラクタ。EP は初期値 0 でスタート。
     */
    Player(const std::string& color)
        : color(color), energy_points(0) {}

    /**
     * @brief ターン開始時に EP を回復する（+3、上限 6）
     */
    void recover_ep() {
        if (energy_points < 6) {
            ++energy_points;
        }
    }

    /**
     * @brief カードをプレイし、コストを支払う
     *        @param card プレイヤーの手札にあるカードのポインタ
     */
    void play_card(Card& card);

    /**
     * @brief 手札から指定されたカードを除去する（プレイ済み）
     */
    void discard_from_hand(std::vector<Card*>& hand) {
        // 簡易実装：hand に入ったカードは「使用済み」としてマークする
        for (auto& card : hand) {
            if (!card || !card->name.empty()) {
                // カードを「使用済み」に設定（リサイクル可能なものもある）
            }
        }
    }

    /**
     * @brief 手札からカードを検索する
     */
    [[nodiscard]] Card* find_card(const std::string& name) const;

private:
    // プライベートメソッド：簡易実装用
};

// ─── Game エンジン（ゲームループ・ターン管理）───────────────────────────

class Game {
public:
    Board board_;                // 8×8 ボード
    Player black_;              // Black プレイヤー
    Player white_;              // White プレイヤー
    int round{1};               // 現在のラウンド数（0-based）
    std::string winner{};       // "black" / "white" / ""（まだ終了してないなら空）

    /**
     * @brief ゲームを開始する。中央 4 マスに黒・白の石を交互に配置する。
     */
    void start() {
        // 中央 4 マス：(3,3), (3,4), (4,3), (4,4) に黒・白が交互に置かれる
        board_.place_stone("black", 3, 3);   // 黒
        board_.place_stone("white", 3, 4);    // 白
        board_.place_stone("black", 4, 3);    // 黒
        board_.place_stone("white", 4, 4);    // 白
    }

    /**
     * @brief Black のターンを処理する
     */
    void black_turn() {
        black_.recover_ep();
        // TODO: UI で「どのカードを出すか」を受け付ける
        if (!has_valid_move(board_)) {
            // 有効な移動ができない → パスターン（EP は回復済みなので OK）
        } else {
            white_.turn();
        }

        check_game_over();
    }

    /**
     * @brief White のターンを処理する
     */
    void white_turn() {
        white_.recover_ep();
        // TODO: UI で「どのカードを出すか」を受け付ける
        if (!has_valid_move(board_)) {
            // 有効な移動ができない → パスターン
        } else {
            black_.turn();
        }

        check_game_over();
    }

    /**
     * @brief Black のターンに有効な移動があるか判定する
     */
    [[nodiscard]] bool has_valid_move(const Board& board) const;

    /**
     * @brief White のターンに有効な移動があるか判定する
     */
    [[nodiscard]] bool has_valid_move_white() const;

private:
    void check_game_over();  // ゲーム終了条件をチェックして勝敗を宣言する
};

} // namespace game
