#include "inventory.h"
#include <string>

const FlowerInfo FLOWERS[(int)Flower::COUNT] = {
    { "Floare alba",      10,  25, 3.0f,    0 },
    { "Floare roz",       40,  90, 4.0f,  150 },
    { "Floare rosie",    120, 280, 5.0f,  600 },
    { "Floarea-soarelui", 400, 950, 7.0f, 3000 },
};

// Dreptunghiuri sursă în FG_Grass_Summer.png (16x16): floarea mare (matură) per culoare.
static const Rectangle kFlowerBig[4] = {
    {  0, 0, 16, 16 }, { 16, 0, 16, 16 }, { 32, 0, 16, 16 }, { 48, 0, 16, 16 }
};
// Iconița de monede din FG_Item_Icons.png
static const Rectangle kCoinIcon = { 0, 80, 16, 16 };

void Inventory::CycleSeed() {
    for (int i = 1; i <= (int)Flower::COUNT; i++) {
        int idx = (selectedSeed + i) % (int)Flower::COUNT;
        if (unlocked[idx]) { selectedSeed = idx; return; }
    }
}

void Inventory::Draw(const Texture2D& flowers, const Texture2D& icons) const {
    // Bani (sus-stânga) cu iconița de monede
    DrawTexturePro(icons, kCoinIcon, Rectangle{ 16, 14, 28, 28 }, { 0, 0 }, 0.0f, WHITE);
    DrawText(TextFormat("%d", money), 50, 16, 26, Color{ 255, 220, 90, 255 });

    // Hotbar jos: sămânța selectată + recolta ei
    const int slot = 64;
    const int x0 = GetScreenWidth() / 2 - slot;
    const int y0 = GetScreenHeight() - slot - 16;

    // slot sămânță selectată
    DrawRectangle(x0, y0, slot, slot, Color{ 40, 30, 20, 210 });
    DrawRectangleLinesEx(Rectangle{ (float)x0, (float)y0, (float)slot, (float)slot }, 3,
                         Color{ 255, 220, 90, 255 });
    Rectangle src = kFlowerBig[selectedSeed];
    DrawTexturePro(flowers, src, Rectangle{ x0 + 12.0f, y0 + 8.0f, 40, 40 }, { 0, 0 }, 0.0f, WHITE);
    DrawText(TextFormat("x%d", seeds[selectedSeed]), x0 + 6, y0 + slot - 20, 18, WHITE);

    // numele florii selectate + câte are recoltate
    DrawText(FLOWERS[selectedSeed].name, x0 + slot + 12, y0 + 8, 18, WHITE);
    DrawText(TextFormat("Recoltate: %d", harvested[selectedSeed]), x0 + slot + 12, y0 + 34, 16,
             Color{ 200, 200, 200, 255 });
}
