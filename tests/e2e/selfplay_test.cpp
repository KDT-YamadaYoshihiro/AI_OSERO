// T031: seeded self-play e2e — HARD vs EASY completes valid result < 60s (SC-003)
#include "../harness/test.hpp"
#include "../../src/core/rules.hpp"
#include "../../src/ai/ai.hpp"
#include <chrono>

using namespace gco;

int main() {
    for (unsigned seed : {7u, 42u, 123u}) {
        GameState gs = createInitialState(seed);
        auto t0 = std::chrono::steady_clock::now();
        int guard = 0;
        while (gameOver(gs) == NONE && guard < 200) {
            startTurn(gs);
            Difficulty d = gs.current == 0 ? HARD : EASY;
            Move mv = chooseMove(gs, d);
            if (mv.kind == Move::PASS) gs.passCounter++;
            else { CHECK(applyMove(gs, mv)); gs.passCounter = 0; }
            endTurn(gs);
            ++guard;
        }
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t0).count();
        CHECK(ms < 60000); // SC-003: full game under 60s
        Result r = gameOver(gs);
        CHECK(r != NONE);
        int stones = gs.board.countStones(BLACK) + gs.board.countStones(WHITE);
        CHECK(stones >= 4 && stones <= 64);
        std::printf("seed=%u result=%d moves=%d ms=%lld\n", seed, r, guard, (long long)ms);
    }
    return report();
}
