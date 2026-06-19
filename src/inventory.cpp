#include "inventory.h"
#include <string>
#include <ostream>
#include <istream>

// Helperi pt. dreptunghiurile florilor FG (16x16): boboc (stadiu 1) + floare mare (stadiu 2).
#define FBUD(col)   { 64.0f + (col)*16.0f, 0, 16, 16 }
#define FBLOOM(col) { (col)*16.0f, 0, 16, 16 }

// growTime = secunde per stadiu (2 stadii până la matur). Joc bazat pe timp:
// florile 1-6 min, copacii ~25 min/udare (scumpi, dar valoroși).
//   name             seed  sell   grow unlock tex  r1(tânăr)        r2(matur)         scale tree
const FlowerInfo FLOWERS[(int)Flower::COUNT] = {
    // FG vară (verde) — comune, ieftine
    { "Floare alba",      8,   30,  45.f,    0, 0, FBUD(0), FBLOOM(0), 2.0f, false },
    { "Floare roz",      20,   80,  70.f,  150, 0, FBUD(1), FBLOOM(1), 2.0f, false },
    { "Floare rosie",    45,  190, 100.f,  450, 0, FBUD(2), FBLOOM(2), 2.0f, false },
    { "Floare galbena",  80,  320, 130.f,  800, 0, FBUD(3), FBLOOM(3), 2.0f, false },
    // FG iarnă (albastre) — rare
    { "Floare albastra",140,  520, 160.f, 1400, 1, FBUD(0), FBLOOM(0), 2.0f, false },
    { "Gheata roz",     220,  780, 190.f, 2200, 1, FBUD(1), FBLOOM(1), 2.0f, false },
    { "Gheata rosie",   320, 1100, 220.f, 3400, 1, FBUD(2), FBLOOM(2), 2.0f, false },
    { "Gheata violet",  460, 1550, 260.f, 5000, 1, FBUD(3), FBLOOM(3), 2.0f, false },
    // FG toamnă (olive)
    { "Toamna alba",     60,  240, 100.f,  700, 2, FBUD(0), FBLOOM(0), 2.0f, false },
    { "Toamna roz",     100,  400, 130.f, 1200, 2, FBUD(1), FBLOOM(1), 2.0f, false },
    { "Toamna rosie",   160,  600, 160.f, 1900, 2, FBUD(2), FBLOOM(2), 2.0f, false },
    { "Toamna aurie",   240,  880, 190.f, 2800, 2, FBUD(3), FBLOOM(3), 2.0f, false },
    // Plants&supplies — floarea-soarelui (țel) + copaci (scumpi, lenți, valoroși)
    { "Floarea-soarelui",600,2200, 300.f, 6000, 3, { 33,405,30,28 }, { 63,388,36,46 }, 1.5f, false },
    { "Copac de mere",   700, 700,1500.f, 4000, 3, { 248,146,56,60 }, { 325,144,55,62 }, 1.0f, true  },
    { "Copac de prune",  900, 950,1500.f, 5000, 3, { 248, 96,58,46 }, { 325, 96,55,46 }, 1.1f, true  },
};
#undef FBUD
#undef FBLOOM

// Iconița de monede din FG_Item_Icons.png
static const Rectangle kCoinIcon = { 0, 80, 16, 16 };

// Dificultăți — toate păstrează un ritm realist (timpi lungi), dar reglează economia.
//                   grow   sell   seed  startMoney
struct DiffCfg { float grow, sell, seed; int money; const char* name; const char* desc; };
static const DiffCfg kDiff[3] = {
    { 0.85f, 1.20f, 0.85f, 150, "Gradinar",        "Pornire blanda: bani in plus, florile cresc putin mai repede si valoreaza mai mult." },
    { 1.00f, 1.00f, 1.00f,  60, "Fermier",         "Echilibrat. Asa e gandit jocul." },
    { 1.20f, 0.85f, 1.15f,  30, "Supravietuitor",  "Realist si aspru: totul costa mai mult, creste mai incet si se vinde mai ieftin." },
};
static int ClampDiff(int d) { return (d < 0) ? 0 : (d > 2) ? 2 : d; }

const char* Inventory::DifficultyName(int d) { return kDiff[ClampDiff(d)].name; }
const char* Inventory::DifficultyDesc(int d) { return kDiff[ClampDiff(d)].desc; }
int Inventory::StartMoneyFor(int d) { return kDiff[ClampDiff(d)].money; }
float Inventory::GrowMul() const { return kDiff[ClampDiff(difficulty)].grow; }
float Inventory::SellMul() const { return kDiff[ClampDiff(difficulty)].sell; }
float Inventory::SeedMul() const { return kDiff[ClampDiff(difficulty)].seed; }
int   Inventory::StartMoney() const { return kDiff[ClampDiff(difficulty)].money; }

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
    o << difficulty << " " << playSeconds << "\n";
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
    in >> difficulty >> playSeconds;
}

void Inventory::TickTime(float dt) {
    dayTimer += dt;
    if (dayTimer >= DaySeconds) { dayTimer -= DaySeconds; day++; }
    if (levelUpTimer > 0) levelUpTimer--;
    playSeconds += dt;
}

float Inventory::PriceFactor(int flower) const {
    unsigned int h = (unsigned int)(flower * 2654435761u) ^ (unsigned int)(day * 40503u);
    h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
    return 0.6f + (h % 1001) / 1000.0f;   // 0.60 .. 1.60
}

int Inventory::CurrentSell(int flower) const {
    float p = FLOWERS[flower].sellPrice * PriceFactor(flower) * SellMul();
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
            if (!unlocked[i]) { unlocked[i] = true; msg += TextFormat("  +floare noua: %s", FLOWERS[i].name); break; }

        if (level == 3) msg += "  (apasa L: cumpara teren!)";
        if (level == 5 && !hasAxe)      { hasAxe = true;      msg += "  +Topor"; }
        if (level == 7 && !hasPickaxe)  { hasPickaxe = true;  msg += "  +Tarnacop"; }
        if (level % 4 == 0)             { roadCount += 3;     msg += "  +3 drum"; }

        levelUpMsg = msg;
        levelUpTimer = 220;
        levelUpPending = true;     // declanșează modalul de level-up
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
}

void Inventory::Draw(const Texture2D* ftex, const Texture2D& icons) const {
    // Bani (sus-stânga) cu iconița de monede
    DrawTexturePro(icons, kCoinIcon, Rectangle{ 16, 14, 28, 28 }, { 0, 0 }, 0.0f, WHITE);
    DrawText(TextFormat("%d", money), 50, 16, 26, Color{ 255, 220, 90, 255 });

    // Resurse (lemn, cristale, materiale de construcție)
    DrawText(TextFormat("Lemn: %d", wood), 16, 46, 18, Color{ 200, 160, 110, 255 });
    DrawText(TextFormat("Cristale: %d", crystals), 120, 46, 18, Color{ 130, 210, 230, 255 });
    if (roadCount > 0 || stoneCount > 0)
        DrawText(TextFormat("Drum: %d  Piatra: %d  (B = construieste)", roadCount, stoneCount),
                 16, 66, 15, Color{ 210, 190, 150, 255 });

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
