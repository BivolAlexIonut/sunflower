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
#include "particles.h"
#include "memories.h"
#include "ambient.h"
#include <vector>
#include <cmath>
#include <string>

enum class Scene { Menu, World, Market };
enum class MPage { Main, Diff, Slot, Load, Help };   // paginile meniului principal

int main(int argc, char** argv) {
    std::string arg1 = (argc > 1) ? argv[1] : "";
    bool shotMenu = (arg1 == "--shotmenu");
    bool shotMode = (arg1 == "--shot") || shotMenu;
    const int screenW = 960, screenH = 540;
    InitWindow(screenW, screenH, "Gradina Florii-Soarelui");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    ChangeDirectory(GetApplicationDirectory());

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
    Particles fx;                       // particule (apă, petale, monede, stele, inimi)
    Memories memories;                  // momentele relației (locuri + bilețele)
    Ambient ambient;   ambient.Load();  // albine/fluturi + flori decorative
    Scene scene = Scene::Menu;
    MPage mpage = MPage::Main;
    int  msel = 0;            // selecție în meniu
    int  newDiff = 1;         // dificultatea aleasă la joc nou
    Vector2 worldReturnPos = player.position;

    // mesaj scurt (toast) pentru recompense la cumpărat teren etc.
    std::string toastMsg; int toastTimer = 0;
    // popup "bine ai revenit" (progres offline)
    std::string welcomeMsg; int welcomeTimer = 0;
    int prevLevel = inventory.level;    // pentru a detecta level-up (stele)

    Texture2D chestTex = LoadTexture("sprites/NewAssets/Buildings/Buildings/Houses/Wood/House_1_Wood_Base_Red.png");
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

    // calea fiecărui slot de salvare (3 sloturi)
    std::string savePath;
    auto slotPath = [](int slot) { return std::string("save") + std::to_string(slot) + ".txt"; };

    // PROGRES OFFLINE (idle): plantele UDATE cresc mai departe, albinele strang nectar (bani).
    auto applyOffline = [&](double offlineSeconds) {
        if (offlineSeconds <= 60.0) return;
        // creșterea + setea/moartea folosesc timpul REAL (neglijarea omoară plantele)
        int died = 0;
        int grown = farm.ApplyOffline(offlineSeconds, died);
        // banii de la albine + ziua folosesc un plafon de 24h (să nu explodeze)
        const double MaxOffline = 24.0 * 3600.0;
        double capped = (offlineSeconds < MaxOffline) ? offlineSeconds : MaxOffline;
        int crops = farm.CropCount();
        int bees = (int)((1.0 + 0.6 * crops) * (capped / 60.0));
        if (bees > 50000) bees = 50000;
        inventory.money += bees;
        inventory.day += (int)(capped / Inventory::DaySeconds);
        int hh = (int)(offlineSeconds / 3600), mm = ((int)offlineSeconds % 3600) / 60;
        std::string m = TextFormat("Ai lipsit %dh %dm.\nPlantele udate au crescut %d stadii.\n", hh, mm, grown);
        if (died > 0) m += TextFormat("%d plante au murit de sete!\n", died);
        m += TextFormat("Albinele au strans %d bani din florile tale.", bees);
        welcomeMsg = m;
        welcomeTimer = 600;   // ~10s
    };

    auto startNewGame = [&](int slot, int diff) {
        savePath = slotPath(slot);
        inventory = Inventory{};
        inventory.difficulty = diff;
        inventory.money = inventory.StartMoney();
        memories = Memories{};
        shop = Shop{};
        farm.Reset();
        farm.SetGrowMul(inventory.GrowMul());
        map.Build();
        world.Generate(map);
        world.PopulateOwnedPlots(map);
        ambient.Init(map);
        shop.ApplySkin(player);
        player.position = { 24 * TS + 16.0f, 12 * TS + 16.0f };
        prevLevel = 1; welcomeTimer = 0; fx.Clear();
        scene = Scene::World;
    };

    auto loadGame = [&](int slot) -> bool {
        std::string path = slotPath(slot);
        Vector2 lp = player.position; double off = 0.0;
        if (!SaveIO::Load(path.c_str(), inventory, shop, farm, map, memories, lp, off)) return false;
        savePath = path;
        player.position = lp;
        shop.ApplySkin(player);
        farm.SetGrowMul(inventory.GrowMul());
        world.Generate(map);
        world.PopulateOwnedPlots(map);
        ambient.Init(map);
        prevLevel = inventory.level; welcomeTimer = 0; fx.Clear();
        applyOffline(off);
        scene = Scene::World;
        return true;
    };

    if (shotMode && !shotMenu) {
        startNewGame(1, 1); player.position = { 40*TS + 16.0f, 15*TS + 16.0f };
    }
    float autosaveTimer = 0.0f;
    bool landMode = false;                 // modul hartă (cumpărat teren)
    const int LandLevel = 3;               // nivelul la care se deblochează
    int landMsgTimer = 0;                  // afișează scurt "deblocat la nivel X"

    int digTx = -1, digTy = -1;            // săpat: ții click apăsat câteva secunde
    float digProgress = 0.0f;
    const float DigTime = 3.0f;            // secunde de săpat per tile

    Camera2D camera{};
    camera.target = player.position;
    camera.offset = { screenW / 2.0f, screenH / 2.0f };
    camera.zoom = 1.7f;

    const float reach = 2.6f * TS;

    int shotFrame = 0;
    float menuT = 0.0f;
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // ===== MENIU PRINCIPAL =====
        if (scene == Scene::Menu) {
            menuT += dt;
            // ambianță: petale + inimi
            if (GetRandomValue(0, 100) < 7)
                fx.Petals({ (float)GetRandomValue(0, screenW), -10 }, Color{ 255, 210, 90, 255 });
            if (GetRandomValue(0, 100) < 3)
                fx.Hearts({ (float)GetRandomValue(0, screenW), (float)screenH + 10 }, 1);
            fx.Update(dt);

            // rezumatele sloturilor (pentru paginile Slot / Load)
            struct Slot { bool used; int level, money, diff; double play; };
            Slot slots[3];
            for (int i = 0; i < 3; i++) {
                slots[i].used = SaveIO::ReadSummary(slotPath(i+1).c_str(),
                    slots[i].level, slots[i].money, slots[i].diff, slots[i].play);
            }
            auto fmtTime = [](double s) {
                int h = (int)(s/3600), m = ((int)s % 3600)/60;
                return std::string(TextFormat("%dh %dm", h, m));
            };

            // --- input ---
            int nOpt = (mpage == MPage::Main) ? 4 : (mpage == MPage::Diff) ? 3 :
                       (mpage == MPage::Slot || mpage == MPage::Load) ? 3 : 0;
            if (nOpt > 0) {
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) msel = (msel + 1) % nOpt;
                if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) msel = (msel + nOpt - 1) % nOpt;
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                if (mpage == MPage::Main) { /* rămâi */ }
                else if (mpage == MPage::Slot) { mpage = MPage::Diff; msel = newDiff; }
                else { mpage = MPage::Main; msel = 0; }
            }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (mpage == MPage::Main) {
                    if (msel == 0)      { mpage = MPage::Diff; msel = 1; }
                    else if (msel == 1) { mpage = MPage::Load; msel = 0; }
                    else if (msel == 2) { mpage = MPage::Help; }
                    else                { break; }     // Iesire
                }
                else if (mpage == MPage::Diff) { newDiff = msel; mpage = MPage::Slot; msel = 0; }
                else if (mpage == MPage::Slot) { startNewGame(msel + 1, newDiff); }
                else if (mpage == MPage::Load) { if (slots[msel].used) loadGame(msel + 1); }
            }

            // --- desen ---
            BeginDrawing();
            ClearBackground(Color{ 56, 120, 70, 255 });
            // floarea-soarelui mică, în colțul de sus
            float cx = screenW / 2.0f, cy = 96.0f, sway = sinf(menuT * 1.5f) * 5.0f;
            DrawRectangle((int)cx - 3, (int)cy, 6, 70, Color{ 70, 130, 60, 255 });
            for (int i = 0; i < 14; i++) {
                float a = i / 14.0f * 2 * PI + menuT * 0.3f;
                DrawPoly(Vector2{ cx + sway + cosf(a)*34, cy + sinf(a)*34 }, 3, 16, a*RAD2DEG, Color{ 255,205,70,255 });
            }
            DrawCircle((int)(cx+sway), (int)cy, 24, Color{ 235,175,55,255 });
            DrawCircle((int)(cx+sway), (int)cy, 19, Color{ 90,55,30,255 });
            fx.Draw();

            const char* title = "Gradina Florii-Soarelui";
            int tw = MeasureText(title, 40);
            DrawText(title, screenW/2 - tw/2 + 2, 150, 40, Color{ 0,0,0,120 });
            DrawText(title, screenW/2 - tw/2, 148, 40, Color{ 255, 235, 150, 255 });

            auto drawList = [&](const char** items, const char** subs, int n, int top) {
                for (int i = 0; i < n; i++) {
                    int ry = top + i * 56;
                    bool sel = (i == msel);
                    int bw = 460, bx = screenW/2 - bw/2;
                    DrawRectangle(bx, ry, bw, 48, sel ? Color{ 70,55,35,235 } : Color{ 30,40,30,200 });
                    DrawRectangleLinesEx(Rectangle{ (float)bx,(float)ry,(float)bw,48 }, sel ? 3 : 1,
                                         sel ? Color{ 255,220,90,255 } : Color{ 110,120,100,255 });
                    DrawText(items[i], bx + 18, ry + (subs ? 5 : 13), 24,
                             sel ? Color{ 255,235,150,255 } : WHITE);
                    if (subs && subs[i][0]) DrawText(subs[i], bx + 18, ry + 30, 13, Color{ 190,205,185,235 });
                }
            };

            if (mpage == MPage::Main) {
                const char* it[4] = { "Joc nou", "Incarca", "Ajutor", "Iesire" };
                drawList(it, nullptr, 4, 230);
                DrawText("Sus/Jos alege   |   ENTER confirma", screenW/2 - 150, screenH - 36, 16, Color{ 210,220,200,220 });
            }
            else if (mpage == MPage::Diff) {
                DrawText("Alege dificultatea", screenW/2 - MeasureText("Alege dificultatea",24)/2, 205, 24, Color{255,235,150,255});
                const char* it[3]; const char* sb[3];
                for (int i = 0; i < 3; i++) { it[i] = Inventory::DifficultyName(i); sb[i] = Inventory::DifficultyDesc(i); }
                drawList(it, sb, 3, 240);
                DrawText("ESC inapoi", screenW/2 - 50, screenH - 36, 16, Color{ 210,220,200,220 });
            }
            else if (mpage == MPage::Slot || mpage == MPage::Load) {
                const char* hdr = (mpage == MPage::Slot) ? "Alege slotul pentru jocul nou" : "Incarca o salvare";
                DrawText(hdr, screenW/2 - MeasureText(hdr,24)/2, 205, 24, Color{255,235,150,255});
                std::string lbl[3], sub[3];
                for (int i = 0; i < 3; i++) {
                    lbl[i] = TextFormat("Slot %d", i+1);
                    if (slots[i].used)
                        sub[i] = TextFormat("%s  -  Nivel %d  -  %d bani  -  %s",
                            Inventory::DifficultyName(slots[i].diff), slots[i].level, slots[i].money,
                            fmtTime(slots[i].play).c_str());
                    else
                        sub[i] = (mpage == MPage::Load) ? "gol" : "liber";
                }
                const char* it[3] = { lbl[0].c_str(), lbl[1].c_str(), lbl[2].c_str() };
                const char* sb[3] = { sub[0].c_str(), sub[1].c_str(), sub[2].c_str() };
                drawList(it, sb, 3, 240);
                if (mpage == MPage::Slot && slots[msel].used)
                    DrawText("ATENTIE: slotul are deja o salvare, va fi suprascrisa!",
                             screenW/2 - 230, screenH - 60, 16, Color{ 255,150,120,255 });
                DrawText("ESC inapoi", screenW/2 - 50, screenH - 36, 16, Color{ 210,220,200,220 });
            }
            else if (mpage == MPage::Help) {
                const char* lines[] = {
                    "CUM SE JOACA",
                    "Misca-te: WASD / sageti",
                    "Tinteste cu mausul un patrat din apropiere; click stanga lucreaza",
                    "(sapa tinand apasat, planteaza, uda, recolteaza).",
                    "Q / rotita: schimba samanta.   TAB: meniul din joc (magazin, amintiri).",
                    "Uda florile ca sa creasca. Cresc si cat esti plecat (idle).",
                    "L (nivel 3): cumpara teren.   E: market / depozit / power-up / amintiri.",
                    "",
                    "Scop: fa bani din flori tot mai rare, pana la Floarea-Soarelui.",
                };
                int n = sizeof(lines)/sizeof(lines[0]);
                int bx = screenW/2 - 320;
                DrawRectangle(bx - 16, 205, 672, n*26 + 24, Color{ 25,35,25,220 });
                for (int i = 0; i < n; i++)
                    DrawText(lines[i], bx, 218 + i*26, (i==0)?20:16,
                             (i==0)?Color{255,220,90,255}:Color{215,220,205,255});
                DrawText("ESC inapoi", screenW/2 - 50, screenH - 36, 16, Color{ 210,220,200,220 });
            }
            EndDrawing();
            if (shotMenu) {
                if (++shotFrame == 20) TakeScreenshot("shot.png");
                if (shotFrame >= 21) break;
            }
            continue;
        }

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

        // locuri-amintire: E deschide bilețelul, ESC/E îl închide
        int nearMemory = memories.NearUnlocked(player.position, inventory.level);
        if (memories.NoteOpen()) {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_E)) memories.CloseNote();
        } else if (!frozen && !landMode && !storageOpen && nearChest == false &&
                   nearPowerup < 0 && nearMemory >= 0 && IsKeyPressed(KEY_E)) {
            memories.OpenNote(nearMemory);
        }

        // modalul de level-up: pauză până apeși o tastă
        if (inventory.levelUpPending &&
            (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE)))
            inventory.levelUpPending = false;

        bool blockGameplay = frozen || landMode || storageOpen ||
                             memories.NoteOpen() || inventory.levelUpPending;

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
                int theme = map.PlotTheme(hpc, hpr);
                world.PopulatePlot(hpc, hpr, theme);            // surpriza: resurse pe parcelă
                inventory.AddXP(15);
                // recompensă imediată în funcție de temă
                const char* rw;
                switch (theme) {
                    case 1:  inventory.crystals += 3; rw = "+3 cristale"; break;
                    case 2:  inventory.wood += 8;     rw = "+8 lemn"; break;
                    case 3:  inventory.seeds[13]++; inventory.seeds[14]++;
                             rw = "+1 samanta Copac de mere, +1 Copac de prune"; break;
                    case 4:  inventory.crystals += 6; rw = "+6 cristale"; break;
                    default: inventory.wood += 5;     rw = "+5 lemn"; break;
                }
                toastMsg = TextFormat("Teren nou (%s)!  %s  +15 XP", TileMap::ThemeName(theme), rw);
                toastTimer = 240;
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
            farm.SetGrowMul(inventory.GrowMul());                        // ritm după dificultate
            farm.Update(dt, inventory.BuffActive(1), inventory.BuffActive(2));
            world.Update(dt);
            inventory.TickTime(dt);
            inventory.UpdateBuffs(dt);

            // B: intră / ciclează modul construire (dacă ai materiale cumpărate)
            if (IsKeyPressed(KEY_B)) {
                if (inventory.buildSel == 0) {
                    if (inventory.roadCount > 0) inventory.buildSel = 1;
                    else if (inventory.stoneCount > 0) inventory.buildSel = 2;
                } else if (inventory.buildSel == 1 && inventory.stoneCount > 0) inventory.buildSel = 2;
                else inventory.buildSel = 0;
            }

            if (nearMarket && IsKeyPressed(KEY_E)) {           // intră în market
                worldReturnPos = player.position;
                market.Enter(player);
                scene = Scene::Market;
                digTx = -1; digProgress = 0;
            }
            else if (inventory.buildSel != 0) {                // MOD CONSTRUIRE
                if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) inventory.buildSel = 0;
                else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && inRange) {
                    if (inventory.buildSel == 1 && inventory.roadCount > 0) {
                        map.Place(tx, ty, Terrain::Dirt);  inventory.roadCount--;
                    } else if (inventory.buildSel == 2 && inventory.stoneCount > 0) {
                        map.Place(tx, ty, Terrain::Wall);  inventory.stoneCount--;
                    }
                }
                digTx = -1; digProgress = 0;
            }
            else if (inRange) {
                bool nodeHere = world.HasNode(tx, ty);
                bool grassDig = map.CanFarm(tx, ty) && farm.Diggable(tx, ty);
                if (nodeHere) {                                // copac/cristal: click = taie/minează
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !player.IsBusy()) {
                        player.FaceTo(tileCenter);
                        int bw = inventory.wood, bc = inventory.crystals;
                        if (world.Interact(tx, ty, inventory, player) == 1) {
                            if (inventory.wood > bw) fx.WoodChips(tileCenter);
                            else if (inventory.crystals > bc)
                                fx.Burst(tileCenter, 14, Color{ 150,220,240,255 }, 120, 0.7f, 200, 3.5f, 2);
                            else fx.Burst(tileCenter, 6, Color{ 210,210,210,255 }, 90, 0.45f, 220, 2.5f, 1);
                        }
                    }
                    digTx = -1; digProgress = 0;
                }
                else if (grassDig && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    player.FaceTo(tileCenter);                 // ții apăsat ca să sapi
                    if (tx == digTx && ty == digTy) digProgress += dt;
                    else { digTx = tx; digTy = ty; digProgress = 0; }
                    if (!player.IsBusy()) player.StartAction(Action::Hoe);
                    if (digProgress >= DigTime) {
                        farm.Till(tx, ty); ambient.ClearDecoTile(tx, ty);
                        fx.Dirt(tileCenter); digTx = -1; digProgress = 0;
                    }
                }
                else {                                         // pământ/cultură: click = plantează/udă/recoltează
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !player.IsBusy()) {
                        player.FaceTo(tileCenter);
                        int ev = farm.Interact(tx, ty, inventory, player);
                        if (ev == 1) fx.Plant(tileCenter);
                        else if (ev == 2) fx.Water(tileCenter);
                        else if (ev == 3) { fx.Petals(tileCenter, Color{ 255,200,120,255 }); fx.Coins(tileCenter); }
                        else if (ev == 4) { fx.WoodChips(tileCenter); fx.Coins(tileCenter); }
                        else if (ev == 6) fx.Burst(tileCenter, 12, Color{ 120,115,110,255 }, 70, 0.6f, 160, 3.0f, 1);
                    }
                    digTx = -1; digProgress = 0;
                }
            }
            else { digTx = -1; digProgress = 0; }
        }

        // particule, amintiri și efecte — actualizate mereu (chiar și cu meniul deschis)
        fx.Update(dt);
        ambient.Update(dt);
        memories.Update(dt, inventory.level, fx);
        if (inventory.level > prevLevel) {       // sărbătoare la level-up
            fx.Stars({ player.position.x, player.position.y - 24 });
            fx.Hearts({ player.position.x, player.position.y - 10 }, 6);
            prevLevel = inventory.level;
        }
        if (toastTimer > 0) toastTimer--;
        if (welcomeTimer > 0) welcomeTimer--;

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
            if (!savePath.empty())
                SaveIO::Save(savePath.c_str(), inventory, shop, farm, map, memories, player.position);
        }

        if (shotMode) { camera.target = player.position; camera.zoom = 1.05f; }

        BeginDrawing();
        ClearBackground(Color{ 40, 90, 50, 255 });

        BeginMode2D(camera);
        map.Draw(camera);
        DrawRectangleRec(dunRect, Color{ 12, 14, 40, 110 });

        farm.DrawGround(camera);
        ambient.DrawGroundDecor();                 // flori decorative (sub jucător)
        if (!blockGameplay && inventory.buildSel == 0) {
            int hax, hay, hsize;
            farm.TargetArea(tx, ty, inventory, hax, hay, hsize);
            farm.DrawTargetHighlight(hax, hay, hsize, inRange);
        } else if (!blockGameplay) {
            farm.DrawTargetHighlight(tx, ty, 1, inRange);   // mod construire: 1x1
        }
        // bara de progres la săpat
        if (digTx >= 0 && digProgress > 0) {
            float frac = digProgress / DigTime;
            int bx = digTx*TS + 2, byy = digTy*TS - 8;
            DrawRectangle(bx, byy, TS - 4, 6, Color{ 0,0,0,180 });
            DrawRectangle(bx, byy, (int)((TS - 4) * frac), 6, Color{ 220, 180, 90, 255 });
        }

        // Y-sorting: panoul de market + copaci/cristale + jucător
        float pFeet = player.position.y + 22;
        auto drawSign = [&](){
            float h = 64, w = marketSign.width * (h / marketSign.height);
            DrawTexturePro(marketSign, { 0,0,(float)marketSign.width,(float)marketSign.height },
                { marketSpot.x, marketSpot.y, w, h }, { w/2, h }, 0, WHITE);
        };
        auto drawChest = [&](){   // CASA (depozit de seminte) — sprite bottom-anchored
            DrawTexturePro(chestTex, { 0,0,(float)chestTex.width,(float)chestTex.height },
                { chestSpot.x + 36, chestSpot.y + 18, 104, 138 }, { 52, 130 }, 0, WHITE);
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
        if (!landMode) ambient.DrawCritters();     // albine/fluturi (deasupra)

        // locurile-amintire (inimi pe stâlpi)
        if (!landMode) memories.Draw(player.position, inventory.level);
        // particule peste lume
        fx.Draw();

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
        if (nearMemory >= 0 && !blockGameplay) {
            const char* m = "[E] Aminteste-ti...";
            int w = MeasureText(m, 22);
            DrawRectangle(screenW/2 - w/2 - 12, 60, w + 24, 34, Color{ 0,0,0,170 });
            DrawText(m, screenW/2 - w/2, 66, 22, Color{ 255, 190, 210, 255 });
        }
        // buff-uri active (sub resurse, stânga)
        {
            static const char* bn[5] = { "Viteza", "Auto-udare", "Crestere rapida", "Bonus bani", "Bonus XP" };
            int by = 90;
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
        shop.Draw(inventory, flowerTex, iconTex, memories);

        // bilețelul de amintire (peste tot)
        memories.DrawNote();

        // popup "bine ai revenit" (progres offline) — centru
        if (welcomeTimer > 0 && !welcomeMsg.empty()) {
            int bw = 520, bh = 150;
            int bx = screenW/2 - bw/2, by = 70;
            DrawRectangle(bx, by, bw, bh, Color{ 30, 45, 30, 235 });
            DrawRectangleLinesEx(Rectangle{ (float)bx,(float)by,(float)bw,(float)bh }, 3, Color{ 255,210,120,255 });
            DrawText("Bine ai revenit!", bx + 20, by + 14, 24, Color{ 255, 230, 150, 255 });
            // liniile mesajului (sărim peste prima, care e titlul)
            const char* msg = welcomeMsg.c_str();
            int ly = by + 52;
            const char* p = msg;
            char line[256]; int li = 0;
            for (;; p++) {
                if (*p == '\n' || *p == 0) {
                    line[li] = 0;
                    DrawText(line, bx + 20, ly, 18, Color{ 220, 235, 210, 255 }); ly += 26;
                    li = 0;
                    if (*p == 0) break;
                } else if (li < 255) line[li++] = *p;
            }
        }

        // MODAL DE LEVEL-UP (pauză): ce ai deblocat + cât ai realizat până acum
        if (inventory.levelUpPending) {
            DrawRectangle(0, 0, screenW, screenH, Color{ 0, 0, 0, 170 });
            int pw = 600, ph = 310, px = screenW/2 - pw/2, py = screenH/2 - ph/2;
            DrawRectangle(px, py, pw, ph, Color{ 30, 55, 32, 245 });
            DrawRectangleLinesEx(Rectangle{ (float)px,(float)py,(float)pw,(float)ph }, 3, Color{ 255,220,90,255 });
            DrawRectangle(px, py, pw, 6, Color{ 255,220,90,255 });

            const char* t = TextFormat("NIVEL %d!", inventory.level);
            DrawText(t, screenW/2 - MeasureText(t, 40)/2, py + 18, 40, Color{ 255, 235, 150, 255 });

            // ce ai deblocat (mesajul construit la AddXP)
            DrawText("Ai deblocat:", px + 28, py + 74, 18, Color{ 200, 230, 200, 255 });
            DrawText(inventory.levelUpMsg.c_str(), px + 28, py + 98, 17, Color{ 255, 245, 200, 255 });

            DrawLine(px + 24, py + 132, px + pw - 24, py + 132, Color{ 120, 140, 110, 255 });
            DrawText("Cat ai realizat pana acum:", px + 28, py + 142, 18, Color{ 200, 230, 200, 255 });

            int ps = (int)inventory.playSeconds;
            const char* stats[5] = {
                TextFormat("Dificultate:        %s", inventory.DifficultyName()),
                TextFormat("Timp in joc:        %dh %02dm", ps/3600, (ps%3600)/60),
                TextFormat("Flori in gradina:   %d", farm.CropCount()),
                TextFormat("Nivel:              %d  (XP %d/%d)", inventory.level, inventory.xp, inventory.XPForNext()),
                TextFormat("Rating gradina:     %d", farm.GardenRating()),
            };
            for (int i = 0; i < 5; i++)
                DrawText(stats[i], px + 40, py + 172 + i*24, 18,
                         (i==4) ? Color{255,210,120,255} : Color{ 225, 230, 215, 255 });

            const char* hint = "ENTER - continua";
            DrawText(hint, screenW/2 - MeasureText(hint,18)/2, py + ph - 30, 18, Color{ 200,210,190,230 });
        }

        // toast (recompense teren etc.) — jos-centru
        if (toastTimer > 0 && !toastMsg.empty()) {
            int w = MeasureText(toastMsg.c_str(), 18);
            DrawRectangle(screenW/2 - w/2 - 14, screenH - 96, w + 28, 32, Color{ 25, 40, 25, 220 });
            DrawRectangleLinesEx(Rectangle{ (float)(screenW/2 - w/2 - 14), (float)(screenH - 96), (float)(w + 28), 32 },
                                 2, Color{ 160, 230, 140, 255 });
            DrawText(toastMsg.c_str(), screenW/2 - w/2, screenH - 89, 18, Color{ 210, 245, 200, 255 });
        }

        EndDrawing();

        if (shotMode) {
            if (++shotFrame == 20) TakeScreenshot("shot.png");
            if (shotFrame >= 21) break;
        }
    }

    // la ieșire salvează poziția din LUME (nu cea din scena de market)
    if (!shotMode && !savePath.empty())
        SaveIO::Save(savePath.c_str(), inventory, shop, farm, map, memories,
                     (scene == Scene::Market) ? worldReturnPos : player.position);

    ambient.Unload();
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
