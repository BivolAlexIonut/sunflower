#include "market.h"
#include "inventory.h"
#include "player.h"
#include <cmath>

// Tarabele interactive: x în stradă, etichetă, fel (0 = cumpără sămânță, 1 = vinde flori).
struct Station { float x; const char* label; int kind; };
static const Station kStations[] = {
    {  500, "SEMINTE",  0 },
    { 1000, "VINDE",    1 },
};
static const int kStationCount = 2;

void Market::Load() {
    bg        = LoadTexture("sprites/Market/Background/Background_01.png");
    ground    = LoadTexture("sprites/Market/Platformer/Ground_11.png");   // suprafață cu iarbă
    groundFill= LoadTexture("sprites/Market/Platformer/Ground_06.png");   // pământ dedesubt
    wall      = LoadTexture("sprites/Market/Building/Wall_A_02.png");
    roof      = LoadTexture("sprites/Market/Building/Roof_A_04.png");      // acoperiș înclinat
    door      = LoadTexture("sprites/Market/Building/Door_01.png");
    furnace   = LoadTexture("sprites/Market/Environment/Furnace.png");
    barrel    = LoadTexture("sprites/Market/Environment/Wooden_Barrel.png");
    crate     = LoadTexture("sprites/Market/Environment/Wooden_Crate.png");
    goods     = LoadTexture("sprites/Market/Environment/Goods_01.png");
    sign      = LoadTexture("sprites/Market/Environment/Sign_01.png");
    stall     = LoadTexture("sprites/Market/Environment/Stall_02.png");    // tarabă completă
    lantern   = LoadTexture("sprites/Market/Environment/Street_Lantern.png");
    herbs     = LoadTexture("sprites/Market/Environment/Herbs.png");

    cam.offset = { 480, 270 };
    cam.zoom = 1.0f;
    cam.rotation = 0;
}

void Market::Unload() {
    UnloadTexture(bg); UnloadTexture(ground); UnloadTexture(groundFill); UnloadTexture(wall);
    UnloadTexture(roof); UnloadTexture(door); UnloadTexture(furnace); UnloadTexture(barrel);
    UnloadTexture(crate); UnloadTexture(goods); UnloadTexture(sign); UnloadTexture(stall);
    UnloadTexture(lantern); UnloadTexture(herbs);
}

void Market::Enter(Player& player) {
    player.position = { 130, GroundY - 40 };
    cam.target = { player.position.x, 270 };
}

// construiește lista florilor deblocate (le poți cumpăra semințe)
static int BuildUnlocked(const Inventory& inv, int* out) {
    int n = 0;
    for (int i = 0; i < (int)Flower::COUNT; i++) if (inv.unlocked[i]) out[n++] = i;
    return n;
}

bool Market::Update(float dt, Player& player, Inventory& inv) {
    // ----- panoul de cumpărare semințe (mai multe tipuri) -----
    if (buyOpen) {
        int list[(int)Flower::COUNT];
        int n = BuildUnlocked(inv, list);
        if (n > 0) {
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) buyRow = (buyRow + 1) % n;
            if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) buyRow = (buyRow + n - 1) % n;
            if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_B)) {
                int f = list[buyRow];
                int price = (int)(FLOWERS[f].seedCost * 0.85f * inv.SeedMul());   // la market e mai ieftin
                if (inv.money >= price) { inv.money -= price; inv.seeds[f]++; inv.SelectFlower(f); }
            }
        }
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q)) buyOpen = false;
        return false;
    }

    player.UpdateSide(dt, 40, StreetW - 40);
    player.position.y = GroundY - 30;

    cam.target.x += (player.position.x - cam.target.x) * 10.0f * dt;
    cam.target.y = 270;
    float minX = 480, maxX = StreetW - 480;
    if (cam.target.x < minX) cam.target.x = minX;
    if (cam.target.x > maxX) cam.target.x = maxX;

    nearStation = -1;
    for (int i = 0; i < kStationCount; i++)
        if (fabsf(player.position.x - kStations[i].x) < 80) { nearStation = i; break; }

    if (nearStation >= 0 && IsKeyPressed(KEY_E)) {
        if (kStations[nearStation].kind == 0) {                 // deschide panoul de semințe
            buyOpen = true; buyRow = 0;
        } else {                                                 // vinde toate florile (+20% bonus)
            for (int f = 0; f < (int)Flower::COUNT; f++) {
                if (inv.harvested[f] > 0) {
                    inv.money += (int)(inv.harvested[f] * inv.CurrentSell(f) * 1.2f);
                    inv.harvested[f] = 0;
                }
            }
        }
    }

    // ieșire (doar dacă nu e panoul deschis)
    if (IsKeyPressed(KEY_ESCAPE) || player.position.x <= 42)
        return true;
    return false;
}

