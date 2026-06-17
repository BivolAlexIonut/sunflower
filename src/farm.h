// Ferma — parcele lucrate cu uneltele personajului: sapă, plantează, udă, recoltează.
#pragma once
#include "raylib.h"
#include <vector>
#include <iosfwd>

class Inventory;
class Player;

enum class Plot { Grass, Soil, Crop };

struct Cell {
    Plot plot = Plot::Grass;
    int flower = 0;       // tip floare (index în FLOWERS)
    int stage = 0;        // 0 = plantat, 1 = răsărit, 2 = matură
    float growth = 0.0f;
    bool watered = false; // crește doar dacă e udată; se resetează la fiecare stadiu
};

class Farm {
public:
    Farm();
    void Load();
    void Unload();

    void Update(float dt);
    void DrawGround(const Camera2D& cam) const;
    void DrawTargetHighlight(int tx, int ty, bool inRange) const;

    // Acțiune contextuală pe parcela țintă; declanșează animația de unealtă potrivită.
    void Interact(int tx, int ty, Inventory& inv, Player& player);

    void Serialize(std::ostream&) const;
    void Deserialize(std::istream&);

    int CropCount() const;   // câte plante sunt în pământ (pentru XP/level)

    static constexpr int MatureStage = 2;

private:
    std::vector<Cell> cells;
    Texture2D soilAtlas{};   // FG_Grounds (pentru tile-ul de pământ)
    Texture2D ftex[4]{};     // 0 vară, 1 iarnă, 2 toamnă, 3 Plants

    int Idx(int tx, int ty) const;
    bool InBounds(int tx, int ty) const;
};
