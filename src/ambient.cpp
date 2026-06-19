#include "ambient.h"
#include "tilemap.h"
#include <cmath>

static float Rnd(float a, float b) { return a + (b - a) * (GetRandomValue(0, 10000) / 10000.0f); }

void Ambient::Load() {
    const char* NA = "sprites/NewAssets/";
    beeTex    = LoadTexture(TextFormat("%sAnimals/Bee/Bee_Flying_Animation.png", NA));
    flyTex    = LoadTexture(TextFormat("%sAnimals/Butterfly/Butterfly.png", NA));
    flowerTex = LoadTexture(TextFormat("%sOutdoor decoration/Flowers.png", NA));
}
void Ambient::Unload() {
    UnloadTexture(beeTex); UnloadTexture(flyTex); UnloadTexture(flowerTex);
}

void Ambient::NewTarget(Critter& c) {
    c.target = { Rnd(bounds.x, bounds.x + bounds.width), Rnd(bounds.y, bounds.y + bounds.height) };
}

void Ambient::Init(const TileMap& map) {
    const int TS = TileMap::TileSize;
    bounds = { 3.0f*TS, 4.0f*TS, 30.0f*TS, 26.0f*TS };   // zona unde se plimbă

    critters.clear();
    for (int i = 0; i < 16; i++) {
        Critter c;
        c.type = (i % 5 == 0) ? 0 : (i % 2);   // mix albine/fluturi (mai mulți fluturi colorați)
        c.colorRow = GetRandomValue(0, 7);     // culoarea fluturelui (8 culori)
        c.pos = { Rnd(bounds.x, bounds.x + bounds.width), Rnd(bounds.y, bounds.y + bounds.height) };
        c.wob = Rnd(0, 6.28f);
        NewTarget(c);
        critters.push_back(c);
    }

    // flori decorative pe iarbă, ÎN AFARA grădinii (ca să nu încurce farmatul)
    decos.clear();
    for (int n = 0; n < 80; n++) {
        int tx = GetRandomValue(2, TileMap::Width - 2);
        int ty = GetRandomValue(2, TileMap::Height - 2);
        if (map.At(tx, ty) != Terrain::Grass) continue;          // doar iarbă curată
        bool inGarden = (tx >= TileMap::GX0 && tx <= TileMap::GX1 &&
                         ty >= TileMap::GY0 && ty <= TileMap::GY1);
        if (inGarden) continue;
        int col = GetRandomValue(0, 4);                           // jumătatea stângă = flori de sol
        int row = GetRandomValue(0, 7);
        Deco d;
        d.src = { col * 16.0f, row * 16.0f, 16, 16 };
        d.pos = { tx * (float)TS + TS/2.0f, ty * (float)TS + TS/2.0f };
        decos.push_back(d);
    }
}

void Ambient::Update(float dt) {
    for (auto& c : critters) {
        float speed = (c.type == 0) ? 55.0f : 38.0f;             // albina mai iute
        Vector2 d = { c.target.x - c.pos.x, c.target.y - c.pos.y };
        float len = sqrtf(d.x*d.x + d.y*d.y);
        if (len < 6.0f) { NewTarget(c); }
        else {
            c.pos.x += d.x / len * speed * dt;
            c.pos.y += d.y / len * speed * dt;
        }
        c.wob += dt * ((c.type == 0) ? 9.0f : 7.0f);             // legănat
        c.frameTimer += dt;
        float ft = (c.type == 0) ? 0.07f : 0.18f;
        int frames = (c.type == 0) ? 4 : 2;                      // albină 4 cadre, fluture 2 (aripi)
        if (c.frameTimer >= ft) { c.frameTimer -= ft; c.frame = (c.frame + 1) % frames; }
    }
}

void Ambient::ClearDecoTile(int tx, int ty) {
    const int TS = TileMap::TileSize;
    for (int i = (int)decos.size() - 1; i >= 0; i--) {
        int dtx = (int)(decos[i].pos.x / TS), dty = (int)(decos[i].pos.y / TS);
        if (dtx == tx && dty == ty) decos.erase(decos.begin() + i);
    }
}

void Ambient::DrawGroundDecor() const {
    for (const auto& d : decos) {
        float w = 22, h = 24;
        DrawTexturePro(flowerTex, d.src,
            Rectangle{ d.pos.x - w/2, d.pos.y - h + 6, w, h }, { 0, 0 }, 0, WHITE);
    }
}

void Ambient::DrawCritters() const {
    for (const auto& c : critters) {
        float bob = sinf(c.wob) * 3.0f;
        if (c.type == 0) {   // albină: Bee_Flying 4 cadre 16x16 (rândul de sus), aripi animate
            DrawTexturePro(beeTex, Rectangle{ c.frame * 16.0f, 0, 16, 16 },
                Rectangle{ c.pos.x - 9, c.pos.y - 9 + bob, 18, 18 }, { 0, 0 }, 0, WHITE);
        } else {             // fluture: 8x8 (col0/1 = aripi strânse/deschise, rândul = culoarea)
            DrawTexturePro(flyTex, Rectangle{ c.frame * 8.0f, c.colorRow * 8.0f, 8, 8 },
                Rectangle{ c.pos.x - 8, c.pos.y - 8 + bob, 16, 16 }, { 0, 0 }, 0, WHITE);
        }
    }
}
