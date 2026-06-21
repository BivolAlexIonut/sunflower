#include "sudoku.h"
#include "inventory.h"      // FLOWERS, FlowerTexCount
#include <cstring>

// Cele 9 simboluri — plante foarte distincte ca formă și culoare (item-uri din Crops.png + flori).
struct Sym { int tex; Rectangle src; };
static const Sym kSym[9] = {
    { 3, { 63,388,36,46 } },   // 1 Floarea-soarelui (galben mare)
    { 4, { 96, 48,16,16 } },   // 2 Rosie (rosu)
    { 4, { 96, 80,16,16 } },   // 3 Morcov (portocaliu)
    { 4, { 96,112,16,16 } },   // 4 Vanata (mov)
    { 4, { 96,240,16,16 } },   // 5 Varza (verde)
    { 4, { 96,208,16,16 } },   // 6 Ridiche (alb)
    { 1, {  0,  0,16,16 } },   // 7 Floare albastra
    { 0, { 16,  0,16,16 } },   // 8 Floare roz
    { 4, { 96, 16,16,16 } },   // 9 Grau (auriu)
};

static void Shuffle(int* a, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = GetRandomValue(0, i);
        int t = a[i]; a[i] = a[j]; a[j] = t;
    }
}

void Sudoku::Enter() { phase = 0; selected = -1; pick = -1; t = 0; }

void Sudoku::Generate(int diff) {
    // soluție validă: tipar de bază + permutări aleatorii de rânduri/coloane/valori
    int rperm[9], cperm[9], dperm[9];
    int ri = 0, ci = 0;
    int bands[3] = { 0,1,2 }; Shuffle(bands, 3);
    for (int b = 0; b < 3; b++) { int w[3] = { 0,1,2 }; Shuffle(w, 3); for (int j = 0; j < 3; j++) rperm[ri++] = bands[b]*3 + w[j]; }
    int stacks[3] = { 0,1,2 }; Shuffle(stacks, 3);
    for (int s = 0; s < 3; s++) { int w[3] = { 0,1,2 }; Shuffle(w, 3); for (int j = 0; j < 3; j++) cperm[ci++] = stacks[s]*3 + w[j]; }
    for (int d = 0; d < 9; d++) dperm[d] = d; Shuffle(dperm, 9);
    for (int r = 0; r < 9; r++) for (int c = 0; c < 9; c++) {
        int pr = rperm[r], pc = cperm[c];
        solution[r*9 + c] = dperm[(3*(pr%3) + pr/3 + pc) % 9];
    }
    // scoate celule în funcție de dificultate
    int keep = (diff == 0) ? 44 : (diff == 1) ? 34 : 27;
    for (int i = 0; i < 81; i++) { grid[i] = solution[i]; given[i] = true; }
    int order[81]; for (int i = 0; i < 81; i++) order[i] = i; Shuffle(order, 81);
    for (int k = 0; k < 81 - keep; k++) { grid[order[k]] = -1; given[order[k]] = false; }
    selected = -1; pick = -1; phase = 1; t = 0; rewardTaken = false;
}

void Sudoku::Place(int idx, int val) {
    if (idx < 0 || idx >= 81 || given[idx]) return;
    grid[idx] = val;
    if (Solved()) { phase = 2; t = 0; }
}

bool Sudoku::Conflict(int idx) const {
    int v = grid[idx]; if (v < 0) return false;
    int r = idx / 9, c = idx % 9;
    for (int k = 0; k < 9; k++) {
        if (k != c && grid[r*9 + k] == v) return true;
        if (k != r && grid[k*9 + c] == v) return true;
    }
    int br = (r/3)*3, bc = (c/3)*3;
    for (int dr = 0; dr < 3; dr++) for (int dc = 0; dc < 3; dc++) {
        int j = (br+dr)*9 + (bc+dc);
        if (j != idx && grid[j] == v) return true;
    }
    return false;
}

bool Sudoku::Solved() const {
    for (int i = 0; i < 81; i++) if (grid[i] < 0 || Conflict(i)) return false;
    return true;
}

