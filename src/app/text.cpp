#include "cardflow/app/text.hpp"

namespace cardflow::app {

std::string formatPosition(Position pos)
{
    std::string text;
    text += static_cast<char>('a' + pos.col);
    text += static_cast<char>('1' + pos.row);
    return text;
}

std::string_view colorName(Color color) noexcept
{
    return color == Color::Black ? "黒" : "白";
}

std::string_view categoryName(CardCategory category) noexcept
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

std::array<std::string_view, 2> cardDescription(CardType type) noexcept
{
    switch (type) {
    case CardType::Place: return {"石を2つ置き", "挟んだ石を裏返す"};
    case CardType::Move: return {"自分の石を直線", "1〜3マス動かす"};
    case CardType::Cover: return {"自分の全石を", "次の番まで保護"};
    case CardType::Burst: return {"挟まれた敵石を", "すべて取り除く"};
    case CardType::Swap: return {"自分の石と隣の", "敵石を入れ替え"};
    case CardType::Block: return {"空マスに", "壁を置く"};
    case CardType::Shield: return {"自分の石1つを", "1回だけ守る"};
    case CardType::Reversal: return {"自分の石の周囲", "の敵石を裏返す"};
    case CardType::Reserve: return {"次の番に追加", "Placeを2枚得る"};
    case CardType::Restore: return {"EPを2回復", ""};
    }
    return {"", ""};
}

std::string_view targetPrompt(CardType type, int step) noexcept
{
    switch (type) {
    case CardType::Place: return step == 0 ? "1 つ目の石を置く空マスを選んでください" : "2 つ目の石を置く空マスを選んでください";
    case CardType::Move: return step == 0 ? "動かす自分の石を選んでください" : "移動先（直線 1〜3 マス）を選んでください";
    case CardType::Swap: return step == 0 ? "入れ替える自分の石を選んでください" : "入れ替える隣の敵石を選んでください";
    case CardType::Block: return "壁を置く空マスを選んでください";
    case CardType::Shield: return "Shield を付ける自分の石を選んでください";
    case CardType::Reversal: return "中心にする自分の石を選んでください";
    case CardType::Cover:
    case CardType::Burst:
    case CardType::Reserve:
    case CardType::Restore:
        return "「使う」を押すと発動します";
    }
    return "";
}

std::string_view statusMessage(PlayStatus status) noexcept
{
    switch (status) {
    case PlayStatus::Ok: return "";
    case PlayStatus::NotInHand: return "そのカードは手札にありません";
    case PlayStatus::NotEnoughEp: return "EP が足りません";
    case PlayStatus::InvalidTarget: return "いまは使える対象がありません";
    case PlayStatus::CardLimitReached: return "このターンはすでにカードを使いました（追加 Place のみ使えます）";
    case PlayStatus::GameOver: return "ゲームは終了しています";
    }
    return "";
}

std::string describeResult(const PlayResult& result)
{
    std::string text;
    auto add = [&](std::string_view label, int value) {
        if (value > 0) {
            text += ' ';
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

} // namespace cardflow::app
