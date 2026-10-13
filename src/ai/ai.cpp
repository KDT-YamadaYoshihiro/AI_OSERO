// AI実装: Easy (uniform random), Normal (1-ply greedy), Hard (alpha-beta minimax depth 5-6) (R6)
#include "ai.hpp"
#include <algorithm>
#include <chrono>
#include <utility>

namespace gco {

std::vector<Move> generateMoves(const GameState& gs) {
    std::vector<Move> out;
    auto legal = legalDropColumns(gs);
    for (int c : legal) {
        Move m; m.kind = Move::DROP; m.col = c;
        out.push_back(m);
    }
    const PlayerState& p = gs.players[gs.current];

    auto hasCard = [&](CardType t) { return std::find(p.hand.begin(), p.hand.end(), t) != p.hand.end(); };

    // Jet: 連鎖bypass (FR-017)
    if (hasCard(CardType::JET) && p.ep >= cardCost(CardType::JET)) {
        for (int c = 0; c < kBoardSize; ++c) {
            if (gs.board.columnFull(c)) continue;
            if (gs.blockedCol == c && gs.turnNumber == gs.blockedForTurn) continue;
            Move m; m.kind = Move::JET; m.card = CardType::JET; m.col = c;
            out.push_back(m);
        }
    }
    // Double: 2石drop (FR-017)
    if (hasCard(CardType::DOUBLE) && p.ep >= cardCost(CardType::DOUBLE)) {
        for (int c1 : legal) {
            GameState sim = gs;
            applyDropOnly(sim, c1);
            for (int c2 : legalDropColumns(sim)) {
                Move m; m.kind = Move::DOUBLE; m.card = CardType::DOUBLE; m.col = c1; m.col2 = c2;
                out.push_back(m);
            }
        }
    }
    // Charge + drop combos
    if (hasCard(CardType::CHARGE)) {
        for (int c : legal) {
            Move m; m.kind = Move::CARD; m.card = CardType::CHARGE; m.col = c;
            out.push_back(m);
        }
    }
    // Bomb + drop combos (敵stone 1個破壊, FR-018)
    if (hasCard(CardType::BOMB) && p.ep >= cardCost(CardType::BOMB)) {
        Cell enemy = gs.current == 0 ? WHITE : BLACK;
        for (int r = 0; r < kBoardSize; ++r) {
            for (int c = 0; c < kBoardSize; ++c) {
                if (gs.board.cells[idx(r, c)].color != enemy) continue;
                for (int d : legal) {
                    Move m; m.kind = Move::CARD; m.card = CardType::BOMB; m.col = d; m.r1 = r; m.c1 = c;
                    out.push_back(m);
                }
            }
        }
    }
    // Gust + drop combos (FR-018)
    if (hasCard(CardType::GUST) && p.ep >= cardCost(CardType::GUST)) {
        for (int src = 0; src < kBoardSize; ++src) {
            if (gs.board.topRow(src) < 0) continue;
            for (int dst : { src - 1, src + 1 }) {
                if (dst < 0 || dst >= kBoardSize || gs.board.columnFull(dst)) continue;
                for (int d : legal) {
                    Move m; m.kind = Move::CARD; m.card = CardType::GUST; m.col = d; m.srcCol = src; m.dstCol = dst;
                    out.push_back(m);
                }
            }
        }
    }
    return out;
}

// heuristic: 石差 + 機動力 + EP (Hard searchのleaf eval)
int evaluate(const GameState& gs, int perspective) {
    int mine = gs.board.countStones(perspective == 0 ? BLACK : WHITE);
    int other = gs.board.countStones(perspective == 0 ? WHITE : BLACK);
    int score = (mine - other) * 10;
    if (gameOver(gs) != NONE) {
        Result r = gameOver(gs);
        if (r == DRAW) return 0;
        bool win = (r == BLACK_WIN && perspective == 0) || (r == WHITE_WIN && perspective == 1);
        return win ? 10000 : -10000;
    }
    int mobility = static_cast<int>(legalDropColumns(gs).size());
    if (gs.current == perspective) score += mobility * 2;
    score += gs.players[gs.current].ep; // EP is a resource for the side to move
    return score;
}

// Hard search用move pruning: immediate eval順top-K (branching control, R6)
static std::vector<Move> pruneMoves(const GameState& gs, const std::vector<Move>& moves, int cap) {
    std::vector<std::pair<int, size_t>> scored;
    scored.reserve(moves.size());
    for (size_t i = 0; i < moves.size(); ++i) {
        GameState copy = gs;
        if (!applyMove(copy, moves[i])) continue;
        scored.push_back({ evaluate(copy, gs.current), i });
    }
    std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    std::vector<Move> out;
    for (size_t i = 0; i < scored.size() && static_cast<int>(i) < cap; ++i) out.push_back(moves[scored[i].second]);
    return out;
}

static int minimax(GameState& gs, int depth, int alpha, int beta, int persp,
                  const std::chrono::steady_clock::time_point& deadline) {
    if (depth == 0 || gameOver(gs) != NONE) return evaluate(gs, persp);
    if (std::chrono::steady_clock::now() > deadline) return evaluate(gs, persp);

    std::vector<Move> moves = pruneMoves(gs, generateMoves(gs), 16);
    bool maximizing = gs.current == persp;
    if (moves.empty()) { // pass: counter++ then switch (edge case FR-019)
        GameState copy = gs;
        copy.passCounter++;
        if (gameOver(copy) != NONE) return evaluate(copy, persp);
        endTurn(copy); startTurn(copy);
        return minimax(copy, depth - 1, alpha, beta, persp, deadline);
    }
    int best = maximizing ? -1000000 : 1000000;
    for (const Move& mv : moves) {
        GameState copy = gs;
        if (!applyMove(copy, mv)) continue;
        endTurn(copy); startTurn(copy);
        int v = minimax(copy, depth - 1, alpha, beta, persp, deadline);
        if (maximizing) { best = std::max(best, v); alpha = std::max(alpha, best); }
        else { best = std::min(best, v); beta = std::min(beta, best); }
        if (alpha >= beta) break;
    }
    return best;
}

Move chooseMove(GameState& gs, Difficulty d) {
    auto legal = legalDropColumns(gs);
    auto plainPick = [&](int col) { Move m; m.kind = Move::DROP; m.col = col; return m; };

    if (legal.empty()) {
        // Jetしか手段なし: 列選択 (Easy/Normal/Hard共通 fallback)
        const PlayerState& p = gs.players[gs.current];
        for (CardType c : p.hand) {
            if (c == CardType::JET && p.ep >= cardCost(CardType::JET)) {
                for (int col = 0; col < kBoardSize; ++col) {
                    if (!gs.board.columnFull(col) &&
                        !(gs.blockedCol == col && gs.turnNumber == gs.blockedForTurn)) {
                        Move m; m.kind = Move::JET; m.card = CardType::JET; m.col = col;
                        return m;
                    }
                }
            }
        }
        Move none; none.kind = Move::PASS; // applyMove rejects PASS → caller counts pass (FR-019)
        return none;
    }

    if (d == EASY) { // uniform random legal drop, no cards (FR-022)
        std::uniform_int_distribution<int> dist(0, static_cast<int>(legal.size()) - 1);
        return plainPick(legal[dist(gs.rng)]);
    }

    std::vector<Move> moves = generateMoves(gs);
    if (d == NORMAL) { // 1-ply greedy: cascade後の石差最大化 (R6)
        int bestScore = -1000000;
        Move best = plainPick(legal[0]);
        for (const Move& mv : moves) {
            GameState copy = gs;
            if (!applyMove(copy, mv)) continue;
            int s = evaluate(copy, gs.current);
            if (s > bestScore) { bestScore = s; best = mv; }
        }
        return best;
    }

    // HARD: iterative deepening alpha-beta depth 5→6, 5s cap (SC-003)
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    Move best = plainPick(legal[0]);
    int bestScore = -1000000;
    for (int depth = 5; depth <= 6; ++depth) {
        int alpha = -1000000, beta = 1000000;
        Move localBest = best;
        int localScore = -1000000;
        std::vector<Move> pruned = pruneMoves(gs, moves, 24);
        for (const Move& mv : pruned) {
            GameState copy = gs;
            if (!applyMove(copy, mv)) continue;
            endTurn(copy); startTurn(copy);
            int v = minimax(copy, depth - 1, alpha, beta, gs.current, deadline);
            if (v > localScore) { localScore = v; localBest = mv; }
            alpha = std::max(alpha, localScore);
            if (std::chrono::steady_clock::now() > deadline) break;
        }
        best = localBest;
        bestScore = localScore;
        if (std::chrono::steady_clock::now() > deadline) break;
    }
    (void)bestScore;
    return best;
}

} // namespace gco
