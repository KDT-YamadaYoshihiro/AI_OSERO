#pragma once

#include <string>
#include <functional>
#include <variant>
#include "stone.hpp"
#include "board.hpp"

namespace game {

// ─── Result エラー型 ──────────────────────────────────────────────────

struct Result {
    enum class Status { Ok, Invalid, OutOfEp };
    Status status = Status::Ok;
    std::string reason{};

    [[nodiscard]] static Result ok() { return Result{.status = Status::Ok}; }
    [[nodiscard]] static Result invalid(const std::string& msg) {
        return Result{.status = Status::Invalid, .reason = msg};
    }
    [[nodiscard]] static Result out_of_ep() {
        return Result{.status = Status::OutOfEp, .reason = "Not enough EP"};
    }

    operator bool() const { return status == Status::Ok; }
};

// ─── カードカテゴリ（Green/Red/Blue/Violet/Orange/Yellow）─────────────

enum class CardCategory : uint8_t {
    Green,   // 基礎操作：Place, Move, Cover
    Red,     // 攻撃・展開：Burst, Swap
    Blue,    // 防御・ブロック：Block, Shield
    Violet,  // 特殊操作：Reversal
    Orange,  // 補助・強化：Reserve
    Yellow   // 調整・回復：Restore
};

// ─── カードの発動結果型 ───────────────────────────────────────────────

struct CardResult {
    enum class Outcome : uint8_t {
        Played,           // 成功してプレイされた
        InvalidTarget,    // ターゲットが空でない／越えられない
        OutOfEp          // EP が不足している
    };
    Outcome outcome{Outcome::Played};
};

// ─── Card ベースクラス（虚関数）────────────────────────────────────────

class Card {
public:
    std::string name;
    CardCategory category{};
    int cost{};           // EP コスト
    std::function<CardResult(const Board&)> effect_fn = nullptr;

    [[nodiscard]] static Card empty() { return Card{.name = "", .cost = 0}; }

    /**
     * @brief カードをプレイできるか判定する（コストとタールの状態のみ）
     */
    [[nodiscard]] Result can_play(const Player& player, const Board& board) const;

protected:
    // 仮想関数：各サブクラスが実装
    virtual CardResult play_impl(Player& player, Board& board) = 0;

public:
    /**
     * @brief カードをプレイする（コストを支払い、効果を実行）
     */
    [[nodiscard]] Result play(Player& player, Board& board);

    /**
     * @brief プレイヤーの手札からこのカードを検索する
     */
    static Card* find_in_hand(const std::vector<Card*>& hand);
};

// ─── Place カード（緑）：空マス 2 つに石を置く ──────────────────────────

class Place : public Card {
public:
    int cost = 2; // EP コスト：2

    [[nodiscard]] static constexpr Card* make() {
        auto* ptr = new (std::nothrow) Place();
        if (!ptr) return nullptr;
        ptr->name = "Place";
        ptr->category = CardCategory::Green;
        ptr->cost = 2;
        ptr->effect_fn = &Place::execute;
        return ptr;
    }

private:
    static CardResult execute(Player& player, Board& board) {
        // プレイヤーの色で石を 2 マス配置する
        const auto my_color = (player.color == "black") ? "Black" : "White";
        // TODO: UI やコマンドから「どこに置くか」を受け取る（簡易版：ランダムまたはデフォルト位置）
        for (int i = 0; i < 2; ++i) {
            int r = board.is_empty(3, i % 4 + 1); // TODO: 正しい空マスを検出するロジック
            if (!board.place_stone(my_color, r, c)) {
                return CardResult{.outcome = CardResult::Outcome::InvalidTarget};
            }
        }
        return CardResult{.outcome = CardResult::Outcome::Played};
    }

public:
    [[nodiscard]] static CardResult execute(Player& player, Board& board); // 宣言

    Card() : name("Place"), category(CardCategory::Green), cost(2) {}
};

// ─── Move カード（緑）：自分の石を最大 3 マス移動させる ──────────────────

class Move : public Card {
public:
    int cost = 3; // EP コスト：3

