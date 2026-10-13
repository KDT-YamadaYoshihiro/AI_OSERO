// 技能カード: 定義 + EPコスト (data-model.md 表 verbatim)
#pragma once

namespace gco {

enum class CardType : int {
    NONE = 0,
    JET = 1,      // ジェット: 連鎖無視任意列drop (EP2)
    ANCHOR = 2,   // 錨: 1列封锁 opponent次のturn (EP2)
    GUST = 3,     // 風: 列top stone横移動 (EP3)
    BOMB = 4,     // 爆弾: 敵stone 1個破壊 (EP4)
    SHIELD = 5,   // 盾: 自stone裏返防止 (EP2)
    MAGNET = 6,   // 磁石: 敵列top stone移動 (EP3)
    DOUBLE = 7,   // 双: 2石drop (EP4)
    COLLAPSE = 8, // 崩落: 自列bottom stone除去 (EP3)
    SWAP = 9,     // 交換: 隣接2石交換 (EP3)
    CHARGE = 10,  // 充電: EP+2 (EP0)
};

inline int cardCost(CardType c) {
    switch (c) {
        case CardType::JET: return 2;
        case CardType::ANCHOR: return 2;
        case CardType::GUST: return 3;
        case CardType::BOMB: return 4;
        case CardType::SHIELD: return 2;
        case CardType::MAGNET: return 3;
        case CardType::DOUBLE: return 4;
        case CardType::COLLAPSE: return 3;
        case CardType::SWAP: return 3;
        case CardType::CHARGE: return 0;
        default: return 0;
    }
}

// UI表示用日本語名 (FR-025)
inline const char* cardName(CardType c) {
    switch (c) {
        case CardType::JET: return "ジェット";
        case CardType::ANCHOR: return "錨";
        case CardType::GUST: return "風";
        case CardType::BOMB: return "爆弾";
        case CardType::SHIELD: return "盾";
        case CardType::MAGNET: return "磁石";
        case CardType::DOUBLE: return "双";
        case CardType::COLLAPSE: return "崩落";
        case CardType::SWAP: return "交換";
        case CardType::CHARGE: return "充電";
        default: return "";
    }
}

} // namespace gco
