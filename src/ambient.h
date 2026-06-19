// Viață ambientală: albine + fluturi care zboară, și flori decorative împrăștiate pe iarbă.
// Pur cosmetic (nu se salvează, nu afectează gameplay-ul).
#pragma once
#include "raylib.h"
#include <vector>

class TileMap;

class Ambient {
public:
    void Load();
    void Unload();
    void Init(const TileMap& map);     // populează critters + flori decorative
    void Update(float dt);
    void DrawGroundDecor() const;      // flori (în spatele jucătorului)
    void DrawCritters() const;         // albine/fluturi (deasupra)
    void ClearDecoTile(int tx, int ty);   // scoate floarea decorativă de pe un tile (la săpat)

private:
    Texture2D beeTex{}, flyTex{}, flowerTex{};

    struct Critter {
        Vector2 pos{}, target{};
        int type = 0;        // 0 = albină, 1 = fluture
        int colorRow = 0;    // fluture: rândul de culoare (0-3)
        float frameTimer = 0; int frame = 0;
        float wob = 0;       // fază pentru legănat / fâlfâit
    };
    std::vector<Critter> critters;
    Rectangle bounds{};

    struct Deco { Vector2 pos; Rectangle src; };
    std::vector<Deco> decos;

    void NewTarget(Critter& c);
};