    [[nodiscard]] static constexpr Card* make() {
        auto* ptr = new (std::nothrow) Move();
        if (!ptr) return nullptr;
        ptr->name = "Move";
        ptr->category = CardCategory::Green;
        ptr->cost = 3;
        ptr->effect_fn = &Move::execute;
        return ptr;
    }

private:
    static CardResult execute(Player& player, Board& board);

public:
    [[nodiscard]] static CardResult execute(Player& player, Board& board);

    Card() : name("Move"), category(CardCategory::Green), cost(3) {}
};

// ─── Cover カード（緑）：自分のすべての石にカバーを付与する ──────────────

class Cover : public Card {
public:
    int cost = 2; // EP コスト：2

    [[nodiscard]] static constexpr Card* make() {
        auto* ptr = new (std::nothrow) Cover();
        if (!ptr) return nullptr;
        ptr->name = "Cover";
        ptr->category = CardCategory::Green;
        ptr->cost = 2;
        ptr->effect_fn = &Cover::execute;
        return ptr;
    }

private:
    static CardResult execute(Player& player, Board& board);

public:
    [[nodiscard]] static CardResult execute(Player& player, Board& board) {
        for (int r = 0; r < Board::SIZE; ++r) {
            for (int c = 0; c < Board::SIZE; ++c) {
                if (board.is_occupied(r, c)) {
                    auto stone = board.get_stone_at(r, c);
                    if (stone.color == player.color) {
                        stone.cover(); // カバーフラグを立てる
                    }
                }
            }
        }
        return CardResult{.outcome = CardResult::Outcome::Played};
    }

public:
    Card() : name("Cover"), category(CardCategory::Green), cost(2) {}
};

// ─── Burst カード（赤）：「壁」に囲まれた敵石をすべて捕獲する ─────────────

class Burst : public Card {
public:
    int cost = 4; // EP コスト：4（高コストで強力な効果）

    [[nodiscard]] static constexpr Card* make() {
        auto* ptr = new (std::nothrow) Burst();
        if (!ptr) return nullptr;
        ptr->name = "Burst";
        ptr->category = CardCategory::Red;
        ptr->cost = 4;
        ptr->effect_fn = &Burst::execute;
        return ptr;
    }

private:
    static CardResult execute(Player& player, Board& board);

public:
    [[nodiscard]] static CardResult execute(Player& player, Board& board) {
        // 「壁」に囲まれた敵石を検出・捕獲するロジックを実装する
        return CardResult{.outcome = CardResult::Outcome::Played};
    }

public:
    Card() : name("Burst"), category(CardCategory::Red), cost(4) {}
};

// ─── Swap カード（赤）：隣接する 2 つの石を入れ替える ─────────────────────

class Swap : public Card {
public:
    int cost = 3; // EP コスト：3

    [[nodiscard]] static constexpr Card* make() {
        auto* ptr = new (std::nothrow) Swap();
        if (!ptr) return nullptr;
        ptr->name = "Swap";
        ptr->category = CardCategory::Red;
        ptr->cost = 3;
        ptr->effect_fn = &Swap::execute;
        return ptr;
    }

private:
    static CardResult execute(Player& player, Board& board);

public:
    [[nodiscard]] static CardResult execute(Player& player, Board& board) {
        // 隣接する石同士を入れ替える（簡易版：自分の石と隣接の敵石を交換）
        return CardResult{.outcome = CardResult::Outcome::Played};
    }

public:
    Card() : name("Swap"), category(CardCategory::Red), cost(3) {}
};

// ─── Block カード（青）：指定マスを「壁」としてブロックする ──────────────

class Block : public Card {
public:
    int cost = 3; // EP コスト：3