int Sudoku::CellAt(Vector2 m) const {
    if (m.x < GX || m.y < GY || m.x >= GX + 9*Cell || m.y >= GY + 9*Cell) return -1;
    int c = (int)((m.x - GX) / Cell), r = (int)((m.y - GY) / Cell);
    return r*9 + c;
}
Rectangle Sudoku::PalRect(int d) const {
    int col = d % 3, row = d / 3;
    return { 712.0f + col*60, 130.0f + row*60, 54, 54 };
}
Rectangle Sudoku::EraseRect() const { return { 712, 318, 174, 44 }; }
Rectangle Sudoku::DiffRect(int d) const { return { 250.0f + d*160, 250, 140, 80 }; }

bool Sudoku::Update() {
    t += GetFrameTime();
    Vector2 m = GetMousePosition();
    bool lc = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool rc = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);

    if (phase == 0) {
        if (IsKeyPressed(KEY_ESCAPE)) return true;
        if (lc) for (int d = 0; d < 3; d++)
            if (CheckCollisionPointRec(m, DiffRect(d))) { difficulty = d; Generate(d); }
        return false;
    }
    if (phase == 2) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || lc) return true;
        return false;
    }
    // phase 1 — joc
    if (IsKeyPressed(KEY_ESCAPE)) return true;
    for (int k = 0; k < 9; k++) if (IsKeyPressed(KEY_ONE + k) && selected >= 0) Place(selected, k);
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ZERO))
        { if (selected >= 0 && !given[selected]) grid[selected] = -1; }
    if (lc) {
        // paletă
        for (int d = 0; d < 9; d++) if (CheckCollisionPointRec(m, PalRect(d))) {
            pick = d; if (selected >= 0) Place(selected, d); return false;
        }
        if (CheckCollisionPointRec(m, EraseRect())) {
            pick = -1; if (selected >= 0 && !given[selected]) grid[selected] = -1; return false;
        }
        int idx = CellAt(m);
        if (idx >= 0) {
            selected = idx;
            if (!given[idx] && pick >= 0) Place(idx, pick);
        }
    }
    if (rc) { int idx = CellAt(m); if (idx >= 0 && !given[idx]) grid[idx] = -1; }
    return false;
}

bool Sudoku::TakeWinReward() {
    if (phase == 2 && !rewardTaken) { rewardTaken = true; return true; }
    return false;
}

static void DrawFlower(const Texture2D* ftex, int d, float x, float y, float s) {
    const Sym& sy = kSym[d];
    DrawTexturePro(ftex[sy.tex], sy.src, Rectangle{ x, y, s, s }, { 0, 0 }, 0, WHITE);
}

