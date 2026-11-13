#pragma once

#include <cstdint>
#include <optional>
#include <random>
#include <string_view>
#include <vector>

#include "cardflow/card.hpp"
#include "cardflow/game.hpp"
#include "cardflow/types.hpp"

// CPU 対戦（仕様書「CPU 対戦の確定」）。CPU は相手の手札を見ない。
namespace cardflow::ai {

// 1 回のカード使用（カードと対象）
struct Action {
    CardType card = CardType::Place;
    std::vector<Position> targets;

    friend bool operator==(const Action&, const Action&) = default;
};

// 手番のプレイヤーが、いま実行できる行動をすべて列挙する。
// Place の 2 マスは順序を区別しない（行優先で小さい方が先）。
[[nodiscard]] std::vector<Action> enumerateActions(const Game& game);

enum class Difficulty : std::uint8_t {
    Easy,    // 弱い: ランダム
    Normal,  // 普通: 石差・EP・相手の次の番の脅威を 1 手評価
    Hard,    // 強い: 普通＋自分の次の番の好機・角/辺・保護も評価
};

[[nodiscard]] std::string_view difficultyName(Difficulty difficulty) noexcept;

// me から見た局面の点数（大きいほど me に有利）
[[nodiscard]] int evaluate(const Game& game, Color me, Difficulty difficulty);

class CpuPlayer {
public:
    CpuPlayer(Difficulty difficulty, std::uint32_t seed);

    [[nodiscard]] Difficulty difficulty() const noexcept { return m_difficulty; }

    // 次に行う行動。ターンを終えるべきなら std::nullopt
    [[nodiscard]] std::optional<Action> chooseAction(const Game& game);

private:
    Difficulty m_difficulty;
    std::mt19937 m_rng;
};

} // namespace cardflow::ai
