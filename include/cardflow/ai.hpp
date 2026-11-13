#pragma once

#include <cstdint>
#include <optional>
#include <random>
#include <string_view>

#include "cardflow/card.hpp"
#include "cardflow/game.hpp"
#include "cardflow/types.hpp"

// CPU（仕様書 2）。CPU は相手の手札を見ない。
namespace cardflow::ai {

// CPU の 1 ステップの行動
struct Action {
    enum class Kind : std::uint8_t { UseCard, Place, Pass };

    Kind kind = Kind::Pass;
    CardType card = CardType::Double;  // UseCard のとき
    std::optional<Position> target;    // UseCard で対象を取るとき
    Position pos;                      // Place のとき

    friend bool operator==(const Action&, const Action&) = default;
};

enum class Difficulty : std::uint8_t {
    Easy,    // 弱い: ランダム
    Normal,  // 普通: 1 手読み
    Hard,    // 強い: 相手の応手まで読む
};

[[nodiscard]] std::string_view difficultyName(Difficulty difficulty) noexcept;

// me から見た局面の点数（大きいほど me に有利）
[[nodiscard]] int evaluate(const Game& game, Color me);

class CpuPlayer {
public:
    CpuPlayer(Difficulty difficulty, std::uint32_t seed);

    [[nodiscard]] Difficulty difficulty() const noexcept { return m_difficulty; }

    // 手番のプレイヤーとして次の行動を選ぶ（終局後は Pass）
    [[nodiscard]] Action chooseAction(const Game& game);

private:
    [[nodiscard]] Action chooseRandom(const Game& game);
    [[nodiscard]] Action chooseBest(const Game& game);

    Difficulty m_difficulty;
    std::mt19937 m_rng;
};

// Action を Game に適用する（成功時 true）
bool applyAction(Game& game, const Action& action);

} // namespace cardflow::ai
