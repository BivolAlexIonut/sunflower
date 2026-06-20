#include "shop.h"
#include "inventory.h"
#include "player.h"
#include "memories.h"
#include <string>
#include <ostream>
#include <istream>

static const char* kTabNames[Shop::TabCount] =
    { "MAGAZIN", "UNELTE", "CONSTR.", "SKIN-URI", "AJUTOR", "AMINTIRI", "ANIMALE" };

static const int kAxeCost = 80;
static const int kPickaxeCost = 220;

static const char* kSkinNames[Shop::SkinCount] = {
    "Character01", "Character05", "Character10", "Knight_10", "Knight_11"
};
static const char* kSkinLabels[Shop::SkinCount] = {
    "Fermierul (start)", "Fata", "Batranul", "Cavalerul rosu", "Cavalerul albastru"
};
static const int kSkinCost[Shop::SkinCount] = { 0, 200, 500, 1200, 1200 };

static const Rectangle kCoinIcon = { 0, 80, 16, 16 };

void Shop::HandleInput(Inventory& inv, Player& player) {
    if (IsKeyPressed(KEY_TAB)) { open = !open; row = 0; }
    if (!open) return;
    if (IsKeyPressed(KEY_ESCAPE)) { open = false; return; }

    // schimbă fila cu stânga/dreapta sau direct cu 1/2/3
    if (IsKeyPressed(KEY_RIGHT)) { tab = (tab + 1) % TabCount; row = 0; }
    if (IsKeyPressed(KEY_LEFT))  { tab = (tab + TabCount - 1) % TabCount; row = 0; }
    if (IsKeyPressed(KEY_ONE))   { tab = 0; row = 0; }
    if (IsKeyPressed(KEY_TWO))   { tab = 1; row = 0; }
    if (IsKeyPressed(KEY_THREE)) { tab = 2; row = 0; }
    if (IsKeyPressed(KEY_FOUR))  { tab = 3; row = 0; }
    if (IsKeyPressed(KEY_FIVE))  { tab = 4; row = 0; }
    if (IsKeyPressed(KEY_SIX))   { tab = 5; row = 0; }
    if (IsKeyPressed(KEY_SEVEN)) { tab = 6; row = 0; }

    if (tab == 0) {                                   // MAGAZIN (flori)
        int n = (int)Flower::COUNT;
        if (IsKeyPressed(KEY_DOWN)) row = (row + 1) % n;
        if (IsKeyPressed(KEY_UP))   row = (row + n - 1) % n;

        if (IsKeyPressed(KEY_U) && !inv.unlocked[row] &&
            inv.money >= FLOWERS[row].unlockCost) {
            inv.money -= FLOWERS[row].unlockCost;
            inv.unlocked[row] = true;
        }
        int seedCost = (int)(FLOWERS[row].seedCost * inv.SeedMul());
        if (IsKeyPressed(KEY_B) && inv.unlocked[row] && inv.money >= seedCost) {
            inv.money -= seedCost;
            inv.seeds[row]++;
            inv.SelectFlower(row);         // o pun în hotbar și o selectez
        }
        if (IsKeyPressed(KEY_S) && inv.harvested[row] > 0) {
            inv.money += inv.harvested[row] * inv.CurrentSell(row);   // preț fluctuant
            inv.harvested[row] = 0;
        }
        // vinde resurse
        if (IsKeyPressed(KEY_W) && inv.wood > 0) {
            inv.money += inv.wood * Inventory::WoodPrice; inv.wood = 0;
        }
        if (IsKeyPressed(KEY_C) && inv.crystals > 0) {
            inv.money += inv.crystals * Inventory::CrystalPrice; inv.crystals = 0;
        }
    }
    else if (tab == 1) {                              // UNELTE + UPGRADE-uri
        int n = Inventory::UpgCount;
        if (IsKeyPressed(KEY_DOWN)) row = (row + 1) % n;
        if (IsKeyPressed(KEY_UP))   row = (row + n - 1) % n;
        if (IsKeyPressed(KEY_B)) {
            int t = row;
            if (t == 3 && !inv.hasAxe) {                       // întâi deblochezi toporul
                if (inv.money >= kAxeCost) { inv.money -= kAxeCost; inv.hasAxe = true; }
            } else if (t == 4 && !inv.hasPickaxe) {            // întâi deblochezi târnăcopul
                if (inv.money >= kPickaxeCost) { inv.money -= kPickaxeCost; inv.hasPickaxe = true; }
            } else {                                           // upgrade nivel următor
                bool toolOk = !((t == 3 && !inv.hasAxe) || (t == 4 && !inv.hasPickaxe));
                int cost = inv.UpgCost(t);
                if (toolOk && cost > 0 && inv.money >= cost) { inv.money -= cost; inv.upg[t]++; }
            }
        }
    }
    else if (tab == 6) {                              // ANIMALE (produc bani)
        int n = Inventory::AnimalCount;
        if (IsKeyPressed(KEY_DOWN)) row = (row + 1) % n;
        if (IsKeyPressed(KEY_UP))   row = (row + n - 1) % n;
        if (IsKeyPressed(KEY_B) && animalsUnlocked) {
            int cost = Inventory::AnimalCost(row);
            if (inv.money >= cost) { inv.money -= cost; inv.animals[row]++; }
        }
    }
    else if (tab == 2) {                              // CONSTRUIESTE (drum / piatră)
        if (IsKeyPressed(KEY_DOWN)) row = (row + 1) % 2;
        if (IsKeyPressed(KEY_UP))   row = (row + 1) % 2;
        if (IsKeyPressed(KEY_B)) {                     // cumpără material
            if (row == 0 && inv.money >= Inventory::RoadCost)  { inv.money -= Inventory::RoadCost;  inv.roadCount++; }
            if (row == 1 && inv.money >= Inventory::StoneCost) { inv.money -= Inventory::StoneCost; inv.stoneCount++; }
        }
        if (IsKeyPressed(KEY_ENTER)) {                 // intră în modul construire
            inv.buildSel = (row == 0) ? 1 : 2;
            open = false;
        }
    }
    else if (tab == 3) {                              // SKIN-URI
        if (IsKeyPressed(KEY_DOWN)) row = (row + 1) % SkinCount;
        if (IsKeyPressed(KEY_UP))   row = (row + SkinCount - 1) % SkinCount;

        if (IsKeyPressed(KEY_ENTER)) {
            if (!skinUnlocked[row] && inv.money >= kSkinCost[row]) {
                inv.money -= kSkinCost[row];
                skinUnlocked[row] = true;
            }
            if (skinUnlocked[row]) {
                skinApplied = row;
                player.SetSkin(kSkinNames[row]);
            }
        }
    }
}

