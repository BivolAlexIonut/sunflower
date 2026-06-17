// Harta cu zone: pădure (iarbă+copaci), grădină (iarbă deschisă), dungeon (piatră+ziduri).
#pragma once
#include "raylib.h"
#include <vector>
#include <iosfwd>

enum class Terrain { Grass, GrassDark, Dirt, Stone, Wall };

class TileMap {
public:
    static constexpr int TileSize = 32;
    static constexpr int Width  = 70;    // mărit: 50x32 conținut + teren nou de cumpărat
    static constexpr int Height = 48;

    // Limitele grădinii (incintă cu zid) și ale dungeon-ului (în tile-uri).
    static constexpr int GX0 = 17, GY0 = 8, GX1 = 31, GY1 = 23;
    static constexpr int DunX0 = 34, DunY0 = 5, DunX1 = 49, DunY1 = 26;

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
    Texture2D grounds{};    // FG_Grounds (iarbă/pământ, 16px)
    Texture2D fortress{};   // FG_Fortress_A5 (piatră/zid, 32px)

    int Idx(int tx, int ty) const { return ty * Width + tx; }
    void Set(int tx, int ty, Terrain t);
};
