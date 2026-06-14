// Grădina Florii-Soarelui — prototip stil Forager 0.2
// Hartă top-down + personaj care merge cu WASD. Camera urmărește jucătorul.

#include "raylib.h"
#include "player.h"
#include "tilemap.h"

int main() {
    const int screenW = 960;
    const int screenH = 540;

    InitWindow(screenW, screenH, "Gradina Florii-Soarelui");
    SetTargetFPS(60);

    // Asigură că path-urile relative la sprite-uri funcționează indiferent
    // din ce folder e pornit jocul.
    ChangeDirectory(GetApplicationDirectory());

    TileMap map;
    Player player;
    player.Load();
    player.position = { map.WorldWidth() / 2, map.WorldHeight() / 2 };

    Camera2D camera{};
    camera.target = player.position;
    camera.offset = { screenW / 2.0f, screenH / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        player.Update(dt);

        // ține personajul în limitele hărții
        if (player.position.x < 0) player.position.x = 0;
        if (player.position.y < 0) player.position.y = 0;
        if (player.position.x > map.WorldWidth())  player.position.x = map.WorldWidth();
        if (player.position.y > map.WorldHeight()) player.position.y = map.WorldHeight();

        // camera urmărește lin jucătorul
        camera.target.x += (player.position.x - camera.target.x) * 10.0f * dt;
        camera.target.y += (player.position.y - camera.target.y) * 10.0f * dt;

        BeginDrawing();
        ClearBackground(Color{ 90, 150, 70, 255 });

        BeginMode2D(camera);
        map.Draw(camera);
        player.Draw();
        EndMode2D();

        // HUD
        DrawText("WASD / sageti = miscare", 16, 16, 20, Color{ 255, 255, 255, 220 });
        DrawFPS(screenW - 90, 16);

        EndDrawing();
    }

    player.Unload();
    CloseWindow();
    return 0;
}
