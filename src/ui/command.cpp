#include "cardflow/ui/command.hpp"

#include <cctype>

#include "cardflow/board.hpp"

namespace cardflow::ui {

namespace {

std::vector<std::string_view> splitWords(std::string_view line)
{
    std::vector<std::string_view> words;
    std::size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i])) != 0) {
            ++i;
        }
        const std::size_t start = i;
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i])) == 0) {
            ++i;
        }
        if (i > start) {
            words.push_back(line.substr(start, i - start));
        }
    }
    return words;
}

bool equalsIgnoreCase(std::string_view a, std::string_view b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

std::optional<int> parseNumber(std::string_view text)
{
    if (text.empty() || text.size() > 2) {
        return std::nullopt;
    }
    int value = 0;
    for (const char ch : text) {
        if (ch < '0' || ch > '9') {
            return std::nullopt;
        }
        value = value * 10 + (ch - '0');
    }
    return value;
}

} // namespace

std::optional<Position> parsePosition(std::string_view text)
{
    if (text.size() != 2) {
        return std::nullopt;
    }
    const int col = std::tolower(static_cast<unsigned char>(text[0])) - 'a';
    const int row = text[1] - '1';
    const Position pos{row, col};
    if (!Board::isInside(pos)) {
        return std::nullopt;
    }
    return pos;
}

std::string formatPosition(Position pos)
{
    std::string text;
    text += static_cast<char>('a' + pos.col);
    text += static_cast<char>('1' + pos.row);
    return text;
}

std::optional<Command> parseCommand(std::string_view line)
{
    // UTF-8 BOM（パイプ入力で付くことがある）を無視し、全角スペース（IME 入力）を区切りとして扱う
    constexpr std::string_view kBom = "\xEF\xBB\xBF";
    constexpr std::string_view kFullWidthSpace = "\xE3\x80\x80";
    std::string normalized{line};
    for (std::size_t pos = 0; (pos = normalized.find(kBom, pos)) != std::string::npos;) {
        normalized.erase(pos, kBom.size());
    }
    for (std::size_t pos = 0; (pos = normalized.find(kFullWidthSpace, pos)) != std::string::npos;) {
        normalized.replace(pos, kFullWidthSpace.size(), " ");
    }

    const auto words = splitWords(normalized);
    if (words.empty()) {
        return std::nullopt;
    }
    const std::string_view head = words.front();
    if (words.size() == 1) {
        if (equalsIgnoreCase(head, "p") || equalsIgnoreCase(head, "pass")) {
            return Command{.kind = Command::Kind::EndTurn};
        }
        if (equalsIgnoreCase(head, "h") || equalsIgnoreCase(head, "help") || head == "?") {
            return Command{.kind = Command::Kind::Help};
        }
        if (equalsIgnoreCase(head, "q") || equalsIgnoreCase(head, "quit")) {
            return Command{.kind = Command::Kind::Quit};
        }
    }

    const auto number = parseNumber(head);
    if (!number.has_value() || *number < 1) {
        return std::nullopt;
    }
    Command command{.kind = Command::Kind::PlayCard, .cardNumber = *number};
    for (std::size_t i = 1; i < words.size(); ++i) {
        const auto pos = parsePosition(words[i]);
        if (!pos.has_value()) {
            return std::nullopt;
        }
        command.targets.push_back(*pos);
    }
    return command;
}

} // namespace cardflow::ui
