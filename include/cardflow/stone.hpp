#pragma once

#include <optional>

#include "cardflow/types.hpp"

namespace cardflow {

// 盤面上の石。保護状態（Cover / Shield）を持つ
struct Stone {
    Color color = Color::Black;
    bool covered = false;   // Cover カードによる保護
    bool shielded = false;  // Shield カードによる保護

    // いずれかの保護が掛かっているか
    [[nodiscard]] constexpr bool isProtected() const noexcept { return covered || shielded; }

    friend constexpr bool operator==(const Stone&, const Stone&) = default;
};

// Block カードで置かれる壁。石を置けず、移動の障害物になる
struct Wall {
    std::optional<Color> owner;  // 設置したプレイヤー（std::nullopt は中立）

    friend constexpr bool operator==(const Wall&, const Wall&) = default;
};

} // namespace cardflow
