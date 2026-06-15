// Grădina Florii-Soarelui — 0.6 (pachetul FG)
// Hartă + personaj cu unelte + farmat contextual + coliziuni + economie.

#include "raylib.h"
#include "player.h"
#include "tilemap.h"
#include "farm.h"
#include "inventory.h"
#include "shop.h"
#include <vector>
#include <cmath>

struct Tree { Vector2 pos; int variant; };

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
    const int screenW = 960, screenH = 540;
    InitWindow(screenW, screenH, "Gradina Florii-Soarelui");
    SetTargetFPS(60);
    SetExitKey(KEY_ESCAPE);
    ChangeDirectory(GetApplicationDirectory());

    TileMap map;       map.Load();
    Farm farm;         farm.Load();
    Inventory inventory;
    Shop shop;
    Player player;     player.Load("Character01");
    player.position = { map.WorldWidth() / 2, map.WorldHeight() / 2 };

    Texture2D treeTex   = LoadTexture("sprites/Objects/FG_Tree_Summer.png");
    Texture2D iconTex   = LoadTexture("sprites/Item Icons/FG_Item_Icons.png");
    Texture2D flowerTex = LoadTexture("sprites/Objects/FG_Grass_Summer.png");

    // Copaci pe marginile hărții (decor + obstacole). variant 0..3 din sheet (64x80).
    std::vector<Tree> trees;
    for (int x = 2; x < TileMap::Width - 2; x += 3) {
        trees.push_back({ { x * 32.0f + 16, 1 * 32.0f + 64 }, (x) % 4 });
        trees.push_back({ { x * 32.0f + 16, (TileMap::Height - 2) * 32.0f + 64 }, (x + 1) % 4 });
    }
    for (int y = 3; y < TileMap::Height - 3; y += 3) {
        trees.push_back({ { 1 * 32.0f + 16, y * 32.0f + 64 }, y % 4 });
        trees.push_back({ { (TileMap::Width - 2) * 32.0f + 16, y * 32.0f + 64 }, (y + 1) % 4 });
    }

    auto treeBlocks = [&](Vector2 feet) -> bool {
        for (const Tree& t : trees) {
            // trunchiul: dreptunghi mic la baza copacului
            Rectangle trunk{ t.pos.x - 12, t.pos.y - 14, 24, 14 };
            if (CheckCollisionPointRec(feet, trunk)) return true;
        }
        return false;
    };

    Camera2D camera{};
    camera.target = player.position;
    camera.offset = { screenW / 2.0f, screenH / 2.0f };
    camera.zoom = 1.7f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        shop.HandleInput(inventory, player);

        // cât e un meniu deschis, jocul e înghețat
        if (shop.BlocksGameplay()) {
            BeginDrawing();
            ClearBackground(Color{ 60, 130, 60, 255 });
            BeginMode2D(camera);
            map.Draw(camera);
            farm.DrawGround(camera);
            player.Draw();
            EndMode2D();
            inventory.Draw(flowerTex, iconTex);
            shop.Draw(inventory, flowerTex, iconTex);
            EndDrawing();
            continue;
        }

        if (IsKeyPressed(KEY_Q)) inventory.CycleSeed();

        Vector2 prev = player.position;
        player.Update(dt);

        // coliziuni: picioarele personajului ~ puțin sub centru
        Vector2 feet{ player.position.x, player.position.y + 22 };
        if (treeBlocks(feet)) player.position = prev;

        // limite hartă
        if (player.position.x < 16) player.position.x = 16;
        if (player.position.y < 16) player.position.y = 16;
        if (player.position.x > map.WorldWidth() - 16)  player.position.x = map.WorldWidth() - 16;
        if (player.position.y > map.WorldHeight() - 16) player.position.y = map.WorldHeight() - 16;

        farm.Update(dt);

        int tx, ty;
        TargetTile(player, tx, ty);

        // acțiune contextuală: sapă / plantează / udă / recoltează
        if (IsKeyPressed(KEY_E) && !player.IsBusy())
            farm.Interact(tx, ty, inventory, player);

        camera.target.x += (player.position.x - camera.target.x) * 10.0f * dt;
        camera.target.y += (player.position.y - camera.target.y) * 10.0f * dt;

        BeginDrawing();
        ClearBackground(Color{ 60, 130, 60, 255 });

        BeginMode2D(camera);
        map.Draw(camera);
        farm.DrawGround(camera);
        farm.DrawTargetHighlight(tx, ty);

        // copacii + jucătorul sortați după Y (cine e mai jos se desenează peste)
        std::vector<float> ys;
        // desenăm copacii care sunt deasupra jucătorului întâi
        for (const Tree& t : trees) {
            if (t.pos.y <= player.position.y + 22) {
                Rectangle src{ t.variant * 64.0f, 0, 64, 80 };
                DrawTexturePro(treeTex, src,
                    Rectangle{ t.pos.x, t.pos.y, 64, 80 }, { 32, 80 }, 0, WHITE);
            }
        }
        player.Draw();
        for (const Tree& t : trees) {
            if (t.pos.y > player.position.y + 22) {
                Rectangle src{ t.variant * 64.0f, 0, 64, 80 };
                DrawTexturePro(treeTex, src,
                    Rectangle{ t.pos.x, t.pos.y, 64, 80 }, { 32, 80 }, 0, WHITE);
            }
        }
        EndMode2D();

        // HUD
        inventory.Draw(flowerTex, iconTex);
        DrawText("WASD misca | E sapa/planteaza/uda/recolteaza | Q samanta | TAB magazin | K skin-uri",
                 16, screenH - 28, 17, Color{ 255, 255, 255, 220 });
        shop.Draw(inventory, flowerTex, iconTex);
        DrawFPS(screenW - 90, 16);

        EndDrawing();
    }

    UnloadTexture(flowerTex);
    UnloadTexture(iconTex);
    UnloadTexture(treeTex);
    farm.Unload();
    player.Unload();
    map.Unload();
    CloseWindow();
    return 0;
}
