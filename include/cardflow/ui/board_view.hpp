#pragma once

#include <string>

#include "cardflow/board.hpp"

namespace cardflow::ui {

// 盤面をコンソール表示用の UTF-8 文字列に変換する。
// 列は a〜h、行は 1〜8。マスは全角文字（・ 空 / ● 黒 / ○ 白 / ■ 壁）で、
// 日本語フォントのコンソールで見出しと桁が揃う。
[[nodiscard]] std::string renderBoard(const Board& board);

} // namespace cardflow::ui
