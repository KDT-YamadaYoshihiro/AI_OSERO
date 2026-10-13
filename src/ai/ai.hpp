// AI: 3 difficulty tiers (FR-022, contracts C-8)
#pragma once
#include <vector>
#include "../core/rules.hpp"

namespace gco {

enum Difficulty { EASY = 0, NORMAL = 1, HARD = 2 };

std::vector<Move> generateMoves(const GameState& gs); // all legal turn moves
int evaluate(const GameState& gs, int perspective);   // heuristic score
Move chooseMove(GameState& gs, Difficulty d);         // always passes C-1/C-3 validation

} // namespace gco
