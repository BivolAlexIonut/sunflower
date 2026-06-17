#include "inventory.h"
#include <string>
#include <ostream>
#include <istream>

// Helperi pt. dreptunghiurile florilor FG (16x16): boboc (stadiu 1) + floare mare (stadiu 2).
#define FBUD(col)   { 64.0f + (col)*16.0f, 0, 16, 16 }
#define FBLOOM(col) { (col)*16.0f, 0, 16, 16 }

//   name             seed  sell   grow unlock tex  r1(tânăr)        r2(matur)         scale tree
const FlowerInfo FLOWERS[(int)Flower::COUNT] = {
    // FG vară (verde)
    { "Floare alba",     10,   25, 12.f,    0, 0, FBUD(0), FBLOOM(0), 2.0f, false },
    { "Floare roz",      25,   65, 14.f,  120, 0, FBUD(1), FBLOOM(1), 2.0f, false },
    { "Floare rosie",    60,  160, 16.f,  350, 0, FBUD(2), FBLOOM(2), 2.0f, false },
    { "Floare galbena",  90,  240, 17.f,  600, 0, FBUD(3), FBLOOM(3), 2.0f, false },
    // FG iarnă (albastre)
    { "Floare albastra",120,  320, 18.f,  900, 1, FBUD(0), FBLOOM(0), 2.0f, false },
    { "Gheata roz",     180,  470, 20.f, 1500, 1, FBUD(1), FBLOOM(1), 2.0f, false },
    { "Gheata rosie",   260,  680, 22.f, 2300, 1, FBUD(2), FBLOOM(2), 2.0f, false },
    { "Gheata violet",  360,  920, 24.f, 3400, 1, FBUD(3), FBLOOM(3), 2.0f, false },
    // FG toamnă (olive)
    { "Toamna alba",     70,  190, 16.f,  500, 2, FBUD(0), FBLOOM(0), 2.0f, false },
    { "Toamna roz",     110,  300, 18.f,  900, 2, FBUD(1), FBLOOM(1), 2.0f, false },
    { "Toamna rosie",   170,  440, 20.f, 1400, 2, FBUD(2), FBLOOM(2), 2.0f, false },
    { "Toamna aurie",   240,  640, 22.f, 2100, 2, FBUD(3), FBLOOM(3), 2.0f, false },
    // Plants&supplies (sprite-uri bogate)
    { "Floarea-soarelui",500,1300, 30.f, 4500, 3, { 33,405,30,28 }, { 63,388,36,46 }, 1.5f, false },
    { "Copac de mere",   300, 220, 26.f, 2800, 3, { 248,215,62,30 }, { 325,144,55,62 }, 1.1f, true  },
    { "Copac de prune",  340, 260, 28.f, 3200, 3, { 248,215,62,30 }, { 325, 96,55,46 }, 1.2f, true  },
};
#undef FBUD
#undef FBLOOM

// Iconița de monede din FG_Item_Icons.png
static const Rectangle kCoinIcon = { 0, 80, 16, 16 };

void Inventory::Serialize(std::ostream& o) const {
    o << money << " " << selectedSeed << "\n";
    for (int i = 0; i < (int)Flower::COUNT; i++) o << seeds[i] << " ";      o << "\n";
    for (int i = 0; i < (int)Flower::COUNT; i++) o << harvested[i] << " ";  o << "\n";
    for (int i = 0; i < (int)Flower::COUNT; i++) o << (unlocked[i] ? 1 : 0) << " "; o << "\n";
    o << wood << " " << crystals << " " << (hasAxe ? 1 : 0) << " " << (hasPickaxe ? 1 : 0) << "\n";
    o << day << "\n";
}

void Inventory::Deserialize(std::istream& in) {
    in >> money >> selectedSeed;
    for (int i = 0; i < (int)Flower::COUNT; i++) in >> seeds[i];
    for (int i = 0; i < (int)Flower::COUNT; i++) in >> harvested[i];
    for (int i = 0; i < (int)Flower::COUNT; i++) { int u; in >> u; unlocked[i] = (u != 0); }
    int ax, pk; in >> wood >> crystals >> ax >> pk;
    hasAxe = (ax != 0); hasPickaxe = (pk != 0);
    in >> day;
}

void Inventory::TickTime(float dt) {
    dayTimer += dt;
    if (dayTimer >= DaySeconds) { dayTimer -= DaySeconds; day++; }
}

float Inventory::PriceFactor(int flower) const {
    unsigned int h = (unsigned int)(flower * 2654435761u) ^ (unsigned int)(day * 40503u);
    h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
    return 0.6f + (h % 1001) / 1000.0f;   // 0.60 .. 1.60
}

int Inventory::CurrentSell(int flower) const {
    return (int)(FLOWERS[flower].sellPrice * PriceFactor(flower));
}

void Inventory::CycleSeed() {
    for (int i = 1; i <= (int)Flower::COUNT; i++) {
        int idx = (selectedSeed + i) % (int)Flower::COUNT;
        if (unlocked[idx]) { selectedSeed = idx; return; }
    }
}

void Inventory::Draw(const Texture2D* ftex, const Texture2D& icons) const {
    // Bani (sus-stânga) cu iconița de monede
    DrawTexturePro(icons, kCoinIcon, Rectangle{ 16, 14, 28, 28 }, { 0, 0 }, 0.0f, WHITE);
    DrawText(TextFormat("%d", money), 50, 16, 26, Color{ 255, 220, 90, 255 });

    // Resurse (lemn, cristale)
    DrawText(TextFormat("Lemn: %d", wood), 16, 46, 18, Color{ 200, 160, 110, 255 });
    DrawText(TextFormat("Cristale: %d", crystals), 120, 46, 18, Color{ 130, 210, 230, 255 });

    // Hotbar jos: sămânța selectată + recolta ei
    const int slot = 64;
    const int x0 = GetScreenWidth() / 2 - slot;
    const int y0 = GetScreenHeight() - slot - 16;

    DrawRectangle(x0, y0, slot, slot, Color{ 40, 30, 20, 210 });
    DrawRectangleLinesEx(Rectangle{ (float)x0, (float)y0, (float)slot, (float)slot }, 3,
                         Color{ 255, 220, 90, 255 });
    const FlowerInfo& fi = FLOWERS[selectedSeed];
    DrawTexturePro(ftex[fi.tex], fi.r2, Rectangle{ x0 + 8.0f, y0 + 6.0f, 48, 48 }, { 0, 0 }, 0.0f, WHITE);
    DrawText(TextFormat("x%d", seeds[selectedSeed]), x0 + 6, y0 + slot - 20, 18, WHITE);

    // numele florii selectate + câte are recoltate
    DrawText(FLOWERS[selectedSeed].name, x0 + slot + 12, y0 + 8, 18, WHITE);
    DrawText(TextFormat("Recoltate: %d", harvested[selectedSeed]), x0 + slot + 12, y0 + 34, 16,
             Color{ 200, 200, 200, 255 });
}
