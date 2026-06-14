#include "shop.h"
#include "inventory.h"
#include <cmath>
#include <string>

void Shop::Load() {
    house = LoadTexture("sprites/Happy_Harvest_Free/Decorations/House.png");
}

void Shop::Unload() {
    UnloadTexture(house);
}

Vector2 Shop::DoorPos() const {
    // ușa e la baza-centrul casei
    return { worldPos.x + house.width / 2.0f, worldPos.y + house.height };
}

void Shop::DrawBuilding() const {
    DrawTexture(house, (int)worldPos.x, (int)worldPos.y, WHITE);

    // mică firmă "MAGAZIN" deasupra casei
    const char* label = "MAGAZIN";
    int fs = 16;
    int tw = MeasureText(label, fs);
    int lx = (int)(worldPos.x + house.width / 2 - tw / 2);
    int ly = (int)(worldPos.y - 22);
    DrawRectangle(lx - 6, ly - 4, tw + 12, fs + 8, Color{ 60, 45, 30, 220 });
    DrawText(label, lx, ly, fs, Color{ 255, 220, 90, 255 });
}

bool Shop::PlayerNear(Vector2 playerPos) const {
    Vector2 d = DoorPos();
    float dx = playerPos.x - d.x;
    float dy = playerPos.y - d.y;
    return (dx * dx + dy * dy) < (90.0f * 90.0f);
}

void Shop::HandleInput(Inventory& inv) {
    if (!open) return;

    // Cumpără sămânță
    if (IsKeyPressed(KEY_B)) {
        if (inv.money >= SeedCost) {
            inv.money -= SeedCost;
            inv.seeds += 1;
        }
    }
    // Vinde floare
    if (IsKeyPressed(KEY_S) && !IsKeyDown(KEY_LEFT_SHIFT)) {
        if (inv.sunflowers > 0) {
            inv.sunflowers -= 1;
            inv.money += SellPrice;
        }
    }
}

void Shop::DrawUI(const Inventory& inv) const {
    if (!open) return;

    const int w = 440, h = 230;
    const int x = GetScreenWidth() / 2 - w / 2;
    const int y = GetScreenHeight() / 2 - h / 2;

    DrawRectangle(x, y, w, h, Color{ 40, 30, 20, 235 });
    DrawRectangleLinesEx(Rectangle{ (float)x, (float)y, (float)w, (float)h }, 3,
                         Color{ 255, 220, 90, 255 });

    DrawText("MAGAZIN", x + 20, y + 16, 28, Color{ 255, 220, 90, 255 });
    DrawText(TextFormat("Banuti: %d", inv.money), x + w - 150, y + 22, 20, WHITE);

    DrawText(TextFormat("[B]  Cumpara samanta floarea-soarelui   - %d banuti", SeedCost),
             x + 20, y + 70, 18, WHITE);
    DrawText(TextFormat("[S]  Vinde floarea-soarelui              + %d banuti   (ai: %d)",
             SellPrice, inv.sunflowers), x + 20, y + 105, 18, WHITE);

    DrawText(TextFormat("Seminte in inventar: %d", inv.seeds), x + 20, y + 150, 18,
             Color{ 200, 200, 200, 255 });

    DrawText("[E / ESC]  Inchide", x + 20, y + h - 32, 18, Color{ 180, 180, 180, 255 });
}
