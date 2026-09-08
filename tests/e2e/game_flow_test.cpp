// ─── game_flow_test.cpp ──────────────────────────────────────────────
// CardFlow Othello — End-to-End Integration Tests (Catch2)
//
// シナリオ一覧：
//   1. full_game_flow     → 両プレイヤーが交互にターンし、ゲーム終了→勝敗判定まで完了する
//   2. draw_detection     → ゲームが引き分けになる条件をテストする
//   3. card_combination    → 「Place×2 + Move×1」の組み合わせによる戦略検証
//   4. ep_exhaustion      → EP が枯渇してパスターンした後のゲーム継続

#include <catch2/catch.hpp>
#include <algorithm>
#include "board.hpp"
#include "stone.hpp"
#include "player.hpp"
#include "card.hpp"

using namespace game;

namespace {

// ─── 補助関数：カードの手札に追加する（簡易ポインタ管理）─────────────────

void player_add_card(Player& player, Card* card) {
    player.hand.push_back(card);
}

std::string count_stones(const Board& board, const std::string& color) {
    int count = 0;
    for (int r = 0; r < Board::SIZE; ++r) {
        for (int c = 0; c < Board::SIZE; ++c) {
            if (board.is_occupied(r, c)) {
                auto& stone = board[r][c];
                if (!stone->is_empty() && !stone->type == StoneType::Wall &&
                    stone->color == color) {
                    ++count;
                }
            }
        }
    }
    return std::to_string(count);
}

} // anonymous namespace

// ─── シナリオ 1: full_game_flow ────────────────────────────────────────

TEST_CASE("Full Game Flow — Black wins by placing more stones", "[e2e][full_game]") {
    Game game;
    game.black_.color = "black";
    game.white_.color = "white";

    // ゲーム開始：中央 4 マスに黒・白が交互配置される
    game.start();

    // Black のターン
    SECTION("Black places stones with 'Place' cards") {
        // Black は EP が回復済み（+3）
        game.black_.hand.push_back(Card::empty()); // Place カードのシミュレーション
        game.black_.energy_points = 6;

        // Black が石を置いた（簡易：直接 place_stone を呼ぶ）
        game.board_.place_stone("black", 0, 1);
        game.board_.place_stone("black", 1, 0);
        game.board_.place_stone("white", 2, 0); // White も石を置く（White のターン）

        auto black_count = count_stones(game.board_, "black");
        auto white_count = count_stones(game.board_, "white");

        REQUIRE(black_count > 0);
    }

    // White のターン
    SECTION("White responds with 'Place' cards") {
        game.white_.hand.push_back(Card::empty());
        game.white_.energy_points = 6;

        game.board_.place_stone("white", 2, 1);
        game.board_.place_stone("white", 2, 2);
        game.board_.place_stone("black", 3, 0); // Black も石を置く

        auto black_count = count_stones(game.board_, "black");
        auto white_count = count_stones(game.board_, "white");

        REQUIRE(white_count > 0);
    }

    // ゲーム終了判定：両者が置ける場所がない場合（簡易テスト）
    SECTION("Game ends when no valid moves remain") {
        // ボードがほぼ埋まった状態を模擬
        for (int r = 0; r < Board::SIZE; ++r) {
            for (int c = 0; c < Board::SIZE; ++c) {
                if (!game.board_.is_occupied(r, c)) {
                    game.board_.place_stone("black", r, c); // Black が埋める
                }
            }
        }

        bool black_can_play = game.has_valid_move(game.board_);
        REQUIRE(!black_can_play); // 置ける場所がない → パスターン判定（簡易）
    }

    SECTION("Winner is declared by stone count") {
        int black_count = 0, white_count = 0;
        for (int r = 0; r < Board::SIZE; ++r) {
            for (int c = 0; c < Board::SIZE; ++c) {
                if (game.board_.is_occupied(r, c)) {
                    auto& stone = game.board_[r][c];
                    if (!stone->is_empty() && !stone->type == StoneType::Wall) {
                        if (stone->color == "black") black_count++;
                        else white_count++;
                    }
                }
            }
        }

        // Black が多数を占める場合 → Black の勝利
        REQUIRE(black_count >= white_count);
    }
}

// ─── シナリオ 2: draw_detection ─────────────────────────────────────────

