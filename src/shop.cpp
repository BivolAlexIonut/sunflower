#include "shop.h"
#include "inventory.h"
#include "player.h"
#include <string>

// Skin-urile = celelalte caractere din pachet. Costul se plătește din bani.
static const char* kSkinNames[Shop::SkinCount] = {
    "Character01", "Character05", "Character10", "Knight_10", "Knight_11"
};
static const char* kSkinLabels[Shop::SkinCount] = {
    "Fermierul (start)", "Fata", "Batranul", "Cavalerul rosu", "Cavalerul albastru"
};
static const int kSkinCost[Shop::SkinCount] = { 0, 200, 500, 1200, 1200 };

// Iconițe din FG_Item_Icons.png
static const Rectangle kCoinIcon = { 0, 80, 16, 16 };
static const Rectangle kFlowerBig[4] = {
    {  0, 0, 16, 16 }, { 16, 0, 16, 16 }, { 32, 0, 16, 16 }, { 48, 0, 16, 16 }
};

void Shop::HandleInput(Inventory& inv, Player& player) {
    // deschidere/închidere meniuri
    if (IsKeyPressed(KEY_TAB)) { shopOpen = !shopOpen; skinsOpen = false; row = 0; }
    if (IsKeyPressed(KEY_K))   { skinsOpen = !skinsOpen; shopOpen = false; row = 0; }
    if (IsKeyPressed(KEY_ESCAPE)) { shopOpen = skinsOpen = false; }

    if (shopOpen) {
        int n = (int)Flower::COUNT;
        if (IsKeyPressed(KEY_DOWN)) row = (row + 1) % n;
        if (IsKeyPressed(KEY_UP))   row = (row + n - 1) % n;

        if (IsKeyPressed(KEY_U) && !inv.unlocked[row]) {            // deblochează tipul
            if (inv.money >= FLOWERS[row].unlockCost) {
                inv.money -= FLOWERS[row].unlockCost;
                inv.unlocked[row] = true;
            }
        }
        if (IsKeyPressed(KEY_B) && inv.unlocked[row]) {            // cumpără o sămânță
            if (inv.money >= FLOWERS[row].seedCost) {
                inv.money -= FLOWERS[row].seedCost;
                inv.seeds[row]++;
            }
        }
        if (IsKeyPressed(KEY_S) && inv.harvested[row] > 0) {       // vinde toate de acest tip
            inv.money += inv.harvested[row] * FLOWERS[row].sellPrice;
            inv.harvested[row] = 0;
        }
    }
    else if (skinsOpen) {
        if (IsKeyPressed(KEY_DOWN)) row = (row + 1) % SkinCount;
        if (IsKeyPressed(KEY_UP))   row = (row + SkinCount - 1) % SkinCount;

        if (IsKeyPressed(KEY_ENTER)) {
            if (!skinUnlocked[row]) {                              // deblochează skin-ul
                if (inv.money >= kSkinCost[row]) {
                    inv.money -= kSkinCost[row];
                    skinUnlocked[row] = true;
                }
            }
            if (skinUnlocked[row]) {                               // aplică skin-ul
                skinApplied = row;
                player.SetSkin(kSkinNames[row]);
            }
        }
    }
}

void Shop::Draw(const Inventory& inv, const Texture2D& flowers, const Texture2D& icons) const {
    if (shopOpen)  DrawShop(inv, flowers, icons);
    if (skinsOpen) DrawSkins(inv);
}

static void Panel(int x, int y, int w, int h, const char* title) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{ 0, 0, 0, 120 });
    DrawRectangle(x, y, w, h, Color{ 40, 30, 20, 240 });
    DrawRectangleLinesEx(Rectangle{ (float)x, (float)y, (float)w, (float)h }, 3,
                         Color{ 255, 220, 90, 255 });
    DrawText(title, x + 20, y + 16, 26, Color{ 255, 220, 90, 255 });
}

void Shop::DrawShop(const Inventory& inv, const Texture2D& flowers, const Texture2D& icons) const {
    const int w = 560, h = 340;
    const int x = GetScreenWidth() / 2 - w / 2, y = GetScreenHeight() / 2 - h / 2;
    Panel(x, y, w, h, "MAGAZIN");

    DrawTexturePro(icons, kCoinIcon, Rectangle{ (float)(x + w - 110), (float)(y + 18), 24, 24 }, {0,0}, 0, WHITE);
    DrawText(TextFormat("%d", inv.money), x + w - 80, y + 20, 22, Color{ 255, 220, 90, 255 });

    for (int i = 0; i < (int)Flower::COUNT; i++) {
        int ry = y + 60 + i * 60;
        if (i == row)
            DrawRectangle(x + 10, ry - 6, w - 20, 56, Color{ 255, 255, 255, 28 });

        DrawTexturePro(flowers, kFlowerBig[i], Rectangle{ (float)(x + 20), (float)ry, 40, 40 }, {0,0}, 0, WHITE);
        DrawText(FLOWERS[i].name, x + 70, ry, 20, WHITE);

        if (!inv.unlocked[i]) {
            DrawText(TextFormat("BLOCAT  -  [U] Deblocheaza: %d", FLOWERS[i].unlockCost),
                     x + 70, ry + 24, 16, Color{ 230, 160, 160, 255 });
        } else {
            DrawText(TextFormat("[B] Samanta: %d   |   [S] Vinde toate (%d) x %d",
                     FLOWERS[i].seedCost, inv.harvested[i], FLOWERS[i].sellPrice),
                     x + 70, ry + 24, 16, Color{ 200, 220, 200, 255 });
        }
    }
    DrawText("Sus/Jos alege  |  TAB/ESC inchide", x + 20, y + h - 30, 16, Color{ 180, 180, 180, 255 });
}

void Shop::DrawSkins(const Inventory& inv) const {
    const int w = 520, h = 360;
    const int x = GetScreenWidth() / 2 - w / 2, y = GetScreenHeight() / 2 - h / 2;
    Panel(x, y, w, h, "SKIN-URI");

    DrawText(TextFormat("Bani: %d", inv.money), x + w - 150, y + 20, 22, Color{ 255, 220, 90, 255 });

    for (int i = 0; i < SkinCount; i++) {
        int ry = y + 60 + i * 52;
        if (i == row)
            DrawRectangle(x + 10, ry - 6, w - 20, 48, Color{ 255, 255, 255, 28 });

        Color c = WHITE;
        std::string status;
        if (i == skinApplied)            { status = "[ECHIPAT]"; c = Color{ 140, 230, 140, 255 }; }
        else if (skinUnlocked[i])        { status = "[ENTER] echipeaza"; }
        else                             { status = TextFormat("[ENTER] cumpara: %d", kSkinCost[i]); c = Color{ 230, 200, 140, 255 }; }

        DrawText(kSkinLabels[i], x + 24, ry, 20, WHITE);
        DrawText(status.c_str(), x + 250, ry + 2, 18, c);
    }
    DrawText("Sus/Jos alege  |  K/ESC inchide", x + 20, y + h - 30, 16, Color{ 180, 180, 180, 255 });
}
