#include "cardflow/card.hpp"

#include <array>

namespace cardflow {

namespace {

// カード表が CardType の宣言順に並んでいることをコンパイル時に保証する
constexpr bool isCardTableOrdered()
{
    for (std::size_t i = 0; i < kCardTable.size(); ++i) {
        if (static_cast<std::size_t>(kCardTable[i].type) != i) {
            return false;
        }
    }
    return true;
}
static_assert(isCardTableOrdered(), "kCardTable は CardType の宣言順に並べること");

constexpr std::size_t kCategoryCount = 6;

} // namespace

bool isBalancedHand(std::span<const CardType> hand) noexcept
{
    if (hand.size() > kHandSize) {
        return false;
    }
    std::array<int, kCategoryCount> counts{};
    for (const auto type : hand) {
        const auto category = cardInfo(type).category;
        auto& count = counts[static_cast<std::size_t>(category)];
        ++count;
        if (count > maxCardsPerCategory(category)) {
            return false;
        }
    }
    return true;
}

} // namespace cardflow
