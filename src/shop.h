// Magazin (TAB) + meniu de skin-uri (K). Meniuri overlay care îngheață jocul.
#pragma once
#include "raylib.h"

class Inventory;
class Player;

class Shop {
public:
    void HandleInput(Inventory& inv, Player& player);
    void Draw(const Inventory& inv, const Texture2D& flowers, const Texture2D& icons) const;

    bool BlocksGameplay() const { return shopOpen || skinsOpen; }

    static constexpr int SkinCount = 5;

private:
    bool shopOpen = false;
    bool skinsOpen = false;
    int row = 0;          // rândul selectat în meniul curent

    bool skinUnlocked[SkinCount] = { true, false, false, false, false };
    int  skinApplied = 0;

    void DrawShop(const Inventory& inv, const Texture2D& flowers, const Texture2D& icons) const;
    void DrawSkins(const Inventory& inv) const;
};