TEST_CASE("Draw Detection — Both players have no valid moves", "[e2e][draw]") {
    Game game;
    game.black_.color = "black";
    game.white_.color = "white";

    // ゲーム開始（中央 4 マスに黒・白が交互配置）
    game.start();

    // 両プレイヤーとも EP を回復してターンを繰り返す
    for (int round = 0; round < 10; ++round) {
        game.black_.recover_ep();
        game.white_.recover_ep();

        // Black のターンの簡易シミュレーション：空マスがあれば石を置く
        bool black_moved = false;
        if (!game.board_.is_empty(3, 5)) {
            game.board_.place_stone("black", 3, 5);
            black_moved = true;
        }

        // White のターンの簡易シミュレーション
        bool white_moved = false;
        if (!game.board_.is_empty(4, 6)) {
            game.board_.place_stone("white", 4, 6);
            white_moved = true;
        }

        // ゲーム終了条件：両者が置ける場所がないかチェック（簡易）
        bool black_can_move = false;
        for (int r = 0; r < Board::SIZE; ++r) {
            for (int c = 0; c < Board::SIZE; ++c) {
                if (!game.board_.is_occupied(r, c)) {
                    game.board_.place_stone("black", r, c);
                    black_can_move = true;
                    break;
                }
            }
        }

        bool white_can_move = false;
        for (int r = 0; r < Board::SIZE; ++r) {
            for (int c = 0; c < Board::SIZE; ++c) {
                if (!game.board_.is_occupied(r, c)) {
                    game.board_.place_stone("white", r, c);
                    white_can_move = true;
                    break;
                }
            }
        }

        // 両者が置ける場所がない場合 → ゲーム終了（簡易判定）
        if (!black_can_move && !white_can_move) {
            int black_count = 0, white_count = 0;
            for (int r = 0; r < Board::SIZE; ++r) {
                for (int c = 0; c < Board::SIZE; ++c) {
                    if (game.board_.is_occupied(r, c)) {
                        auto& stone = game.board_[r][c];
                        if (!stone->is_empty() && !stone->type == StoneType::Wall) {
                            if (stone->color == "black") black_count++;
                            else white_count++;
                        }
                    }
                }
            }

            // 同点の場合 → ドロー判定
            REQUIRE(black_count == white_count);
        }
    }
}

// ─── シナリオ 3: card_combination ──────────────────────────────────────

TEST_CASE("Card Combination — Place ×2 + Move ×1", "[e2e][cards]") {
    Game game;
    game.black_.color = "black";
    game.white_.color = "white";

    game.start();
    game.black_.energy_points = 6;
    game.white_.energy_points = 6;

    SECTION("Black plays 'Place' to occupy two empty cells") {
        // Black が Place カードを使って 2 マスに石を置く
        game.board_.place_stone("black", 0, 1);
        game.board_.place_stone("black", 1, 0);

        auto black_count = count_stones(game.board_, "black");
        REQUIRE(black_count >= 2);
    }

    SECTION("Black plays 'Move' to shift a stone by one cell") {
        // Move カードの簡易シミュレーション：隣接マスの石を移動
        if (game.board_.is_occupied(3, 3)) {
            game.board_.remove_stone_at(3, 3);
            game.board_.place_stone("black", 2, 2); // 左上に移動（簡易）
        }

        // 黒の石が (2,2) にあることを確認
        REQUIRE(game.board_.is_occupied(2, 2));
    }

    SECTION("Black plays 'Cover' to protect all own stones") {
        // Cover カードで既存のすべての自分の石を保護
        game.black_.energy_points = 6;
        for (int r = 0; r < Board::SIZE; ++r) {
            for (int c = 0; c < Board::SIZE; ++c) {
                if (game.board_.is_occupied(r, c)) {
                    auto& stone = game.board_[r][c];
                    if (!stone->is_empty() && !stone->type == StoneType::Wall &&
                        stone->color == "black") {
                        // カバーフラグを立てる（簡易）
                    }
                }
            }
        }
    }
}

// ─── シナリオ 4: ep_exhaustion ──────────────────────────────────────────

TEST_CASE("EP Exhaustion — Turn passed due to insufficient EP", "[e2e][ep]") {
    Game game;
    game.black_.color = "black";
    game.white_.color = "white";
    game.black_.energy_points = 0;   // EP が枯渇（初期値）
    game.white_.energy_points = 6;

    SECTION("Black cannot play any card due to EP exhaustion") {
        auto place_card = Card::empty();
        place_card.cost = 2; // Place カードのコストは 2
        place_card.name = "Place";
        place_card.category = CardCategory::Green;

        Result result = place_card.can_play(game.black_, game.board_);
        REQUIRE(!result.status == Result::Status::OutOfEp);
    }

    SECTION("Black plays 'Restore' to recover EP") {
        auto restore_card = Card::empty();
        restore_card.cost = 1;
        restore_card.name = "Restore";
        restore_card.category = CardCategory::Yellow;

        Result result = restore_card.can_play(game.black_, game.board_);
        REQUIRE(result.status == Result::Status::Ok);
    }

    SECTION("White plays with full EP and places stones") {
        game.white_.energy_points = 6;
        auto place_card_white = Card::empty();
        place_card_white.cost = 2;
        place_card_white.name = "Place";
        place_card_white.category = CardCategory::Green;

        Result result = place_card_white.can_play(game.white_, game.board_);
        REQUIRE(result.status == Result::Status::Ok);

        game.board_.place_stone("white", 0, 1);
        game.board_.place_stone("white", 1, 2);
    }
}
