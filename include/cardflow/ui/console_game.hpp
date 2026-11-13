#pragma once

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>

#include "cardflow/card.hpp"
#include "cardflow/game.hpp"

namespace cardflow::ui {

// 表示中の番号からカードを引く。1..手札枚数 が手札、その次が追加 Place（あれば）
[[nodiscard]] std::optional<CardType> cardForNumber(const Game& game, int number);

// ゲームの状態（盤面・EP・手札など）を表示用文字列にする
[[nodiscard]] std::string renderStatus(const Game& game);

// 2 人対戦（同じ端末で交互に入力）を実行する。入力が尽きるか q で終了
void runConsoleGame(std::istream& in, std::ostream& out, std::uint32_t seed);

} // namespace cardflow::ui
