#include "tilemap.h"
#include <cmath>

// Tile-uri de umplere solidă găsite în atlas (16x16, pixeli sursă).
static const Rectangle kGrass[3] = {
    { 304, 16, 16, 16 },   // verde deschis
    { 400, 16, 16, 16 },   // verde mai închis
    { 304, 80, 16, 16 },   // verde cu textură ușoară
};

static unsigned int TileHash(int x, int y) {
    unsigned int h = (unsigned int)(x * 73856093) ^ (unsigned int)(y * 19349663);
    h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
    return h;
}

void TileMap::Load() {
    atlas = LoadTexture("sprites/Tilesets/FG_Grounds.png");
}

void TileMap::Unload() {
    UnloadTexture(atlas);
}

void TileMap::Draw(const Camera2D& cam) const {
    // culling: doar tile-urile vizibile
    Vector2 tl = GetScreenToWorld2D({ 0, 0 }, cam);
    Vector2 br = GetScreenToWorld2D({ (float)GetScreenWidth(), (float)GetScreenHeight() }, cam);
    int x0 = (int)floorf(tl.x / TileSize) - 1, y0 = (int)floorf(tl.y / TileSize) - 1;
    int x1 = (int)ceilf(br.x / TileSize) + 1,  y1 = (int)ceilf(br.y / TileSize) + 1;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 > Width)  x1 = Width;
    if (y1 > Height) y1 = Height;

    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            unsigned int h = TileHash(x, y);
            // mai ales verde deschis, ocazional celelalte variante
            int v = (h % 6 == 0) ? 1 : (h % 11 == 0) ? 2 : 0;
            Rectangle src = kGrass[v];
            Rectangle dst{ (float)(x * TileSize), (float)(y * TileSize),
                           (float)TileSize, (float)TileSize };
            DrawTexturePro(atlas, src, dst, { 0, 0 }, 0.0f, WHITE);
        }
    }
}
