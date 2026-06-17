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
        case Terrain::Fence:     return { 304, 80, 16, 16 };
    }
    return { 304, 80, 16, 16 };
}

void TileMap::Load() {
    grounds  = LoadTexture("sprites/Tilesets/FG_Grounds.png");
    fortress = LoadTexture("sprites/RPG Maker/RPG Maker MZ (32x32)/tilesets/FG_Fortress_A5.png");
    fence    = LoadTexture("sprites/Tilesets/spr_fencePost.png");
    tiles.assign(Width * Height, Terrain::Grass);
}

void TileMap::Unload() {
    UnloadTexture(grounds);
    UnloadTexture(fortress);
    UnloadTexture(fence);
}

void TileMap::Set(int tx, int ty, Terrain t) {
    if (tx >= 0 && ty >= 0 && tx < Width && ty < Height) tiles[Idx(tx, ty)] = t;
}

Terrain TileMap::At(int tx, int ty) const {
    if (tx < 0 || ty < 0 || tx >= Width || ty >= Height) return Terrain::Grass;
    return tiles[Idx(tx, ty)];
}

bool TileMap::IsSolid(int tx, int ty) const {
    Terrain t = At(tx, ty);
    return t == Terrain::Wall || t == Terrain::Fence;
}

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

bool TileMap::PlotOwned(int pc, int pr) const {
    if (pc < 0 || pr < 0 || pc >= PlotCols || pr >= PlotRows) return false;
    return owned[pr * PlotCols + pc] != 0;
}

bool TileMap::TileLocked(int tx, int ty) const {
    if (tx < 0 || ty < 0 || tx >= Width || ty >= Height) return true;
    return !PlotOwned(tx / PlotW, ty / PlotH);
}

int TileMap::PlotCost(int pc, int pr) const {
    int tier = (pc > 4 ? pc - 4 : 0) + (pr > 3 ? pr - 3 : 0);
    if (tier < 1) tier = 1;
    return 200 * tier;
}
int TileMap::PlotLevel(int pc, int pr) const {
    int tier = (pc > 4 ? pc - 4 : 0) + (pr > 3 ? pr - 3 : 0);
    return 2 + tier;
}
bool TileMap::PlotAdjacentOwned(int pc, int pr) const {
    return PlotOwned(pc-1, pr) || PlotOwned(pc+1, pr) ||
           PlotOwned(pc, pr-1) || PlotOwned(pc, pr+1);
}
void TileMap::BuyPlot(int pc, int pr) {
    if (pc >= 0 && pr >= 0 && pc < PlotCols && pr < PlotRows)
        owned[pr * PlotCols + pc] = 1;
}

int TileMap::PlotTheme(int pc, int pr) const {
    return (int)(Hash(pc * 13 + 7, pr * 17 + 3) % 3);
}
const char* TileMap::ThemeName(int t) {
    switch (t) {
        case 0:  return "Padure (lemn)";
        case 1:  return "Mina de cristale";
        default: return "Crang des";
    }
}

void TileMap::DrawFog(const Camera2D& cam, bool landMode) const {
    for (int pr = 0; pr < PlotRows; pr++)
        for (int pc = 0; pc < PlotCols; pc++) {
            if (PlotOwned(pc, pr)) continue;
            Rectangle r{ (float)(pc*PlotW*TileSize), (float)(pr*PlotH*TileSize),
                         (float)(PlotW*TileSize), (float)(PlotH*TileSize) };
            // în modul hartă: doar un voal ușor + chenar; în joc: ceață densă
            DrawRectangleRec(r, landMode ? Color{ 10,12,25,110 } : Color{ 8,10,20,205 });
            if (landMode) {
                bool avail = PlotAdjacentOwned(pc, pr);
                Color edge = avail ? Color{ 255,220,90,255 } : Color{ 90,90,110,255 };
                DrawRectangleLinesEx(r, 2, edge);
            }
        }
}

void TileMap::Serialize(std::ostream& o) const {
    o << placed.size() << "\n";
    for (int i : placed) o << i << " " << (int)tiles[i] << "\n";
    for (char c : owned) o << (int)c << " ";
    o << "\n";
}

void TileMap::Deserialize(std::istream& in) {
    placed.clear();
    int n; in >> n;
    for (int k = 0; k < n; k++) {
        int i, t; in >> i >> t;
        if (i >= 0 && i < (int)tiles.size()) { tiles[i] = (Terrain)t; placed.push_back(i); }
    }
    for (int k = 0; k < (int)owned.size(); k++) { int v; in >> v; owned[k] = (char)v; }
}

void TileMap::Build() {
    tiles.assign(Width * Height, Terrain::Grass);

    // parcele: deții regiunea originală (tx<50, ty<32); restul = teren de cumpărat
    owned.assign(PlotCols * PlotRows, 0);
    for (int pr = 0; pr < PlotRows; pr++)
        for (int pc = 0; pc < PlotCols; pc++)
            if (pc <= 4 && pr <= 3) owned[pr * PlotCols + pc] = 1;

    // puține pete de iarbă mai închisă (textură discretă), nu prea multe
    for (int ty = 0; ty < Height; ty++)
        for (int tx = 0; tx < Width; tx++)
            if (Hash(tx, ty) % 11 == 0)
                Set(tx, ty, Terrain::GrassDark);

    // GRĂDINA de start (mică) — gard de lemn în jur, iarbă înăuntru
    for (int ty = GY0; ty <= GY1; ty++) {
        for (int tx = GX0; tx <= GX1; tx++) {
            bool border = (tx == GX0 || tx == GX1 || ty == GY0 || ty == GY1);
            Set(tx, ty, border ? Terrain::Fence : Terrain::Grass);
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

    // curtea casei (jos, sub grădină) — platformă de pământ pentru power-up-uri + cufăr
    for (int ty = 28; ty <= 30; ty++)
        for (int tx = 10; tx <= 33; tx++)
            Set(tx, ty, Terrain::Dirt);
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
            // gardul: iarbă dedesubt + stâlp de lemn (bottom-anchored, puțin mai înalt)
            if (t == Terrain::Fence) {
                DrawTexturePro(grounds, SrcFor(Terrain::Grass),
                    Rectangle{ (float)(x*TileSize), (float)(y*TileSize), (float)TileSize, (float)TileSize },
                    {0,0}, 0, WHITE);
                Rectangle fsrc{ 44, 36, 26, 44 };   // un stâlp de gard din spr_fencePost
                DrawTexturePro(fence, fsrc,
                    Rectangle{ (float)(x*TileSize), (float)(y*TileSize) - 10, (float)TileSize, (float)TileSize + 10 },
                    {0,0}, 0, WHITE);
                continue;
            }
            const Texture2D& tex = (t == Terrain::Stone || t == Terrain::Wall) ? fortress : grounds;
            Rectangle src = SrcFor(t);
            Rectangle dst{ (float)(x * TileSize), (float)(y * TileSize),
                           (float)TileSize, (float)TileSize };
            DrawTexturePro(tex, src, dst, { 0, 0 }, 0.0f, WHITE);
        }
    }
}
