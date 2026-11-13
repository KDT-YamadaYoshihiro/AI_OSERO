#pragma once

#include <gtest/gtest.h>

#include "cardflow/game.hpp"

namespace cardflow::test {

// カードをすぐ使えるようにした対戦（自分の番の回数を kFirstCardTurn にそろえる）。
// カードの効果を確かめるテスト用。盤面・手札・山札はそのまま
inline Game cardsReady(const Game& game)
{
    auto state = game.state();
    EXPECT_TRUE(state.has_value());
    state->ownTurns = {Game::kFirstCardTurn, Game::kFirstCardTurn};
    auto ready = Game::restore(*state);
    EXPECT_TRUE(ready.has_value());
    return ready.value_or(game);
}

} // namespace cardflow::test
