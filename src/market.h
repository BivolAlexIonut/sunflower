// Scena de market — vedere 2D side-view (ca un orășel), shop avansat.
// Se intră din lumea top-down; ieși cu ESC sau pe marginea din stânga.
#pragma once
#include "raylib.h"

class Inventory;
class Player;

class Market {
public:
    void Load();
    void Unload();
    void Enter(Player& player);                              // poziționează jucătorul la intrare

    bool Update(float dt, Player& player, Inventory& inv);    // true = ieși înapoi în lume
    void Draw(const Player& player, const Inventory& inv) const;

    static constexpr float StreetW = 1920.0f;
    static constexpr float GroundY = 440.0f;

private:
    Texture2D bg{}, ground{}, groundFill{}, wall{}, roof{}, door{}, furnace{};
    Texture2D barrel{}, crate{}, goods{}, sign{}, stall{}, lantern{}, herbs{};

    Camera2D cam{};
    int nearStation = -1;   // taraba din apropiere (-1 = niciuna)
};
