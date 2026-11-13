#pragma once

#include <array>
#include <string>
#include <string_view>

#include "cardflow/card.hpp"
#include "cardflow/card_effects.hpp"
#include "cardflow/types.hpp"

// 画面に出す文言（描画ライブラリに依存しない）
namespace cardflow::app {

// 盤面のマス表記（例: {2, 4} → "e3"）
[[nodiscard]] std::string formatPosition(Position pos);

[[nodiscard]] std::string_view colorName(Color color) noexcept;          // "黒" / "白"
[[nodiscard]] std::string_view categoryName(CardCategory category) noexcept;  // "緑" など

// カードに書く効果の説明（2 行）
[[nodiscard]] std::array<std::string_view, 2> cardDescription(CardType type) noexcept;

// 対象を選ぶときの案内（step = すでに選んだ対象の数）
[[nodiscard]] std::string_view targetPrompt(CardType type, int step) noexcept;

// カードを使えなかった理由
[[nodiscard]] std::string_view statusMessage(PlayStatus status) noexcept;

// 効果の結果（例: " 裏返し2 捕獲1"）。何も起きなければ空
[[nodiscard]] std::string describeResult(const PlayResult& result);

} // namespace cardflow::app
