// Magazinul — clădirea (casa) unde cumperi semințe și vinzi flori.
#pragma once
#include "raylib.h"

class Inventory;

class Shop {
public:
    void Load();
    void Unload();

    void DrawBuilding() const;                 // în coordonate de lume
    bool PlayerNear(Vector2 playerPos) const;  // jucătorul e lângă ușă?

    void Open()  { open = true; }
    void Close() { open = false; }
    bool IsOpen() const { return open; }

    void HandleInput(Inventory& inv);          // B = cumpără, S = vinde
    void DrawUI(const Inventory& inv) const;   // panoul, în coordonate de ecran

    Vector2 worldPos{ 0, 0 };                  // colțul stânga-sus al casei

    static constexpr int SeedCost  = 10;       // preț sămânță floarea-soarelui
    static constexpr int SellPrice = 25;       // preț de vânzare floare

private:
    Texture2D house{};
    bool open = false;

    Vector2 DoorPos() const;
};
