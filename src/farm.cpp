#include "farm.h"
#include "tilemap.h"
#include "inventory.h"
#include "player.h"
#include <cmath>
#include <ostream>
#include <istream>

// Tile de pământ săpat din FG_Grounds.png (16x16).
static const Rectangle kSoil = { 176, 80, 16, 16 };

// Floricele din FG_Grass_Summer.png (16x16): boboc mic (stadiu 1) + floare mare (stadiu 2).
static const Rectangle kBud[4] = {
    { 64, 0, 16, 16 }, { 80, 0, 16, 16 }, { 96, 0, 16, 16 }, { 112, 0, 16, 16 }
};
static const Rectangle kBloom[4] = {
    {  0, 0, 16, 16 }, { 16, 0, 16, 16 }, { 32, 0, 16, 16 }, { 48, 0, 16, 16 }
};

Farm::Farm() { cells.resize(TileMap::Width * TileMap::Height); }

void Farm::Load() {
    soilAtlas = LoadTexture("sprites/Tilesets/FG_Grounds.png");
    flowers   = LoadTexture("sprites/Objects/FG_Grass_Summer.png");
}
void Farm::Unload() {
    UnloadTexture(soilAtlas);
    UnloadTexture(flowers);
}

int Farm::Idx(int tx, int ty) const { return ty * TileMap::Width + tx; }
bool Farm::InBounds(int tx, int ty) const {
    return tx >= 0 && ty >= 0 && tx < TileMap::Width && ty < TileMap::Height;
}

void Farm::Update(float dt) {
    for (auto& c : cells) {
        if (c.plot == Plot::Crop && c.stage < MatureStage) {
            c.growth += dt;
            if (c.growth >= FLOWERS[c.flower].growTime) {
                c.growth = 0.0f;
                c.stage++;
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
            }
            break;
        case Plot::Crop:
            if (c.stage >= MatureStage) {            // recoltează
                inv.harvested[c.flower]++;
                c.plot = Plot::Soil;
                c.stage = 0;
                c.growth = 0.0f;
            } else {                                  // udă → grăbește creșterea
                player.StartAction(Action::Watercan);
                c.growth += FLOWERS[c.flower].growTime * 0.5f;
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
        o << i << " " << (int)c.plot << " " << c.flower << " " << c.stage << " " << c.growth << "\n";
    }
}

void Farm::Deserialize(std::istream& in) {
    for (auto& c : cells) c = Cell{};
    int n; in >> n;
    for (int k = 0; k < n; k++) {
        int i, p, fl, st; float g;
        in >> i >> p >> fl >> st >> g;
        if (i >= 0 && i < (int)cells.size())
            cells[i] = Cell{ (Plot)p, fl, st, g };
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

    // pământul săpat
    for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) {
        const Cell& c = cells[Idx(x, y)];
        if (c.plot == Plot::Soil || c.plot == Plot::Crop)
            DrawTexturePro(soilAtlas, kSoil,
                Rectangle{ (float)(x*TS), (float)(y*TS), (float)TS, (float)TS }, {0,0}, 0, WHITE);
    }
    // florile (bottom-anchored, scalate)
    const float sc = 2.0f;
    for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) {
        const Cell& c = cells[Idx(x, y)];
        if (c.plot != Plot::Crop || c.stage == 0) continue;
        Rectangle src = (c.stage == 1) ? kBud[c.flower] : kBloom[c.flower];
        float w = src.width * sc, h = src.height * sc;
        DrawTexturePro(flowers, src,
            Rectangle{ x*TS + TS/2.0f - w/2.0f, (y+1.0f)*TS - h, w, h }, {0,0}, 0, WHITE);
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
