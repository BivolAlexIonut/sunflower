// Meniul jocului (TAB) cu file: Magazin, Skin-uri, Ajutor.
#pragma once
#include "raylib.h"

class Inventory;
class Player;

class Shop {
public:
    void HandleInput(Inventory& inv, Player& player);
    void Draw(const Inventory& inv, const Texture2D& flowers, const Texture2D& icons) const;

    bool BlocksGameplay() const { return open; }

    static constexpr int SkinCount = 5;
    static constexpr int TabCount = 3;

private:
    bool open = false;
    int tab = 0;          // 0 = Magazin, 1 = Skin-uri, 2 = Ajutor
    int row = 0;

    bool skinUnlocked[SkinCount] = { true, false, false, false, false };
    int  skinApplied = 0;

    void DrawFrame(int& x, int& y, int& w, int& h) const;
    void DrawTabs(int x, int y, int w) const;
    void DrawShop(int x, int y, int w, int h, const Inventory& inv,
                  const Texture2D& flowers, const Texture2D& icons) const;
    void DrawSkins(int x, int y, int w, int h, const Inventory& inv) const;
    void DrawHelp(int x, int y, int w, int h) const;
};
