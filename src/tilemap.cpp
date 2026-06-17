#include "tilemap.h"
#include <cmath>
#include <ostream>
#include <istream>

static unsigned int Hash(int x, int y) {
    unsigned int h = (unsigned int)(x * 73856093) ^ (unsigned int)(y * 19349663);
    h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
    return h;
}

// Surse în atlas (px). Iarbă/pământ = FG_Grounds (16px); piatră/zid = A5 (32px).
static Rectangle SrcFor(Terrain t) {
    switch (t) {
        case Terrain::Grass:     return { 304, 80, 16, 16 };
        case Terrain::GrassDark: return { 400, 80, 16, 16 };
        case Terrain::Dirt:      return {  80, 16, 16, 16 };
        case Terrain::Stone:     return {  32, 32, 32, 32 };
        case Terrain::Wall:      return {  64,  0, 32, 32 };
    }
    return { 304, 80, 16, 16 };
}

void TileMap::Load() {
    grounds  = LoadTexture("sprites/Tilesets/FG_Grounds.png");
    fortress = LoadTexture("sprites/RPG Maker/RPG Maker MZ (32x32)/tilesets/FG_Fortress_A5.png");
    tiles.assign(Width * Height, Terrain::Grass);
}

void TileMap::Unload() {
    UnloadTexture(grounds);
    UnloadTexture(fortress);
}

void TileMap::Set(int tx, int ty, Terrain t) {
    if (tx >= 0 && ty >= 0 && tx < Width && ty < Height) tiles[Idx(tx, ty)] = t;
}

Terrain TileMap::At(int tx, int ty) const {
    if (tx < 0 || ty < 0 || tx >= Width || ty >= Height) return Terrain::Grass;
    return tiles[Idx(tx, ty)];
}

bool TileMap::IsSolid(int tx, int ty) const { return At(tx, ty) == Terrain::Wall; }

bool TileMap::CanFarm(int tx, int ty) const {
    Terrain t = At(tx, ty);
    return t == Terrain::Grass || t == Terrain::GrassDark;
}

void TileMap::Place(int tx, int ty, Terrain t) {
    if (tx < 0 || ty < 0 || tx >= Width || ty >= Height) return;
    int i = Idx(tx, ty);
    tiles[i] = t;
    bool found = false;
    for (int p : placed) if (p == i) { found = true; break; }
    if (!found) placed.push_back(i);
}

void TileMap::Serialize(std::ostream& o) const {
    o << placed.size() << "\n";
    for (int i : placed) o << i << " " << (int)tiles[i] << "\n";
}

void TileMap::Deserialize(std::istream& in) {
    placed.clear();
    int n; in >> n;
    for (int k = 0; k < n; k++) {
        int i, t; in >> i >> t;
        if (i >= 0 && i < (int)tiles.size()) { tiles[i] = (Terrain)t; placed.push_back(i); }
    }
}

void TileMap::Build() {
    tiles.assign(Width * Height, Terrain::Grass);

    // pădurea (stânga) — pete de iarbă mai închisă pentru textură
    for (int ty = 0; ty < Height; ty++)
        for (int tx = 0; tx < 16; tx++)
            if (Hash(tx, ty) % 4 == 0) Set(tx, ty, Terrain::GrassDark);

    // GRĂDINA (centru) — incintă cu zid, iarbă înăuntru (se plantează doar aici)
    for (int ty = GY0; ty <= GY1; ty++) {
        for (int tx = GX0; tx <= GX1; tx++) {
            bool border = (tx == GX0 || tx == GX1 || ty == GY0 || ty == GY1);
            Set(tx, ty, border ? Terrain::Wall : Terrain::Grass);
        }
    }

    // DUNGEON (dreapta) — podea de piatră cu zid pe contur
    for (int ty = DunY0; ty <= DunY1; ty++) {
        for (int tx = DunX0; tx <= DunX1; tx++) {
            bool border = (tx == DunX0 || tx == DunX1 || ty == DunY0 || ty == DunY1);
            Set(tx, ty, border ? Terrain::Wall : Terrain::Stone);
        }
    }

    // cărarea de pământ: pădure → intrarea în grădină, și grădină → intrarea în dungeon
    for (int tx = 2; tx < GX0; tx++)        { Set(tx, 15, Terrain::Dirt); Set(tx, 16, Terrain::Dirt); }
    for (int tx = GX1 + 1; tx <= DunX0; tx++) { Set(tx, 15, Terrain::Dirt); Set(tx, 16, Terrain::Dirt); }

    // intrări (spărturi în ziduri la nivelul cărării)
    Set(GX0, 15, Terrain::Grass);  Set(GX0, 16, Terrain::Grass);   // grădină stânga
    Set(GX1, 15, Terrain::Grass);  Set(GX1, 16, Terrain::Grass);   // grădină dreapta
    Set(DunX0, 15, Terrain::Stone); Set(DunX0, 16, Terrain::Stone); // dungeon stânga
}

void TileMap::Draw(const Camera2D& cam) const {
    Vector2 tl = GetScreenToWorld2D({ 0, 0 }, cam);
    Vector2 br = GetScreenToWorld2D({ (float)GetScreenWidth(), (float)GetScreenHeight() }, cam);
    int x0 = (int)floorf(tl.x / TileSize) - 1, y0 = (int)floorf(tl.y / TileSize) - 1;
    int x1 = (int)ceilf(br.x / TileSize) + 1,  y1 = (int)ceilf(br.y / TileSize) + 1;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 > Width)  x1 = Width;
    if (y1 > Height) y1 = Height;

    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            Terrain t = tiles[Idx(x, y)];
            const Texture2D& tex = (t == Terrain::Stone || t == Terrain::Wall) ? fortress : grounds;
            Rectangle src = SrcFor(t);
            Rectangle dst{ (float)(x * TileSize), (float)(y * TileSize),
                           (float)TileSize, (float)TileSize };
            DrawTexturePro(tex, src, dst, { 0, 0 }, 0.0f, WHITE);
        }
    }
}