void Sudoku::Draw(const Texture2D* ftex) const {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangleGradientV(0, 0, sw, sh, Color{ 60, 110, 75, 255 }, Color{ 40, 80, 55, 255 });
    DrawRectangle(0, 0, sw, 50, Color{ 25, 45, 32, 235 });   // bară pt. lizibilitate
    DrawText("SUDOKU CU FLORI", 24, 8, 28, Color{ 255, 235, 150, 255 });
    DrawText("Fiecare rand, coloana si patrat 3x3 are toate cele 9 flori, fara repetare.",
             320, 17, 16, Color{ 255, 255, 255, 255 });

    if (phase == 0) {
        const char* hdr = "Alege dificultatea";
        DrawText(hdr, sw/2 - MeasureText(hdr, 26)/2, 190, 26, Color{ 255, 235, 150, 255 });
        const char* nm[3] = { "USOR", "MEDIU", "GREU" };
        const char* ds[3] = { "multe indicii", "echilibrat", "putine indicii" };
        for (int d = 0; d < 3; d++) {
            Rectangle r = DiffRect(d);
            bool hov = CheckCollisionPointRec(GetMousePosition(), r);
            DrawRectangleRec(r, hov ? Color{ 80,60,40,255 } : Color{ 45,40,30,255 });
            DrawRectangleLinesEx(r, 3, Color{ 255,220,90,255 });
            DrawText(nm[d], (int)(r.x + r.width/2 - MeasureText(nm[d],26)/2), (int)r.y + 16, 26, WHITE);
            DrawText(ds[d], (int)(r.x + r.width/2 - MeasureText(ds[d],13)/2), (int)r.y + 50, 13, Color{ 200,210,190,255 });
        }
        DrawText("ESC - inapoi in joc", sw/2 - 90, 360, 16, Color{ 210,220,200,220 });
        return;
    }

    // tabla
    for (int r = 0; r < 9; r++) for (int c = 0; c < 9; c++) {
        int idx = r*9 + c;
        float x = GX + c*Cell, y = GY + r*Cell;
        Color bg = Color{ 245, 244, 230, 255 };
        if (given[idx])                     bg = Color{ 222, 218, 198, 255 };
        if (idx == selected)                bg = Color{ 255, 244, 180, 255 };
        if (grid[idx] >= 0 && Conflict(idx)) bg = Color{ 250, 190, 185, 255 };
        DrawRectangle((int)x, (int)y, Cell, Cell, bg);
        DrawRectangleLines((int)x, (int)y, Cell, Cell, Color{ 180, 175, 155, 255 });
        if (grid[idx] >= 0) DrawFlower(ftex, grid[idx], x + 6, y + 5, Cell - 12);
    }
    // liniile groase 3x3
    for (int i = 0; i <= 9; i += 3) {
        DrawRectangle(GX + i*Cell - 1, GY - 1, 3, 9*Cell + 3, Color{ 60, 45, 35, 255 });
        DrawRectangle(GX - 1, GY + i*Cell - 1, 9*Cell + 3, 3, Color{ 60, 45, 35, 255 });
    }

    // paleta de flori
    DrawText("Flori:", 712, 108, 16, Color{ 230, 230, 210, 255 });
    for (int d = 0; d < 9; d++) {
        Rectangle r = PalRect(d);
        DrawRectangleRec(r, (d == pick) ? Color{ 90,70,45,255 } : Color{ 45,40,30,235 });
        DrawRectangleLinesEx(r, (d == pick) ? 3 : 2, (d == pick) ? Color{ 255,220,90,255 } : Color{ 110,95,65,255 });
        DrawFlower(ftex, d, r.x + 8, r.y + 6, 40);
    }
    Rectangle er = EraseRect();
    DrawRectangleRec(er, Color{ 50,40,35,235 });
    DrawRectangleLinesEx(er, 2, Color{ 150,110,90,255 });
    DrawText("STERGE  (click dreapta)", (int)er.x + 12, (int)er.y + 14, 15, Color{ 230,180,160,255 });

    DrawText("Click celula, apoi click floarea (sau 1-9).  ESC iese.", GX, GY + 9*Cell + 12, 15, Color{ 220,225,205,235 });

    if (phase == 2) {
        DrawRectangle(0, 0, sw, sh, Color{ 0,0,0,150 });
        int bw = 460, bh = 180, bx = sw/2 - bw/2, by = sh/2 - bh/2;
        DrawRectangle(bx, by, bw, bh, Color{ 40, 70, 45, 245 });
        DrawRectangleLinesEx(Rectangle{ (float)bx,(float)by,(float)bw,(float)bh }, 4, Color{ 255,220,90,255 });
        const char* w1 = "FELICITARI!";
        DrawText(w1, sw/2 - MeasureText(w1, 40)/2, by + 28, 40, Color{ 255, 235, 150, 255 });
        const char* w2 = "Ai rezolvat sudoku-ul cu flori.";
        DrawText(w2, sw/2 - MeasureText(w2, 20)/2, by + 84, 20, Color{ 230, 240, 220, 255 });
        const char* w3 = "Te iubesc!  (+500 bani)";
        DrawText(w3, sw/2 - MeasureText(w3, 20)/2, by + 116, 20, Color{ 255, 180, 200, 255 });
        const char* w4 = "click / ENTER - inapoi";
        DrawText(w4, sw/2 - MeasureText(w4, 15)/2, by + bh - 26, 15, Color{ 200,210,190,230 });
        // inimioare
        for (int i = 0; i < 10; i++) {
            float hx = bx + 30 + i*40, hy = by - 10 + sinf(t*2 + i)*8;
            DrawCircle((int)hx-3, (int)hy, 4, Color{ 255,120,160,255 });
            DrawCircle((int)hx+3, (int)hy, 4, Color{ 255,120,160,255 });
            DrawTriangle({ hx-6,hy+1 }, { hx+6,hy+1 }, { hx,hy+8 }, Color{ 255,120,160,255 });
        }
    }
}
