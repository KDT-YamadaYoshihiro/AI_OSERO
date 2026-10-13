// 重力カスケードオセロ (Gravity Cascade Othello) — DxLib GUI 主プログラム
// 操作: 数字キー1-9=手札カード選択 / 盤クリック=列投入(または目標指定) / H=ヘルプ / ESC=取消
#include <DxLib.h>
#include <cstdio>
#include <vector>

#include "core/rules.hpp"
#include "ai/ai.hpp"

using namespace gco;

namespace {

constexpr int CELL = 44;          // 1cell像素
constexpr int BX = 16, BY = 30;   // 盤原点
int gFont = -1;

// UTF-8字面 → Shift-JIS (DxLib文字描画用)
const char* sj(const char* utf8) {
    static char buf[512];
    ConvertStringCharCodeFormat(DX_CHARCODEFORMAT_UTF8, utf8, DX_CHARCODEFORMAT_SHIFTJIS, buf);
    return buf;
}

struct UI {
    GameState gs = createInitialState(2026);
    int screen = 0;              // 0=title 1=difficulty 2=game 3=result
    int difficulty = 1;
    int mode = 0;                // 0=CPU battle 1=hot seat
    int selCard = -1;            // 選択中手札index
    std::vector<std::pair<int,int>> targets; // SWAP目標
    const char* msg = "";
    bool help = false;
    bool turnStarted = false;
    int prevKey[256] = {0};
    Result outcome = NONE;
};

bool keyEdge(UI& u, int key) {
    int now = CheckHitKey(key);
    bool edge = now && !u.prevKey[key];
    u.prevKey[key] = now;
    return edge;
}

int firstLegal(const GameState& gs) {
    auto v = legalDropColumns(gs);
    return v.empty() ? -1 : v[0];
}

int adjacentNotFull(const GameState& gs, int col) {
    for (int c : {col - 1, col + 1})
        if (c >= 0 && c < kBoardSize && !gs.board.columnFull(c)) return c;
    return -1;
}

// 人間操作: 列/cellクリックでmove実行 (cards_test.cppと同じderived fields)
bool humanMove(UI& u, int col, int row) {
    Move mv{};
    if (u.selCard < 0 || u.selCard >= (int)u.gs.players[u.gs.current].hand.size()) {
        mv.kind = Move::DROP; mv.col = col;
    } else {
        CardType c = u.gs.players[u.gs.current].hand[u.selCard];
        switch (c) {
            case CardType::JET:
                mv.kind = Move::JET; mv.card = c; mv.col = col; break;
            case CardType::DOUBLE:
                mv.kind = Move::DOUBLE; mv.card = c; mv.col = col; mv.col2 = adjacentNotFull(u.gs, col); break;
            case CardType::CHARGE:
                mv.kind = Move::CARD; mv.card = c; mv.col = col; break;
            case CardType::ANCHOR:
                mv.kind = Move::CARD; mv.card = c; mv.srcCol = col; mv.col = firstLegal(u.gs); break;
            case CardType::GUST:
            case CardType::MAGNET:
                mv.kind = Move::CARD; mv.card = c; mv.srcCol = col; mv.dstCol = adjacentNotFull(u.gs, col);
                mv.col = firstLegal(u.gs); break;
            case CardType::BOMB:
            case CardType::SHIELD:
                mv.kind = Move::CARD; mv.card = c; mv.r1 = u.gs.board.topRow(col); mv.c1 = col;
                mv.col = firstLegal(u.gs); break;
            case CardType::COLLAPSE:
                mv.kind = Move::CARD; mv.card = c; mv.srcCol = col; mv.col = firstLegal(u.gs); break;
            case CardType::SWAP:
                u.targets.push_back({row, col});
                if (u.targets.size() < 2) return false; // 2つ目のクリック待ち
                mv.kind = Move::CARD; mv.card = c;
                mv.r1 = u.targets[0].first; mv.c1 = u.targets[0].second;
                mv.r2 = u.targets[1].first; mv.c2 = u.targets[1].second;
                mv.col = firstLegal(u.gs);
                u.targets.clear(); break;
            default: return false;
        }
    }
    if (mv.col < 0) { u.msg = "投入できる列がありません(パス)"; return false; }
    if (!applyMove(u.gs, mv)) { u.msg = "その操作は合法ではありません"; return false; }
    u.msg = ""; u.selCard = -1;
    return true;
}

void drawBoard(const UI& u) {
    DrawBox(BX - 2, BY - 2, BX + CELL * kBoardSize + 2, BY + CELL * kBoardSize + 2, GetColor(40, 40, 50), TRUE);
    for (int r = 0; r <= kBoardSize; ++r)
        DrawLine(BX, BY + r * CELL, BX + kBoardSize * CELL, BY + r * CELL, GetColor(90, 90, 100), 1);
    for (int c = 0; c <= kBoardSize; ++c)
        DrawLine(BX + c * CELL, BY, BX + c * CELL, BY + kBoardSize * CELL, GetColor(90, 90, 100), 1);

    // 錨: 封锁列の表示
    if (u.gs.blockedCol >= 0 && u.gs.blockedForTurn == u.gs.turnNumber) {
        DrawBox(BX + u.gs.blockedCol * CELL, BY, BX + (u.gs.blockedCol + 1) * CELL, BY + kBoardSize * CELL,
                GetColor(70, 50, 50), TRUE);
        DrawStringToHandle(BX + u.gs.blockedCol * CELL + 12, BY - 20, sj("封锁"), GetColor(255, 80, 80), gFont);
    }
    // 連鎖anchor列
    if (u.gs.anchor.has_value()) {
        int a = *u.gs.anchor;
        DrawLine(BX + a * CELL, BY - 6, BX + (a + 1) * CELL, BY - 6, GetColor(255, 200, 0), 3);
    }

    // 石
    for (int r = 0; r < kBoardSize; ++r) {
        for (int c = 0; c < kBoardSize; ++c) {
            const CellState& cs = u.gs.board.cells[idx(r, c)];
            if (cs.color == EMPTY) continue;
            int cx = BX + c * CELL + CELL / 2, cy = BY + r * CELL + CELL / 2;
            unsigned colr = cs.color == BLACK ? GetColor(15, 15, 15) : GetColor(240, 240, 240);
            DrawCircle(cx, cy, CELL / 2 - 4, colr, TRUE, 1);
            if (cs.shieldUntil > u.gs.turnNumber)
                DrawCircle(cx, cy, CELL / 2 - 1, GetColor(255, 220, 0), FALSE, 2); // 盾=黄リング
        }
    }

    // 投入列hover: 落地点preview
    int mx, my;
    if (GetMousePoint(&mx, &my) == 0) {
        int c = (mx - BX) / CELL;
        if (c >= 0 && c < kBoardSize && mx >= BX && my >= BY && my < BY + kBoardSize * CELL) {
            bool legal = false;
            for (int lc : legalDropColumns(u.gs)) if (lc == c) { legal = true; break; }
            if (!legal && u.selCard >= 0) {
                CardType cc = u.gs.players[u.gs.current].hand[u.selCard];
                if (cc == CardType::JET) legal = !u.gs.board.columnFull(c);
            }
            if (legal && !u.gs.board.columnFull(c)) {
                int lr = u.gs.board.lowestEmpty(c);
                DrawBox(BX + c * CELL + 6, BY + lr * CELL + 6, BX + (c + 1) * CELL - 6, BY + (lr + 1) * CELL - 6,
                        GetColor(120, 180, 120), FALSE, 2);
            }
        }
    }
}

void drawPanel(const UI& u) {
    int px = BX + kBoardSize * CELL + 14; // x=382
    DrawStringToHandle(px, 6, sj("重力カスケードオセロ"), GetColor(255, 255, 255), gFont);
    char buf[128];
    std::sprintf(buf, "ターン%d  黒:%d 白:%d", u.gs.turnNumber,
                 u.gs.board.countStones(BLACK), u.gs.board.countStones(WHITE));
    DrawStringToHandle(px, 24, sj(buf), GetColor(200, 200, 200), gFont);

    const char* who = u.gs.current == 0 ? "黒" : "白";
    bool cpu = (u.mode == 0 && u.gs.current == 1);
    std::sprintf(buf, "%s%sの番", who, cpu ? " (CPU)" : "");
    DrawStringToHandle(px, 42, sj(buf), GetColor(255, 220, 120), gFont);

    // EP gauge
    int ep = u.gs.players[u.gs.current].ep;
    DrawStringToHandle(px, 60, sj("EP"), GetColor(120, 200, 255), gFont);
    for (int i = 0; i < 6; ++i) {
        int x = px + 24 + i * 18;
        DrawBox(x, 60, x + 14, 74, i < ep ? GetColor(120, 200, 255) : GetColor(40, 40, 60), TRUE);
    }

    // 連鎖情報
    if (u.gs.anchor.has_value())
        std::sprintf(buf, "連鎖: 列%dの隣(%d,%d)のみ投入可", *u.gs.anchor,
                    *u.gs.anchor - 1, *u.gs.anchor + 1);
    else
        std::sprintf(buf, "連鎖: 自由(初手)");
    DrawStringToHandle(px, 80, sj(buf), GetColor(255, 200, 0), gFont);

    // 手札
    DrawStringToHandle(px, 98, sj("手札(数字キー1-9):"), GetColor(180, 180, 180), gFont);
    const auto& hand = u.gs.players[u.gs.current].hand;
    for (int i = 0; i < (int)hand.size() && i < 9; ++i) {
        char line[96];
        std::sprintf(line, "%d:%s %dEP", i + 1, cardName(hand[i]), cardCost(hand[i]));
        DrawStringToHandle(px, 114 + i * 18, sj(line), i == u.selCard ? GetColor(255, 255, 0) : GetColor(160, 160, 160), gFont);
    }
    std::sprintf(buf, "相手手札: %d枚", (int)u.gs.players[1 - u.gs.current].hand.size());
    DrawStringToHandle(px, 290, sj(buf), GetColor(140, 140, 140), gFont);

    if (u.msg[0]) DrawStringToHandle(px, 310, sj(u.msg), GetColor(255, 80, 80), gFont);

    if (u.help) {
        const char* lines[] = {
            "石は上から列に投入→重力で最低空cellへ落ちる",
            "投入列は連鎖anchor列±1のみ(同列不可)",
            "8方向挟まれば裏返 + cascade固定点自動処理",
            "数字キー=カード選択→列クリックで実行",
            "交換:2cellクリック / ESC:取消 / R:restart",
        };
        for (int i = 0; i < 5; ++i)
            DrawStringToHandle(px, 330 + i * 16, sj(lines[i]), GetColor(120, 220, 120), gFont);
    }
}

void drawTitle(const UI& u) {
    DrawStringToHandle(40, 60, sj("重力カスケードオセロ"), GetColor(255, 255, 255), gFont);
    DrawStringToHandle(40, 90, sj("Gravity Cascade Othello - スキルカード付き"), GetColor(180, 180, 180), gFont);
    DrawStringToHandle(40, 140, sj("1: CPU対戦   2: 2人ホットシート"), GetColor(255, 220, 120), gFont);
    DrawStringToHandle(40, 170, sj("H: 規則ヘルプ"), GetColor(120, 220, 120), gFont);
    (void)u;
}

void drawDifficulty(const UI& u) {
    DrawStringToHandle(40, 60, sj("CPU難度: 1初級 2中級 3上級(minimax)"), GetColor(255, 255, 255), gFont);
    (void)u;
}

void drawResult(const UI& u) {
    const char* w = u.outcome == BLACK_WIN ? "黒の勝ち!" : u.outcome == WHITE_WIN ? "白の勝ち!" : "引き分け";
    DrawStringToHandle(40, 60, sj(w), GetColor(255, 220, 120), gFont);
    char buf[64];
    std::sprintf(buf, "最終盤: 黒%d 白%d", u.gs.board.countStones(BLACK), u.gs.board.countStones(WHITE));
    DrawStringToHandle(40, 90, sj(buf), GetColor(200, 200, 200), gFont);
    DrawStringToHandle(40, 130, sj("R: タイトルへ   ESC: 終了"), GetColor(180, 180, 180), gFont);
}

} // namespace

