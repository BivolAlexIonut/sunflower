// Grădina Florii-Soarelui — 0.6 (pachetul FG)
// Hartă + personaj cu unelte + farmat contextual + coliziuni + economie.

#include "raylib.h"
#include "player.h"
#include "tilemap.h"
#include "farm.h"
#include "inventory.h"
#include "shop.h"
#include "saveio.h"
#include <vector>
#include <cmath>

struct Tree { Vector2 pos; int variant; };

int main() {
    const int screenW = 960, screenH = 540;
    InitWindow(screenW, screenH, "Gradina Florii-Soarelui");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);   // ESC închide meniul, nu jocul
    ChangeDirectory(GetApplicationDirectory());

    const char* SAVE_PATH = "save.txt";

    TileMap map;       map.Load();
    Farm farm;         farm.Load();
    Inventory inventory;
    Shop shop;
    Player player;     player.Load("Character01");
    player.position = { map.WorldWidth() / 2, map.WorldHeight() / 2 };

    Texture2D treeTex   = LoadTexture("sprites/Objects/FG_Tree_Summer.png");
    Texture2D iconTex   = LoadTexture("sprites/Item Icons/FG_Item_Icons.png");
    Texture2D flowerTex = LoadTexture("sprites/Objects/FG_Grass_Summer.png");
    Texture2D groundTex = LoadTexture("sprites/Tilesets/FG_Grounds.png");
    Texture2D chestTex  = LoadTexture("sprites/Objects/FG_Treasure_Big.png");

    auto hash = [](int a, int b) -> unsigned int {
        unsigned int h = (unsigned int)(a*73856093) ^ (unsigned int)(b*19349663);
        h ^= h>>13; h *= 0x5bd1e995; h ^= h>>15; return h;
    };

    // Zona centrală de fermă rămâne liberă; copacii și decorul stau în jur.
    const int cx0 = 8, cy0 = 7, cx1 = TileMap::Width - 8, cy1 = TileMap::Height - 7;
    auto inFarmArea = [&](int tx, int ty) {
        return tx >= cx0 && tx < cx1 && ty >= cy0 && ty < cy1;
    };

    // Copaci așezați natural pe marginea hărții (cu jitter), nu în zona de fermă.
    std::vector<Tree> trees;
    for (int ty = 1; ty < TileMap::Height - 1; ty++) {
        for (int tx = 1; tx < TileMap::Width - 1; tx++) {
            if (inFarmArea(tx, ty)) continue;
            unsigned int h = hash(tx, ty);
            if (h % 5 != 0) continue;                       // densitate
            float jx = (float)((h >> 4) % 16) - 8;
            float jy = (float)((h >> 8) % 16) - 8;
            trees.push_back({ { tx*32.0f + 16 + jx, ty*32.0f + 64 + jy }, (int)(h % 4) });
        }
    }

    // Flori decorative (vizuale) pe marginea zonei, ca să nu fie pustiu.
    struct Deco { Vector2 pos; int variant; };
    std::vector<Deco> deco;
    for (int ty = 1; ty < TileMap::Height - 1; ty++) {
        for (int tx = 1; tx < TileMap::Width - 1; tx++) {
            if (inFarmArea(tx, ty)) continue;
            unsigned int h = hash(tx*7+1, ty*13+3);
            if (h % 6 != 0) continue;
            deco.push_back({ { tx*32.0f + 8 + (float)(h%16), ty*32.0f + 8 + (float)((h>>4)%16) },
                             (int)(h % 4) });
        }
    }

    // Cărare de pământ (vizuală) din zona de fermă spre margini.
    std::vector<Vector2> path;
    int pcx = TileMap::Width/2, pcy = TileMap::Height/2;
    for (int x = 2; x < TileMap::Width-2; x++)  path.push_back({ (float)x, (float)pcy });
    for (int y = 2; y < TileMap::Height-2; y++) path.push_back({ (float)pcx, (float)y });

    Vector2 chestPos = { (cx0 - 1) * 32.0f, (pcy) * 32.0f };   // "market" lângă fermă
    Rectangle pathTile{ 80, 16, 16, 16 };                       // pământ auriu

    auto treeBlocks = [&](Vector2 feet) -> bool {
        for (const Tree& t : trees) {
            // trunchiul: dreptunghi mic la baza copacului
            Rectangle trunk{ t.pos.x - 12, t.pos.y - 14, 24, 14 };
            if (CheckCollisionPointRec(feet, trunk)) return true;
        }
        return false;
    };

    // încarcă progresul salvat (dacă există)
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

    const int TS = TileMap::TileSize;
    const float reach = 2.6f * TS;   // cât de departe poate lucra jucătorul

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        shop.HandleInput(inventory, player);
        bool frozen = shop.BlocksGameplay();

        // tile-ul țintit cu mausul + dacă e în raza de lucru
        Vector2 mw = GetScreenToWorld2D(GetMousePosition(), camera);
        int tx = (int)floorf(mw.x / TS), ty = (int)floorf(mw.y / TS);
        Vector2 tileCenter{ tx * TS + TS / 2.0f, ty * TS + TS / 2.0f };
        float dx = tileCenter.x - player.position.x, dy = tileCenter.y - player.position.y;
        bool inRange = (dx*dx + dy*dy) <= reach*reach;

        if (!frozen) {
            // schimbă floarea de plantat: rotița mausului sau Q
            if (GetMouseWheelMove() != 0 || IsKeyPressed(KEY_Q)) inventory.CycleSeed();

            Vector2 prev = player.position;
            player.Update(dt);

            Vector2 feet{ player.position.x, player.position.y + 22 };
            if (treeBlocks(feet)) player.position = prev;

            if (player.position.x < 16) player.position.x = 16;
            if (player.position.y < 16) player.position.y = 16;
            if (player.position.x > map.WorldWidth() - 16)  player.position.x = map.WorldWidth() - 16;
            if (player.position.y > map.WorldHeight() - 16) player.position.y = map.WorldHeight() - 16;

            farm.Update(dt);

            // click stânga pe un tile din rază → sapă / plantează / udă / recoltează
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && inRange && !player.IsBusy()) {
                player.FaceTo(tileCenter);
                farm.Interact(tx, ty, inventory, player);
            }
        }

        camera.target.x += (player.position.x - camera.target.x) * 10.0f * dt;
        camera.target.y += (player.position.y - camera.target.y) * 10.0f * dt;

        // autosave la fiecare 20 secunde
        autosaveTimer += dt;
        if (autosaveTimer >= 20.0f) {
            autosaveTimer = 0.0f;
            SaveIO::Save(SAVE_PATH, inventory, shop, farm, player.position);
        }

        BeginDrawing();
        ClearBackground(Color{ 60, 130, 60, 255 });

        BeginMode2D(camera);
        map.Draw(camera);

        for (const Vector2& p : path)
            DrawTexturePro(groundTex, pathTile,
                Rectangle{ p.x*TS, p.y*TS, (float)TS, (float)TS }, {0,0}, 0, WHITE);

        farm.DrawGround(camera);
        if (!frozen) farm.DrawTargetHighlight(tx, ty, inRange);

        for (const Deco& d : deco)
            DrawTexturePro(flowerTex, Rectangle{ d.variant*16.0f, 0, 16, 16 },
                Rectangle{ d.pos.x, d.pos.y, 24, 24 }, {12,24}, 0, WHITE);

        float pFeet = player.position.y + 22;
        auto drawTree = [&](const Tree& t){
            DrawTexturePro(treeTex, Rectangle{ t.variant*64.0f, 0, 64, 80 },
                Rectangle{ t.pos.x, t.pos.y, 64, 80 }, { 32, 80 }, 0, WHITE);
        };
        for (const Tree& t : trees) if (t.pos.y <= pFeet) drawTree(t);
        if (chestPos.y <= pFeet)
            DrawTexturePro(chestTex, Rectangle{0,0,64,32},
                Rectangle{ chestPos.x, chestPos.y, 64, 32 }, {32,32}, 0, WHITE);

        player.Draw();

        if (chestPos.y > pFeet)
            DrawTexturePro(chestTex, Rectangle{0,0,64,32},
                Rectangle{ chestPos.x, chestPos.y, 64, 32 }, {32,32}, 0, WHITE);
        for (const Tree& t : trees) if (t.pos.y > pFeet) drawTree(t);
        EndMode2D();

        // HUD minimal: bani + hotbar (restul se explică în meniu)
        inventory.Draw(flowerTex, iconTex);
        DrawText("TAB  -  Meniu", screenW - 150, screenH - 28, 18, Color{ 255, 255, 255, 200 });
        shop.Draw(inventory, flowerTex, iconTex);

        EndDrawing();
    }

    // salvează la ieșire
    SaveIO::Save(SAVE_PATH, inventory, shop, farm, player.position);

    UnloadTexture(chestTex);
    UnloadTexture(groundTex);
    UnloadTexture(flowerTex);
    UnloadTexture(iconTex);
    UnloadTexture(treeTex);
    farm.Unload();
    player.Unload();
    map.Unload();
    CloseWindow();
    return 0;
}
