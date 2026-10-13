// 10技能カード効果 (data-model.md table verbatim, contracts C-3/C-4)
#include "../cards/card.hpp"
#include "../core/rules.hpp"
#include <algorithm>
#include <cstdlib>

namespace gco {

static bool swapAdjacent(int r1, int c1, int r2, int c2) {
    return std::abs(r1 - r2) <= 1 && std::abs(c1 - c2) <= 1 && !(r1 == r2 && c1 == c2);
}

bool canPlayCard(const GameState& gs, const Move& mv) {
    if (mv.card == CardType::NONE) return false;
    const PlayerState& p = gs.players[gs.current];
    if (p.ep < cardCost(mv.card)) return false; // FR-016 EP validation

    auto colOK = [&](int c) {
        return c >= 0 && c < kBoardSize && !gs.board.columnFull(c) &&
               !(gs.blockedCol == c && gs.turnNumber == gs.blockedForTurn);
    };

    switch (mv.card) {
        case CardType::JET:
            return colOK(mv.col);
        case CardType::ANCHOR:
            return mv.srcCol >= 0 && mv.srcCol < kBoardSize; // blocked column = srcCol
        case CardType::GUST: {
            if (gs.board.topRow(mv.srcCol) < 0) return false;
            int dst = mv.dstCol;
            return (dst == mv.srcCol - 1 || dst == mv.srcCol + 1) &&
                   dst >= 0 && dst < kBoardSize && !gs.board.columnFull(dst);
        }
        case CardType::BOMB:
            return inBounds(mv.r1, mv.c1) &&
                   gs.board.cells[idx(mv.r1, mv.c1)].color == opposite(gs.current == 0 ? BLACK : WHITE);
        case CardType::SHIELD:
            return inBounds(mv.r1, mv.c1) &&
                   gs.board.cells[idx(mv.r1, mv.c1)].color == (gs.current == 0 ? BLACK : WHITE);
        case CardType::MAGNET: {
            int r = gs.board.topRow(mv.srcCol);
            if (r < 0) return false;
            if (gs.board.cells[idx(r, mv.srcCol)].color == (gs.current == 0 ? BLACK : WHITE)) return false; // 敵stoneのみ
            return mv.dstCol >= 0 && mv.dstCol < kBoardSize && mv.dstCol != mv.srcCol &&
                   !gs.board.columnFull(mv.dstCol);
        }
        case CardType::DOUBLE: {
            auto legal = legalDropColumns(gs);
            if (std::find(legal.begin(), legal.end(), mv.col) == legal.end()) return false;
            GameState sim = gs;
            applyDropOnly(sim, mv.col); // 第二drop legality simulation
            auto legal2 = legalDropColumns(sim);
            return std::find(legal2.begin(), legal2.end(), mv.col2) != legal2.end();
        }
        case CardType::COLLAPSE: {
            int bottom = kBoardSize - 1;
            return mv.srcCol >= 0 && mv.srcCol < kBoardSize &&
                   gs.board.cells[idx(bottom, mv.srcCol)].color == (gs.current == 0 ? BLACK : WHITE);
        }
        case CardType::SWAP: {
            if (!inBounds(mv.r1, mv.c1) || !inBounds(mv.r2, mv.c2)) return false;
            if (gs.board.cells[idx(mv.r1, mv.c1)].color == EMPTY) return false;
            if (gs.board.cells[idx(mv.r2, mv.c2)].color == EMPTY) return false;
            return swapAdjacent(mv.r1, mv.c1, mv.r2, mv.c2); // 8-adjacent, colors any
        }
        case CardType::CHARGE:
            return true;
        default:
            return false;
    }
}

void applyCardEffect(GameState& gs, const Move& mv) {
    switch (mv.card) {
        case CardType::JET:
            applyDropOnly(gs, mv.col); // drop included in Jet move
            break;
        case CardType::ANCHOR:
            gs.blockedCol = mv.srcCol;
            gs.blockedForTurn = gs.turnNumber + 1; // opponent next turn only (FR-024)
            break;
        case CardType::GUST: {
            int r = gs.board.topRow(mv.srcCol);
            CellState s = gs.board.cells[idx(r, mv.srcCol)];
            gs.board.cells[idx(r, mv.srcCol)] = CellState{};
            int dr = gs.board.lowestEmpty(mv.dstCol);
            gs.board.cells[idx(dr, mv.dstCol)] = s;
            resolveCascade(gs, { idx(dr, mv.dstCol) }); // settle+flip cascade (FR-018)
            break;
        }
        case CardType::BOMB:
            gs.board.cells[idx(mv.r1, mv.c1)] = CellState{}; // 盾関係なく破壊 (edge case)
            resolveCascade(gs, {}); // gap → settle → cascade flip
            break;
        case CardType::SHIELD:
            gs.board.cells[idx(mv.r1, mv.c1)].shieldUntil = gs.turnNumber + 2; // own next turn startまで
            break;
        case CardType::MAGNET: {
            int r = gs.board.topRow(mv.srcCol);
            CellState s = gs.board.cells[idx(r, mv.srcCol)];
            gs.board.cells[idx(r, mv.srcCol)] = CellState{};
            int dr = gs.board.lowestEmpty(mv.dstCol);
            gs.board.cells[idx(dr, mv.dstCol)] = s;
            resolveCascade(gs, { idx(dr, mv.dstCol) });
            break;
        }
        case CardType::DOUBLE:
            // both drops validated in canPlayCard (FR-017 edge case: refund on illegal 2nd)
            applyDropOnly(gs, mv.col);
            applyDropOnly(gs, mv.col2);
            break;
        case CardType::COLLAPSE:
            gs.board.cells[idx(kBoardSize - 1, mv.srcCol)] = CellState{}; // bottom own stone
            resolveCascade(gs, {});
            break;
        case CardType::SWAP: {
            CellState a = gs.board.cells[idx(mv.r1, mv.c1)];
            CellState b = gs.board.cells[idx(mv.r2, mv.c2)];
            gs.board.cells[idx(mv.r1, mv.c1)] = b;
            gs.board.cells[idx(mv.r2, mv.c2)] = a;
            resolveCascade(gs, { idx(mv.r1, mv.c1), idx(mv.r2, mv.c2) });
            break;
        }
        case CardType::CHARGE:
            gs.players[gs.current].ep = std::min(6, gs.players[gs.current].ep + 2);
            break;
        default: break;
    }
}

} // namespace gco
