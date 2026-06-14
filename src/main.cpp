// Grădina Florii-Soarelui — prototip tycoon 0.4
// Personaj Happy Harvest + farmat + economie: bani, magazin (cumpără/vinde).

#include "raylib.h"
#include "player.h"
#include "tilemap.h"
#include "farm.h"
#include "inventory.h"
#include "shop.h"

static void TargetTile(const Player& p, int& tx, int& ty) {
    int px = (int)(p.position.x / TileMap::TileSize);
    int py = (int)(p.position.y / TileMap::TileSize);
    switch (p.facing()) {
        case Dir::Down:  py += 1; break;
        case Dir::Up:    py -= 1; break;
        case Dir::Left:  px -= 1; break;
        case Dir::Right: px += 1; break;
    }
    tx = px; ty = py;
}

int main() {
    const int screenW = 960;
    const int screenH = 540;

    InitWindow(screenW, screenH, "Gradina Florii-Soarelui");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);   // ca ESC să închidă magazinul, nu jocul
    ChangeDirectory(GetApplicationDirectory());

    TileMap map;
    Farm farm;
    Inventory inventory;
    Shop shop;
    Player player;
    player.Load();
    shop.Load();

    Texture2D plants = LoadTexture("sprites/plants/Plants & Flowers.png");
    Texture2D soilTex = LoadTexture("sprites/Happy_Harvest_Free/Tilesets/Soil_Tilled.png");
    Texture2D treeTex = LoadTexture("sprites/Happy_Harvest_Free/Decorations/Trees.png");

    // poziționăm casa-magazin și jucătorul
    shop.worldPos = { 20 * TileMap::TileSize, 3 * TileMap::TileSize };
    player.position = { 21 * TileMap::TileSize, 9 * TileMap::TileSize };

    // câțiva copaci ca decor (colț stânga-sus al fiecărui sprite, în lume)
    const Vector2 trees[] = {
        { 200, 250 }, { 360, 900 }, { 1500, 350 }, { 1650, 800 }, { 250, 1100 }
    };

    Camera2D camera{};
    camera.target = player.position;
    camera.offset = { screenW / 2.0f, screenH / 2.0f };
    camera.zoom = 1.5f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        inventory.HandleInput();

        // mișcarea e înghețată cât e magazinul deschis
        if (!shop.IsOpen())
            player.Update(dt);

        farm.Update(dt);

        // limite hartă
        if (player.position.x < 0) player.position.x = 0;
        if (player.position.y < 0) player.position.y = 0;
        if (player.position.x > map.WorldWidth())  player.position.x = map.WorldWidth();
        if (player.position.y > map.WorldHeight()) player.position.y = map.WorldHeight();

        int tx, ty;
        TargetTile(player, tx, ty);
        bool nearShop = shop.PlayerNear(player.position);

        // tasta E: prioritate magazinul, altfel farmat
        if (IsKeyPressed(KEY_E)) {
            if (shop.IsOpen())       shop.Close();
            else if (nearShop)       shop.Open();
            else                     farm.Interact(tx, ty, inventory);
        }
        if (IsKeyPressed(KEY_ESCAPE)) shop.Close();
        shop.HandleInput(inventory);

        // camera urmărește lin
        camera.target.x += (player.position.x - camera.target.x) * 10.0f * dt;
        camera.target.y += (player.position.y - camera.target.y) * 10.0f * dt;

        BeginDrawing();
        ClearBackground(Color{ 50, 150, 90, 255 });

        BeginMode2D(camera);
        map.Draw(camera);
        farm.DrawGround(camera, plants, soilTex);
        if (!shop.IsOpen())
            farm.DrawTargetHighlight(tx, ty);

        for (const Vector2& t : trees)
            DrawTexture(treeTex, (int)t.x, (int)t.y, WHITE);

        shop.DrawBuilding();
        player.Draw();
        EndMode2D();

        // HUD
        DrawText(TextFormat("Banuti: %d", inventory.money), 16, 16, 24,
                 Color{ 255, 220, 90, 255 });
        if (nearShop && !shop.IsOpen())
            DrawText("[E] Intra in magazin", 16, 48, 18, WHITE);
        else if (!shop.IsOpen())
            DrawText("WASD = miscare   |   E = sapa / planteaza / recolteaza", 16, 48, 18,
                     Color{ 255, 255, 255, 220 });

        inventory.Draw(plants);
        shop.DrawUI(inventory);
        DrawFPS(screenW - 90, 16);

        EndDrawing();
    }

    UnloadTexture(treeTex);
    UnloadTexture(soilTex);
    UnloadTexture(plants);
    shop.Unload();
    player.Unload();
    CloseWindow();
    return 0;
}
