// Inventar simplu cu hotbar — semințe și flori recoltate.
#pragma once
#include "raylib.h"

class Inventory {
public:
    void HandleInput();
    void Draw(const Texture2D& plants) const;

    bool ConsumeSeed();     // true dacă exista cel puțin o sămânță
    void AddHarvest();      // recoltă: +1 floare (se vinde la magazin)

    int seeds = 3;          // câteva semințe de start
    int sunflowers = 0;
    int money = 50;         // banii de start

private:
    int selected = 0;       // 0 = semințe, 1 = flori
    static constexpr int SlotCount = 2;
};
