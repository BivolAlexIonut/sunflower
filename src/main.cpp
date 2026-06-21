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
#include "sudoku.h"
#include <vector>
#include <cmath>
#include <string>

enum class Scene { Menu, World, Market, Sudoku };
enum class MPage { Main, Diff, Slot, Load, Help };   // paginile meniului principal

// material de construcție (index) → terenul pus pe hartă
static Terrain BuildTerrainFor(int mat) {
    switch (mat) {
        case 0: return Terrain::Dirt;   case 1: return Terrain::Wall;
        case 2: return Terrain::Fence;  case 3: return Terrain::Water;
        case 4: return Terrain::Sand;   default: return Terrain::Dirt;
    }
}

int main(int argc, char** argv) {
    std::string arg1 = (argc > 1) ? argv[1] : "";
    bool shotMenu = (arg1 == "--shotmenu");
    bool shotMode = (arg1 == "--shot") || shotMenu;
    bool makeSave = (arg1 == "--makesave");
    int  shotShopTab = -1;
    if (arg1 == "--shopupg")  shotShopTab = 1;   // verifică fila UNELTE/upgrade
    if (arg1 == "--shopanim") shotShopTab = 6;   // verifică fila ANIMALE
    bool shotShop = (shotShopTab >= 0);
    bool shotBag = (arg1 == "--shotbag");
    bool shotSudoku = (arg1 == "--shotsudoku");
    bool shotSell = (arg1 == "--shotsell");
    shotMode = shotMode || shotShop || shotBag || shotSudoku || shotSell;
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
    Texture2D flowerTex[5] = {
        LoadTexture("sprites/Objects/FG_Grass_Summer.png"),
        LoadTexture("sprites/Objects/FG_Grass_Winter.png"),
        LoadTexture("sprites/Objects/FG_Grass_Fall.png"),
        LoadTexture("sprites/Plants&supplies/Plants.png"),
        LoadTexture("sprites/NewAssets/Crops/Crops.png"),
    };

    Market market;     market.Load();
    Particles fx;                       // particule (apă, petale, monede, stele, inimi)
    Memories memories;                  // momentele relației (locuri + bilețele)
    Ambient ambient;   ambient.Load();  // albine/fluturi + flori decorative
    Sudoku  sudoku;                     // minigame la han (sudoku cu flori)
    Vector2 innSpot = { 65 * TS + 16.0f, 44 * TS + 0.0f };   // intrarea în han (oraș)

    // NEGUSTORI (NPC-uri) lângă tarabe — personaje cu cadru frontal clar (Walk_Down, 4 cadre 32x32)
    Texture2D npcTex[3] = {
        LoadTexture("sprites/Characters/Character05/Character05_Walk_Down.png"),
        LoadTexture("sprites/Characters/Character10/Character10_Walk_Down.png"),
        LoadTexture("sprites/Characters/Knight_11/Knight_11_Walk_Down.png"),
    };
    Vector2 npcPos[3] = {
        { 72 * TS + 16.0f, 46 * TS + 8.0f },
        { 75 * TS + 16.0f, 45 * TS + 8.0f },
        { 79 * TS + 16.0f, 46 * TS + 8.0f },
    };
    bool sellOpen = false;
    Scene scene = Scene::Menu;
    MPage mpage = MPage::Main;
    int  msel = 0;            // selecție în meniu
    int  newDiff = 1;         // dificultatea aleasă la joc nou
    bool bagOpen = false;     // inventarul (tasta I)
    int  dragFlower = -1;     // floarea trasă cu mausul în inventar
    Vector2 worldReturnPos = player.position;

    // mesaj scurt (toast) pentru recompense la cumpărat teren etc.
    std::string toastMsg; int toastTimer = 0;
    // popup "bine ai revenit" (progres offline)
    std::string welcomeMsg; int welcomeTimer = 0;
    int prevLevel = inventory.level;    // pentru a detecta level-up (stele)
    int prevAnimTotal = 0;              // pentru a re-sincroniza animalele când cumperi
    float rainTimer = 0.0f;             // secunde de ploaie rămase
    float rainCheckTimer = 0.0f;        // cât până la următoarea verificare de ploaie aleatorie

    Texture2D chestTex = LoadTexture("sprites/NewAssets/Buildings/Buildings/Houses/Wood/House_1_Wood_Base_Red.png");
    Texture2D suppliesTex = LoadTexture("sprites/Plants&supplies/Supplies.png");

    // intrarea în market: un panou lângă cărare (în pădure, sub drum)
    Vector2 marketSpot = { 7 * TS + 16.0f, 18 * TS + 0.0f };
    // casa/depozitul de semințe, chiar sub grădină (curtea)
    Vector2 chestSpot = { 30 * TS + 0.0f, 23 * TS + 0.0f };
    bool storageOpen = false;
    int  storageRow = 0;

    // power-up-uri (lăzi cu provizii) lângă casă: fiecare dă un buff de 5 min
    struct PowerUp { Vector2 pos; Rectangle icon; int buff; int cost; const char* name; };
    PowerUp powerups[] = {
        { { 13*TS+16.0f, 24*TS+0.0f }, {   0, 2, 30, 42 }, 0,  60, "Viteza"          },
        { { 16*TS+16.0f, 24*TS+0.0f }, {  32, 2, 30, 42 }, 1, 130, "Auto-udare"      },
        { { 19*TS+16.0f, 24*TS+0.0f }, {  64, 2, 30, 42 }, 2, 110, "Crestere rapida" },
        { { 22*TS+16.0f, 24*TS+0.0f }, { 192, 2, 30, 42 }, 3, 150, "Bonus bani"      },
        { { 25*TS+16.0f, 24*TS+0.0f }, { 224, 2, 30, 42 }, 4, 100, "Bonus XP"        },
        { { 28*TS+16.0f, 24*TS+0.0f }, { 128, 2, 30, 42 }, 5, 250, "Ploaie 10min"   },
    };
    const int powerupCount = 6;
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
        // animalele au produs alimente cât ai fost plecată
        int foodTotal = 0;
        for (int i = 0; i < Inventory::FoodCount; i++) {
            int produced = (int)(inventory.animals[i] * Inventory::AnimalFoodPerMin(i) * (capped / 60.0));
            inventory.food[i] += produced; foodTotal += produced;
        }
        int hh = (int)(offlineSeconds / 3600), mm = ((int)offlineSeconds % 3600) / 60;
        std::string m = TextFormat("Ai lipsit %dh %dm.\nPlantele udate au crescut %d stadii.\n", hh, mm, grown);
        if (died > 0) m += TextFormat("%d plante au murit de sete!\n", died);
        m += TextFormat("Albinele au strans %d bani.", bees);
        if (foodTotal > 0) m += TextFormat("\nAnimalele au produs %d alimente (vinde-le in oras).", foodTotal);
        welcomeMsg = m;
        welcomeTimer = 600;   // ~10s
    };

    auto startNewGame = [&](int slot, int diff) {
        savePath = slotPath(slot);
        inventory = Inventory{};
        inventory.difficulty = diff;
        inventory.money = inventory.StartMoney();
        inventory.unlocked[15] = true;   // Grau — legumă de start, ca ferma să pornească imediat
        inventory.seeds[15] = 5;
        inventory.hotbar[1] = 15;        // grâul în al doilea slot din hotbar
        inventory.SyncSelected();
        memories = Memories{};
        shop = Shop{};
        farm.Reset();
        farm.SetGrowMul(inventory.GrowMul());
        map.Build();
        world.Generate(map);
        world.PopulateOwnedPlots(map);
        ambient.Init(map); ambient.SyncAnimals(inventory.animals);
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
        ambient.Init(map); ambient.SyncAnimals(inventory.animals);
        prevLevel = inventory.level; welcomeTimer = 0; fx.Clear();
        applyOffline(off);
        scene = Scene::World;
        return true;
    };

    if (shotMode && !shotMenu) {
        startNewGame(1, 1);
        for (int pr = 0; pr < TileMap::PlotRows; pr++)          // dezvăluie toată harta
            for (int pc = 0; pc < TileMap::PlotCols; pc++) map.BuyPlot(pc, pr);
        world.Generate(map); world.PopulateOwnedPlots(map);
        for (int i = 0; i < 4; i++) inventory.animals[i] = 3;
        ambient.SyncAnimals(inventory.animals);
        // demo: o grădiniță împrejmuită cu gard construit + nisip/apă (verifică auto-îmbinarea)
        for (int xx = 4; xx <= 11; xx++) { map.Place(xx, 40, Terrain::Fence); map.Place(xx, 46, Terrain::Fence); }
        for (int yy = 40; yy <= 46; yy++) { map.Place(4, yy, Terrain::Fence); map.Place(11, yy, Terrain::Fence); }
        map.Place(7, 40, Terrain::Grass); map.Place(8, 40, Terrain::Grass);   // poartă
        map.Place(6, 42, Terrain::Sand); map.Place(9, 44, Terrain::Water);
        player.position = { 7*TS + 16.0f, 43*TS + 16.0f };
        if (shotShop) {                                         // verifică o filă din meniu
            inventory.money = 99999; inventory.hasAxe = true; inventory.hasPickaxe = true;
            inventory.upg[0] = 2; inventory.upg[2] = 1; inventory.upg[3] = 1;
            shop.DebugOpen(shotShopTab);
        }
        if (shotBag) {                                          // verifică inventarul
            for (int f = 0; f < 14; f++) { inventory.unlocked[f] = true; inventory.seeds[f] = 9; }
            bagOpen = true;
        }
        if (shotSudoku) { sudoku.DebugStart(1); scene = Scene::Sudoku; }
        if (shotSell) {
            for (int i = 0; i < Inventory::FoodCount; i++) inventory.food[i] = 12 + i*7;
            inventory.harvested[0] = 8; inventory.harvested[15] = 14; inventory.harvested[12] = 3; inventory.harvested[17] = 20;
            sellOpen = true;
        }
    }

    // --makesave: creează o salvare cu TOTUL maxat în slotul 1 (pentru testare ușoară), apoi iese
    if (makeSave) {
        startNewGame(0, 0);                      // dificultate Gradinar (blând)
        inventory.level = 30; inventory.xp = 0;
        inventory.money = 999999;
        for (int i = 0; i < (int)Flower::COUNT; i++) { inventory.unlocked[i] = true; inventory.seeds[i] = 20; }
        inventory.hasAxe = inventory.hasPickaxe = true;
        for (int i = 0; i < Inventory::UpgCount; i++)    inventory.upg[i] = Inventory::UpgMax;
        for (int i = 0; i < Inventory::AnimalCount; i++) inventory.animals[i] = 5;
        for (int i = 0; i < Inventory::FoodCount; i++) inventory.food[i] = 20;   // alimente de vândut
        int hb[6] = { 0, 15, 16, 17, 12, 13 };           // hotbar variat (floare, grâu, legume, soare, copac)
        for (int i = 0; i < 6; i++) inventory.hotbar[i] = hb[i];
        inventory.selectedSlot = 0; inventory.SyncSelected();
        inventory.wood = 999; inventory.crystals = 999;
        for (int i = 0; i < Inventory::BuildMatCount; i++) inventory.buildMat[i] = 99;
        for (int pr = 0; pr < TileMap::PlotRows; pr++)
            for (int pc = 0; pc < TileMap::PlotCols; pc++) map.BuyPlot(pc, pr);
        world.Generate(map); world.PopulateOwnedPlots(map);
        SaveIO::Save("save1.txt", inventory, shop, farm, map, memories,
                     Vector2{ 24 * TS + 16.0f, 12 * TS + 16.0f });
        CloseWindow();
        return 0;
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

        // ===== SCENA SUDOKU (minigame la han) =====
        if (scene == Scene::Sudoku) {
            if (sudoku.Update()) { scene = Scene::World; player.position = worldReturnPos; }
            if (sudoku.TakeWinReward()) inventory.money += 500;
            BeginDrawing();
            sudoku.Draw(flowerTex);
            EndDrawing();
            if (shotMode) { if (++shotFrame == 20) TakeScreenshot("shot.png"); if (shotFrame >= 21) break; }
            continue;
        }

        // ===== SCENA LUMII (top-down) =====
        shop.SetAnimalsUnlocked(map.PenOwned());   // animalele necesită țarcul cumpărat
        shop.HandleInput(inventory, player);
        bool frozen = shop.BlocksGameplay();

        // PLOAIE: aleatorie din când în când; udă plantele și le grăbește mult creșterea
        if (rainTimer > 0) rainTimer -= dt;
        else {
            rainCheckTimer += dt;
            if (rainCheckTimer >= 150.0f) {        // verifică la ~2.5 min
                rainCheckTimer = 0.0f;
                if (GetRandomValue(0, 100) < 35) rainTimer = (float)GetRandomValue(70, 130);
            }
        }
        bool raining = rainTimer > 0;

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
                if (IsKeyPressed(KEY_ENTER)) { inventory.SelectFlower(owned[storageRow]); storageOpen = false; }
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
                if (powerups[nearPowerup].buff == 5) rainTimer = 600.0f;   // ploaie 10 min
                else inventory.AddBuff(powerups[nearPowerup].buff, BuffDuration);
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

        // inventarul (I): deschide/închide
        if (!frozen && !landMode && !storageOpen && !memories.NoteOpen() &&
            !inventory.levelUpPending && IsKeyPressed(KEY_I))
            bagOpen = !bagOpen;
        if (bagOpen && IsKeyPressed(KEY_ESCAPE)) { bagOpen = false; dragFlower = -1; }
        if (sellOpen && IsKeyPressed(KEY_ESCAPE)) sellOpen = false;

        bool blockGameplay = frozen || landMode || storageOpen || memories.NoteOpen() ||
                             inventory.levelUpPending || bagOpen || sellOpen;

        Vector2 mw = GetScreenToWorld2D(GetMousePosition(), camera);
        int tx = (int)floorf(mw.x / TS), ty = (int)floorf(mw.y / TS);
        Vector2 tileCenter{ tx * TS + TS / 2.0f, ty * TS + TS / 2.0f };
        float ddx = tileCenter.x - player.position.x, ddy = tileCenter.y - player.position.y;
        bool inRange = (ddx*ddx + ddy*ddy) <= reach*reach;

        bool nearMarket = (fabsf(player.position.x - marketSpot.x) < 48 &&
                           fabsf(player.position.y - (marketSpot.y - 24)) < 48);
        bool nearInn = (fabsf(player.position.x - innSpot.x) < 64 &&
                        fabsf(player.position.y - innSpot.y) < 80);
        int nearNpc = -1;
        for (int i = 0; i < 3; i++)
            if (fabsf(player.position.x - npcPos[i].x) < 42 &&
                fabsf(player.position.y - npcPos[i].y) < 52) { nearNpc = i; break; }

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
                world.PopulatePlot(hpc, hpr, theme, map);       // surpriza: resurse pe parcelă
                // ORAȘUL se deblochează tot odată (col 6-8, rândul 5)
                if (hpr == 5 && hpc >= 6)
                    for (int pc = 6; pc <= 8; pc++) if (!map.PlotOwned(pc, 5)) {
                        map.BuyPlot(pc, 5); world.PopulatePlot(pc, 5, map.PlotTheme(pc, 5), map);
                    }
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
            if (IsKeyPressed(KEY_ONE))   inventory.SelectSlot(0);   // selectează slotul din hotbar
            if (IsKeyPressed(KEY_TWO))   inventory.SelectSlot(1);
            if (IsKeyPressed(KEY_THREE)) inventory.SelectSlot(2);
            if (IsKeyPressed(KEY_FOUR))  inventory.SelectSlot(3);
            if (IsKeyPressed(KEY_FIVE))  inventory.SelectSlot(4);
            if (IsKeyPressed(KEY_SIX))   inventory.SelectSlot(5);

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

            player.speed = (inventory.BuffActive(0) ? 200.0f : 130.0f) * inventory.MoveMul();  // viteză + upgrade
            farm.SetGrowMul(inventory.GrowMul() * inventory.GardenMul());   // dificultate + upgrade grădinărit
            farm.Update(dt, inventory.BuffActive(1) || raining, inventory.BuffActive(2) || raining);   // ploaia udă + grăbește
            world.Update(dt);
            inventory.TickTime(dt);
            inventory.UpdateBuffs(dt);

            // B: intră / ciclează modul construire (printre materialele pe care le ai)
            if (IsKeyPressed(KEY_B)) {
                int s = inventory.buildSel;
                do { s = (s + 1) % (Inventory::BuildMatCount + 1); }
                while (s != 0 && inventory.buildMat[s-1] == 0);
                inventory.buildSel = s;
            }

            if (nearMarket && IsKeyPressed(KEY_E)) {           // intră în market
                worldReturnPos = player.position;
                market.Enter(player);
                scene = Scene::Market;
                digTx = -1; digProgress = 0;
            }
            else if (nearInn && IsKeyPressed(KEY_E)) {         // intră la han → SUDOKU
                worldReturnPos = player.position;
                sudoku.Enter();
                scene = Scene::Sudoku;
                digTx = -1; digProgress = 0;
            }
            else if (nearNpc >= 0 && IsKeyPressed(KEY_E)) {    // negustor → vinde alimente/recolte
                sellOpen = true; digTx = -1; digProgress = 0;
            }
            else if (inventory.buildSel != 0) {                // MOD CONSTRUIRE
                if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) inventory.buildSel = 0;
                else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && inRange) {
                    int mat = inventory.buildSel - 1;
                    if (mat >= 0 && mat < Inventory::BuildMatCount && inventory.buildMat[mat] > 0
                        && map.At(tx, ty) != BuildTerrainFor(mat)) {   // nu irosi pe același tile
                        map.Place(tx, ty, BuildTerrainFor(mat));
                        inventory.buildMat[mat]--;
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
                    if (digProgress >= DigTime / inventory.DigSpeedMul()) {   // upgrade Sapă
                        farm.Till(tx, ty); ambient.ClearDecoTile(tx, ty);
                        fx.Dirt(tileCenter); digTx = -1; digProgress = 0;
                    }
                }
                else {                                         // pământ/cultură: click = plantează/udă/recoltează
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !player.IsBusy()) {
                        player.FaceTo(tileCenter);
                        int ev = farm.Interact(tx, ty, inventory, player);
                        if (ev == 1) fx.Plant(tileCenter);
                        else if (ev == 2) {
                            fx.Water(tileCenter);
                            if (inventory.WaterRadius() > 0)        // upgrade Stropitoare: udă o zonă
                                farm.WaterArea(tx, ty, inventory.WaterRadius());
                        }
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
        { int at = 0; for (int i = 0; i < 4; i++) at += inventory.animals[i];   // animale cumpărate
          if (at != prevAnimTotal) { ambient.SyncAnimals(inventory.animals); prevAnimTotal = at; } }
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
        float zoomGoal = landMode ? 0.27f : 1.7f;
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

        if (shotMode) { camera.target = { 7.5f*TS, 43.0f*TS }; camera.zoom = 1.5f; }

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
            float frac = digProgress / (DigTime / inventory.DigSpeedMul());
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
        auto drawNpcs = [&](bool front){   // negustori (cadrul frontal, ca jucătorul)
            for (int i = 0; i < 3; i++) {
                if ((npcPos[i].y > pFeet) != front) continue;
                DrawTexturePro(npcTex[i], { 0,0,32,32 },
                    { npcPos[i].x, npcPos[i].y, 56, 56 }, { 28, 46 }, 0, WHITE);
            }
        };
        world.DrawBehind(pFeet);
        if (!landMode) ambient.DrawAnimals(pFeet, false);    // animale în spatele jucătorului
        if (!landMode) drawNpcs(false);
        if (marketSpot.y <= pFeet) drawSign();
        if (chestSpot.y  <= pFeet) drawChest();
        drawPowerups(false);
        player.Draw();
        if (marketSpot.y > pFeet) drawSign();
        if (chestSpot.y  > pFeet) drawChest();
        drawPowerups(true);
        if (!landMode) drawNpcs(true);
        world.DrawFront(pFeet);
        if (!landMode) ambient.DrawAnimals(pFeet, true);     // animale în fața jucătorului
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

        // PLOAIE (peste lume): voal umed + dâre diagonale
        if (raining && !landMode) {
            DrawRectangle(0, 0, screenW, screenH, Color{ 60, 80, 110, 30 });
            float t = (float)GetTime();
            for (int i = 0; i < 170; i++) {
                float rx = (float)((i * 89) % screenW);
                float ry = fmodf(i * 47 + t * 950.0f, (float)(screenH + 24)) - 12;
                DrawLineEx(Vector2{ rx, ry }, Vector2{ rx - 6, ry + 16 }, 2, Color{ 180, 205, 240, 130 });
            }
        }

        // HUD
        inventory.Draw(flowerTex, iconTex);
        inventory.DrawLevel(farm.CropCount());
        if (raining && !landMode) {
            int s = (int)rainTimer;
            const char* m = TextFormat("Ploaie  %d:%02d", s/60, s%60);
            int mw = MeasureText(m, 18);
            DrawRectangle(screenW/2 - mw/2 - 10, 8, mw + 20, 26, Color{ 30, 45, 70, 200 });
            DrawText(m, screenW/2 - mw/2, 11, 18, Color{ 185, 215, 245, 255 });
        }

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
            int mat = inventory.buildSel - 1;
            const char* bm = TextFormat("CONSTRUIESTI: %s (%d) - tine click pune, B schimba, dreapta iesi",
                                        Inventory::BuildName(mat), inventory.buildMat[mat]);
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
        if (nearInn && !blockGameplay) {
            const char* m = "[E] Joaca Sudoku cu flori";
            int w = MeasureText(m, 22);
            DrawRectangle(screenW/2 - w/2 - 12, 60, w + 24, 34, Color{ 0,0,0,170 });
            DrawText(m, screenW/2 - w/2, 66, 22, Color{ 255, 200, 220, 255 });
        }
        if (nearNpc >= 0 && !blockGameplay) {
            const char* m = "[E] Vinde la negustor";
            int w = MeasureText(m, 22);
            DrawRectangle(screenW/2 - w/2 - 12, 60, w + 24, 34, Color{ 0,0,0,170 });
            DrawText(m, screenW/2 - w/2, 66, 22, Color{ 255, 230, 150, 255 });
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

        // INVENTAR (I) — stil Minecraft: trage cu mausul semințele în hotbar (max 6)
        if (bagOpen) {
            DrawRectangle(0, 0, screenW, screenH, Color{ 0,0,0,175 });
            int pw = 560, ph = 430, px = screenW/2 - pw/2, py = screenH/2 - ph/2;
            DrawRectangle(px, py, pw, ph, Color{ 38,28,20,248 });
            DrawRectangleLinesEx(Rectangle{ (float)px,(float)py,(float)pw,(float)ph }, 3, Color{ 255,220,90,255 });
            DrawText("INVENTAR", px + 16, py + 12, 22, Color{ 255,220,90,255 });
            DrawText("Trage o samanta jos in cele 6 sloturi. I / ESC inchide.", px + 150, py + 18, 14, Color{ 200,200,180,255 });

            Vector2 mp = GetMousePosition();
            bool press = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
            bool release = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
            const int N = (int)Flower::COUNT;

            // grila tuturor florilor deblocate
            int cols = 7, cell = 60, gap = 8, gx = px + 20, gy = py + 48, vis = 0;
            for (int f = 0; f < N; f++) {
                if (!inventory.unlocked[f]) continue;
                int c = vis % cols, r = vis / cols; vis++;
                float cx = gx + c*(cell+gap), cy = gy + r*(cell+gap);
                Rectangle cr{ cx, cy, (float)cell, (float)cell };
                DrawRectangle((int)cx, (int)cy, cell, cell, Color{ 46,34,22,230 });
                DrawRectangleLinesEx(cr, 1, Color{ 110,90,60,255 });
                const FlowerInfo& fi = FLOWERS[f];
                DrawTexturePro(flowerTex[fi.tex], fi.r2, Rectangle{ cx+10, cy+6, 40, 40 }, {0,0}, 0, WHITE);
                DrawText(TextFormat("%d", inventory.seeds[f]), (int)cx+4, (int)cy+cell-16, 13, WHITE);
                if (press && CheckCollisionPointRec(mp, cr)) dragFlower = f;
            }

            // hotbar din panou (jos)
            int hbN = Inventory::HotbarSize, hsl = 56, hgap = 6;
            int hbTotal = hbN*(hsl+hgap)-hgap, hbX = px + pw/2 - hbTotal/2, hbY = py + ph - 74;
            for (int s = 0; s < hbN; s++) {
                float hx = hbX + s*(hsl+hgap);
                Rectangle hr{ hx, (float)hbY, (float)hsl, (float)hsl };
                bool sel = (s == inventory.selectedSlot);
                DrawRectangle((int)hx, hbY, hsl, hsl, Color{ 52,40,28,235 });
                DrawRectangleLinesEx(hr, sel?3.0f:2.0f, sel?Color{255,220,90,255}:Color{130,100,70,255});
                DrawText(TextFormat("%d", s+1), (int)hx+3, hbY+2, 12, Color{ 150,140,120,255 });
                int f = inventory.hotbar[s];
                if (f >= 0 && f < N)
                    DrawTexturePro(flowerTex[FLOWERS[f].tex], FLOWERS[f].r2, Rectangle{ hx+8, (float)hbY+8, 40, 40 }, {0,0}, 0, WHITE);
                if (press && CheckCollisionPointRec(mp, hr) && f >= 0) { dragFlower = f; inventory.hotbar[s] = -1; }
                if (release && dragFlower >= 0 && CheckCollisionPointRec(mp, hr)) {
                    for (int k = 0; k < hbN; k++) if (inventory.hotbar[k] == dragFlower) inventory.hotbar[k] = -1;
                    inventory.hotbar[s] = dragFlower; dragFlower = -1; inventory.EnsureValidSeed();
                }
            }
            if (release && dragFlower >= 0) { dragFlower = -1; inventory.EnsureValidSeed(); }   // aruncat afară

            if (dragFlower >= 0 && dragFlower < N)   // icoana trasă, la maus
                DrawTexturePro(flowerTex[FLOWERS[dragFlower].tex], FLOWERS[dragFlower].r2,
                    Rectangle{ mp.x-20, mp.y-20, 40, 40 }, {0,0}, 0, WHITE);
        }

        // NEGUSTOR — vinde alimente (de la animale) + recolte (flori/legume)
        if (sellOpen) {
            DrawRectangle(0, 0, screenW, screenH, Color{ 0,0,0,175 });
            int pw = 620, ph = 440, px = screenW/2 - pw/2, py = screenH/2 - ph/2;
            DrawRectangle(px, py, pw, ph, Color{ 38,28,20,248 });
            DrawRectangleLinesEx(Rectangle{ (float)px,(float)py,(float)pw,(float)ph }, 3, Color{ 255,220,90,255 });
            DrawText("NEGUSTOR", px + 16, py + 12, 24, Color{ 255,220,90,255 });
            DrawText("Click pe un produs ca sa-l vinzi.  ESC inchide.", px + 200, py + 18, 14, Color{ 200,200,180,255 });

            Vector2 sm = GetMousePosition();
            bool sclick = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
            long long totalValue = 0;

            // ALIMENTE (stânga)
            DrawText("Alimente", px + 24, py + 50, 18, Color{ 230,230,200,255 });
            for (int i = 0; i < Inventory::FoodCount; i++) {
                int ry = py + 76 + i * 34;
                int price = Inventory::FoodPrice(i), qty = inventory.food[i];
                Rectangle r{ (float)(px+18), (float)ry-2, 260, 30 };
                if (qty > 0) totalValue += (long long)qty * price;
                bool hov = qty > 0 && CheckCollisionPointRec(sm, r);
                if (hov) DrawRectangleRec(r, Color{ 255,255,255,28 });
                Color tc = qty > 0 ? WHITE : Color{ 120,120,110,255 };
                DrawText(TextFormat("%-8s x%-3d  @%d = %d", Inventory::FoodName(i), qty, price, qty*price),
                         px + 24, ry, 17, tc);
                if (hov && sclick) { inventory.money += qty * price; inventory.food[i] = 0; }
            }

            // RECOLTE (dreapta) — doar cele cu cantitate
            DrawText("Recolte", px + 330, py + 50, 18, Color{ 230,230,200,255 });
            int rcRow = 0;
            for (int f = 0; f < (int)Flower::COUNT; f++) {
                if (inventory.harvested[f] <= 0) continue;
                int ry = py + 76 + rcRow * 32; rcRow++;
                if (rcRow > 10) break;
                int price = inventory.CurrentSell(f), qty = inventory.harvested[f];
                totalValue += (long long)qty * price;
                Rectangle r{ (float)(px+326), (float)ry-2, 276, 28 };
                bool hov = CheckCollisionPointRec(sm, r);
                if (hov) DrawRectangleRec(r, Color{ 255,255,255,28 });
                const FlowerInfo& fi = FLOWERS[f];
                DrawTexturePro(flowerTex[fi.tex], fi.r2, Rectangle{ (float)(px+328), (float)ry-2, 24, 24 }, {0,0}, 0, WHITE);
                DrawText(TextFormat("%s x%d  @%d", fi.name, qty, price), px + 356, ry, 14, WHITE);
                if (hov && sclick) { inventory.money += qty * price; inventory.harvested[f] = 0; }
            }
            if (rcRow == 0) DrawText("(nimic recoltat inca)", px + 330, py + 80, 14, Color{ 180,170,150,255 });

            // VINDE TOT
            Rectangle allR{ (float)(px + pw/2 - 120), (float)(py + ph - 56), 240, 38 };
            bool ah = CheckCollisionPointRec(sm, allR);
            DrawRectangleRec(allR, ah ? Color{ 90,70,45,255 } : Color{ 55,45,30,255 });
            DrawRectangleLinesEx(allR, 2, Color{ 255,220,90,255 });
            DrawText(TextFormat("VINDE TOT  (%lld bani)", totalValue),
                     (int)allR.x + 18, (int)allR.y + 10, 18, Color{ 255,235,150,255 });
            if (ah && sclick) {
                for (int i = 0; i < Inventory::FoodCount; i++) { inventory.money += inventory.food[i]*Inventory::FoodPrice(i); inventory.food[i] = 0; }
                for (int f = 0; f < (int)Flower::COUNT; f++) { inventory.money += inventory.harvested[f]*inventory.CurrentSell(f); inventory.harvested[f] = 0; }
            }
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
    for (int i = 0; i < 3; i++) UnloadTexture(npcTex[i]);
    market.Unload();
    UnloadTexture(suppliesTex);
    UnloadTexture(chestTex);
    UnloadTexture(marketSign);
    for (int i = 0; i < 5; i++) UnloadTexture(flowerTex[i]);
    UnloadTexture(iconTex);
    world.Unload();
    farm.Unload();
    player.Unload();
    map.Unload();
    CloseWindow();
    return 0;
}