void Shop::ApplySkin(Player& player) const {
    player.SetSkin(kSkinNames[skinApplied]);
}

void Shop::Serialize(std::ostream& o) const {
    for (int i = 0; i < SkinCount; i++) o << (skinUnlocked[i] ? 1 : 0) << " ";
    o << skinApplied << "\n";
}

void Shop::Deserialize(std::istream& in) {
    for (int i = 0; i < SkinCount; i++) { int u; in >> u; skinUnlocked[i] = (u != 0); }
    in >> skinApplied;
}

void Shop::DrawFrame(int& x, int& y, int& w, int& h) const {
    w = 620; h = 480;
    x = GetScreenWidth() / 2 - w / 2;
    y = GetScreenHeight() / 2 - h / 2;
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{ 0, 0, 0, 140 });
    DrawRectangle(x, y, w, h, Color{ 38, 28, 20, 245 });
    DrawRectangleLinesEx(Rectangle{ (float)x, (float)y, (float)w, (float)h }, 3,
                         Color{ 255, 220, 90, 255 });
}

void Shop::DrawTabs(int x, int y, int w) const {
    int tw = w / TabCount;
    for (int i = 0; i < TabCount; i++) {
        int tx = x + i * tw;
        bool sel = (i == tab);
        DrawRectangle(tx, y, tw, 40, sel ? Color{ 70, 55, 35, 255 } : Color{ 30, 22, 16, 255 });
        Color tc = sel ? Color{ 255, 220, 90, 255 } : Color{ 150, 140, 120, 255 };
        int lw = MeasureText(kTabNames[i], 15);
        DrawText(kTabNames[i], tx + tw / 2 - lw / 2, y + 13, 15, tc);
    }
    DrawRectangle(x, y + 40, w, 3, Color{ 255, 220, 90, 255 });
}

