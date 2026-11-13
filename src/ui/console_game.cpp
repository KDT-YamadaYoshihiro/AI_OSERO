#include "cardflow/ui/console_game.hpp"

#include <cstddef>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include "cardflow/ui/board_view.hpp"
#include "cardflow/ui/command.hpp"

namespace cardflow::ui {

namespace {

std::string_view colorName(Color color)
{
    return color == Color::Black ? "黒 ●" : "白 ○";
}

std::string_view categoryName(CardCategory category)
{
    switch (category) {
    case CardCategory::Green: return "緑";
    case CardCategory::Red: return "赤";
    case CardCategory::Blue: return "青";
    case CardCategory::Violet: return "紫";
    case CardCategory::Orange: return "橙";
    case CardCategory::Yellow: return "黄";
    }
    return "";
}

std::string_view cardUsage(CardType type)
{
    switch (type) {
    case CardType::Place: return "空マス2つに置いて挟んだ石を裏返す      例: 1 c4 f5";
    case CardType::Move: return "自分の石を直線1〜3マス移動、着地で裏返す 例: 1 d4 d6";
    case CardType::Cover: return "自分の全石を次の自分の番まで保護        例: 1";
    case CardType::Burst: return "挟まれている敵石をすべて取り除く        例: 1";
    case CardType::Swap: return "自分の石と隣の敵石を入れ替える          例: 1 d4 e4";
    case CardType::Block: return "空マスに壁を置く                        例: 1 a1";
    case CardType::Shield: return "自分の石1つに Shield（1回防ぐ）         例: 1 d4";
    case CardType::Reversal: return "自分の石の周囲8マスの敵石を裏返す       例: 1 d4";
    case CardType::Reserve: return "次の自分の番に追加 Place を2枚得る      例: 1";
    case CardType::Restore: return "EP を +2 回復                           例: 1";
    }
    return "";
}

std::string_view statusMessage(PlayStatus status)
{
    switch (status) {
    case PlayStatus::Ok: return "";
    case PlayStatus::NotInHand: return "そのカードは手札にありません。";
    case PlayStatus::NotEnoughEp: return "EP が足りません。";
    case PlayStatus::InvalidTarget: return "そのマスには使えません（対象を確認してください）。";
    case PlayStatus::CardLimitReached: return "このターンはすでにカードを使いました（追加 Place のみ使えます）。";
    case PlayStatus::GameOver: return "ゲームは終了しています。";
    }
    return "";
}

// コンソール上の表示幅（ASCII = 1、それ以外の UTF-8 文字 = 2 とみなす）
std::size_t displayWidth(std::string_view text)
{
    std::size_t width = 0;
    for (const char ch : text) {
        const auto byte = static_cast<unsigned char>(ch);
        if (byte < 0x80) {
            ++width;
        } else if ((byte & 0xC0) != 0x80) {  // 先頭バイトのみ数える
            width += 2;
        }
    }
    return width;
}

void appendCardLine(std::string& out, int number, CardType type, std::string_view suffix)
{
    constexpr std::size_t kNameWidth = 18;
    const auto& info = cardInfo(type);
    out += "  " + std::to_string(number) + ") ";
    std::string name{info.name};
    name += suffix;
    const std::size_t width = displayWidth(name);
    name.append(width < kNameWidth ? kNameWidth - width : 1, ' ');
    out += name;
    out += "[";
    out += categoryName(info.category);
    out += "] EP" + std::to_string(info.cost) + "  ";
    out += cardUsage(type);
    out += '\n';
}

std::string describeResult(const PlayResult& result)
{
    std::string text;
    auto add = [&](std::string_view label, int value) {
        if (value > 0) {
            text += " ";
            text += label;
            text += std::to_string(value);
        }
    };
    add("裏返し", result.flipped);
    add("捕獲", result.captured);
    add("Coverで防御", result.blockedByCover);
    add("Shield消費", result.shieldsBroken);
    return text;
}

constexpr std::string_view kHelp =
    "操作:\n"
    "  <番号> [マス ...]  カードを使う（マスは a1〜h8。例: 1 c4 f5）\n"
    "  p                  ターン終了（カードを使っていなければパス）\n"
    "  h                  このヘルプ\n"
    "  q                  ゲームをやめる\n"
    "ルール: 通常のカードは1ターンに1枚、追加 Place は別枠。両者が続けてパスすると終了し、石の多い方の勝ち。\n";

} // namespace

std::optional<CardType> cardForNumber(const Game& game, int number)
{
    const Player& player = game.currentPlayer();
    const auto& hand = player.hand();
    if (number >= 1 && static_cast<std::size_t>(number) <= hand.size()) {
        return hand[static_cast<std::size_t>(number - 1)];
    }
    if (player.bonusPlaces() > 0 && static_cast<std::size_t>(number) == hand.size() + 1) {
        return CardType::Place;
    }
    return std::nullopt;
}

std::string renderStatus(const Game& game)
{
    const Board& board = game.board();
    const Player& black = game.player(Color::Black);
    const Player& white = game.player(Color::White);
    const Player& me = game.currentPlayer();

    std::string out;
    out += "\n=== ラウンド " + std::to_string(game.round()) + " ／ ";
    out += colorName(game.currentColor());
    out += " の番 ===\n";
    out += renderBoard(board);
    out += "（◉◎ = 保護中の黒・白、■ = 壁）\n";
    out += "黒 ● 石 " + std::to_string(board.countStones(Color::Black)) + "  EP " + std::to_string(black.ep()) +
           "    白 ○ 石 " + std::to_string(board.countStones(Color::White)) + "  EP " + std::to_string(white.ep()) +
           "\n";
    out += "山札 " + std::to_string(game.deck().drawPileSize()) + " 枚 ／ 捨て札 " +
           std::to_string(game.deck().discardPileSize()) + " 枚";
    if (me.reservedPlaces() > 0) {
        out += " ／ 次の番に追加 Place " + std::to_string(me.reservedPlaces()) + " 枚";
    }
    out += "\n手札";
    if (game.hasPlayedCardThisTurn()) {
        out += "（このターンは使用済み）";
    }
    out += ":\n";
    int number = 1;
    for (const auto card : me.hand()) {
        appendCardLine(out, number++, card, "");
    }
    if (me.bonusPlaces() > 0) {
        appendCardLine(out, number, CardType::Place, "（追加×" + std::to_string(me.bonusPlaces()) + "）");
    }
    return out;
}

namespace {

// メニューの番号入力（1..max）。q / 入力切れは std::nullopt
std::optional<int> readChoice(std::istream& in, std::ostream& out, std::string_view prompt, int max)
{
    while (true) {
        out << prompt << "\n> " << std::flush;
        std::string line;
        if (!std::getline(in, line)) {
            return std::nullopt;
        }
        const auto command = parseCommand(line);
        if (command.has_value() && command->kind == Command::Kind::Quit) {
            return std::nullopt;
        }
        if (command.has_value() && command->kind == Command::Kind::PlayCard && command->targets.empty() &&
            command->cardNumber <= max) {
            return command->cardNumber;
        }
        out << "1〜" << max << " の番号で選んでください。\n";
    }
}

std::string playerLabel(Color color, const GameOptions& options)
{
    std::string label{colorName(color)};
    if (options.cpuColor.has_value()) {
        label += *options.cpuColor == color ? "（CPU・" + std::string{ai::difficultyName(options.difficulty)} + "）"
                                            : "（あなた）";
    }
    return label;
}

std::string describeAction(const ai::Action& action)
{
    std::string text{cardInfo(action.card).name};
    for (const auto& pos : action.targets) {
        text += ' ';
        text += formatPosition(pos);
    }
    return text;
}

// CPU の 1 ターン分を進める
void runCpuTurn(Game& game, ai::CpuPlayer& cpu, std::ostream& out, const GameOptions& options)
{
    const std::string label = playerLabel(game.currentColor(), options);
    bool acted = false;
    while (const auto action = cpu.chooseAction(game)) {
        const auto result = game.play(action->card, action->targets);
        if (!result.ok()) {
            break;  // 列挙済みの手なので通常は起きない
        }
        acted = true;
        out << label << "： " << describeAction(*action) << describeResult(result) << '\n';
        if (game.isOver()) {
            return;
        }
    }
    out << label << (acted ? "：ターン終了\n" : "：パス\n");
    game.endTurn();
}

} // namespace

std::optional<GameOptions> askGameOptions(std::istream& in, std::ostream& out)
{
    out << "CardFlow Othello へようこそ！（q で終了）\n";
    const auto mode = readChoice(in, out, "モードを選んでください: 1) 2人対戦  2) CPU対戦", 2);
    if (!mode.has_value()) {
        return std::nullopt;
    }
    GameOptions options;
    if (*mode == 1) {
        return options;
    }
    const auto level = readChoice(in, out, "CPU の強さ: 1) 弱い  2) 普通  3) 強い", 3);
    if (!level.has_value()) {
        return std::nullopt;
    }
    options.difficulty = static_cast<ai::Difficulty>(*level - 1);
    const auto side = readChoice(in, out, "あなたの色: 1) 黒 ●（先手）  2) 白 ○（後手）", 2);
    if (!side.has_value()) {
        return std::nullopt;
    }
    options.cpuColor = *side == 1 ? Color::White : Color::Black;
    return options;
}

void runConsoleGame(std::istream& in, std::ostream& out, std::uint32_t seed, const GameOptions& options)
{
    Game game(seed);
    std::optional<ai::CpuPlayer> cpu;
    if (options.cpuColor.has_value()) {
        cpu.emplace(options.difficulty, seed ^ 0x9E3779B9u);
    }
    out << "対戦を始めます（h でヘルプ）\n";
    bool showStatus = true;

    while (!game.isOver()) {
        const Color current = game.currentColor();
        if (cpu.has_value() && current == options.cpuColor) {
            runCpuTurn(game, *cpu, out, options);
            showStatus = true;
            continue;
        }
        const std::string label = playerLabel(current, options);
        if (!game.canPlayAnything()) {
            out << renderStatus(game);
            out << label
                << (game.hasActedThisTurn() ? "：これ以上使えるカードがないためターン終了します。\n"
                                            : "：使えるカードがないためパスします。\n");
            game.endTurn();
            showStatus = true;
            continue;
        }
        if (showStatus) {
            out << renderStatus(game);
            showStatus = false;
        }
        out << label << " > " << std::flush;

        std::string line;
        if (!std::getline(in, line)) {
            out << "\n入力が終わったため終了します。\n";
            return;
        }
        const auto command = parseCommand(line);
        if (!command.has_value()) {
            out << "入力を解釈できません（h でヘルプ）。\n";
            continue;
        }

        switch (command->kind) {
        case Command::Kind::Help:
            out << kHelp;
            break;
        case Command::Kind::Quit:
            out << "ゲームを中断しました。\n";
            return;
        case Command::Kind::EndTurn:
            out << label << (game.hasActedThisTurn() ? "：ターン終了\n" : "：パス\n");
            game.endTurn();
            showStatus = true;
            break;
        case Command::Kind::PlayCard: {
            const auto card = cardForNumber(game, command->cardNumber);
            if (!card.has_value()) {
                out << "その番号のカードはありません。\n";
                break;
            }
            const int needed = targetCount(*card);
            if (static_cast<int>(command->targets.size()) != needed) {
                out << cardInfo(*card).name << " にはマスを " << needed << " つ指定してください。\n";
                break;
            }
            const auto result = game.play(*card, command->targets);
            if (!result.ok()) {
                out << statusMessage(result.status) << '\n';
                break;
            }
            out << cardInfo(*card).name << " を使いました。" << describeResult(result) << '\n';
            showStatus = true;
            break;
        }
        }
    }

    const auto& result = *game.result();
    out << renderStatus(game);
    out << "\n=== ゲーム終了 ===\n黒 ● " << result.blackStones << " 対 白 ○ " << result.whiteStones << " … ";
    if (result.winner.has_value()) {
        out << playerLabel(*result.winner, options) << " の勝ち！\n";
    } else {
        out << "引き分け\n";
    }
}

} // namespace cardflow::ui
