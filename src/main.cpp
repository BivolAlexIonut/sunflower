// Grădina Florii-Soarelui — prototip stil Forager 0.3
// Hartă top-down + personaj + farmat (sapă / plantează / recoltează) + inventar.

#include "raylib.h"
#include "player.h"
#include "tilemap.h"
#include "farm.h"
#include "inventory.h"

// Parcela din fața personajului, în funcție de direcția în care privește.
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
    ChangeDirectory(GetApplicationDirectory());

    TileMap map;
    Farm farm;
    Inventory inventory;
    Player player;
    player.Load();
    player.position = { map.WorldWidth() / 2, map.WorldHeight() / 2 };

    Texture2D plants = LoadTexture("sprites/plants/Plants & Flowers.png");

    Camera2D camera{};
    camera.target = player.position;
    camera.offset = { screenW / 2.0f, screenH / 2.0f };
    camera.zoom = 1.5f;     // zoom mai aproape, ca un joc de farmat

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        player.Update(dt);
        inventory.HandleInput();
        farm.Update(dt);

        // ține personajul în limitele hărții
        if (player.position.x < 0) player.position.x = 0;
        if (player.position.y < 0) player.position.y = 0;
        if (player.position.x > map.WorldWidth())  player.position.x = map.WorldWidth();
        if (player.position.y > map.WorldHeight()) player.position.y = map.WorldHeight();

        int tx, ty;
        TargetTile(player, tx, ty);

        // acțiune de farmat pe parcela țintă
        if (IsKeyPressed(KEY_E))
            farm.Interact(tx, ty, inventory);

        // camera urmărește lin jucătorul
        camera.target.x += (player.position.x - camera.target.x) * 10.0f * dt;
        camera.target.y += (player.position.y - camera.target.y) * 10.0f * dt;

        BeginDrawing();
        ClearBackground(Color{ 90, 150, 70, 255 });

        BeginMode2D(camera);
        map.Draw(camera);
        farm.DrawGround(camera, plants);
        farm.DrawTargetHighlight(tx, ty);
        player.Draw();
        EndMode2D();

        // HUD
        DrawText("WASD = miscare   |   E = sapa / planteaza / recolteaza", 16, 16, 18,
                 Color{ 255, 255, 255, 220 });
        inventory.Draw(plants);
        DrawFPS(screenW - 90, 16);

        EndDrawing();
    }

    UnloadTexture(plants);
    player.Unload();
    CloseWindow();
    return 0;
}
