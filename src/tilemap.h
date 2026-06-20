// Harta cu zone: pădure (iarbă+copaci), grădină (iarbă deschisă), dungeon (piatră+ziduri).
#pragma once
#include "raylib.h"
#include <vector>
#include <iosfwd>

enum class Terrain { Grass, GrassDark, Dirt, Stone, Wall, Fence, Water, Sand, Bridge };

class TileMap {
public:
    static constexpr int TileSize = 32;
    static constexpr int Width  = 90;    // hartă mare: acasă (50x32) + zone de deblocat
    static constexpr int Height = 56;

    // Grădina de start: mică, cu gard în jur (în tile-uri).
    static constexpr int GX0 = 20, GY0 = 11, GX1 = 29, GY1 = 19;
    static constexpr int DunX0 = 34, DunY0 = 5, DunX1 = 49, DunY1 = 26;

    // Țarcul de animale: o parcelă de cumpărat (sub grădină). Conturul = gard.
    static constexpr int PenPC = 2, PenPR = 4;
    static constexpr int PenX0 = 20, PenY0 = 33, PenX1 = 28, PenY1 = 39;
    bool PenOwned() const { return PlotOwned(PenPC, PenPR); }

    // Parcele de teren (Forager): grilă peste hartă; cele de start sunt deținute.
    static constexpr int PlotW = 10, PlotH = 8;
    static constexpr int PlotCols = Width / PlotW;   // 7
    static constexpr int PlotRows = Height / PlotH;  // 6

    bool PlotOwned(int pc, int pr) const;
    bool TileLocked(int tx, int ty) const;           // tile într-o parcelă necumpărată
    int  PlotCost(int pc, int pr) const;
    int  PlotLevel(int pc, int pr) const;            // nivel necesar
    bool PlotAdjacentOwned(int pc, int pr) const;    // lipită de o parcelă deținută
    void BuyPlot(int pc, int pr);
    int  PlotTheme(int pc, int pr) const;            // 0 pădure, 1 mină cristale, 2 crâng des
    static const char* ThemeName(int theme);
    void DrawFog(const Camera2D& cam, bool landMode) const;

    void Load();
    void Unload();
    void Build();
    void Draw(const Camera2D& cam) const;

    Terrain At(int tx, int ty) const;
    bool IsSolid(int tx, int ty) const;   // Wall = blochează mișcarea
    bool CanFarm(int tx, int ty) const;   // se poate săpa (iarbă)

    void Place(int tx, int ty, Terrain t);  // construire jucător (drum/piatră), se salvează
    void Serialize(std::ostream&) const;    // doar tile-urile construite de jucător
    void Deserialize(std::istream&);

    float WorldWidth()  const { return Width  * (float)TileSize; }
    float WorldHeight() const { return Height * (float)TileSize; }

private:
    std::vector<Terrain> tiles;
    std::vector<int> placed;   // indicii tile-urilor construite de jucător (pentru salvare)
    std::vector<char> owned;   // parcele deținute (PlotCols*PlotRows)
    Texture2D grass[4]{};   // NewAssets Grass_1..4_Middle (iarbă lush, 16px)
    Texture2D path{};       // NewAssets Path_Middle (cărare)
    Texture2D hedge{};      // NewAssets Hedge_Tiles (gard verde, autotile 4x4)
    Texture2D water{};      // NewAssets Water_Tile_1 (iaz, autotile)
    Texture2D caveFloor{};  // NewAssets Cave_Floor_Middle (podea peșteră)
    Texture2D caveWall{};   // NewAssets Cave_Walls (perete de stâncă)
    Texture2D beach{};      // NewAssets Beach_Tiles (nisip)
    Texture2D bridge{};     // NewAssets Bridge_Wood (pod peste apă)

    int Idx(int tx, int ty) const { return ty * Width + tx; }
    void Set(int tx, int ty, Terrain t);
    void DrawGrass(int tx, int ty) const;    // iarbă cu variație
    void DrawHedge(int tx, int ty) const;    // gard verde (9-slice după poziția în grădină)
    void DrawWater(int tx, int ty) const;    // iaz (9-slice după vecini)
};
