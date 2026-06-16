// Harta cu zone: pădure (iarbă+copaci), grădină (iarbă deschisă), dungeon (piatră+ziduri).
#pragma once
#include "raylib.h"
#include <vector>

enum class Terrain { Grass, GrassDark, Dirt, Stone, Wall };

class TileMap {
public:
    static constexpr int TileSize = 32;
    static constexpr int Width  = 50;
    static constexpr int Height = 32;

    // Limitele grădinii (incintă cu zid) și ale dungeon-ului (în tile-uri).
    static constexpr int GX0 = 17, GY0 = 8, GX1 = 31, GY1 = 23;
    static constexpr int DunX0 = 34, DunY0 = 5, DunX1 = 49, DunY1 = 26;

    void Load();
    void Unload();
    void Build();
    void Draw(const Camera2D& cam) const;

    Terrain At(int tx, int ty) const;
    bool IsSolid(int tx, int ty) const;   // Wall = blochează mișcarea
    bool CanFarm(int tx, int ty) const;   // se poate săpa (iarbă)

    float WorldWidth()  const { return Width  * (float)TileSize; }
    float WorldHeight() const { return Height * (float)TileSize; }

private:
    std::vector<Terrain> tiles;
    Texture2D grounds{};    // FG_Grounds (iarbă/pământ, 16px)
    Texture2D fortress{};   // FG_Fortress_A5 (piatră/zid, 32px)

    int Idx(int tx, int ty) const { return ty * Width + tx; }
    void Set(int tx, int ty, Terrain t);
};
