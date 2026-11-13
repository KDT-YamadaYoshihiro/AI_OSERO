#pragma once

#include "cardflow/types.hpp"

namespace cardflow {

// 盤面上の石
struct Stone {
    Color color = Color::Black;
    bool guarded = false;  // 「守り」: 次に挟まれたとき 1 回だけ裏返らない

    friend constexpr bool operator==(const Stone&, const Stone&) = default;
};

// 「壁」カードで置かれるマス。誰も置けず、挟みを止める
struct Wall {
    friend constexpr bool operator==(const Wall&, const Wall&) = default;
};

} // namespace cardflow
