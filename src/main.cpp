// Grădina Florii-Soarelui — repornire 0.5 (pachetul FG)
// Hartă din tileset + personaj cu TOATE abilitățile (mers + unelte).

#include "raylib.h"
#include "player.h"
#include "tilemap.h"

static const char* ActionLabel(Action a) {
    switch (a) {
        case Action::Walk:     return "Merge";
        case Action::Hoe:      return "Sapa (Hoe)";
        case Action::Axe:      return "Topor (Axe)";
        case Action::Pickaxe:  return "Tarnacop (Pickaxe)";
        case Action::Sword:    return "Sabie (Sword)";
        case Action::Watercan: return "Stropitoare (Watercan)";
        case Action::Dead:     return "K.O.";
        default:               return "";
    }
}

int main() {
    const int screenW = 960;
    const int screenH = 540;

    InitWindow(screenW, screenH, "Gradina Florii-Soarelui");
    SetTargetFPS(60);
    SetExitKey(KEY_ESCAPE);
    ChangeDirectory(GetApplicationDirectory());

    TileMap map;
    Player player;
    map.Load();
    player.Load("Character01");
    player.position = { map.WorldWidth() / 2, map.WorldHeight() / 2 };

    Texture2D treeTex = LoadTexture("sprites/Objects/FG_Tree_Summer.png");
    Rectangle treeSrc{ 0, 0, 64, 80 };   // primul copac din sheet
    const Vector2 trees[] = {
        { 160, 200 }, { 360, 720 }, { 980, 260 }, { 1120, 640 }, { 200, 900 }, { 1050, 880 }
    };

    Camera2D camera{};
    camera.target = player.position;
    camera.offset = { screenW / 2.0f, screenH / 2.0f };
    camera.zoom = 1.6f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // taste de abilități (one-shot) — doar dacă nu lucrează deja
        if (IsKeyPressed(KEY_ONE))   player.StartAction(Action::Hoe);
        if (IsKeyPressed(KEY_TWO))   player.StartAction(Action::Axe);
        if (IsKeyPressed(KEY_THREE)) player.StartAction(Action::Pickaxe);
        if (IsKeyPressed(KEY_FOUR))  player.StartAction(Action::Sword);
        if (IsKeyPressed(KEY_FIVE))  player.StartAction(Action::Watercan);
        if (IsKeyPressed(KEY_SIX))   player.StartAction(Action::Dead);

        player.Update(dt);

        // limite hartă
        if (player.position.x < 0) player.position.x = 0;
        if (player.position.y < 0) player.position.y = 0;
        if (player.position.x > map.WorldWidth())  player.position.x = map.WorldWidth();
        if (player.position.y > map.WorldHeight()) player.position.y = map.WorldHeight();

        // camera urmărește lin
        camera.target.x += (player.position.x - camera.target.x) * 10.0f * dt;
        camera.target.y += (player.position.y - camera.target.y) * 10.0f * dt;

        BeginDrawing();
        ClearBackground(Color{ 60, 130, 60, 255 });

        BeginMode2D(camera);
        map.Draw(camera);

        // copacii + jucătorul, sortați după Y ca să se suprapună corect
        // (simplu: desenăm copacii, apoi jucătorul; copacii sunt bottom-anchored)
        for (const Vector2& t : trees)
            DrawTexturePro(treeTex, treeSrc,
                Rectangle{ t.x, t.y, treeSrc.width * 2, treeSrc.height * 2 },
                { treeSrc.width, treeSrc.height * 2 }, 0.0f, WHITE);

        player.Draw();
        EndMode2D();

        // HUD
        DrawText("WASD = miscare   |   1 Hoe  2 Axe  3 Pickaxe  4 Sword  5 Watercan  6 Dead",
                 16, 16, 18, Color{ 255, 255, 255, 230 });
        DrawText(TextFormat("Actiune: %s", ActionLabel(player.CurrentAction())),
                 16, 42, 20, Color{ 255, 230, 120, 255 });
        DrawFPS(screenW - 90, 16);

        EndDrawing();
    }

    UnloadTexture(treeTex);
    player.Unload();
    map.Unload();
    CloseWindow();
    return 0;
}
