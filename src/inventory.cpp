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
    { "Copac de mere",   300, 220, 26.f, 2800, 3, { 248,146,56,60 }, { 325,144,55,62 }, 1.0f, true  },
    { "Copac de prune",  340, 260, 28.f, 3200, 3, { 248, 96,58,46 }, { 325, 96,55,46 }, 1.1f, true  },
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
    o << day << " " << xp << " " << level << "\n";
    o << roadCount << " " << stoneCount << "\n";
    for (int i = 0; i < BuffCount; i++) o << buff[i] << " ";
    o << "\n";
}

void Inventory::Deserialize(std::istream& in) {
    in >> money >> selectedSeed;
    for (int i = 0; i < (int)Flower::COUNT; i++) in >> seeds[i];
    for (int i = 0; i < (int)Flower::COUNT; i++) in >> harvested[i];
    for (int i = 0; i < (int)Flower::COUNT; i++) { int u; in >> u; unlocked[i] = (u != 0); }
    int ax, pk; in >> wood >> crystals >> ax >> pk;
    hasAxe = (ax != 0); hasPickaxe = (pk != 0);
    in >> day >> xp >> level;
    in >> roadCount >> stoneCount;
    for (int i = 0; i < BuffCount; i++) in >> buff[i];
}

void Inventory::TickTime(float dt) {
    dayTimer += dt;
    if (dayTimer >= DaySeconds) { dayTimer -= DaySeconds; day++; }
    if (levelUpTimer > 0) levelUpTimer--;
}

float Inventory::PriceFactor(int flower) const {
    unsigned int h = (unsigned int)(flower * 2654435761u) ^ (unsigned int)(day * 40503u);
    h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
    return 0.6f + (h % 1001) / 1000.0f;   // 0.60 .. 1.60
}

int Inventory::CurrentSell(int flower) const {
    float p = FLOWERS[flower].sellPrice * PriceFactor(flower);
    if (BuffActive(3)) p *= 1.5f;   // power-up bonus bani
    return (int)p;
}

void Inventory::CycleSeed() {
    // trece la următoarea floare pentru care AI semințe (ce poți planta acum)
    for (int i = 1; i <= (int)Flower::COUNT; i++) {
        int idx = (selectedSeed + i) % (int)Flower::COUNT;
        if (seeds[idx] > 0) { selectedSeed = idx; return; }
    }
}

void Inventory::EnsureValidSeed() {
    if (seeds[selectedSeed] > 0) return;
    for (int i = 0; i < (int)Flower::COUNT; i++)
        if (seeds[i] > 0) { selectedSeed = i; return; }
}

void Inventory::AddXP(int amount) {
    if (BuffActive(4)) amount *= 2;   // power-up bonus XP
    xp += amount;
    while (xp >= XPForNext()) {
        xp -= XPForNext();
        level++;

        int reward = 20 + level * 15;          // bani la fiecare nivel
        money += reward;
        std::string msg = TextFormat("Nivel %d!  +%d bani", level, reward);

        for (int i = 0; i < (int)Flower::COUNT; i++)   // deblochează următoarea floare gratis
            if (!unlocked[i]) { unlocked[i] = true; msg += "  +floare noua"; break; }

        if (level == 3) msg += "  (apasa L: cumpara teren!)";
        if (level == 5 && !hasAxe)      { hasAxe = true;      msg += "  +Topor"; }
        if (level == 7 && !hasPickaxe)  { hasPickaxe = true;  msg += "  +Tarnacop"; }
        if (level % 4 == 0)             { roadCount += 3;     msg += "  +3 drum"; }

        levelUpMsg = msg;
        levelUpTimer = 220;
    }
}