int main() {
    if (DxLib_Init() == -1) return 1;
    gFont = CreateFontToHandle("Meiryo", 14, -1);
    SetMouseDispFlag(TRUE);

    UI u;
    while (GetWindowActiveFlag()) {
        ClearDrawScreen();

        if (u.screen == 0) {
            drawTitle(u);
            if (keyEdge(u, KEY_INPUT_1)) { u.mode = 0; u.screen = 1; }
            else if (keyEdge(u, KEY_INPUT_2)) { u.mode = 1; u.gs = createInitialState(2026); u.screen = 2; }
            else if (keyEdge(u, KEY_INPUT_H)) u.help = !u.help;
        } else if (u.screen == 1) {
            drawDifficulty(u);
            if (keyEdge(u, KEY_INPUT_1)) { u.difficulty = 0; u.gs = createInitialState(2026); u.screen = 2; }
            else if (keyEdge(u, KEY_INPUT_2)) { u.difficulty = 1; u.gs = createInitialState(2026); u.screen = 2; }
            else if (keyEdge(u, KEY_INPUT_3)) { u.difficulty = 2; u.gs = createInitialState(2026); u.screen = 2; }
        } else if (u.screen == 2) {
            // turn lifecycle
            if (u.gs.turnNumber == 0 || !u.turnStarted) { startTurn(u.gs); u.turnStarted = true; }

            Result r = gameOver(u.gs);
            if (r != NONE) { u.outcome = r; u.screen = 3; continue; }

            bool human = (u.mode == 1) || (u.gs.current == 0);
            if (human) {
                if (keyEdge(u, KEY_INPUT_H)) u.help = !u.help;
                if (keyEdge(u, KEY_INPUT_R)) { u.gs = createInitialState(2026); u.turnStarted = false; u.selCard = -1; }
                if (keyEdge(u, KEY_INPUT_ESCAPE)) { u.selCard = -1; u.targets.clear(); }
                for (int k = KEY_INPUT_1; k <= KEY_INPUT_9; ++k) {
                    if (keyEdge(u, k)) { int i = k - KEY_INPUT_1; if (i < (int)u.gs.players[u.gs.current].hand.size()) u.selCard = i; }
                }
                if (GetMouseInput() != -1) {
                    int mx, my; GetMousePoint(&mx, &my);
                    int c = (mx - BX) / CELL, row = (my - BY) / CELL;
                    if (c >= 0 && c < kBoardSize && row >= 0 && row < kBoardSize && mx >= BX && my >= BY) {
                        if (humanMove(u, c, row)) { endTurn(u.gs); u.turnStarted = false; }
                    }
                }
            } else {
                Move mv = chooseMove(u.gs, static_cast<Difficulty>(u.difficulty));
                if (mv.kind == Move::PASS) u.gs.passCounter++;
                else applyMove(u.gs, mv);
                endTurn(u.gs);
                u.turnStarted = false;
                SleepThread(300); // CPU思考演出
            }
            drawBoard(u);
            drawPanel(u);
        } else {
            drawResult(u);
            if (keyEdge(u, KEY_INPUT_R)) u.screen = 0;
            if (keyEdge(u, KEY_INPUT_ESCAPE)) break;
        }

        SleepThread(120);
    }

    DxLib_End();
    return 0;
}
