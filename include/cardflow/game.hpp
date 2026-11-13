#pragma once

#include <cstdint>
#include <optional>
#include <span>

#include "cardflow/board.hpp"
#include "cardflow/card.hpp"
#include "cardflow/card_effects.hpp"
#include "cardflow/deck.hpp"
#include "cardflow/player.hpp"
#include "cardflow/types.hpp"

namespace cardflow {

// 終局結果
struct GameResult {
    std::optional<Color> winner;  // std::nullopt は引き分け
    int blackStones = 0;
    int whiteStones = 0;
};

// ゲーム進行（仕様書「ゲーム進行の確定」）
//   ターン開始処理 → カードを使う（任意、通常カードは 1 枚まで＋追加 Place）
//   → endTurn（手札補充）→ 相手のターン。
//   両者連続パス、またはカード使用直後にどちらかの石が 0 個になったら終了。
class Game {
public:
    // 初期盤面・標準デッキ（seed でシャッフル）で開始し、黒のターンを始める
    explicit Game(std::uint32_t seed);
    // 任意の盤面・デッキで開始する（テスト用）
    Game(Board board, Deck deck);

    [[nodiscard]] const Board& board() const noexcept { return m_board; }
    [[nodiscard]] const Deck& deck() const noexcept { return m_deck; }
    [[nodiscard]] const Player& player(Color color) const noexcept;
    [[nodiscard]] const Player& currentPlayer() const noexcept { return player(m_current); }
    [[nodiscard]] Color currentColor() const noexcept { return m_current; }
    [[nodiscard]] int round() const noexcept { return m_round; }
    [[nodiscard]] bool isOver() const noexcept { return m_result.has_value(); }
    [[nodiscard]] const std::optional<GameResult>& result() const noexcept { return m_result; }

    // このターンに通常のカードを使ったか
    [[nodiscard]] bool hasPlayedCardThisTurn() const noexcept { return m_playedNormalCard; }
    // このターンにカード（追加 Place を含む）を 1 枚でも使ったか
    [[nodiscard]] bool hasActedThisTurn() const noexcept { return m_actedThisTurn; }
    // 直前のターンがパスだったか
    [[nodiscard]] bool lastTurnWasPass() const noexcept { return m_previousTurnPassed; }

    // 手番のプレイヤーが、いま使えるカードを 1 枚でも持っているか
    [[nodiscard]] bool canPlayAnything() const;
    // そのカードを、いま使えるか（手札・EP・1 ターン 1 枚・対象の存在）
    [[nodiscard]] bool canPlay(CardType type) const;

    // 手番のプレイヤーがカードを使う
    PlayResult play(CardType type, std::span<const Position> targets);

    // 手番を終える（手札補充 → 終了判定 → 相手のターン開始）
    void endTurn();

private:
    [[nodiscard]] Player& mutablePlayer(Color color) noexcept;
    [[nodiscard]] bool usesBonusPlace(CardType type) const noexcept;
    void finish();

    Board m_board;
    Deck m_deck;
    Player m_black{Color::Black};
    Player m_white{Color::White};
    Color m_current = Color::Black;
    int m_round = 1;
    bool m_playedNormalCard = false;
    bool m_actedThisTurn = false;
    bool m_previousTurnPassed = false;
    std::optional<GameResult> m_result;
};

} // namespace cardflow
