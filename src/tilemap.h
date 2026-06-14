// Harta de joc — deocamdată iarbă generată procedural.
// Mai târziu o înlocuim cu tile-uri reale (Sprout Lands) și teren editabil.
#pragma once
#include "raylib.h"

class TileMap {
public:
    static constexpr int TileSize = 32;
    static constexpr int Width  = 64;   // tile-uri pe orizontală
    static constexpr int Height = 48;   // tile-uri pe verticală

    void Draw(const Camera2D& cam) const;

    // limitele lumii în pixeli (pentru a ține personajul în hartă)
    float WorldWidth()  const { return Width  * (float)TileSize; }
    float WorldHeight() const { return Height * (float)TileSize; }
};
