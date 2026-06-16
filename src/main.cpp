// Grădina Florii-Soarelui — 0.8
// Hartă cu 3 zone (pădure / grădină / dungeon), farmat, unelte+resurse, economie, save.

#include "raylib.h"
#include "player.h"
#include "tilemap.h"
#include "farm.h"
#include "inventory.h"
#include "shop.h"
#include "world.h"
#include "saveio.h"
#include <vector>
#include <cmath>

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

    Texture2D iconTex     = LoadTexture("sprites/Item Icons/FG_Item_Icons.png");
    Texture2D flowerSummer = LoadTexture("sprites/Objects/FG_Grass_Summer.png");
    Texture2D flowerWinter = LoadTexture("sprites/Objects/FG_Grass_Winter.png");
    Texture2D chestTex    = LoadTexture("sprites/Objects/FG_Treasure_Big.png");

    Vector2 chestPos    = { 13 * TS + 0.0f, 14 * TS + 0.0f };    // "market" lângă grădină
    Vector2 dunChestPos = { 42 * TS + 0.0f, 14 * TS + 0.0f };    // cufăr-comoară în dungeon

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

        shop.HandleInput(inventory, player);
        bool frozen = shop.BlocksGameplay();

        // tile-ul țintit cu mausul + raza de lucru
        Vector2 mw = GetScreenToWorld2D(GetMousePosition(), camera);
        int tx = (int)floorf(mw.x / TS), ty = (int)floorf(mw.y / TS);
        Vector2 tileCenter{ tx * TS + TS / 2.0f, ty * TS + TS / 2.0f };
        float ddx = tileCenter.x - player.position.x, ddy = tileCenter.y - player.position.y;
        bool inRange = (ddx*ddx + ddy*ddy) <= reach*reach;

        if (!frozen) {
            if (GetMouseWheelMove() != 0 || IsKeyPressed(KEY_Q)) inventory.CycleSeed();

            Vector2 prev = player.position;
            player.Update(dt);

            // coliziuni: ziduri (hartă) + copaci/cristale (world)
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

            // click stânga în rază: întâi noduri, apoi farmat (doar pe iarbă)
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && inRange && !player.IsBusy()) {
                player.FaceTo(tileCenter);
                int r = world.Interact(tx, ty, inventory, player);
                if (r == 0 && map.CanFarm(tx, ty))
                    farm.Interact(tx, ty, inventory, player);
            }
        }

        camera.target.x += (player.position.x - camera.target.x) * 10.0f * dt;
        camera.target.y += (player.position.y - camera.target.y) * 10.0f * dt;

        // ține camera în interiorul hărții (fără să se vadă capătul lumii)
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

        // ambianță de dungeon: voal întunecat peste podeaua de piatră
        // (desenat înainte de cristale/jucător, ca ele să "strălucească")
        DrawRectangleRec(dunRect, Color{ 12, 14, 40, 110 });

        farm.DrawGround(camera);
        if (!frozen) farm.DrawTargetHighlight(tx, ty, inRange);

        // Y-sorting: copaci/cristale + cufere + jucător
        float pFeet = player.position.y + 22;
        auto drawChest = [&](Vector2 p){
            DrawTexturePro(chestTex, Rectangle{0,0,64,32}, Rectangle{ p.x, p.y, 64, 32 }, {32,32}, 0, WHITE);
        };
        world.DrawBehind(pFeet);
        if (chestPos.y    <= pFeet) drawChest(chestPos);
        if (dunChestPos.y <= pFeet) drawChest(dunChestPos);
        player.Draw();
        if (chestPos.y    > pFeet) drawChest(chestPos);
        if (dunChestPos.y > pFeet) drawChest(dunChestPos);
        world.DrawFront(pFeet);
        EndMode2D();

        // HUD
        inventory.Draw(flowerSummer, flowerWinter, iconTex);
        DrawText("TAB  -  Meniu", screenW - 150, screenH - 28, 18, Color{ 255, 255, 255, 200 });
        shop.Draw(inventory, flowerSummer, flowerWinter, iconTex);

        EndDrawing();
    }

    SaveIO::Save(SAVE_PATH, inventory, shop, farm, player.position);

    UnloadTexture(chestTex);
    UnloadTexture(flowerSummer);
    UnloadTexture(flowerWinter);
    UnloadTexture(iconTex);
    world.Unload();
    farm.Unload();
    player.Unload();
    map.Unload();
    CloseWindow();
    return 0;
}
