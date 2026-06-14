#include "inventory.h"
#include <string>

// Surse din spritesheet-ul de plante pentru iconițe (x,y,w,h în pixeli).
static const Rectangle kSeedIcon   = { 96, 103, 20, 26 };   // floarea-soarelui tânără
static const Rectangle kFlowerIcon = { 77,  95, 21, 35 };   // floarea-soarelui matură

void Inventory::HandleInput() {
    if (IsKeyPressed(KEY_ONE)) selected = 0;
    if (IsKeyPressed(KEY_TWO)) selected = 1;
}

bool Inventory::ConsumeSeed() {
    if (seeds <= 0) return false;
    seeds--;
    return true;
}

void Inventory::AddHarvest() {
    sunflowers += 1;
    seeds += 2;     // fiecare floare dă semințe noi → economia crește singură
}

void Inventory::Draw(const Texture2D& plants) const {
    const int slot = 64;
    const int pad = 8;
    const int totalW = SlotCount * slot + (SlotCount - 1) * pad;
    const int x0 = GetScreenWidth() / 2 - totalW / 2;
    const int y0 = GetScreenHeight() - slot - 16;

    for (int i = 0; i < SlotCount; i++) {
        int x = x0 + i * (slot + pad);

        // fundalul slotului + chenar (evidențiat dacă e selectat)
        DrawRectangle(x, y0, slot, slot, Color{ 60, 45, 30, 210 });
        Color border = (i == selected) ? Color{ 255, 220, 90, 255 }
                                       : Color{ 120, 95, 60, 255 };
        DrawRectangleLinesEx(Rectangle{ (float)x, (float)y0, (float)slot, (float)slot },
                             (i == selected) ? 3.0f : 2.0f, border);

        // iconița + numărul
        Rectangle srcIcon = (i == 0) ? kSeedIcon : kFlowerIcon;
        int count = (i == 0) ? seeds : sunflowers;

        float iconScale = 1.4f;
        float iw = srcIcon.width * iconScale;
        float ih = srcIcon.height * iconScale;
        Rectangle dst{ x + slot / 2.0f - iw / 2.0f, y0 + slot / 2.0f - ih / 2.0f, iw, ih };
        DrawTexturePro(plants, srcIcon, dst, { 0, 0 }, 0.0f, WHITE);

        std::string num = std::to_string(count);
        DrawText(num.c_str(), x + slot - MeasureText(num.c_str(), 20) - 6,
                 y0 + slot - 22, 20, WHITE);

        // tasta de selecție
        std::string key = std::to_string(i + 1);
        DrawText(key.c_str(), x + 4, y0 + 2, 16, Color{ 255, 255, 255, 180 });
    }
}
