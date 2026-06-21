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
    // LEGUME (Crops.png, tex 4) — recolte rapide de fermă: stadiu tânăr (48,y) + matur (80,y)
    // rândurile sunt la y = 16,48,80,112,144,176,208,240
    //   name          seed  sell  grow unlock tex  r1(tanar)        r2(matur)        scale tree
    { "Grau",            5,   18,  40.f,    0, 4, { 48, 16,16,16 }, { 80, 16,16,16 }, 1.7f, false },
    { "Ridiche",         6,   24,  45.f,   30, 4, { 48,208,16,16 }, { 80,208,16,16 }, 1.7f, false },
    { "Morcov",          8,   30,  50.f,   40, 4, { 48, 80,16,16 }, { 80, 80,16,16 }, 1.7f, false },
    { "Rosie",          15,   55,  70.f,  120, 4, { 48, 48,16,16 }, { 80, 48,16,16 }, 1.7f, false },
    { "Varza",          18,   65,  75.f,  180, 4, { 48,240,16,16 }, { 80,240,16,16 }, 1.7f, false },
    { "Vanata",         20,   70,  80.f,  200, 4, { 48,112,16,16 }, { 80,112,16,16 }, 1.7f, false },
    { "Porumb",         25,   90,  95.f,  300, 4, { 48,144,16,16 }, { 80,144,16,16 }, 1.8f, false },
    { "Dovleac",        35,  150, 130.f,  500, 4, { 48,176,16,16 }, { 80,176,16,16 }, 1.8f, false },
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

// ---- UPGRADE-uri ----
struct UpgCfg { const char* name; const char* desc; int baseCost; };
static const UpgCfg kUpg[Inventory::UpgCount] = {
    { "Sapa",        "Sapi pamantul mai repede",                 120 },
    { "Stropitoare", "Uzi o zona mai mare dintr-o data",         150 },
    { "Gradinarit",  "Plantele cresc mai repede",                250 },
    { "Topor",       "Tai copacii mai repede, mai mult lemn",    200 },
    { "Tarnacop",    "Spargi cristalele mai repede, mai multe",  200 },
    { "Viteza",      "Te misti mai repede",                      180 },
};
const char* Inventory::UpgName(int i) { return (i>=0&&i<UpgCount)?kUpg[i].name:""; }
const char* Inventory::UpgDesc(int i) { return (i>=0&&i<UpgCount)?kUpg[i].desc:""; }
int Inventory::UpgCost(int i) const {
    if (i < 0 || i >= UpgCount || upg[i] >= UpgMax) return 0;
    return kUpg[i].baseCost * (upg[i] + 1);     // crește cu nivelul
}

// ---- ANIMALE + ALIMENTE ----
struct AnimCfg { const char* name; int cost; float foodPerMin; };
static const AnimCfg kAnim[Inventory::AnimalCount] = {
    { "Gaina",  120, 1.0f },     // → Oua
    { "Porc",   350, 0.4f },     // → Sunca
    { "Oaie",   600, 0.4f },     // → Lana
    { "Vaca",  1400, 0.5f },     // → Lapte
};
struct FoodCfg { const char* name; int price; };
static const FoodCfg kFood[Inventory::FoodCount] = {
    { "Oua",   10 }, { "Sunca", 50 }, { "Lana", 40 }, { "Lapte", 24 },
};
const char* Inventory::AnimalName(int i) { return (i>=0&&i<AnimalCount)?kAnim[i].name:""; }
int   Inventory::AnimalCost(int i)       { return (i>=0&&i<AnimalCount)?kAnim[i].cost:0; }
float Inventory::AnimalFoodPerMin(int i) { return (i>=0&&i<AnimalCount)?kAnim[i].foodPerMin:0; }
const char* Inventory::FoodName(int i)   { return (i>=0&&i<FoodCount)?kFood[i].name:""; }
int Inventory::FoodPrice(int i)          { return (i>=0&&i<FoodCount)?kFood[i].price:0; }

// ---- MATERIALE de construcție ----
struct BuildCfg { const char* name; const char* desc; int cost; };
static const BuildCfg kBuild[Inventory::BuildMatCount] = {
    { "Drum",   "Cale de pamant pe care poti merge",        12 },
    { "Piatra", "Bloc solid de piatra (zid)",               25 },
    { "Gard",   "Gard alb - imprejmuieste gradini frumoase", 18 },
    { "Apa",    "Apa decorativa (nu poti merge pe ea)",      20 },
    { "Nisip",  "Nisip de plaja (decorativ)",                10 },
};
const char* Inventory::BuildName(int i) { return (i>=0&&i<BuildMatCount)?kBuild[i].name:""; }
const char* Inventory::BuildDesc(int i) { return (i>=0&&i<BuildMatCount)?kBuild[i].desc:""; }
int Inventory::BuildCost(int i)         { return (i>=0&&i<BuildMatCount)?kBuild[i].cost:0; }
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
    for (int i = 0; i < BuildMatCount; i++) o << buildMat[i] << " ";
    o << "\n";
    for (int i = 0; i < BuffCount; i++) o << buff[i] << " ";
    o << "\n";
    o << difficulty << " " << playSeconds << "\n";
    for (int i = 0; i < UpgCount; i++) o << upg[i] << " ";
    for (int i = 0; i < AnimalCount; i++) o << animals[i] << " ";
    o << "\n";
    for (int i = 0; i < HotbarSize; i++) o << hotbar[i] << " ";
    o << selectedSlot << "\n";
    for (int i = 0; i < FoodCount; i++) o << food[i] << " ";
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
    for (int i = 0; i < BuildMatCount; i++) in >> buildMat[i];
    for (int i = 0; i < BuffCount; i++) in >> buff[i];
    in >> difficulty >> playSeconds;
    for (int i = 0; i < UpgCount; i++) in >> upg[i];
    for (int i = 0; i < AnimalCount; i++) in >> animals[i];
    for (int i = 0; i < HotbarSize; i++) in >> hotbar[i];
    in >> selectedSlot;
    for (int i = 0; i < FoodCount; i++) in >> food[i];
    SyncSelected();
}

