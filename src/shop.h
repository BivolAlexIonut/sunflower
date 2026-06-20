// Meniul jocului (TAB) cu file: Magazin, Skin-uri, Ajutor.
#pragma once
#include "raylib.h"
#include <iosfwd>

class Inventory;
class Player;
class Memories;

class Shop {
public:
    void HandleInput(Inventory& inv, Player& player);
    void Draw(const Inventory& inv, const Texture2D* ftex, const Texture2D& icons,
              const Memories& mem) const;

    bool BlocksGameplay() const { return open; }
    void DebugOpen(int t) { open = true; tab = t; row = 0; }   // doar pentru screenshot-uri
    void SetAnimalsUnlocked(bool b) { animalsUnlocked = b; }    // țarcul de animale e cumpărat?

    void ApplySkin(Player& player) const;     // aplică skin-ul curent (după load)
    void Serialize(std::ostream&) const;
    void Deserialize(std::istream&);

    static constexpr int SkinCount = 5;
    static constexpr int TabCount = 7;

private:
    bool open = false;
    int tab = 0;          // 0 = Magazin, 1 = Skin-uri, 2 = Ajutor
    int row = 0;

    bool skinUnlocked[SkinCount] = { true, false, false, false, false };
    int  skinApplied = 0;
    bool animalsUnlocked = false;   // runtime: setat din main = map.PenOwned()

    void DrawFrame(int& x, int& y, int& w, int& h) const;
    void DrawTabs(int x, int y, int w) const;
    void DrawShop(int x, int y, int w, int h, const Inventory& inv, const Texture2D* ftex) const;
    void DrawUpgrades(int x, int y, int w, int h, const Inventory& inv) const;
    void DrawAnimals(int x, int y, int w, int h, const Inventory& inv) const;
    void DrawBuild(int x, int y, int w, int h, const Inventory& inv) const;
    void DrawSkins(int x, int y, int w, int h, const Inventory& inv) const;
    void DrawHelp(int x, int y, int w, int h) const;
};