void Shop::Draw(const Inventory& inv, const Texture2D* ftex, const Texture2D& icons,
                const Memories& mem) const {
    if (!open) return;
    int x, y, w, h;
    DrawFrame(x, y, w, h);
    DrawTabs(x, y, w);

    int cx = x + 20, cy = y + 56, cw = w - 40, ch = h - 76;
    if (tab == 0) DrawShop(cx, cy, cw, ch, inv, ftex);
    if (tab == 1) DrawUpgrades(cx, cy, cw, ch, inv);
    if (tab == 2) DrawBuild(cx, cy, cw, ch, inv);
    if (tab == 3) DrawSkins(cx, cy, cw, ch, inv);
    if (tab == 4) DrawHelp(cx, cy, cw, ch);
    if (tab == 5) mem.DrawAlbum(cx, cy, cw, ch, inv.level);
    if (tab == 6) DrawAnimals(cx, cy, cw, ch, inv);

    DrawText("1-7 sau Stanga/Dreapta: fila  |  Sus/Jos: alege  |  TAB/ESC: inchide",
             x + 20, y + h - 26, 14, Color{ 170, 170, 170, 255 });
    // bani (jos-dreapta, separat de file)
    DrawTexturePro(icons, kCoinIcon, Rectangle{ (float)(x + w - 130), (float)(y + h - 32), 22, 22 }, {0,0}, 0, WHITE);
    DrawText(TextFormat("%d", inv.money), x + w - 102, y + h - 31, 22, Color{ 255, 220, 90, 255 });
}

void Shop::DrawShop(int x, int y, int w, int h, const Inventory& inv, const Texture2D* ftex) const {
    DrawText(TextFormat("Ziua %d - preturile fluctueaza. Sus/Jos deruleaza lista.", inv.day),
             x, y, 14, Color{ 190, 200, 190, 255 });

    const int n = (int)Flower::COUNT;
    const int visible = 8;                       // câte rânduri arătăm odată
    int first = row - visible / 2;               // fereastra de derulare în jurul selecției
    if (first < 0) first = 0;
    if (first > n - visible) first = (n > visible) ? n - visible : 0;
    int last = (first + visible < n) ? first + visible : n;

    for (int i = first; i < last; i++) {
        int ry = y + 22 + (i - first) * 40;
        if (i == row) DrawRectangle(x - 6, ry - 4, w + 12, 38, Color{ 255, 255, 255, 30 });

        const FlowerInfo& fi = FLOWERS[i];
        DrawTexturePro(ftex[fi.tex], fi.r2, Rectangle{ (float)x, (float)ry, 32, 32 }, {0,0}, 0, WHITE);
        DrawText(fi.name, x + 40, ry, 17, WHITE);

        if (!inv.unlocked[i]) {
            DrawText(TextFormat("BLOCAT  [U] %d", fi.unlockCost),
                     x + 40, ry + 19, 14, Color{ 230, 150, 150, 255 });
        } else {
            float f = inv.PriceFactor(i);
            const char* arrow = (f > 1.15f) ? "++" : (f < 0.85f) ? "--" : "=";
            Color ac = (f > 1.15f) ? Color{ 140,230,140,255 }
                     : (f < 0.85f) ? Color{ 230,150,150,255 } : Color{ 200,200,200,255 };
            DrawText(TextFormat("[B] sam.%d   [S] vinde %d @ %d",
                     (int)(fi.seedCost * inv.SeedMul()), inv.harvested[i], inv.CurrentSell(i)), x + 40, ry + 19, 14,
                     Color{ 190, 215, 190, 255 });
            DrawText(arrow, x + w - 26, ry + 8, 18, ac);
        }
    }

    // indicator scroll + vânzare resurse
    DrawText(TextFormat("(%d/%d)", row + 1, n), x + w - 70, y, 14, Color{ 170,170,170,255 });
    int ry = y + 22 + visible * 40 + 4;
    DrawText(TextFormat("[W] Vinde lemn: %d x%d     [C] Vinde cristale: %d x%d",
             inv.wood, Inventory::WoodPrice, inv.crystals, Inventory::CrystalPrice),
             x, ry, 15, Color{ 210, 190, 150, 255 });
}

