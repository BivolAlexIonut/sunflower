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
static const int kSeedPrice = 8;   // mai ieftin decât în magazinul rapid

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

bool Market::Update(float dt, Player& player, Inventory& inv) {
    player.UpdateSide(dt, 40, StreetW - 40);
    player.position.y = GroundY - 30;   // rămâne pe sol

    // cameră orizontală, limitată la stradă
    cam.target.x += (player.position.x - cam.target.x) * 10.0f * dt;
    cam.target.y = 270;
    float minX = 480, maxX = StreetW - 480;
    if (cam.target.x < minX) cam.target.x = minX;
    if (cam.target.x > maxX) cam.target.x = maxX;

    // taraba cea mai apropiată
    nearStation = -1;
    for (int i = 0; i < kStationCount; i++)
        if (fabsf(player.position.x - kStations[i].x) < 80) { nearStation = i; break; }

    if (nearStation >= 0 && IsKeyPressed(KEY_E)) {
        if (kStations[nearStation].kind == 0) {                 // cumpără sămânță albă
            if (inv.money >= kSeedPrice) { inv.money -= kSeedPrice; inv.seeds[0]++; }
        } else {                                                 // vinde toate florile (+20% bonus)
            for (int f = 0; f < (int)Flower::COUNT; f++) {
                if (inv.harvested[f] > 0) {
                    inv.money += (int)(inv.harvested[f] * inv.CurrentSell(f) * 1.2f);
                    inv.harvested[f] = 0;
                }
            }
        }
    }

    // ieșire
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q) || player.position.x <= 42)
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

    // clădiri scunde cu acoperiș vizibil (zid + acoperiș înclinat + ușă)
    auto building = [&](float x){
        DrawProp(wall, x, 190, GroundY);
        DrawProp(roof, x, 105, GroundY - 190);
        DrawProp(door, x, 130, GroundY);
    };
    building(170);
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
    if (nearStation >= 0) {
        const char* msg = (kStations[nearStation].kind == 0)
            ? "[E] Cumpara samanta alba (8)"
            : "[E] Vinde toate florile (+20% bonus)";
        int w = MeasureText(msg, 22);
        DrawRectangle(GetScreenWidth()/2 - w/2 - 12, GetScreenHeight() - 80, w + 24, 36, Color{ 0,0,0,160 });
        DrawText(msg, GetScreenWidth()/2 - w/2, GetScreenHeight() - 72, 22, WHITE);
    }
    DrawText("Stanga/Dreapta: mergi    |    ESC / marginea stanga: iesi", 16, GetScreenHeight() - 30, 16,
             Color{ 255, 255, 255, 200 });
}
