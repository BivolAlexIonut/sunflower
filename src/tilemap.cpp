#include "tilemap.h"
#include <cmath>
#include <ostream>
#include <istream>

static unsigned int Hash(int x, int y) {
    unsigned int h = (unsigned int)(x * 73856093) ^ (unsigned int)(y * 19349663);
    h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
    return h;
}

void TileMap::Load() {
    const char* NA = "sprites/NewAssets/";
    grass[0] = LoadTexture(TextFormat("%sTiles/Grass/Grass_1_Middle.png", NA));
    grass[1] = LoadTexture(TextFormat("%sTiles/Grass/Grass_2_Middle.png", NA));
    grass[2] = LoadTexture(TextFormat("%sTiles/Grass/Grass_3_Middle.png", NA));
    grass[3] = LoadTexture(TextFormat("%sTiles/Grass/Grass_4_Middle.png", NA));
    path     = LoadTexture(TextFormat("%sTiles/Grass/Path_Middle.png", NA));
    hedge    = LoadTexture(TextFormat("%sTiles/Hedge_Tiles.png", NA));
    water     = LoadTexture(TextFormat("%sTiles/Water/Water_Tile_1.png", NA));
    caveFloor = LoadTexture(TextFormat("%sTiles/Cave/Cave_Floor_Middle.png", NA));
    caveWall  = LoadTexture(TextFormat("%sTiles/Cave/Cave_Walls.png", NA));
    tiles.assign(Width * Height, Terrain::Grass);
}

void TileMap::Unload() {
    for (int i = 0; i < 4; i++) UnloadTexture(grass[i]);
    UnloadTexture(path);
    UnloadTexture(hedge);
    UnloadTexture(water);
    UnloadTexture(caveFloor);
    UnloadTexture(caveWall);
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
    return t == Terrain::Wall || t == Terrain::Fence || t == Terrain::Water;
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
    return (int)(Hash(pc * 13 + 7, pr * 17 + 3) % 5);
}
const char* TileMap::ThemeName(int t) {
    switch (t) {
        case 0:  return "Padure (lemn)";
        case 1:  return "Mina de cristale";
        case 2:  return "Crang des (lemn)";
        case 3:  return "Livada (lemn + seminte)";
        default: return "Mina bogata (cristale)";
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

    // iaz decorativ (lângă grădină, stânga) — apă, blochează mersul
    for (int ty = 21; ty <= 24; ty++)
        for (int tx = 15; tx <= 18; tx++)
            Set(tx, ty, Terrain::Water);
}

static const Rectangle kSrc16{ 0, 0, 16, 16 };

void TileMap::DrawGrass(int tx, int ty) const {
    const float TS = (float)TileSize;
    unsigned int h = Hash(tx, ty);
    int v = (h % 13 == 0) ? 1 : (h % 29 == 0) ? 2 : 0;   // variație rară, discretă
    DrawTexturePro(grass[v], kSrc16,
        Rectangle{ tx*TS, ty*TS, TS, TS }, { 0, 0 }, 0, WHITE);
}

// Gard verde (Hedge_Tiles): bloc 9-slice la offset (16,16). Aleg piesa după poziția în grădină.
void TileMap::DrawHedge(int tx, int ty) const {
    const float TS = (float)TileSize;
    int col = (tx == GX0) ? 0 : (tx == GX1) ? 2 : 1;
    int row = (ty == GY0) ? 0 : (ty == GY1) ? 2 : 1;
    Rectangle src{ 16.0f + col*16, 16.0f + row*16, 16, 16 };
    DrawTexturePro(hedge, src, Rectangle{ tx*TS, ty*TS, TS, TS }, { 0, 0 }, 0, WHITE);
}

// Iaz (Water_Tile_1): 9-slice (top 3x3) după vecinii de apă.
void TileMap::DrawWater(int tx, int ty) const {
    const float TS = (float)TileSize;
    bool wl = At(tx-1, ty) == Terrain::Water, wr = At(tx+1, ty) == Terrain::Water;
    bool wu = At(tx, ty-1) == Terrain::Water, wd = At(tx, ty+1) == Terrain::Water;
    int col = (!wl) ? 0 : (!wr) ? 2 : 1;
    int row = (!wu) ? 0 : (!wd) ? 2 : 1;
    Rectangle src{ (float)(col*16), (float)(row*16), 16, 16 };
    DrawTexturePro(water, src, Rectangle{ tx*TS, ty*TS, TS, TS }, { 0, 0 }, 0, WHITE);
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
            Rectangle dst{ (float)(x*TileSize), (float)(y*TileSize), (float)TileSize, (float)TileSize };
            switch (t) {
                case Terrain::Grass:
                case Terrain::GrassDark:
                    DrawGrass(x, y);
                    break;
                case Terrain::Dirt:                       // cărare pe iarbă
                    DrawGrass(x, y);
                    DrawTexturePro(path, kSrc16, dst, { 0, 0 }, 0, WHITE);
                    break;
                case Terrain::Fence:                      // gard verde pe iarbă
                    DrawGrass(x, y);
                    DrawHedge(x, y);
                    break;
                case Terrain::Water:                      // iaz pe iarbă
                    DrawGrass(x, y);
                    DrawWater(x, y);
                    break;
                case Terrain::Stone:                      // podea de peșteră
                    DrawTexturePro(caveFloor, kSrc16, dst, { 0, 0 }, 0, WHITE);
                    break;
                case Terrain::Wall:                       // perete de stâncă
                    DrawTexturePro(caveWall, Rectangle{ 16, 96, 16, 16 }, dst, { 0, 0 }, 0, WHITE);
                    break;
            }
        }
    }
}