void Inventory::DrawLevel(int plantedCount) const {
    const int w = 196, h = 76;
    const int x = GetScreenWidth() - w - 12, y = 12;
    DrawRectangle(x, y, w, h, Color{ 30, 24, 16, 205 });
    DrawRectangleLinesEx(Rectangle{ (float)x, (float)y, (float)w, (float)h }, 2, Color{ 255, 220, 90, 255 });
    DrawText(TextFormat("Nivel %d", level), x + 12, y + 8, 22, Color{ 255, 220, 90, 255 });

    float frac = (float)xp / (float)XPForNext();
    DrawRectangle(x + 12, y + 36, w - 24, 12, Color{ 60, 50, 40, 255 });
    DrawRectangle(x + 12, y + 36, (int)((w - 24) * frac), 12, Color{ 120, 210, 120, 255 });
    DrawText(TextFormat("XP %d/%d", xp, XPForNext()), x + 14, y + 36, 11, WHITE);

    DrawText(TextFormat("Flori plantate: %d", plantedCount), x + 12, y + 54, 15, Color{ 200, 220, 200, 255 });

    // popup de level-up (centru-sus)
    if (levelUpTimer > 0 && !levelUpMsg.empty()) {
        int tw = MeasureText(levelUpMsg.c_str(), 22);
        int px = GetScreenWidth()/2 - tw/2, py = 120;
        DrawRectangle(px - 16, py - 8, tw + 32, 40, Color{ 30, 60, 30, 220 });
        DrawRectangleLinesEx(Rectangle{ (float)(px-16),(float)(py-8),(float)(tw+32),40 }, 2, Color{ 140, 230, 140, 255 });
        DrawText(levelUpMsg.c_str(), px, py, 22, Color{ 200, 255, 200, 255 });
    }
}

void Inventory::Draw(const Texture2D* ftex, const Texture2D& icons) const {
    // Bani (sus-stânga) cu iconița de monede
    DrawTexturePro(icons, kCoinIcon, Rectangle{ 16, 14, 28, 28 }, { 0, 0 }, 0.0f, WHITE);
    DrawText(TextFormat("%d", money), 50, 16, 26, Color{ 255, 220, 90, 255 });

    // Resurse (lemn, cristale)
    DrawText(TextFormat("Lemn: %d", wood), 16, 46, 18, Color{ 200, 160, 110, 255 });
    DrawText(TextFormat("Cristale: %d", crystals), 120, 46, 18, Color{ 130, 210, 230, 255 });

    // Bară de semințe: TOATE semințele pe care le ai (ce poți planta acum)
    int owned[(int)Flower::COUNT], cnt = 0;
    for (int i = 0; i < (int)Flower::COUNT; i++) if (seeds[i] > 0) owned[cnt++] = i;

    const int slot = 52, gap = 6;
    int total = (cnt > 0) ? cnt * (slot + gap) - gap : slot;
    int x0 = GetScreenWidth() / 2 - total / 2;
    int y0 = GetScreenHeight() - slot - 14;

    if (cnt == 0) {
        DrawText("Nicio samanta - cumpara din magazin (TAB) sau market",
                 GetScreenWidth()/2 - 200, y0 + 16, 16, Color{ 230, 220, 180, 220 });
    }
    for (int j = 0; j < cnt; j++) {
        int idx = owned[j];
        int sx = x0 + j * (slot + gap);
        bool sel = (idx == selectedSeed);
        DrawRectangle(sx, y0, slot, slot, Color{ 40, 30, 20, 210 });
        DrawRectangleLinesEx(Rectangle{ (float)sx, (float)y0, (float)slot, (float)slot },
                             sel ? 3.0f : 2.0f, sel ? Color{ 255,220,90,255 } : Color{ 110,90,60,255 });
        const FlowerInfo& fi = FLOWERS[idx];
        DrawTexturePro(ftex[fi.tex], fi.r2, Rectangle{ sx + 8.0f, y0 + 6.0f, 36, 36 }, { 0, 0 }, 0.0f, WHITE);
        DrawText(TextFormat("%d", seeds[idx]), sx + 4, y0 + slot - 16, 14, WHITE);
    }
    // numele florii selectate, deasupra barei
    if (seeds[selectedSeed] > 0) {
        const char* nm = FLOWERS[selectedSeed].name;
        int tw = MeasureText(nm, 18);
        DrawText(nm, GetScreenWidth()/2 - tw/2, y0 - 22, 18, Color{ 255, 240, 200, 255 });
    }
}
