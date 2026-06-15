#include "shop.h"
#include "inventory.h"
#include "player.h"
#include <string>

static const char* kTabNames[Shop::TabCount] = { "MAGAZIN", "SKIN-URI", "AJUTOR" };

static const char* kSkinNames[Shop::SkinCount] = {
    "Character01", "Character05", "Character10", "Knight_10", "Knight_11"
};
static const char* kSkinLabels[Shop::SkinCount] = {
    "Fermierul (start)", "Fata", "Batranul", "Cavalerul rosu", "Cavalerul albastru"
};
static const int kSkinCost[Shop::SkinCount] = { 0, 200, 500, 1200, 1200 };

static const Rectangle kCoinIcon = { 0, 80, 16, 16 };
static const Rectangle kFlowerBig[4] = {
    {  0, 0, 16, 16 }, { 16, 0, 16, 16 }, { 32, 0, 16, 16 }, { 48, 0, 16, 16 }
};

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

    if (tab == 0) {                                   // MAGAZIN
        int n = (int)Flower::COUNT;
        if (IsKeyPressed(KEY_DOWN)) row = (row + 1) % n;
        if (IsKeyPressed(KEY_UP))   row = (row + n - 1) % n;

        if (IsKeyPressed(KEY_U) && !inv.unlocked[row] &&
            inv.money >= FLOWERS[row].unlockCost) {
            inv.money -= FLOWERS[row].unlockCost;
            inv.unlocked[row] = true;
        }
        if (IsKeyPressed(KEY_B) && inv.unlocked[row] &&
            inv.money >= FLOWERS[row].seedCost) {
            inv.money -= FLOWERS[row].seedCost;
            inv.seeds[row]++;
        }
        if (IsKeyPressed(KEY_S) && inv.harvested[row] > 0) {
            inv.money += inv.harvested[row] * FLOWERS[row].sellPrice;
            inv.harvested[row] = 0;
        }
    }
    else if (tab == 1) {                              // SKIN-URI
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

void Shop::DrawFrame(int& x, int& y, int& w, int& h) const {
    w = 600; h = 400;
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
        int lw = MeasureText(kTabNames[i], 20);
        DrawText(kTabNames[i], tx + tw / 2 - lw / 2, y + 10, 20, tc);
    }
    DrawRectangle(x, y + 40, w, 3, Color{ 255, 220, 90, 255 });
}

void Shop::Draw(const Inventory& inv, const Texture2D& flowers, const Texture2D& icons) const {
    if (!open) return;
    int x, y, w, h;
    DrawFrame(x, y, w, h);
    DrawTabs(x, y, w);

    // bani (mereu vizibili sus-dreapta)
    DrawTexturePro(icons, kCoinIcon, Rectangle{ (float)(x + w - 110), (float)(y + 8), 24, 24 }, {0,0}, 0, WHITE);
    DrawText(TextFormat("%d", inv.money), x + w - 80, y + 10, 22, Color{ 255, 220, 90, 255 });

    int cx = x + 20, cy = y + 56, cw = w - 40, ch = h - 76;
    if (tab == 0) DrawShop(cx, cy, cw, ch, inv, flowers, icons);
    if (tab == 1) DrawSkins(cx, cy, cw, ch, inv);
    if (tab == 2) DrawHelp(cx, cy, cw, ch);

    DrawText("1/2/3 sau Stanga/Dreapta: fila   |   Sus/Jos: alege   |   TAB/ESC: inchide",
             x + 20, y + h - 28, 15, Color{ 170, 170, 170, 255 });
}

void Shop::DrawShop(int x, int y, int w, int h, const Inventory& inv,
                    const Texture2D& flowers, const Texture2D& icons) const {
    DrawText("Cumpara seminte si deblocheaza flori noi. Scopul: Floarea-Soarelui.",
             x, y, 15, Color{ 190, 200, 190, 255 });
    for (int i = 0; i < (int)Flower::COUNT; i++) {
        int ry = y + 28 + i * 62;
        if (i == row) DrawRectangle(x - 6, ry - 6, w + 12, 58, Color{ 255, 255, 255, 26 });

        DrawTexturePro(flowers, kFlowerBig[i], Rectangle{ (float)x, (float)ry, 42, 42 }, {0,0}, 0, WHITE);
        DrawText(FLOWERS[i].name, x + 54, ry, 20, WHITE);

        if (!inv.unlocked[i]) {
            DrawText(TextFormat("BLOCAT   [U] Deblocheaza: %d", FLOWERS[i].unlockCost),
                     x + 54, ry + 26, 16, Color{ 230, 150, 150, 255 });
        } else {
            DrawText(TextFormat("[B] Samanta %d   [S] Vinde %d buc x %d   (ai %d sem.)",
                     FLOWERS[i].seedCost, inv.harvested[i], FLOWERS[i].sellPrice, inv.seeds[i]),
                     x + 54, ry + 26, 15, Color{ 190, 215, 190, 255 });
        }
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
