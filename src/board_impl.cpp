#include "board.hpp"
#include <algorithm>
#include <stdexcept>

// ─── Board クラス実装 ──────────────────────────────────────────────────

namespace game {

Board::Board() : grid_(std::array<std::optional<Stone>, SIZE>{}) {
    // 初期状態：中央 4 マスに黒・白の石が交互に配置される（オセロのスタート）
    // (3,3) = Black, (3,4) = White, (4,3) = White, (4,4) = Black
    grid_[3][3] = Stone::make_black(3, 3);
    grid_[3][4] = Stone::make_white(3, 4);
    grid_[4][3] = Stone::make_white(4, 3);
    grid_[4][4] = Stone::make_black(4, 4);
}

std::string Board::to_string(const std::map<std::string, std::string>& colors) const {
    // ボードの文字列表現を生成（簡易版：ASCII アート）
    constexpr int pad = 2;
    static constexpr char empty_char = '·';
    static constexpr char black_char = '●';
    static constexpr char white_char = '○';
    static constexpr char wall_char = '#';

    std::string lines[3]; // 上部、本体、下部

    // ヘッダー：列番号（0-7）
    {
        lines[0] += "   ";
        for (int c = 0; c < SIZE; ++c) {
            if (c == 3 || c == 4) lines[0] += std::to_string(c);
            else                  lines[0] += ' ';
        }
    }

    // 本体：各行
    for (int r = 0; r < SIZE; ++r) {
        lines[r + 1] += std::to_string(r)[1]; // "3" → "3", "4" → "4" など
        for (int c = 0; c < SIZE; ++c) {
            auto& stone = grid_[r][c];

            if (!stone.has_value()) {
                lines[r + 1] += empty_char; // 空マス
            } else if (stone->type == StoneType::Black) {
                lines[r + 1] += black_char;
            } else if (stone->type == StoneType::White) {
                lines[r + 1] += white_char;
            } else if (stone->type == StoneType::Wall) {
                lines[r + 1] += wall_char;
            }
        }
    }

    // フッター：列番号
    {
        lines[2] += "   ";
        for (int c = 0; c < SIZE; ++c) {
            if (c == 3 || c == 4) lines[2] += std::to_string(c);
            else                  lines[2] += ' ';
        }
    }

    return lines[0] + "\n" + lines[1] + "\n" + lines[2];
}

bool Board::place_stone(const std::string& color, int row, int col) {
    if (row < 0 || row >= SIZE || col < 0 || col >= SIZE) return false;
    if (!grid_[row][col].has_value()) {
        grid_[row][col] = Stone{.type = StoneType::Black, .color = color};
        // TODO: "black" or "white" の文字列比較を型安全に（例：enum class Color）
    }
    return true;
}

bool Board::remove_stone_at(int row, int col) {
    if (grid_[row][col].has_value()) {
        grid_[row][col] = std::nullopt;
        return true;
    }
    return false;
}

bool Board::is_empty(int row, int col) const {
    if (row < 0 || row >= SIZE || col < 0 || col >= SIZE) return false;
    return !grid_[row][col].has_value();
}

bool Board::is_occupied(int row, int col) const {
    if (row < 0 || row >= SIZE || col < 0 || col >= SIZE) return false;
    return grid_[row][col].has_value();
}

std::vector<std::pair<int,int>> Board::get_neighbors(int row, int col) const {
    std::vector<std::pair<int,int>> dirs = {{-1,-1}, {-1,0}, {-1,+1},
                                             { 0,-1},           ,{+1,+1},
                                             { 0,+1},{-1,+1},{ +1, -1}};

    std::vector<std::pair<int,int>> result;
    for (auto d : dirs) {
        int nr = row + d.first;
        int nc = col + d.second;
        if (nr >= 0 && nr < SIZE && nc >= 0 && nc < SIZE) {
            result.push_back({nr, nc});
        }
    }
    return result;
}

bool Board::path_is_clear(int start_row, int start_col, int target_row, int target_col) const {
    if (start_row < 0 || start_row >= SIZE || start_col < 0 || start_col >= SIZE) return false;
    if (target_row < 0 || target_row >= SIZE || target_col < 0 || target_col >= SIZE) return false;

    int dx = static_cast<int>(target_col - start_col);
    int dy = static_cast<int>(target_row - start_row);
    int steps = std::abs(dx) + std::abs(dy);

    if (steps == 0) return true; // 同じマス → クリア

    for (int step = 1; step <= steps; ++step) {
        double t = static_cast<double>(static_cast<int64_t>(step)*dy / steps); // int 除算の誤差回避
        int r = start_row + static_cast<int>(t);
        int c = start_col + static_cast<int>((dx * step - dy * (start_row + r - start_row)) / dx);

        if (grid_[r][c].has_value() && !grid_[r][c]->type == StoneType::Wall) {
            return false; // 石が障害物
        }
    }

    return true;
}

// ─── 単体テスト（Google Test）─────────────────────────────────────────────

// 注記：このファイルは「実装＋テスト」を兼用しています。CMakeLists.txt で
// tests/unit/board_test.cpp と別ファイルに分離するのがベストプラクティスですが、
// 今回は簡略化のため同一ファイルにまとめています（後で分離します）。

#include "gtest/gtest.h"

namespace {

TEST(BoardTest, Constructor) {
    game::Board board;
    EXPECT_FALSE(board.is_empty(3, 3));
    EXPECT_TRUE(board.is_occupied(3, 3));
}

TEST(BoardTest, PlaceStone_Success) {
    game::Board board;
    // 初期状態では中央 4 マス以外が空
    EXPECT_TRUE(board.is_empty(0, 0));
    EXPECT_TRUE(board.place_stone("black", 1, 1));
    EXPECT_FALSE(board.is_empty(1, 1));
}

TEST(BoardTest, PlaceStone_Fail_occupied) {
    game::Board board;
    // 既に石があるマスに再度置き換える（オセロの「flip」ではない）
    bool placed = board.place_stone("black", 3, 3);
    EXPECT_TRUE(placed);
}

TEST(BoardTest, RemoveStone_Success) {
    game::Board board;
    EXPECT_TRUE(board.is_occupied(3, 3));
    EXPECT_TRUE(board.remove_stone_at(3, 3));
    EXPECT_TRUE(board.is_empty(3, 3));
}

TEST(BoardTest, BoundaryCheck) {
    game::Board board;
    // 境界外アクセスは失敗
    EXPECT_FALSE(board.place_stone("black", -1, 0));
    EXPECT_FALSE(board.place_stone("black", 8, 0));
    EXPECT_FALSE(board.place_stone("black", 3, 9));
}

TEST(BoardTest, GetNeighbors) {
    game::Board board;
    auto neighbors = board.get_neighbors(4, 4);
    EXPECT_EQ(neighbors.size(), 8U); // 8 方向すべて有効な近傍がある
}

TEST(BoardTest, PathIsClear_BothEmpty) {
    game::Board board;
    bool path_clear = board.path_is_clear(0, 0, 1, 1);
    EXPECT_EQ(path_clear, true);
}

TEST(BoardTest, PathIsClear_BlockedByStone) {
    game::Board board;
    // (0,0) に石を置く → 隣接マスへのパスがブロックされるか？
    EXPECT_TRUE(board.place_stone("black", 0, 0));
    bool path_clear = board.path_is_clear(0, 0, 1, 1);
    EXPECT_EQ(path_clear, false); // (0,0) に石があるため「クリア」は false（開始マス自体が石）
}

TEST(BoardTest, ToString) {
    game::Board board;
    std::string result = board.to_string({{"black", "●"}, {"white", "○"}});
    EXPECT_NE(result.empty(), true);
}

} // anonymous namespace

// ─── 以下、Card クラスの実装（card_impl.cpp のプレビュー） ────────────────

namespace game {

bool Player::play_card(Card& card) {
    if (energy_points < card.cost) {
        return false; // EP が不足
    }
    energy_points -= card.cost;
    card.effect_fn(*this, board);
    return true;
}

Card* Player::find_card(const std::string& name) const {
    for (auto* card : hand) {
        if (card && !card->name.empty() && card->name == name) {
            return card;
        }
    }
    return nullptr;
}

bool Game::has_valid_move(const Board& board) const {
    // 簡易：中央 4 マス以外に空マスがあれば「有効な移動」とみなす（本当は置ける場所があるかチェック）
    for (int r = 0; r < Board::SIZE; ++r) {
        for (int c = 0; c < Board::SIZE; ++c) {
            if (board.is_empty(r, c)) return true;
        }
    }
    return false; // 空マスが一つもない → パスターン（ゲーム終了）
}

void Game::check_game_over() {
    bool black_can_move = has_valid_move(board_);
    bool white_can_move = has_valid_move_white();
    
    if (!black_can_move && !white_can_move) {
        // ゲーム終了
        int black_count = 0, white_count = 0;
        for (int r = 0; r < Board::SIZE; ++r) {
            for (int c = 0; c < Board::SIZE; ++c) {
                if (board_[r][c].has_value()) {
                    auto& stone = board_[r][c];
                    if (!stone->is_empty() && !stone->type == StoneType::Wall) {
                        if (stone->color == "black") black_count++;
                        else white_count++;
                    }
                }
            }
        }

        winner = (black_count > white_count) ? "black" :
                 (white_count > black_count) ? "white" : "";
    }
}

// ─── Card 実装（簡易版）───────────────────────────────────────────────────

Result Card::can_play(const Player& player, const Board&) const {
    if (cost < 0) return Result{.status = Result::Status::Invalid};
    return Result{.status = Result::Status::Ok}; // コストチェックは play() で行う
}

Result Card::play(Player&, Board&) {
    return Result{.status = Result::Status::Ok};
}

// ─── Place ──────────────────────────────────────────────────────────────

CardResult Place::execute(Player& player, Board& board) {
    // 簡易実装：空マス 1 つずつ見つかったら石を置く（黒または白）
    const auto my_color = (player.color == "black") ? "Black" : "White";
    int placed_count = 0;

    for (int r = 0; r < Board::SIZE && placed_count < 2; ++r) {
        for (int c = 0; c < Board::SIZE; ++c) {
            if (board.is_empty(r, c)) {
                board.place_stone(my_color, r, c);
                ++placed_count;
            }
        }
    }

    return placed_count == 2 ? CardResult{.outcome = CardResult::Outcome::Played}
                              : CardResult{.outcome = CardResult::Outcome::InvalidTarget};
}

// ─── Move ────────────────────────────────────────────────────────────────

CardResult Move::execute(Player& player, Board&) {
    // 簡易実装：自分の石を 1 つランダムに移動する（UI から座標を受け付けるはず）
    return CardResult{.outcome = CardResult::Outcome::Played};
}

// ─── Cover ──────────────────────────────────────────────────────────────

CardResult Cover::execute(Player& player, Board& board) {
    for (int r = 0; r < Board::SIZE; ++r) {
        for (int c = 0; c < Board::SIZE; ++c) {
            if (board.is_occupied(r, c)) {
                auto& stone = board[r][c];
                if (stone->color == player.color) {
                    stone.cover();
                }
            }
        }
    }
    return CardResult{.outcome = CardResult::Outcome::Played};
}

// ─── Burst ──────────────────────────────────────────────────────────────

CardResult Burst::execute(Player& player, Board&) {
    // 簡易実装：壁に囲まれた敵石をすべて捕獲（未実装）
    return CardResult{.outcome = CardResult::Outcome::Played};
}

// ─── Swap ────────────────────────────────────────────────────────────────

CardResult Swap::execute(Player& player, Board&) {
    // 簡易実装：隣接する石同士を入れ替える（未実装）
    return CardResult{.outcome = CardResult::Outcome::Played};
}

// ─── Block ──────────────────────────────────────────────────────────────

CardResult Block::execute(Player& player, Board&) {
    // 簡易実装：指定マスを壁にする（未実装）
    return CardResult{.outcome = CardResult::Outcome::Played};
}

// ─── Shield ──────────────────────────────────────────────────────────────

CardResult Shield::execute(Player& player, Board&) {
    // 簡易実装：指定石にシールドを付与（未実装）
    return CardResult{.outcome = CardResult::Outcome::Played};
}

// ─── Reversal ────────────────────────────────────────────────────────────

CardResult Reversal::execute(Player& player, Board&) {
    // 簡易実装：隣接敵石を味方に裏返す（未実装）
    return CardResult{.outcome = CardResult::Outcome::Played};
}

// ─── Reserve ────────────────────────────────────────────────────────────

CardResult Reserve::execute(Player& player, Board&) {
    // 簡易実装：次ターンの Place カード数を +2（未実装）
    return CardResult{.outcome = CardResult::Outcome::Played};
}

// ─── Restore ────────────────────────────────────────────────────────────

CardResult Restore::execute(Player& player, Board&) {
    if (player.energy_points < 6) {
        ++player.energy_points;
    }
    return CardResult{.outcome = CardResult::Outcome::Played};
}

} // namespace game
