#pragma once

#include <string>
#include <string_view>

#include "cardflow/types.hpp"

// 画面に出す文言（描画ライブラリに依存しない）
namespace cardflow::app {

// 盤面のマス表記（例: {2, 4} → "e3"）
[[nodiscard]] std::string formatPosition(Position pos);

[[nodiscard]] std::string_view colorName(Color color) noexcept;  // "黒" / "白"

} // namespace cardflow::app
