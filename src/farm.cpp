#include "farm.h"
#include "tilemap.h"
#include "inventory.h"
#include "player.h"
#include <cmath>
#include <ostream>
#include <istream>

// Tile de pământ săpat din FG_Grounds.png (16x16).
static const Rectangle kSoil = { 176, 80, 16, 16 };

Farm::Farm() { cells.resize(TileMap::Width * TileMap::Height); }

void Farm::Load() {
    soilAtlas = LoadTexture("sprites/Tilesets/FG_Grounds.png");
    ftex[0] = LoadTexture("sprites/Objects/FG_Grass_Summer.png");
    ftex[1] = LoadTexture("sprites/Objects/FG_Grass_Winter.png");
    ftex[2] = LoadTexture("sprites/Objects/FG_Grass_Fall.png");
    ftex[3] = LoadTexture("sprites/Plants&supplies/Plants.png");
}
void Farm::Unload() {
    UnloadTexture(soilAtlas);
    for (int i = 0; i < 4; i++) UnloadTexture(ftex[i]);
}

int Farm::CropCount() const {
    int n = 0;
    for (const auto& c : cells) if (c.plot == Plot::Crop && c.big != 2) n++;  // copacul = 1 plantă
    return n;
}

bool Farm::Diggable(int tx, int ty) const {
    return InBounds(tx, ty) && cells[Idx(tx, ty)].plot == Plot::Grass;
}
void Farm::Till(int tx, int ty) {
    if (InBounds(tx, ty) && cells[Idx(tx, ty)].plot == Plot::Grass)
        cells[Idx(tx, ty)].plot = Plot::Soil;
}

int Farm::Idx(int tx, int ty) const { return ty * TileMap::Width + tx; }
bool Farm::InBounds(int tx, int ty) const {
    return tx >= 0 && ty >= 0 && tx < TileMap::Width && ty < TileMap::Height;
}

void Farm::Update(float dt, bool autoWater, bool fastGrow) {
    float gmul = fastGrow ? 2.0f : 1.0f;   // power-up creștere rapidă
    for (auto& c : cells) {
        if (c.big == 2) continue;   // celulele acoperite de un copac 2x2 nu cresc singure
        if (c.plot == Plot::Crop && c.stage < MatureStage) {
            if (autoWater) c.watered = true;   // power-up auto-udare
            if (c.watered) {
                c.growth += dt * gmul;
                if (c.growth >= FLOWERS[c.flower].growTime) {
                    c.growth = 0.0f;
                    c.stage++;
                    c.watered = false;
                }
            }
        }
    }
}

bool Farm::TreeAnchor(int tx, int ty, int& ax, int& ay) const {
    if (!InBounds(tx, ty)) return false;
    const Cell& c = cells[Idx(tx, ty)];
    if (c.big == 1) { ax = tx; ay = ty; return true; }
    if (c.big == 2) {
        const int dx[3] = { -1, 0, -1 }, dy[3] = { 0, -1, -1 };
        for (int k = 0; k < 3; k++) {
            int bx = tx + dx[k], by = ty + dy[k];
            if (InBounds(bx, by) && cells[Idx(bx, by)].big == 1) { ax = bx; ay = by; return true; }
        }
    }
    return false;
}

bool Farm::Tree2x2Soil(int tx, int ty, int& ax, int& ay) const {
    const int cxs[4] = { tx, tx-1, tx, tx-1 }, cys[4] = { ty, ty, ty-1, ty-1 };
    for (int k = 0; k < 4; k++) {
        int bx = cxs[k], by = cys[k];
        bool ok = true;
        for (int oy = 0; oy < 2 && ok; oy++)
            for (int ox = 0; ox < 2 && ok; ox++)
                if (!InBounds(bx+ox, by+oy) || cells[Idx(bx+ox, by+oy)].plot != Plot::Soil)
                    ok = false;
        if (ok) { ax = bx; ay = by; return true; }
    }
    return false;
}

void Farm::TargetArea(int tx, int ty, const Inventory& inv, int& ax, int& ay, int& size) const {
    int bx, by;
    if (TreeAnchor(tx, ty, bx, by)) { ax = bx; ay = by; size = 2; return; }   // copac existent
    if (FLOWERS[inv.selectedSeed].isTree && Tree2x2Soil(tx, ty, bx, by)) {
        ax = bx; ay = by; size = 2; return;                                    // pregătit de plantat copac
    }
    ax = tx; ay = ty; size = 1;
}

