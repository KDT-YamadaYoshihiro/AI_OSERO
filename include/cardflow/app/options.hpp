#pragma once

#include <optional>

#include "cardflow/ai.hpp"
#include "cardflow/types.hpp"

namespace cardflow::app {

// 対戦の設定
struct GameOptions {
    std::optional<Color> cpuColor;  // std::nullopt なら 2 人対戦
    ai::Difficulty difficulty = ai::Difficulty::Normal;

    friend bool operator==(const GameOptions&, const GameOptions&) = default;
};

} // namespace cardflow::app
