// Inventar + economie: bani, semințe și flori recoltate pe tipuri (rarități).
#pragma once
#include "raylib.h"
#include <iosfwd>

// Tipurile de flori = rarități, după culoarea din FG_Grass_Summer.
// Galben = floarea-soarelui (țelul jocului).
enum class Flower { White, Pink, Red, Yellow, COUNT };

struct FlowerInfo {
    const char* name;
    int   seedCost;     // preț sămânță la magazin
    int   sellPrice;    // preț de vânzare floare
    float growTime;     // secunde per stadiu de creștere
    int   unlockCost;   // cât costă deblocarea acestui tip (0 = de la start)
};

// Indexat după enum Flower.
extern const FlowerInfo FLOWERS[(int)Flower::COUNT];

class Inventory {
public:
    int money = 50;
    int seeds[(int)Flower::COUNT]     = { 3, 0, 0, 0 };   // start: 3 semințe albe
    int harvested[(int)Flower::COUNT] = { 0, 0, 0, 0 };
    bool unlocked[(int)Flower::COUNT] = { true, false, false, false };
    int selectedSeed = 0;

    void CycleSeed();   // Q: trece la următoarea floare deblocată
    void Draw(const Texture2D& flowers, const Texture2D& icons) const;

    void Serialize(std::ostream&) const;
    void Deserialize(std::istream&);
};
