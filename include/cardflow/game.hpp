#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "cardflow/board.hpp"
#include "cardflow/card.hpp"
#include "cardflow/deck.hpp"
#include "cardflow/rules.hpp"
#include "cardflow/types.hpp"

namespace cardflow {

// 終局結果
struct GameResult {
    std::optional<Color> winner;  // std::nullopt は引き分け
    int blackStones = 0;
    int whiteStones = 0;
    std::optional<Color> resignedBy;  // 投了した側（投了で終わった場合）
};

// 対戦の途中状態（中断データの保存・復元に使う）。終局後の状態は扱わない
struct GameState {
    Board board;
    std::vector<CardType> deck;  // 末尾から引く
    std::array<std::vector<CardType>, 2> hands;  // [0] = 黒、[1] = 白
    std::array<int, 2> ownTurns{};               // それぞれの番が来た回数
    Color current = Color::Black;
    int turnNumber = 1;
    bool cardUsed = false;
    int placed = 0;
    int placementsLeft = 1;
    bool freeMove = false;
    bool previousPassed = false;
    std::optional<Position> lastPlaced;
};

enum class CardStatus : std::uint8_t {
    Ok,
    NotInHand,      // 手札にない
    AlreadyUsed,    // このターンはすでにカードを使った（連打で 1 つ目を置いた後も含む）
    InvalidTarget,  // 対象が不正（対象のないカードに対象を渡した場合も含む）
    GameOver,
};

// ゲーム進行（仕様書 1.2）
//   番の始め（3 回ごとに 1 枚引く）→ カードを 1 枚まで使う（任意）→ 石を置く（連打なら 2 つ）
//   → 相手の番。置けなければパス。
class Game {
public:
    static constexpr int kStartingHand = 2;
    static constexpr int kMaxHand = 3;
    static constexpr int kDrawInterval = 3;  // 自分の番が 3 回来るごとに 1 枚引く

    // 初期盤面・標準の山札（seed でシャッフル）で始める
    explicit Game(std::uint32_t seed);
    // 任意の盤面・山札で始める（テスト用）
    Game(Board board, Deck deck);

    [[nodiscard]] const Board& board() const noexcept { return m_board; }
    [[nodiscard]] const Deck& deck() const noexcept { return m_deck; }
    [[nodiscard]] const std::vector<CardType>& hand(Color color) const noexcept;
    [[nodiscard]] Color currentColor() const noexcept { return m_current; }
    [[nodiscard]] int turnNumber() const noexcept { return m_turnNumber; }  // 通算の手番（1 始まり）
    [[nodiscard]] bool isOver() const noexcept { return m_result.has_value(); }
    [[nodiscard]] const std::optional<GameResult>& result() const noexcept { return m_result; }

    // このターンの状態
    [[nodiscard]] bool cardUsedThisTurn() const noexcept { return m_cardUsed; }
    [[nodiscard]] int placedThisTurn() const noexcept { return m_placed; }
    [[nodiscard]] int placementsLeft() const noexcept { return m_placementsLeft; }
    [[nodiscard]] bool freeMoveActive() const noexcept { return m_freeMove; }
    [[nodiscard]] bool lastTurnWasPass() const noexcept { return m_previousPassed; }
    // 直前に置かれた石（なければ std::nullopt）
    [[nodiscard]] const std::optional<Position>& lastPlaced() const noexcept { return m_lastPlaced; }

    // ─── カード ──────────────────────────────────────────────────────
    // いま使えるか（手札・1 ターン 1 枚・対象の存在）
    [[nodiscard]] bool canUseCard(CardType type) const;
    // 対象として選べるマス（対象のないカードは空）
    [[nodiscard]] std::vector<Position> cardTargets(CardType type) const;
    CardStatus useCard(CardType type, std::optional<Position> target = std::nullopt);

    // ─── 石を置く ────────────────────────────────────────────────────
    // いま置けるマス（合法手。自由が有効なら石の隣の空マスも）
    [[nodiscard]] std::vector<Position> placeableCells() const;
    [[nodiscard]] bool canPlace(Position pos) const;
    // 置いて裏返す。置けなければ std::nullopt。置き終えたら自動で相手の番へ
    std::optional<rules::PlaceResult> place(Position pos);

    // 置けるマスがなく、パスするしかないか（このターンに石を置いていない場合のみ）
    [[nodiscard]] bool mustPass() const;
    // パスする（mustPass のときだけ。成功時 true）
    bool pass();

    // 投了する（who の負け）。終局後は false
    bool resign(Color who);

    // ─── 中断・再開 ──────────────────────────────────────────────────
    // 途中状態を取り出す（終局後は std::nullopt）
    [[nodiscard]] std::optional<GameState> state() const;
    // 途中状態から再開する。値が矛盾していれば std::nullopt
    [[nodiscard]] static std::optional<Game> restore(const GameState& state);

private:
    explicit Game(const GameState& state);

    [[nodiscard]] std::vector<CardType>& mutableHand(Color color) noexcept;
    void startTurn();
    void nextTurn(bool passed);
    [[nodiscard]] bool checkFinished();
    void finish();

    Board m_board;
    Deck m_deck;
    std::array<std::vector<CardType>, 2> m_hands;
    std::array<int, 2> m_ownTurns{};  // それぞれの番が来た回数
    Color m_current = Color::Black;
    int m_turnNumber = 1;
    bool m_cardUsed = false;
    int m_placed = 0;
    int m_placementsLeft = 1;
    bool m_freeMove = false;
    bool m_previousPassed = false;
    std::optional<Position> m_lastPlaced;
    std::optional<GameResult> m_result;
};

} // namespace cardflow