void Inventory::TickTime(float dt) {
    dayTimer += dt;
    if (dayTimer >= DaySeconds) { dayTimer -= DaySeconds; day++; }
    if (levelUpTimer > 0) levelUpTimer--;
    playSeconds += dt;
    // animalele produc alimente (pe tip)
    for (int i = 0; i < AnimalCount; i++) {
        foodAccum[i] += animals[i] * AnimalFoodPerMin(i) / 60.0f * dt;
        if (foodAccum[i] >= 1.0f) { int a = (int)foodAccum[i]; food[i] += a; foodAccum[i] -= a; }
    }
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

void Inventory::SelectSlot(int s) {
    if (s >= 0 && s < HotbarSize) { selectedSlot = s; SyncSelected(); }
}

void Inventory::AddSeedToBag(int flower) {
    for (int i = 0; i < HotbarSize; i++) if (hotbar[i] == flower) return;   // deja în hotbar
    for (int i = 0; i < HotbarSize; i++) if (hotbar[i] < 0) { hotbar[i] = flower; return; }
    // hotbar plin: rămâne doar în inventar (accesibil cu I)
}

void Inventory::SelectFlower(int flower) {
    AddSeedToBag(flower);
    for (int i = 0; i < HotbarSize; i++) if (hotbar[i] == flower) { selectedSlot = i; break; }
    SyncSelected();
}

void Inventory::CycleSeed() {
    // trece la următorul slot din hotbar cu o sămânță pe care o ai
    for (int k = 1; k <= HotbarSize; k++) {
        int s = (selectedSlot + k) % HotbarSize;
        int f = hotbar[s];
        if (f >= 0 && seeds[f] > 0) { selectedSlot = s; SyncSelected(); return; }
    }
}

void Inventory::EnsureValidSeed() {
    SyncSelected();
    if (selectedSeed >= 0 && seeds[selectedSeed] > 0) return;
    for (int s = 0; s < HotbarSize; s++) {
        int f = hotbar[s];
        if (f >= 0 && seeds[f] > 0) { selectedSlot = s; SyncSelected(); return; }
    }
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
        if (level % 4 == 0)             { buildMat[0] += 3;  msg += "  +3 drum"; }

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
    {
        bool anyMat = false;
        for (int i = 0; i < BuildMatCount; i++) if (buildMat[i] > 0) anyMat = true;
        if (anyMat)
            DrawText("B = construieste (drum/gard/piatra...)", 16, 66, 15, Color{ 210, 190, 150, 255 });
    }

    // HOTBAR: cele 6 sloturi de jos (restul semințelor: tasta I)
    const int slot = 52, gap = 6;
    int total = HotbarSize * (slot + gap) - gap;
    int x0 = GetScreenWidth() / 2 - total / 2;
    int y0 = GetScreenHeight() - slot - 14;

    for (int s = 0; s < HotbarSize; s++) {
        int sx = x0 + s * (slot + gap);
        bool sel = (s == selectedSlot);
        DrawRectangle(sx, y0, slot, slot, Color{ 40, 30, 20, 210 });
        DrawRectangleLinesEx(Rectangle{ (float)sx, (float)y0, (float)slot, (float)slot },
                             sel ? 3.0f : 2.0f, sel ? Color{ 255,220,90,255 } : Color{ 110,90,60,255 });
        DrawText(TextFormat("%d", s + 1), sx + 3, y0 + 2, 12, Color{ 150, 140, 120, 255 });
        int f = hotbar[s];
        if (f >= 0 && f < (int)Flower::COUNT) {
            const FlowerInfo& fi = FLOWERS[f];
            Color tint = (seeds[f] > 0) ? WHITE : Color{ 120, 120, 120, 160 };
            DrawTexturePro(ftex[fi.tex], fi.r2, Rectangle{ sx + 8.0f, y0 + 6.0f, 36, 36 }, { 0, 0 }, 0.0f, tint);
            DrawText(TextFormat("%d", seeds[f]), sx + 4, y0 + slot - 16, 14,
                     seeds[f] > 0 ? WHITE : Color{ 200, 120, 120, 255 });
        }
    }
    DrawText("I = inventar", x0 + total + 10, y0 + slot - 18, 14, Color{ 200, 200, 180, 200 });

    // numele florii selectate, deasupra barei
    if (selectedSeed >= 0 && selectedSeed < (int)Flower::COUNT && seeds[selectedSeed] > 0) {
        const char* nm = FLOWERS[selectedSeed].name;
        int tw = MeasureText(nm, 18);
        DrawText(nm, GetScreenWidth()/2 - tw/2, y0 - 22, 18, Color{ 255, 240, 200, 255 });
    }
}
