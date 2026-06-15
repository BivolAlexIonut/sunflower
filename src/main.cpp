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

        // cărarea de pământ (sub fermă și decor)
        for (const Vector2& p : path)
            DrawTexturePro(groundTex, pathTile,
                Rectangle{ p.x*32, p.y*32, 32, 32 }, {0,0}, 0, WHITE);

        farm.DrawGround(camera);
        farm.DrawTargetHighlight(tx, ty);

        // flori decorative (ground level, sub jucător)
        for (const Deco& d : deco)
            DrawTexturePro(flowerTex, Rectangle{ d.variant*16.0f, 0, 16, 16 },
                Rectangle{ d.pos.x, d.pos.y, 24, 24 }, {12,24}, 0, WHITE);

        // cufărul-market + copacii + jucătorul, sortați după Y
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

        // HUD
        inventory.Draw(flowerTex, iconTex);
        DrawText("WASD misca | E sapa/planteaza/uda/recolteaza | Q samanta | TAB magazin | K skin-uri",
                 16, screenH - 28, 17, Color{ 255, 255, 255, 220 });
        shop.Draw(inventory, flowerTex, iconTex);
        DrawFPS(screenW - 90, 16);

        EndDrawing();
    }

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
