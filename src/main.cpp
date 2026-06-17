// Grădina Florii-Soarelui — 0.8
// Hartă cu 3 zone (pădure / grădină / dungeon), farmat, unelte+resurse, economie, save.

#include "raylib.h"
#include "player.h"
#include "tilemap.h"
#include "farm.h"
#include "inventory.h"
#include "shop.h"
#include "world.h"
#include "market.h"
#include "saveio.h"
#include <vector>
#include <cmath>

enum class Scene { World, Market };

int main() {
    const int screenW = 960, screenH = 540;
    InitWindow(screenW, screenH, "Gradina Florii-Soarelui");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    ChangeDirectory(GetApplicationDirectory());

    const char* SAVE_PATH = "save.txt";
    const int TS = TileMap::TileSize;

    TileMap map;       map.Load();  map.Build();
    Farm farm;         farm.Load();
    World world;       world.Load(); world.Generate(map);
    Inventory inventory;
    Shop shop;
    Player player;     player.Load("Character01");
    player.position = { 24 * TS + 16.0f, 12 * TS + 16.0f };   // grădina

    Texture2D iconTex    = LoadTexture("sprites/Item Icons/FG_Item_Icons.png");
    Texture2D marketSign = LoadTexture("sprites/Market/Environment/Sign_03.png");
    Texture2D flowerTex[4] = {
        LoadTexture("sprites/Objects/FG_Grass_Summer.png"),
        LoadTexture("sprites/Objects/FG_Grass_Winter.png"),
        LoadTexture("sprites/Objects/FG_Grass_Fall.png"),
        LoadTexture("sprites/Plants&supplies/Plants.png"),
    };

    Market market;     market.Load();
    Scene scene = Scene::World;
    Vector2 worldReturnPos = player.position;

    // intrarea în market: un panou lângă cărare (în pădure, sub drum)
    Vector2 marketSpot = { 7 * TS + 16.0f, 18 * TS + 0.0f };

    // dungeon: dreptunghi în pixeli, pentru ambianța întunecată
    Rectangle dunRect{
        TileMap::DunX0 * (float)TS, TileMap::DunY0 * (float)TS,
        (TileMap::DunX1 - TileMap::DunX0 + 1) * (float)TS,
        (TileMap::DunY1 - TileMap::DunY0 + 1) * (float)TS
    };

    // încarcă progresul salvat
    Vector2 loadedPos = player.position;
    if (SaveIO::Load(SAVE_PATH, inventory, shop, farm, loadedPos)) {
        player.position = loadedPos;
        shop.ApplySkin(player);
    }
    float autosaveTimer = 0.0f;

    Camera2D camera{};
    camera.target = player.position;
    camera.offset = { screenW / 2.0f, screenH / 2.0f };
    camera.zoom = 1.7f;

    const float reach = 2.6f * TS;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // ===== SCENA MARKET (side-view) =====
        if (scene == Scene::Market) {
            if (market.Update(dt, player, inventory)) {
                scene = Scene::World;
                player.position = worldReturnPos;
            }
            BeginDrawing();
            ClearBackground(Color{ 150, 190, 220, 255 });
            market.Draw(player, inventory);
            EndDrawing();
            continue;
        }

        // ===== SCENA LUMII (top-down) =====
        shop.HandleInput(inventory, player);
        bool frozen = shop.BlocksGameplay();

        Vector2 mw = GetScreenToWorld2D(GetMousePosition(), camera);
        int tx = (int)floorf(mw.x / TS), ty = (int)floorf(mw.y / TS);
        Vector2 tileCenter{ tx * TS + TS / 2.0f, ty * TS + TS / 2.0f };
        float ddx = tileCenter.x - player.position.x, ddy = tileCenter.y - player.position.y;
        bool inRange = (ddx*ddx + ddy*ddy) <= reach*reach;

        bool nearMarket = (fabsf(player.position.x - marketSpot.x) < 48 &&
                           fabsf(player.position.y - (marketSpot.y - 24)) < 48);

        if (!frozen) {
            if (GetMouseWheelMove() != 0 || IsKeyPressed(KEY_Q)) inventory.CycleSeed();

            Vector2 prev = player.position;
            player.Update(dt);

            Vector2 feet{ player.position.x, player.position.y + 22 };
            int ftx = (int)(feet.x / TS), fty = (int)(feet.y / TS);
            if (map.IsSolid(ftx, fty) || world.Blocks(feet)) player.position = prev;

            if (player.position.x < 16) player.position.x = 16;
            if (player.position.y < 16) player.position.y = 16;
            if (player.position.x > map.WorldWidth() - 16)  player.position.x = map.WorldWidth() - 16;
            if (player.position.y > map.WorldHeight() - 16) player.position.y = map.WorldHeight() - 16;

            farm.Update(dt);
            world.Update(dt);
            inventory.TickTime(dt);

            if (nearMarket && IsKeyPressed(KEY_E)) {           // intră în market
                worldReturnPos = player.position;
                market.Enter(player);
                scene = Scene::Market;
            }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && inRange && !player.IsBusy()) {
                player.FaceTo(tileCenter);
                int r = world.Interact(tx, ty, inventory, player);
                if (r == 0 && map.CanFarm(tx, ty))
                    farm.Interact(tx, ty, inventory, player);
            }
        }

        camera.target.x += (player.position.x - camera.target.x) * 10.0f * dt;
        camera.target.y += (player.position.y - camera.target.y) * 10.0f * dt;

        float halfW = screenW / 2.0f / camera.zoom, halfH = screenH / 2.0f / camera.zoom;
        float maxX = map.WorldWidth() - halfW, maxY = map.WorldHeight() - halfH;
        camera.target.x = (halfW > maxX) ? map.WorldWidth()/2  : fmaxf(halfW, fminf(camera.target.x, maxX));
        camera.target.y = (halfH > maxY) ? map.WorldHeight()/2 : fmaxf(halfH, fminf(camera.target.y, maxY));

        autosaveTimer += dt;
        if (autosaveTimer >= 20.0f) {
            autosaveTimer = 0.0f;
            SaveIO::Save(SAVE_PATH, inventory, shop, farm, player.position);
        }

        BeginDrawing();
        ClearBackground(Color{ 40, 90, 50, 255 });

        BeginMode2D(camera);
        map.Draw(camera);
        DrawRectangleRec(dunRect, Color{ 12, 14, 40, 110 });

        farm.DrawGround(camera);
        if (!frozen) farm.DrawTargetHighlight(tx, ty, inRange);

        // Y-sorting: panoul de market + copaci/cristale + jucător
        float pFeet = player.position.y + 22;
        auto drawSign = [&](){
            float h = 64, w = marketSign.width * (h / marketSign.height);
            DrawTexturePro(marketSign, { 0,0,(float)marketSign.width,(float)marketSign.height },
                { marketSpot.x, marketSpot.y, w, h }, { w/2, h }, 0, WHITE);
        };
        world.DrawBehind(pFeet);
        if (marketSpot.y <= pFeet) drawSign();
        player.Draw();
        if (marketSpot.y > pFeet) drawSign();
        world.DrawFront(pFeet);
        EndMode2D();

        // HUD
        inventory.Draw(flowerTex, iconTex);
        DrawText("TAB  -  Meniu", screenW - 150, screenH - 28, 18, Color{ 255, 255, 255, 200 });
        if (nearMarket && !frozen) {
            const char* m = "[E] Intra in Market";
            int w = MeasureText(m, 22);
            DrawRectangle(screenW/2 - w/2 - 12, 60, w + 24, 34, Color{ 0,0,0,160 });
            DrawText(m, screenW/2 - w/2, 66, 22, WHITE);
        }
        shop.Draw(inventory, flowerTex, iconTex);

        EndDrawing();
    }

    // la ieșire salvează poziția din LUME (nu cea din scena de market)
    SaveIO::Save(SAVE_PATH, inventory, shop, farm,
                 (scene == Scene::Market) ? worldReturnPos : player.position);

    market.Unload();
    UnloadTexture(marketSign);
    for (int i = 0; i < 4; i++) UnloadTexture(flowerTex[i]);
    UnloadTexture(iconTex);
    world.Unload();
    farm.Unload();
    player.Unload();
    map.Unload();
    CloseWindow();
    return 0;
}