void Shop::DrawUpgrades(int x, int y, int w, int h, const Inventory& inv) const {
    (void)h;
    DrawText("Imbunatateste-ti uneltele si abilitatile. [B] cumpara nivelul urmator.",
             x, y, 14, Color{ 190, 200, 190, 255 });
    for (int t = 0; t < Inventory::UpgCount; t++) {
        int ry = y + 28 + t * 58;
        if (t == row) DrawRectangle(x - 6, ry - 6, w + 12, 52, Color{ 255, 255, 255, 26 });
        DrawText(Inventory::UpgName(t), x, ry, 20, WHITE);
        DrawText(Inventory::UpgDesc(t), x, ry + 24, 14, Color{ 190, 200, 190, 255 });

        // bară de niveluri (puncte)
        for (int lv = 0; lv < Inventory::UpgMax; lv++)
            DrawRectangle(x + 250 + lv * 16, ry + 2, 12, 12,
                          lv < inv.upg[t] ? Color{ 120,210,120,255 } : Color{ 70,70,60,255 });

        // starea / acțiunea
        bool needUnlock = (t == 3 && !inv.hasAxe) || (t == 4 && !inv.hasPickaxe);
        if (needUnlock) {
            int cost = (t == 3) ? kAxeCost : kPickaxeCost;
            DrawText(TextFormat("[B] Deblocheaza: %d", cost), x + 330, ry + 4, 17, Color{ 230,200,140,255 });
        } else if (inv.upg[t] >= Inventory::UpgMax) {
            DrawText("MAXIM", x + 330, ry + 4, 18, Color{ 140, 230, 140, 255 });
        } else {
            DrawText(TextFormat("[B] +1: %d", inv.UpgCost(t)), x + 330, ry + 4, 17, Color{ 230,200,140,255 });
        }
    }
}

void Shop::DrawAnimals(int x, int y, int w, int h, const Inventory& inv) const {
    (void)h;
    DrawText(TextFormat("Animalele aduc bani singure, chiar si cat esti plecat. Venit acum: %.0f bani/min",
             inv.AnimalIncomePerMin()), x, y, 14, Color{ 200, 220, 190, 255 });
    if (!animalsUnlocked)
        DrawText("Mai intai cumpara TARCUL de animale din modul harta (tasta L)!",
                 x, y + 18, 15, Color{ 255, 150, 120, 255 });
    for (int i = 0; i < Inventory::AnimalCount; i++) {
        int ry = y + 30 + i * 76;
        if (i == row) DrawRectangle(x - 6, ry - 6, w + 12, 68, Color{ 255, 255, 255, 26 });
        DrawText(Inventory::AnimalName(i), x, ry, 22, WHITE);
        DrawText(TextFormat("Produce %.0f bani/min", Inventory::AnimalIncome(i)),
                 x, ry + 26, 15, Color{ 190, 200, 190, 255 });
        DrawText(TextFormat("ai: %d", inv.animals[i]), x + w - 80, ry, 18, Color{ 200, 220, 200, 255 });
        DrawText(TextFormat("[B] Cumpara: %d", Inventory::AnimalCost(i)), x + 300, ry + 4, 18,
                 Color{ 230, 200, 140, 255 });
    }
}

