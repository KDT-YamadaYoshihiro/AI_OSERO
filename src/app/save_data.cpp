#include "cardflow/app/save_data.hpp"

#include <charconv>
#include <fstream>
#include <map>
#include <sstream>
#include <system_error>

#include "cardflow/app/text.hpp"
#include "cardflow/board.hpp"

namespace cardflow::app {

namespace {

constexpr std::string_view kHeader = "cardflow-save 1";
constexpr int kCells = Board::kSize * Board::kSize;

// ─── 書き出し ────────────────────────────────────────────────────────

char cellChar(const Board& board, Position pos)
{
    if (board.isWall(pos)) {
        return '#';
    }
    if (const auto stone = board.stoneAt(pos)) {
        if (stone->color == Color::Black) {
            return stone->guarded ? 'b' : 'B';
        }
        return stone->guarded ? 'w' : 'W';
    }
    return '.';
}

std::string cardsText(const std::vector<CardType>& cards)
{
    if (cards.empty()) {
        return "-";
    }
    std::string text;
    for (const auto card : cards) {
        text += static_cast<char>('0' + static_cast<int>(card));
    }
    return text;
}

std::string_view colorText(Color color)
{
    return color == Color::Black ? "black" : "white";
}

// ─── 読み込み ────────────────────────────────────────────────────────

std::optional<int> parseInt(std::string_view text)
{
    int value = 0;
    const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || ptr != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

std::optional<bool> parseBool(std::string_view text)
{
    if (text == "0") {
        return false;
    }
    if (text == "1") {
        return true;
    }
    return std::nullopt;
}

std::optional<Color> parseColor(std::string_view text)
{
    if (text == "black") {
        return Color::Black;
    }
    if (text == "white") {
        return Color::White;
    }
    return std::nullopt;
}

std::optional<std::vector<CardType>> parseCards(std::string_view text)
{
    std::vector<CardType> cards;
    if (text == "-") {
        return cards;
    }
    if (text.empty()) {
        return std::nullopt;
    }
    for (const char ch : text) {
        const int index = ch - '0';
        if (index < 0 || index >= static_cast<int>(kCardTypeCount)) {
            return std::nullopt;
        }
        cards.push_back(static_cast<CardType>(index));
    }
    return cards;
}

std::optional<Position> parsePosition(std::string_view text)
{
    if (text.size() != 2) {
        return std::nullopt;
    }
    const Position pos{text[1] - '1', text[0] - 'a'};
    return Board::isInside(pos) ? std::optional<Position>{pos} : std::nullopt;
}

std::optional<Board> parseBoard(std::string_view text)
{
    if (static_cast<int>(text.size()) != kCells) {
        return std::nullopt;
    }
    Board board;
    for (int i = 0; i < kCells; ++i) {
        const Position pos{i / Board::kSize, i % Board::kSize};
        bool ok = true;
        switch (text[static_cast<std::size_t>(i)]) {
        case '.': break;
        case '#': ok = board.placeWall(pos); break;
        case 'B': ok = board.placeStone(pos, Color::Black); break;
        case 'W': ok = board.placeStone(pos, Color::White); break;
        case 'b': ok = board.placeStone(pos, Color::Black) && board.setStone(pos, Stone{Color::Black, true}); break;
        case 'w': ok = board.placeStone(pos, Color::White) && board.setStone(pos, Stone{Color::White, true}); break;
        default: ok = false; break;
        }
        if (!ok) {
            return std::nullopt;
        }
    }
    return board;
}

} // namespace

std::string serializeSave(const SaveData& data)
{
    const GameState& game = data.game;
    std::string board;
    for (int i = 0; i < kCells; ++i) {
        board += cellChar(game.board, Position{i / Board::kSize, i % Board::kSize});
    }

    std::ostringstream out;
    out << kHeader << '\n';
    out << "seed=" << data.seed << '\n';
    out << "cpu=" << (data.options.cpuColor.has_value() ? colorText(*data.options.cpuColor) : "none") << '\n';
    out << "difficulty=" << static_cast<int>(data.options.difficulty) << '\n';
    out << "board=" << board << '\n';
    out << "current=" << colorText(game.current) << '\n';
    out << "turn=" << game.turnNumber << '\n';
    out << "deck=" << cardsText(game.deck) << '\n';
    out << "hand_black=" << cardsText(game.hands[0]) << '\n';
    out << "hand_white=" << cardsText(game.hands[1]) << '\n';
    out << "own_turns_black=" << game.ownTurns[0] << '\n';
    out << "own_turns_white=" << game.ownTurns[1] << '\n';
    out << "card_used=" << (game.cardUsed ? 1 : 0) << '\n';
    out << "placed=" << game.placed << '\n';
    out << "placements_left=" << game.placementsLeft << '\n';
    out << "free_move=" << (game.freeMove ? 1 : 0) << '\n';
    out << "previous_passed=" << (game.previousPassed ? 1 : 0) << '\n';
    out << "last_placed=" << (game.lastPlaced.has_value() ? formatPosition(*game.lastPlaced) : "-") << '\n';
    return out.str();
}

std::optional<SaveData> parseSave(std::string_view text)
{
    // 1 行目は見出し、以降は key=value
    std::map<std::string, std::string, std::less<>> values;
    bool first = true;
    while (!text.empty()) {
        const auto end = text.find('\n');
        std::string_view line = text.substr(0, end);
        text = end == std::string_view::npos ? std::string_view{} : text.substr(end + 1);
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        if (first) {
            if (line != kHeader) {
                return std::nullopt;
            }
            first = false;
            continue;
        }
        if (line.empty()) {
            continue;
        }
        const auto eq = line.find('=');
        if (eq == std::string_view::npos) {
            return std::nullopt;
        }
        values.emplace(std::string{line.substr(0, eq)}, std::string{line.substr(eq + 1)});
    }
    auto get = [&](std::string_view key) -> std::optional<std::string_view> {
        const auto it = values.find(key);
        return it == values.end() ? std::nullopt : std::optional<std::string_view>{it->second};
    };
    // key の値を parse で解釈する（キーがなければ std::nullopt）
    auto field = [&](std::string_view key, auto parse) -> decltype(parse(std::string_view{})) {
        const auto value = get(key);
        if (!value.has_value()) {
            return std::nullopt;
        }
        return parse(*value);
    };

    SaveData data;
    GameState& game = data.game;

    const auto seed = get("seed");
    std::uint32_t seedValue = 0;
    if (!seed.has_value() || std::from_chars(seed->data(), seed->data() + seed->size(), seedValue).ec != std::errc{}) {
        return std::nullopt;
    }
    data.seed = seedValue;

    const auto cpu = get("cpu");
    if (!cpu.has_value()) {
        return std::nullopt;
    }
    if (*cpu != "none") {
        const auto color = parseColor(*cpu);
        if (!color.has_value()) {
            return std::nullopt;
        }
        data.options.cpuColor = color;
    }
    const auto difficulty = field("difficulty", parseInt);
    if (!difficulty.has_value() || *difficulty < 0 || *difficulty > static_cast<int>(ai::Difficulty::Hard)) {
        return std::nullopt;
    }
    data.options.difficulty = static_cast<ai::Difficulty>(*difficulty);

    const auto board = field("board", parseBoard);
    const auto current = field("current", parseColor);
    const auto turn = field("turn", parseInt);
    const auto deck = field("deck", parseCards);
    const auto handBlack = field("hand_black", parseCards);
    const auto handWhite = field("hand_white", parseCards);
    const auto ownBlack = field("own_turns_black", parseInt);
    const auto ownWhite = field("own_turns_white", parseInt);
    const auto cardUsed = field("card_used", parseBool);
    const auto placed = field("placed", parseInt);
    const auto placementsLeft = field("placements_left", parseInt);
    const auto freeMove = field("free_move", parseBool);
    const auto previousPassed = field("previous_passed", parseBool);
    const auto lastPlaced = get("last_placed");
    if (!board || !current || !turn || !deck || !handBlack || !handWhite || !ownBlack || !ownWhite || !cardUsed ||
        !placed || !placementsLeft || !freeMove || !previousPassed || !lastPlaced) {
        return std::nullopt;
    }
    game.board = *board;
    game.current = *current;
    game.turnNumber = *turn;
    game.deck = *deck;
    game.hands = {*handBlack, *handWhite};
    game.ownTurns = {*ownBlack, *ownWhite};
    game.cardUsed = *cardUsed;
    game.placed = *placed;
    game.placementsLeft = *placementsLeft;
    game.freeMove = *freeMove;
    game.previousPassed = *previousPassed;
    if (*lastPlaced != "-") {
        game.lastPlaced = parsePosition(*lastPlaced);
        if (!game.lastPlaced.has_value()) {
            return std::nullopt;
        }
    }
    return data;
}

bool writeSaveFile(const std::filesystem::path& path, const SaveData& data)
{
    std::error_code ec;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), ec);
    }
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    out << serializeSave(data);
    return static_cast<bool>(out.flush());
}

std::optional<SaveData> readSaveFile(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return parseSave(buffer.str());
}

} // namespace cardflow::app