// desenează o textură ancorată jos-centru la (x, GroundY), scalată ca să aibă înălțimea h
static void DrawProp(const Texture2D& t, float x, float h, float groundY, float yoff = 0) {
    float sc = h / t.height;
    float w = t.width * sc;
    DrawTexturePro(t, { 0,0,(float)t.width,(float)t.height },
        { x, groundY + yoff, w, h }, { w/2, h }, 0, WHITE);
}

void Market::Draw(const Player& player, const Inventory& inv) const {
    // fundal (cer + brazi) în spațiu de ecran — backdrop fix
    DrawTexturePro(bg, { 0,0,(float)bg.width,(float)bg.height },
        { 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() }, {0,0}, 0, WHITE);

    BeginMode2D(cam);

    // solul: un rând de suprafață cu iarbă + pământ dedesubt (două tile-uri diferite)
    int gw = ground.width;
    for (int x = -gw; x < (int)StreetW + gw; x += gw) {
        DrawTexture(ground, x, (int)GroundY, WHITE);              // iarba la suprafață
        DrawTexture(groundFill, x, (int)GroundY + gw, WHITE);     // pământ sub
    }

    // clădiri cu acoperiș LIPIT de zid (zidul are ~20px gol sus → calculăm vârful vizibil)
    auto building = [&](float x){
        float wallH = 205;
        DrawProp(wall, x, wallH, GroundY);
        float wallTop = GroundY - 298.0f * (wallH / 320.0f);   // marginea vizibilă de sus a zidului
        DrawProp(roof, x, 112, wallTop + 22);                  // acoperișul se suprapune peste zid
        DrawProp(door, x, 140, GroundY);
    };
    building(180);
    building(1740);

    // decor pe stradă (spațiat, fără suprapuneri urâte)
    DrawProp(barrel, 340, 90, GroundY);
    DrawProp(crate, 720, 95, GroundY);
    DrawProp(goods, 1200, 75, GroundY);
    DrawProp(furnace, 1500, 220, GroundY);
    DrawProp(lantern, 1340, 250, GroundY);
    DrawProp(herbs, 1720, 130, GroundY - 200);

    // tarabe interactive cu firme deasupra
    for (int i = 0; i < kStationCount; i++) {
        DrawProp(stall, kStations[i].x, 130, GroundY);
        DrawProp(sign, kStations[i].x, 85, GroundY - 150);
        int tw = MeasureText(kStations[i].label, 18);
        DrawText(kStations[i].label, (int)kStations[i].x - tw/2, (int)GroundY - 218, 18,
                 Color{ 60, 40, 20, 255 });
    }

    player.DrawSide(3.0f, GroundY);

    EndMode2D();

    // HUD
    DrawText(TextFormat("MARKET   -   Banuti: %d", inv.money), 16, 16, 24, Color{ 255, 230, 150, 255 });
    if (nearStation >= 0 && !buyOpen) {
        const char* msg = (kStations[nearStation].kind == 0)
            ? "[E] Cumpara seminte"
            : "[E] Vinde toate florile (+20% bonus)";
        int w = MeasureText(msg, 22);
        DrawRectangle(GetScreenWidth()/2 - w/2 - 12, GetScreenHeight() - 80, w + 24, 36, Color{ 0,0,0,160 });
        DrawText(msg, GetScreenWidth()/2 - w/2, GetScreenHeight() - 72, 22, WHITE);
    }
    DrawText("Stanga/Dreapta: mergi    |    ESC / marginea stanga: iesi", 16, GetScreenHeight() - 30, 16,
             Color{ 255, 255, 255, 200 });

    // panoul de cumpărare semințe (mai multe tipuri)
    if (buyOpen) {
        int list[(int)Flower::COUNT];
        int n = BuildUnlocked(inv, list);
        int pw = 420, ph = 60 + n * 30;
        if (ph > 460) ph = 460;
        int px = GetScreenWidth()/2 - pw/2, py = GetScreenHeight()/2 - ph/2;
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{ 0,0,0,140 });
        DrawRectangle(px, py, pw, ph, Color{ 38, 28, 20, 245 });
        DrawRectangleLinesEx(Rectangle{ (float)px,(float)py,(float)pw,(float)ph }, 3, Color{ 255,220,90,255 });
        DrawText("SEMINTE (la market: -15%)", px + 16, py + 12, 20, Color{ 255, 220, 90, 255 });
        for (int j = 0; j < n; j++) {
            int f = list[j];
            int ry = py + 44 + j * 30;
            if (j == buyRow) DrawRectangle(px + 8, ry - 4, pw - 16, 28, Color{ 255,255,255,28 });
            int price = (int)(FLOWERS[f].seedCost * 0.85f * inv.SeedMul());
            DrawText(FLOWERS[f].name, px + 18, ry, 18, WHITE);
            DrawText(TextFormat("%d banuti   (ai %d sem.)", price, inv.seeds[f]),
                     px + 200, ry, 16, Color{ 200, 215, 200, 255 });
        }
        DrawText("Sus/Jos alege   |   E cumpara   |   ESC inchide",
                 px + 16, py + ph - 26, 15, Color{ 180,180,180,255 });
    }
}
