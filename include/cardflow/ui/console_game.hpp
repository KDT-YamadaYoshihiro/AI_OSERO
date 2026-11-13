#pragma once

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>

#include "cardflow/ai.hpp"
#include "cardflow/card.hpp"
#include "cardflow/game.hpp"

namespace cardflow::ui {

// 対戦の設定。cpuColor が空なら 2 人対戦
struct GameOptions {
    std::optional<Color> cpuColor;
    ai::Difficulty difficulty = ai::Difficulty::Normal;
};

// 起動時のメニューでモード・強さ・色を選ぶ。入力が尽きる／q なら std::nullopt
[[nodiscard]] std::optional<GameOptions> askGameOptions(std::istream& in, std::ostream& out);

// 表示中の番号からカードを引く。1..手札枚数 が手札、その次が追加 Place（あれば）
[[nodiscard]] std::optional<CardType> cardForNumber(const Game& game, int number);

// ゲームの状態（盤面・EP・手札など）を表示用文字列にする
[[nodiscard]] std::string renderStatus(const Game& game);

// 対戦を実行する。入力が尽きるか q で終了
void runConsoleGame(std::istream& in, std::ostream& out, std::uint32_t seed, const GameOptions& options = {});

} // namespace cardflow::ui
