#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cardflow/types.hpp"

namespace cardflow::ui {

// "e3" → {row 2, col 4}。列 a〜h（大文字可）、行 1〜8
[[nodiscard]] std::optional<Position> parsePosition(std::string_view text);
// {row 2, col 4} → "e3"
[[nodiscard]] std::string formatPosition(Position pos);

// コンソールからの 1 行コマンド
struct Command {
    enum class Kind { PlayCard, EndTurn, Help, Quit };

    Kind kind = Kind::Help;
    int cardNumber = 0;              // PlayCard: 表示中の番号（1 始まり）
    std::vector<Position> targets;   // PlayCard: 対象マス

    friend bool operator==(const Command&, const Command&) = default;
};

// 書式:
//   <番号> [マス ...]   カードを使う（例: "1 c4 f5"）
//   p / pass            ターン終了（何も使っていなければパス）
//   h / help / ?        ヘルプ
//   q / quit            ゲームをやめる
// 解釈できなければ std::nullopt
[[nodiscard]] std::optional<Command> parseCommand(std::string_view line);

} // namespace cardflow::ui
