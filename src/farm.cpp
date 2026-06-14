#include "farm.h"
#include "tilemap.h"
#include "inventory.h"
#include <cmath>

// Cele 4 stadii de creștere ale florii-soarelui, decupate din spritesheet.
// (x, y, w, h) în pixeli. Ancorate la bază când le desenăm.
static const Rectangle kStages[Farm::MaxStage + 1] = {
    { 80,  20, 12, 10 },   // 0: răsad
    { 95,  33, 18, 21 },   // 1: seedling
    { 96, 103, 20, 26 },   // 2: floarea-soarelui tânără
    { 77,  95, 21, 35 },   // 3: floarea-soarelui matură
};

Farm::Farm() {
    cells.resize(TileMap::Width * TileMap::Height);
}

int Farm::Idx(int tx, int ty) const { return ty * TileMap::Width + tx; }

bool Farm::InBounds(int tx, int ty) const {
    return tx >= 0 && ty >= 0 && tx < TileMap::Width && ty < TileMap::Height;
}

void Farm::Update(float dt) {
    for (auto& c : cells) {
        if (c.plot == Plot::Crop && c.stage < MaxStage) {
            c.growth += dt;
            if (c.growth >= StageTime) {
                c.growth -= StageTime;
                c.stage++;
            }
        }
    }
}

void Farm::Interact(int tx, int ty, Inventory& inv) {
    if (!InBounds(tx, ty)) return;
    Cell& c = cells[Idx(tx, ty)];

    switch (c.plot) {
        case Plot::Grass:
            c.plot = Plot::Soil;               // sapă pământul
            break;
        case Plot::Soil:
            if (inv.ConsumeSeed()) {           // plantează dacă are semințe
                c.plot = Plot::Crop;
                c.stage = 0;
                c.growth = 0.0f;
            }
            break;
        case Plot::Crop:
            if (c.stage >= MaxStage) {         // recoltează floarea matură
                inv.AddHarvest();
                c.plot = Plot::Soil;
                c.stage = 0;
                c.growth = 0.0f;
            }
            break;
    }
}

void Farm::DrawGround(const Camera2D& cam, const Texture2D& plants) const {
    const int TS = TileMap::TileSize;

    // culling: doar tile-urile vizibile
    Vector2 tl = GetScreenToWorld2D({ 0, 0 }, cam);
    Vector2 br = GetScreenToWorld2D({ (float)GetScreenWidth(), (float)GetScreenHeight() }, cam);
    int x0 = (int)floorf(tl.x / TS) - 1, y0 = (int)floorf(tl.y / TS) - 1;
    int x1 = (int)ceilf(br.x / TS) + 1,  y1 = (int)ceilf(br.y / TS) + 1;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 > TileMap::Width)  x1 = TileMap::Width;
    if (y1 > TileMap::Height) y1 = TileMap::Height;

    // 1) pământul săpat (sub culturi)
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            const Cell& c = cells[Idx(x, y)];
            if (c.plot == Plot::Soil || c.plot == Plot::Crop) {
                DrawRectangle(x * TS + 1, y * TS + 1, TS - 2, TS - 2, Color{ 110, 75, 45, 255 });
                DrawRectangleLines(x * TS + 1, y * TS + 1, TS - 2, TS - 2, Color{ 80, 52, 30, 255 });
            }
        }
    }

    // 2) culturile (desenate bottom-anchored ca să "crească" din pământ)
    const float scale = 1.5f;
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            const Cell& c = cells[Idx(x, y)];
            if (c.plot != Plot::Crop) continue;
            Rectangle src = kStages[c.stage];
            float w = src.width * scale, h = src.height * scale;
            Rectangle dst{
                x * TS + TS / 2.0f - w / 2.0f,
                (y + 1) * TS - h,                // baza plantei pe marginea de jos a tile-ului
                w, h
            };
            DrawTexturePro(plants, src, dst, { 0, 0 }, 0.0f, WHITE);
        }
    }
}

void Farm::DrawTargetHighlight(int tx, int ty) const {
    if (!InBounds(tx, ty)) return;
    const int TS = TileMap::TileSize;
    DrawRectangleLinesEx(
        Rectangle{ (float)(tx * TS), (float)(ty * TS), (float)TS, (float)TS },
        2.0f, Color{ 255, 255, 255, 180 });
}