void Farm::Interact(int tx, int ty, Inventory& inv, Player& player) {
    if (!InBounds(tx, ty)) return;

    // dacă am dat click pe o celulă acoperită de un copac 2x2, redirecționăm spre ancoră
    int rax, ray;
    if (TreeAnchor(tx, ty, rax, ray)) { tx = rax; ty = ray; }

    Cell& c = cells[Idx(tx, ty)];

    switch (c.plot) {
        case Plot::Grass:
            player.StartAction(Action::Hoe);     // sapă
            c.plot = Plot::Soil;
            break;
        case Plot::Soil: {
            int f = inv.selectedSeed;
            if (inv.seeds[f] <= 0) break;

            if (FLOWERS[f].isTree) {
                // copac → are nevoie de 2x2 săpat
                int ax, ay;
                if (!Tree2x2Soil(tx, ty, ax, ay)) break;   // nu sunt 4 pătrățele săpate

                inv.seeds[f]--;
                for (int oy = 0; oy < 2; oy++) for (int ox = 0; ox < 2; ox++) {
                    Cell& s = cells[Idx(ax+ox, ay+oy)];
                    s.plot = Plot::Crop; s.flower = f; s.stage = 0; s.growth = 0; s.watered = false;
                    s.big = (ox == 0 && oy == 0) ? 1 : 2;   // colțul stânga-sus = ancoră
                }
                inv.AddXP(5);
                inv.EnsureValidSeed();
            } else {
                // floare normală → un singur pătrat
                inv.seeds[f]--;
                c.plot = Plot::Crop; c.flower = f; c.stage = 0; c.growth = 0; c.watered = false; c.big = 0;
                inv.AddXP(3);
                inv.EnsureValidSeed();
            }
            break;
        }
        case Plot::Crop:
            if (c.stage >= MatureStage) {            // matură
                bool tree = FLOWERS[c.flower].isTree;
                if (tree) {
                    if (!inv.hasAxe) break;           // fără topor nu poți tăia
                    player.StartAction(Action::Axe);
                    inv.harvested[c.flower]++;
                    inv.wood += 4;
                } else {
                    inv.harvested[c.flower]++;
                }
                inv.AddXP(10 + FLOWERS[c.flower].sellPrice / 20);

                if (tree && c.big == 1) {            // copac 2x2 → eliberează toate cele 4 pătrate
                    int ax = tx, ay = ty;
                    for (int oy = 0; oy < 2; oy++) for (int ox = 0; ox < 2; ox++) {
                        Cell& s = cells[Idx(ax+ox, ay+oy)];
                        s = Cell{}; s.plot = Plot::Soil;
                    }
                } else {
                    c.plot = Plot::Soil; c.stage = 0; c.growth = 0; c.watered = false; c.big = 0;
                }
            } else if (!c.watered) {                  // udă → pornește creșterea spre stadiul următor
                player.StartAction(Action::Watercan);
                c.watered = true;
                if (c.big == 1)                       // copac 2x2 → udă toate cele 4 pătrate
                    for (int oy = 0; oy < 2; oy++) for (int ox = 0; ox < 2; ox++)
                        cells[Idx(tx+ox, ty+oy)].watered = true;
            }
            break;
    }
}

void Farm::Serialize(std::ostream& o) const {
    int n = 0;
    for (const auto& c : cells) if (c.plot != Plot::Grass) n++;
    o << n << "\n";
    for (int i = 0; i < (int)cells.size(); i++) {
        const Cell& c = cells[i];
        if (c.plot == Plot::Grass) continue;
        o << i << " " << (int)c.plot << " " << c.flower << " " << c.stage << " "
          << c.growth << " " << (c.watered ? 1 : 0) << " " << c.big << "\n";
    }
}

void Farm::Deserialize(std::istream& in) {
    for (auto& c : cells) c = Cell{};
    int n; in >> n;
    for (int k = 0; k < n; k++) {
        int i, p, fl, st, wt, bg; float g;
        in >> i >> p >> fl >> st >> g >> wt >> bg;
        if (i >= 0 && i < (int)cells.size())
            cells[i] = Cell{ (Plot)p, fl, st, g, wt != 0, bg };
    }
}

