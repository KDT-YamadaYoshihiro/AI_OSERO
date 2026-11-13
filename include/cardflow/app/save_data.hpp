#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "cardflow/app/options.hpp"
#include "cardflow/game.hpp"

// 中断データ（対戦の途中状態と設定）。テキスト形式で 1 ファイルに保存する
namespace cardflow::app {

struct SaveData {
    GameState game;
    GameOptions options;
    std::uint32_t seed = 0;
};

[[nodiscard]] std::string serializeSave(const SaveData& data);
// 形式が正しくなければ std::nullopt（Game として再開できるかは Game::restore で確かめる）
[[nodiscard]] std::optional<SaveData> parseSave(std::string_view text);

// 書き出す。失敗したら false
[[nodiscard]] bool writeSaveFile(const std::filesystem::path& path, const SaveData& data);
// 読み込む。ファイルがない・壊れている場合は std::nullopt
[[nodiscard]] std::optional<SaveData> readSaveFile(const std::filesystem::path& path);

} // namespace cardflow::app