    [[nodiscard]] static constexpr Card* make() {
        auto* ptr = new (std::nothrow) Block();
        if (!ptr) return nullptr;
        ptr->name = "Block";
        ptr->category = CardCategory::Blue;
        ptr->cost = 3;
        ptr->effect_fn = &Block::execute;
        return ptr;
    }

private:
    static CardResult execute(Player& player, Board& board);

public:
    [[nodiscard]] static CardResult execute(Player& player, Board& board) {
        // TODO: UI から「どこをブロックするか」を取得する
        return CardResult{.outcome = CardResult::Outcome::Played};
    }

public:
    Card() : name("Block"), category(CardCategory::Blue), cost(3) {}
};

// ─── Shield カード（青）：1 つの石にシールドを付与する ────────────────────

class Shield : public Card {
public:
    int cost = 2; // EP コスト：2

    [[nodiscard]] static constexpr Card* make() {
        auto* ptr = new (std::nothrow) Shield();
        if (!ptr) return nullptr;
        ptr->name = "Shield";
        ptr->category = CardCategory::Blue;
        ptr->cost = 2;
        ptr->effect_fn = &Shield::execute;
        return ptr;
    }

private:
    static CardResult execute(Player& player, Board& board);

public:
    [[nodiscard]] static CardResult execute(Player& player, Board& board) {
        // TODO: UI から「どの石にシールドをかけるか」を取得する
        return CardResult{.outcome = CardResult::Outcome::Played};
    }

public:
    Card() : name("Shield"), category(CardCategory::Blue), cost(2) {}
};

// ─── Reversal カード（バイオレット）：隣接敵石を味方に裏返す ──────────────

class Reversal : public Card {
public:
    int cost = 4; // EP コスト：4

    [[nodiscard]] static constexpr Card* make() {
        auto* ptr = new (std::nothrow) Reversal();
        if (!ptr) return nullptr;
        ptr->name = "Reversal";
        ptr->category = CardCategory::Violet;
        ptr->cost = 4;
        ptr->effect_fn = &Reversal::execute;
        return ptr;
    }

private:
    static CardResult execute(Player& player, Board& board);

public:
    [[nodiscard]] static CardResult execute(Player& player, Board& board) {
        // 指定した自分の石から、隣接する敵石をすべて味方へ「裏返す」
        return CardResult{.outcome = CardResult::Outcome::Played};
    }

public:
    Card() : name("Reversal"), category(CardCategory::Violet), cost(4) {}
};

// ─── Reserve カード（オレンジ）：次のターンに Place カードが +2 枚補充される ─────

class Reserve : public Card {
public:
    int cost = 1; // EP コスト：1（安くて効果は後で発動）

    [[nodiscard]] static constexpr Card* make() {
        auto* ptr = new (std::nothrow) Reserve();
        if (!ptr) return nullptr;
        ptr->name = "Reserve";
        ptr->category = CardCategory::Orange;
        ptr->cost = 1;
        ptr->effect_fn = &Reserve::execute;
        return ptr;
    }

private:
    static CardResult execute(Player& player, Board& board);

public:
    [[nodiscard]] static CardResult execute(Player& player, Board& board) {
        // 次ターンの Place カード数 +2（簡易実装：Player::reserve_count += 2）
        return CardResult{.outcome = CardResult::Outcome::Played};
    }

public:
    Card() : name("Reserve"), category(CardCategory::Orange), cost(1) {}
};

// ─── Restore カード（イエロー）：EP を +2 回復する ────────────────────────

class Restore : public Card {
public:
    int cost = 1; // EP コスト：1

    [[nodiscard]] static constexpr Card* make() {
        auto* ptr = new (std::nothrow) Restore();
        if (!ptr) return nullptr;
        ptr->name = "Restore";
        ptr->category = CardCategory::Yellow;
        ptr->cost = 1;
        ptr->effect_fn = &Restore::execute;
        return ptr;
    }

private:
    static CardResult execute(Player& player, Board& board);

public:
    [[nodiscard]] static CardResult execute(Player& player, Board& board) {
        // EP += 2（ただし上限 6）
        return CardResult{.outcome = CardResult::Outcome::Played};
    }

public:
    Card() : name("Restore"), category(CardCategory::Yellow), cost(1) {}
};

} // namespace game
