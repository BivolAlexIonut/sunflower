// Harta de joc — iarbă pictată din atlas-ul FG_Grounds (pachetul FG).
#pragma once
#include "raylib.h"

class TileMap {
public:
    static constexpr int TileSize = 32;
    static constexpr int Width  = 40;
    static constexpr int Height = 30;

    void Load();
    void Unload();
    void Draw(const Camera2D& cam) const;

    float WorldWidth()  const { return Width  * (float)TileSize; }
    float WorldHeight() const { return Height * (float)TileSize; }

private:
    Texture2D atlas{};   // FG_Grounds.png
};