void Shop::DrawBuild(int x, int y, int w, int h, const Inventory& inv) const {
    DrawText("Cumpara materiale [B], apoi [ENTER] = mod construire (click pe harta).",
             x, y, 15, Color{ 190, 200, 190, 255 });

    struct Row { const char* name; const char* desc; int have; int cost; };
    Row rows[2] = {
        { "Drum",   "Cale de pamant pe care poti merge", inv.roadCount,  Inventory::RoadCost },
        { "Piatra", "Bloc solid (nu poti merge pe el)",  inv.stoneCount, Inventory::StoneCost },
    };
    for (int i = 0; i < 2; i++) {
        int ry = y + 32 + i * 72;
        if (i == row) DrawRectangle(x - 6, ry - 6, w + 12, 64, Color{ 255, 255, 255, 26 });
        DrawText(rows[i].name, x, ry, 22, WHITE);
        DrawText(rows[i].desc, x, ry + 26, 15, Color{ 190, 200, 190, 255 });
        DrawText(TextFormat("ai: %d", rows[i].have), x + w - 70, ry, 18, Color{ 200, 220, 200, 255 });
        DrawText(TextFormat("[B] Cumpara: %d", rows[i].cost), x + 300, ry + 4, 18,
                 Color{ 230, 200, 140, 255 });
    }
}

void Shop::DrawSkins(int x, int y, int w, int h, const Inventory& inv) const {
    DrawText("Personajul tau. Deblochezi altele cu bani; restul abilitatilor raman.",
             x, y, 15, Color{ 190, 200, 190, 255 });
    for (int i = 0; i < SkinCount; i++) {
        int ry = y + 32 + i * 50;
        if (i == row) DrawRectangle(x - 6, ry - 6, w + 12, 46, Color{ 255, 255, 255, 26 });

        DrawText(kSkinLabels[i], x, ry, 20, WHITE);
        Color c = WHITE; std::string st;
        if (i == skinApplied)     { st = "[ECHIPAT]"; c = Color{ 140, 230, 140, 255 }; }
        else if (skinUnlocked[i]) { st = "[ENTER] echipeaza"; }
        else                      { st = TextFormat("[ENTER] cumpara: %d", kSkinCost[i]); c = Color{ 230, 200, 140, 255 }; }
        DrawText(st.c_str(), x + 280, ry + 2, 18, c);
    }
}

void Shop::DrawHelp(int x, int y, int w, int h) const {
    const char* lines[] = {
        "CUM SE JOACA",
        "",
        "Misca-te:           WASD / sageti",
        "Tinteste:           cu mausul, pe un tile din apropierea ta",
        "Lucreaza:           click stanga (sapa / planteaza / uda / recolteaza)",
        "Schimba floarea:    rotita mausului sau Q",
        "",
        "Patratul alb = poti lucra acolo. Rosu = prea departe, apropie-te.",
        "",
        "SCOP: fa bani din flori, deblocheaza tipuri tot mai rare,",
        "pana ajungi la Floarea-Soarelui.",
    };
    int n = sizeof(lines) / sizeof(lines[0]);
    for (int i = 0; i < n; i++) {
        Color c = (i == 0) ? Color{ 255, 220, 90, 255 } : Color{ 210, 210, 200, 255 };
        int fs = (i == 0) ? 20 : 16;
        DrawText(lines[i], x, y + i * 26, fs, c);
    }
}