void Farm::DrawGround(const Camera2D& cam) const {
    const int TS = TileMap::TileSize;
    Vector2 tl = GetScreenToWorld2D({ 0, 0 }, cam);
    Vector2 br = GetScreenToWorld2D({ (float)GetScreenWidth(), (float)GetScreenHeight() }, cam);
    int x0 = (int)floorf(tl.x / TS) - 1, y0 = (int)floorf(tl.y / TS) - 1;
    int x1 = (int)ceilf(br.x / TS) + 1,  y1 = (int)ceilf(br.y / TS) + 1;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 > TileMap::Width)  x1 = TileMap::Width;
    if (y1 > TileMap::Height) y1 = TileMap::Height;

    // pământul săpat (mai închis = udat)
    for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) {
        const Cell& c = cells[Idx(x, y)];
        if (c.plot == Plot::Soil || c.plot == Plot::Crop) {
            DrawTexturePro(soilAtlas, kSoil,
                Rectangle{ (float)(x*TS), (float)(y*TS), (float)TS, (float)TS }, {0,0}, 0, WHITE);
            if (c.plot == Plot::Crop && c.watered)
                DrawRectangle(x*TS, y*TS, TS, TS, Color{ 20, 30, 80, 70 });   // pământ ud
        }
    }
    // plantele (bottom-anchored) — textură, rect și scală după tipul plantei
    for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) {
        const Cell& c = cells[Idx(x, y)];
        if (c.plot != Plot::Crop || c.big == 2) continue;   // sclavii 2x2 nu se desenează singuri
        if (c.stage > 0) {
            const FlowerInfo& fi = FLOWERS[c.flower];
            Rectangle src = (c.stage == 1) ? fi.r1 : fi.r2;
            // copacul 2x2 = centrat în cele 4 pătrate, dimensiune potrivită (nu enorm)
            float extra = (c.big == 1) ? 1.3f : 1.0f;
            float cx = (c.big == 1) ? (x + 1.0f) * TS : x * TS + TS / 2.0f;
            float by = (c.big == 1) ? (y + 1.85f) * TS : (y + 1.0f) * TS;   // baza spre centrul 2x2
            float w = src.width * fi.scale * extra, h = src.height * fi.scale * extra;
            DrawTexturePro(ftex[fi.tex], src,
                Rectangle{ cx - w/2.0f, by - h, w, h }, {0,0}, 0, WHITE);
        }
        if (c.stage < MatureStage) {
            float cxOff = (c.big == 1) ? TS : TS/2.0f;          // centrat peste 2x2 dacă e copac
            float dx = x*TS + cxOff;
            if (!c.watered) {
                // picătură albastră: are nevoie de apă acum
                float bob = sinf(GetTime() * 3.0f + x + y) * 2.0f;
                float dy = y*TS - 6 + bob;
                DrawCircle((int)dx, (int)dy, 5, Color{ 70, 150, 240, 255 });
                DrawTriangle({ dx-5, dy-1 }, { dx+5, dy-1 }, { dx, dy-9 }, Color{ 70, 150, 240, 255 });
                DrawCircle((int)dx-1, (int)dy-1, 2, Color{ 200, 230, 255, 255 });
            } else {
                // timer: cât mai durează până trebuie udată din nou (mm:ss)
                int rem = (int)ceilf(FLOWERS[c.flower].growTime - c.growth);
                if (rem < 0) rem = 0;
                const char* t = TextFormat("%d:%02d", rem / 60, rem % 60);
                int tw = MeasureText(t, 10);
                int bx = (int)dx - tw/2 - 3, byy = y*TS - 16;
                DrawRectangle(bx, byy, tw + 6, 13, Color{ 20, 40, 25, 200 });
                DrawText(t, bx + 3, byy + 1, 10, Color{ 160, 240, 160, 255 });
            }
        }
    }
}

void Farm::DrawTargetHighlight(int ax, int ay, int size, bool inRange) const {
    if (!InBounds(ax, ay)) return;
    const int TS = TileMap::TileSize;
    Color c = inRange ? Color{ 255, 255, 255, 200 } : Color{ 230, 70, 70, 160 };
    Rectangle r{ (float)(ax*TS), (float)(ay*TS), (float)(size*TS), (float)(size*TS) };
    if (inRange) DrawRectangleRec(r, Color{ 255, 255, 255, 35 });
    DrawRectangleLinesEx(r, 2.0f, c);
}
