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

int Farm::Idx(int tx, int ty) const { return ty * TileMap::Width + tx; }
bool Farm::InBounds(int tx, int ty) const {
    return tx >= 0 && ty >= 0 && tx < TileMap::Width && ty < TileMap::Height;
}

void Farm::Update(float dt) {
    for (auto& c : cells) {
        // crește DOAR dacă e udată; după ce avansează un stadiu, are nevoie de udare din nou
        if (c.plot == Plot::Crop && c.stage < MatureStage && c.watered) {
            c.growth += dt;
            if (c.growth >= FLOWERS[c.flower].growTime) {
                c.growth = 0.0f;
                c.stage++;
                c.watered = false;
            }
        }
    }
}

void Farm::Interact(int tx, int ty, Inventory& inv, Player& player) {
    if (!InBounds(tx, ty)) return;
    Cell& c = cells[Idx(tx, ty)];

    switch (c.plot) {
        case Plot::Grass:
            player.StartAction(Action::Hoe);     // sapă
            c.plot = Plot::Soil;
            break;
        case Plot::Soil:
            if (inv.seeds[inv.selectedSeed] > 0) {   // plantează sămânța selectată
                inv.seeds[inv.selectedSeed]--;
                c.plot = Plot::Crop;
                c.flower = inv.selectedSeed;
                c.stage = 0;
                c.growth = 0.0f;
                c.watered = false;                    // proaspăt plantată — are nevoie de apă
            }
            break;
        case Plot::Crop:
            if (c.stage >= MatureStage) {            // matură
                if (FLOWERS[c.flower].isTree) {       // copac → se taie cu toporul (dă lemn + fruct)
                    if (!inv.hasAxe) break;           // fără topor nu poți tăia
                    player.StartAction(Action::Axe);
                    inv.harvested[c.flower]++;
                    inv.wood += 2;
                } else {                              // floare → se culege
                    inv.harvested[c.flower]++;
                }
                c.plot = Plot::Soil;
                c.stage = 0;
                c.growth = 0.0f;
                c.watered = false;
            } else if (!c.watered) {                  // udă → pornește creșterea spre stadiul următor
                player.StartAction(Action::Watercan);
                c.watered = true;
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
          << c.growth << " " << (c.watered ? 1 : 0) << "\n";
    }
}

void Farm::Deserialize(std::istream& in) {
    for (auto& c : cells) c = Cell{};
    int n; in >> n;
    for (int k = 0; k < n; k++) {
        int i, p, fl, st, wt; float g;
        in >> i >> p >> fl >> st >> g >> wt;
        if (i >= 0 && i < (int)cells.size())
            cells[i] = Cell{ (Plot)p, fl, st, g, wt != 0 };
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
        if (c.plot != Plot::Crop) continue;
        if (c.stage > 0) {
            const FlowerInfo& fi = FLOWERS[c.flower];
            Rectangle src = (c.stage == 1) ? fi.r1 : fi.r2;
            float w = src.width * fi.scale, h = src.height * fi.scale;
            DrawTexturePro(ftex[fi.tex], src,
                Rectangle{ x*TS + TS/2.0f - w/2.0f, (y+1.0f)*TS - h, w, h }, {0,0}, 0, WHITE);
        }
        // indicator "are nevoie de apă": picătură albastră care plutește deasupra
        if (c.stage < MatureStage && !c.watered) {
            float bob = sinf(GetTime() * 3.0f + x + y) * 2.0f;
            float dx = x*TS + TS/2.0f, dy = y*TS - 6 + bob;
            DrawCircle((int)dx, (int)dy, 5, Color{ 70, 150, 240, 255 });
            DrawTriangle({ dx-5, dy-1 }, { dx+5, dy-1 }, { dx, dy-9 }, Color{ 70, 150, 240, 255 });
            DrawCircle((int)dx-1, (int)dy-1, 2, Color{ 200, 230, 255, 255 });
        }
    }
}

void Farm::DrawTargetHighlight(int tx, int ty, bool inRange) const {
    if (!InBounds(tx, ty)) return;
    const int TS = TileMap::TileSize;
    Color c = inRange ? Color{ 255, 255, 255, 200 } : Color{ 230, 70, 70, 160 };
    Rectangle r{ (float)(tx*TS), (float)(ty*TS), (float)TS, (float)TS };
    if (inRange) DrawRectangleRec(r, Color{ 255, 255, 255, 35 });
    DrawRectangleLinesEx(r, 2.0f, c);
}
