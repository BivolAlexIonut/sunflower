#include "tilemap.h"
#include <cmath>

// Hash determinist per-tile → aceeași variație la fiecare frame (fără pâlpâire).
static unsigned int TileHash(int x, int y) {
    unsigned int h = (unsigned int)(x * 73856093) ^ (unsigned int)(y * 19349663);
    h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
    return h;
}

void TileMap::Draw(const Camera2D& cam) const {
    // Desenăm doar tile-urile vizibile pe ecran (culling) — esențial pentru hărți mari.
    Vector2 topLeft = GetScreenToWorld2D({ 0, 0 }, cam);
    Vector2 botRight = GetScreenToWorld2D(
        { (float)GetScreenWidth(), (float)GetScreenHeight() }, cam);

    int x0 = (int)floorf(topLeft.x / TileSize) - 1;
    int y0 = (int)floorf(topLeft.y / TileSize) - 1;
    int x1 = (int)ceilf(botRight.x / TileSize) + 1;
    int y1 = (int)ceilf(botRight.y / TileSize) + 1;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > Width)  x1 = Width;
    if (y1 > Height) y1 = Height;

    // nuanțe de verde apropiate de tileset-ul Happy Harvest (50,150,90)
    const Color greens[3] = {
        Color{ 50, 150, 90, 255 },
        Color{ 56, 158, 96, 255 },
        Color{ 44, 140, 82, 255 },
    };

    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            unsigned int h = TileHash(x, y);
            Color c = greens[h % 3];
            DrawRectangle(x * TileSize, y * TileSize, TileSize, TileSize, c);

            // ocazional un smoc de iarbă mai închisă, ca textură
            if (h % 7 == 0) {
                int px = x * TileSize + (int)(h % TileSize);
                int py = y * TileSize + (int)((h / 7) % TileSize);
                DrawRectangle(px, py, 3, 5, Color{ 38, 120, 70, 255 });
            }
        }
    }
}
