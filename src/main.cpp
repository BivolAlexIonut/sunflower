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

    Texture2D chestTex = LoadTexture("sprites/Objects/FG_Treasure_Big.png");
    Texture2D suppliesTex = LoadTexture("sprites/Plants&supplies/Supplies.png");

    // intrarea în market: un panou lângă cărare (în pădure, sub drum)
    Vector2 marketSpot = { 7 * TS + 16.0f, 18 * TS + 0.0f };
    // casa/depozitul de semințe, jos sub grădină
    Vector2 chestSpot = { 24 * TS + 0.0f, 29 * TS + 0.0f };
    bool storageOpen = false;
    int  storageRow = 0;

    // power-up-uri (lăzi cu provizii) lângă casă: fiecare dă un buff de 5 min
    struct PowerUp { Vector2 pos; Rectangle icon; int buff; int cost; const char* name; };
    PowerUp powerups[] = {
        { { 12*TS+16.0f, 29*TS+0.0f }, {   0, 2, 30, 42 }, 0,  60, "Viteza"          },
        { { 15*TS+16.0f, 29*TS+0.0f }, {  32, 2, 30, 42 }, 1, 130, "Auto-udare"      },
        { { 18*TS+16.0f, 29*TS+0.0f }, {  64, 2, 30, 42 }, 2, 110, "Crestere rapida" },
        { { 28*TS+16.0f, 29*TS+0.0f }, { 192, 2, 30, 42 }, 3, 150, "Bonus bani"      },
        { { 31*TS+16.0f, 29*TS+0.0f }, { 224, 2, 30, 42 }, 4, 100, "Bonus XP"        },
    };
    const int powerupCount = 5;
    const float BuffDuration = 300.0f;   // 5 minute

    // dungeon: dreptunghi în pixeli, pentru ambianța întunecată
    Rectangle dunRect{
        TileMap::DunX0 * (float)TS, TileMap::DunY0 * (float)TS,
        (TileMap::DunX1 - TileMap::DunX0 + 1) * (float)TS,
        (TileMap::DunY1 - TileMap::DunY0 + 1) * (float)TS
    };

    // încarcă progresul salvat
    Vector2 loadedPos = player.position;
    if (SaveIO::Load(SAVE_PATH, inventory, shop, farm, map, loadedPos)) {
        player.position = loadedPos;
        shop.ApplySkin(player);
    }
    world.PopulateOwnedPlots(map);   // resurse pe parcelele noi deja deținute
    float autosaveTimer = 0.0f;
    bool landMode = false;                 // modul hartă (cumpărat teren)
    const int LandLevel = 3;               // nivelul la care se deblochează
    int landMsgTimer = 0;                  // afișează scurt "deblocat la nivel X"

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

        // comutarea modului hartă (cumpărat teren) — deblocat de la un nivel
        if (!frozen && IsKeyPressed(KEY_L)) {
            if (inventory.level >= LandLevel) landMode = !landMode;
            else landMsgTimer = 120;
        }
        if (landMode && IsKeyPressed(KEY_ESCAPE)) landMode = false;
        if (landMsgTimer > 0) landMsgTimer--;

        bool nearChest = (fabsf(player.position.x - (chestSpot.x + 32)) < 56 &&
                          fabsf(player.position.y - chestSpot.y) < 56);

        // depozit de semințe (cufăr): deschide cu E, navighează, ENTER alege, ESC închide
        if (storageOpen) {
            int owned[(int)Flower::COUNT], cnt = 0;
            for (int i = 0; i < (int)Flower::COUNT; i++) if (inventory.seeds[i] > 0) owned[cnt++] = i;
            if (cnt > 0) {
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) storageRow = (storageRow + 1) % cnt;
                if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) storageRow = (storageRow + cnt - 1) % cnt;
                if (IsKeyPressed(KEY_ENTER)) { inventory.selectedSeed = owned[storageRow]; storageOpen = false; }
            }
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_E)) storageOpen = false;
        }
        else if (!frozen && !landMode && nearChest && IsKeyPressed(KEY_E)) {
            storageOpen = true; storageRow = 0;
        }

        // power-up-uri: stația cea mai apropiată
        int nearPowerup = -1;
        for (int i = 0; i < powerupCount; i++)
            if (fabsf(player.position.x - powerups[i].pos.x) < 40 &&
                fabsf(player.position.y - powerups[i].pos.y + 16) < 48) { nearPowerup = i; break; }
        if (!frozen && !landMode && !storageOpen && nearPowerup >= 0 && IsKeyPressed(KEY_E)) {
            if (inventory.money >= powerups[nearPowerup].cost) {
                inventory.money -= powerups[nearPowerup].cost;
                inventory.AddBuff(powerups[nearPowerup].buff, BuffDuration);
            }
        }

        bool blockGameplay = frozen || landMode || storageOpen;

        Vector2 mw = GetScreenToWorld2D(GetMousePosition(), camera);
        int tx = (int)floorf(mw.x / TS), ty = (int)floorf(mw.y / TS);
        Vector2 tileCenter{ tx * TS + TS / 2.0f, ty * TS + TS / 2.0f };
        float ddx = tileCenter.x - player.position.x, ddy = tileCenter.y - player.position.y;
        bool inRange = (ddx*ddx + ddy*ddy) <= reach*reach;

        bool nearMarket = (fabsf(player.position.x - marketSpot.x) < 48 &&
                           fabsf(player.position.y - (marketSpot.y - 24)) < 48);

        // parcela de sub maus (în modul hartă)
        int hpc = (int)floorf(mw.x / (TileMap::PlotW * TS));
        int hpr = (int)floorf(mw.y / (TileMap::PlotH * TS));

        if (landMode) {
            // cumpără parcela dacă e validă (lângă teren deținut, nivel ok, bani ok)
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                hpc >= 0 && hpr >= 0 && hpc < TileMap::PlotCols && hpr < TileMap::PlotRows &&
                !map.PlotOwned(hpc, hpr) && map.PlotAdjacentOwned(hpc, hpr) &&
                inventory.level >= map.PlotLevel(hpc, hpr) &&
                inventory.money >= map.PlotCost(hpc, hpr)) {
                inventory.money -= map.PlotCost(hpc, hpr);
                map.BuyPlot(hpc, hpr);
                world.PopulatePlot(hpc, hpr, map.PlotTheme(hpc, hpr));   // surpriza: resurse
            }
        }

        if (!blockGameplay) {
            if (GetMouseWheelMove() != 0 || IsKeyPressed(KEY_Q)) inventory.CycleSeed();

            Vector2 prev = player.position;
            player.Update(dt);

            Vector2 feet{ player.position.x, player.position.y + 22 };
            int ftx = (int)(feet.x / TS), fty = (int)(feet.y / TS);
            if (map.IsSolid(ftx, fty) || map.TileLocked(ftx, fty) || world.Blocks(feet))
                player.position = prev;   // nu poți intra pe teren necumpărat

            if (player.position.x < 16) player.position.x = 16;
            if (player.position.y < 16) player.position.y = 16;
            if (player.position.x > map.WorldWidth() - 16)  player.position.x = map.WorldWidth() - 16;
            if (player.position.y > map.WorldHeight() - 16) player.position.y = map.WorldHeight() - 16;

            player.speed = inventory.BuffActive(0) ? 200.0f : 130.0f;   // power-up viteză
            farm.Update(dt, inventory.BuffActive(1), inventory.BuffActive(2));
            world.Update(dt);
            inventory.TickTime(dt);
            inventory.UpdateBuffs(dt);

            if (nearMarket && IsKeyPressed(KEY_E)) {           // intră în market
                worldReturnPos = player.position;
                market.Enter(player);
                scene = Scene::Market;
            }
            else if (inventory.buildSel != 0) {                // MOD CONSTRUIRE
                if (IsKeyPressed(KEY_B) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
                    inventory.buildSel = 0;                    // ieși din construire
                else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && inRange) {
                    if (inventory.buildSel == 1 && inventory.roadCount > 0) {
                        map.Place(tx, ty, Terrain::Dirt);  inventory.roadCount--;
                    } else if (inventory.buildSel == 2 && inventory.stoneCount > 0) {
                        map.Place(tx, ty, Terrain::Wall);  inventory.stoneCount--;
                    }
                }
            }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && inRange && !player.IsBusy()) {
                player.FaceTo(tileCenter);
                int r = world.Interact(tx, ty, inventory, player);
                if (r == 0 && map.CanFarm(tx, ty))
                    farm.Interact(tx, ty, inventory, player);
            }
        }

        // camera: în modul hartă privim toată harta (zoom out), altfel urmărim jucătorul
        Vector2 camGoal = landMode ? Vector2{ map.WorldWidth()/2, map.WorldHeight()/2 }
                                   : player.position;
        float zoomGoal = landMode ? 0.34f : 1.7f;
        camera.target.x += (camGoal.x - camera.target.x) * 8.0f * dt;
        camera.target.y += (camGoal.y - camera.target.y) * 8.0f * dt;
        camera.zoom += (zoomGoal - camera.zoom) * 8.0f * dt;

        if (!landMode) {
            float halfW = screenW / 2.0f / camera.zoom, halfH = screenH / 2.0f / camera.zoom;
            float maxX = map.WorldWidth() - halfW, maxY = map.WorldHeight() - halfH;
            camera.target.x = (halfW > maxX) ? map.WorldWidth()/2  : fmaxf(halfW, fminf(camera.target.x, maxX));
            camera.target.y = (halfH > maxY) ? map.WorldHeight()/2 : fmaxf(halfH, fminf(camera.target.y, maxY));
        }

        autosaveTimer += dt;
        if (autosaveTimer >= 20.0f) {
            autosaveTimer = 0.0f;
            SaveIO::Save(SAVE_PATH, inventory, shop, farm, map, player.position);
        }

        BeginDrawing();
        ClearBackground(Color{ 40, 90, 50, 255 });

        BeginMode2D(camera);
        map.Draw(camera);
        DrawRectangleRec(dunRect, Color{ 12, 14, 40, 110 });

        farm.DrawGround(camera);
        if (!blockGameplay && inventory.buildSel == 0) {
            int hax, hay, hsize;
            farm.TargetArea(tx, ty, inventory, hax, hay, hsize);
            farm.DrawTargetHighlight(hax, hay, hsize, inRange);
        } else if (!blockGameplay) {
            farm.DrawTargetHighlight(tx, ty, 1, inRange);   // mod construire: 1x1
        }

        // Y-sorting: panoul de market + copaci/cristale + jucător
        float pFeet = player.position.y + 22;
        auto drawSign = [&](){
            float h = 64, w = marketSign.width * (h / marketSign.height);
            DrawTexturePro(marketSign, { 0,0,(float)marketSign.width,(float)marketSign.height },
                { marketSpot.x, marketSpot.y, w, h }, { w/2, h }, 0, WHITE);
        };
        auto drawChest = [&](){
            DrawTexturePro(chestTex, { 0,0,(float)chestTex.width,(float)chestTex.height },
                { chestSpot.x, chestSpot.y, 72, 36 }, { 36, 36 }, 0, WHITE);
        };
        auto drawPowerups = [&](bool front){
            for (int i = 0; i < powerupCount; i++) {
                if ((powerups[i].pos.y > pFeet) != front) continue;
                float h = 50, w = powerups[i].icon.width * (h / powerups[i].icon.height);
                DrawTexturePro(suppliesTex, powerups[i].icon,
                    { powerups[i].pos.x, powerups[i].pos.y, w, h }, { w/2, h }, 0, WHITE);
            }
        };
        world.DrawBehind(pFeet);
        if (marketSpot.y <= pFeet) drawSign();
        if (chestSpot.y  <= pFeet) drawChest();
        drawPowerups(false);
        player.Draw();
        if (marketSpot.y > pFeet) drawSign();
        if (chestSpot.y  > pFeet) drawChest();
        drawPowerups(true);
        world.DrawFront(pFeet);

        // ceața peste parcelele necumpărate (densă în joc, ușoară în modul hartă)
        map.DrawFog(camera, landMode);
        // evidențiază parcela de sub maus în modul hartă
        if (landMode && hpc >= 0 && hpr >= 0 && hpc < TileMap::PlotCols && hpr < TileMap::PlotRows
            && !map.PlotOwned(hpc, hpr)) {
            Rectangle r{ (float)(hpc*TileMap::PlotW*TS), (float)(hpr*TileMap::PlotH*TS),
                         (float)(TileMap::PlotW*TS), (float)(TileMap::PlotH*TS) };
            bool ok = map.PlotAdjacentOwned(hpc,hpr) && inventory.level >= map.PlotLevel(hpc,hpr)
                      && inventory.money >= map.PlotCost(hpc,hpr);
            DrawRectangleRec(r, ok ? Color{ 90,200,90,70 } : Color{ 200,80,80,60 });
            DrawRectangleLinesEx(r, 3, ok ? Color{ 140,255,140,255 } : Color{ 255,120,120,255 });
        }
        EndMode2D();

        // HUD
        inventory.Draw(flowerTex, iconTex);
        inventory.DrawLevel(farm.CropCount());

        if (landMode) {
            DrawRectangle(0, 0, screenW, 38, Color{ 0,0,0,150 });
            DrawText("MOD HARTA - cumpara teren. Mausul peste o parcela, click ca sa cumperi. ESC/L iesi.",
                     16, 10, 18, Color{ 255, 230, 150, 255 });
            // info despre parcela de sub maus
            if (hpc >= 0 && hpr >= 0 && hpc < TileMap::PlotCols && hpr < TileMap::PlotRows
                && !map.PlotOwned(hpc, hpr)) {
                int cost = map.PlotCost(hpc,hpr), lvl = map.PlotLevel(hpc,hpr);
                const char* st;
                if (!map.PlotAdjacentOwned(hpc,hpr)) st = "trebuie sa fie langa terenul tau";
                else if (inventory.level < lvl)      st = "nivel insuficient";
                else if (inventory.money < cost)     st = "bani insuficienti";
                else                                  st = "CLICK pentru a cumpara";
                const char* info = TextFormat("%s  |  cost %d  |  nivel %d  |  %s",
                                              TileMap::ThemeName(map.PlotTheme(hpc,hpr)), cost, lvl, st);
                int w = MeasureText(info, 20);
                DrawRectangle(screenW/2 - w/2 - 12, screenH - 52, w + 24, 34, Color{ 0,0,0,170 });
                DrawText(info, screenW/2 - w/2, screenH - 46, 20, WHITE);
            }
        } else {
            DrawText("TAB  -  Meniu", screenW - 150, screenH - 28, 18, Color{ 255, 255, 255, 200 });
            const char* lh = (inventory.level >= LandLevel) ? "L - Cumpara teren" : "";
            if (lh[0]) DrawText(lh, screenW - 320, screenH - 28, 18, Color{ 200, 230, 160, 220 });
        }
        if (landMsgTimer > 0) {
            const char* m = TextFormat("Cumparatul de teren se deblocheaza la nivelul %d", LandLevel);
            int w = MeasureText(m, 20);
            DrawRectangle(screenW/2 - w/2 - 12, 60, w + 24, 32, Color{ 0,0,0,180 });
            DrawText(m, screenW/2 - w/2, 66, 20, Color{ 255, 200, 150, 255 });
        }
        if (inventory.buildSel != 0 && !landMode) {
            const char* bm = (inventory.buildSel == 1)
                ? TextFormat("CONSTRUIESTI: Drum (%d) - click pune, B/dreapta iesi", inventory.roadCount)
                : TextFormat("CONSTRUIESTI: Piatra (%d) - click pune, B/dreapta iesi", inventory.stoneCount);
            int w = MeasureText(bm, 20);
            DrawRectangle(screenW/2 - w/2 - 12, 96, w + 24, 32, Color{ 0,0,0,170 });
            DrawText(bm, screenW/2 - w/2, 102, 20, Color{ 255, 230, 150, 255 });
        }
        if (nearMarket && !blockGameplay) {
            const char* m = "[E] Intra in Market";
            int w = MeasureText(m, 22);
            DrawRectangle(screenW/2 - w/2 - 12, 60, w + 24, 34, Color{ 0,0,0,160 });
            DrawText(m, screenW/2 - w/2, 66, 22, WHITE);
        }
        if (nearChest && !blockGameplay) {
            const char* m = "[E] Depozit de seminte";
            int w = MeasureText(m, 22);
            DrawRectangle(screenW/2 - w/2 - 12, 60, w + 24, 34, Color{ 0,0,0,160 });
            DrawText(m, screenW/2 - w/2, 66, 22, WHITE);
        }
        if (nearPowerup >= 0 && !blockGameplay) {
            const char* m = TextFormat("[E] %s  -  %d bani  (5 min)",
                                       powerups[nearPowerup].name, powerups[nearPowerup].cost);
            int w = MeasureText(m, 22);
            DrawRectangle(screenW/2 - w/2 - 12, 60, w + 24, 34, Color{ 0,0,0,170 });
            DrawText(m, screenW/2 - w/2, 66, 22, Color{ 255, 230, 150, 255 });
        }
        // buff-uri active (sub resurse, stânga)
        {
            static const char* bn[5] = { "Viteza", "Auto-udare", "Crestere rapida", "Bonus bani", "Bonus XP" };
            int by = 72;
            for (int i = 0; i < Inventory::BuffCount; i++) if (inventory.buff[i] > 0) {
                int s = (int)inventory.buff[i];
                DrawText(TextFormat("%s  %d:%02d", bn[i], s/60, s%60), 16, by, 16,
                         Color{ 180, 240, 180, 255 });
                by += 20;
            }
        }
        // panoul depozitului de semințe
        if (storageOpen) {
            int owned[(int)Flower::COUNT], cnt = 0;
            for (int i = 0; i < (int)Flower::COUNT; i++) if (inventory.seeds[i] > 0) owned[cnt++] = i;
            int pw = 440, ph = 70 + (cnt > 0 ? cnt : 1) * 30;
            if (ph > 460) ph = 460;
            int px = screenW/2 - pw/2, py = screenH/2 - ph/2;
            DrawRectangle(0, 0, screenW, screenH, Color{ 0,0,0,140 });
            DrawRectangle(px, py, pw, ph, Color{ 38, 28, 20, 245 });
            DrawRectangleLinesEx(Rectangle{ (float)px,(float)py,(float)pw,(float)ph }, 3, Color{ 255,220,90,255 });
            DrawText("DEPOZIT DE SEMINTE", px + 16, py + 12, 22, Color{ 255, 220, 90, 255 });
            if (cnt == 0) DrawText("Nicio samanta. Cumpara din magazin sau market.", px + 16, py + 50, 16,
                                   Color{ 220,200,160,255 });
            for (int j = 0; j < cnt; j++) {
                int f = owned[j], ry = py + 48 + j * 30;
                if (j == storageRow) DrawRectangle(px + 8, ry - 4, pw - 16, 28, Color{ 255,255,255,28 });
                const FlowerInfo& fi = FLOWERS[f];
                DrawTexturePro(flowerTex[fi.tex], fi.r2, Rectangle{ (float)(px+16), (float)ry, 24, 24 }, {0,0}, 0, WHITE);
                DrawText(TextFormat("%s   x%d", fi.name, inventory.seeds[f]), px + 48, ry + 2, 18, WHITE);
            }
            DrawText("Sus/Jos alege  |  ENTER planteaza-o  |  ESC/E inchide",
                     px + 16, py + ph - 26, 15, Color{ 180,180,180,255 });
        }
        shop.Draw(inventory, flowerTex, iconTex);

        EndDrawing();
    }

    // la ieșire salvează poziția din LUME (nu cea din scena de market)
    SaveIO::Save(SAVE_PATH, inventory, shop, farm, map,
                 (scene == Scene::Market) ? worldReturnPos : player.position);

    market.Unload();
    UnloadTexture(suppliesTex);
    UnloadTexture(chestTex);
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
