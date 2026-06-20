#include "ambient.h"
#include "tilemap.h"
#include <cmath>

static float Rnd(float a, float b) { return a + (b - a) * (GetRandomValue(0, 10000) / 10000.0f); }

void Ambient::Load() {
    const char* NA = "sprites/NewAssets/";
    beeTex    = LoadTexture(TextFormat("%sAnimals/Bee/Bee_Flying_Animation.png", NA));
    flyTex    = LoadTexture(TextFormat("%sAnimals/Butterfly/Butterfly.png", NA));
    flowerTex = LoadTexture(TextFormat("%sOutdoor decoration/Flowers.png", NA));
    decorTex  = LoadTexture(TextFormat("%sOutdoor decoration/Outdoor_Decor.png", NA));
    animalTex[0] = LoadTexture(TextFormat("%sAnimals/Chicken/Chicken_01.png", NA));
    animalTex[1] = LoadTexture(TextFormat("%sAnimals/Pig/Pig_01.png", NA));
    animalTex[2] = LoadTexture(TextFormat("%sAnimals/Sheep/Sheep_01.png", NA));
    animalTex[3] = LoadTexture(TextFormat("%sAnimals/Cow/Cow_01.png", NA));
}
void Ambient::Unload() {
    UnloadTexture(beeTex); UnloadTexture(flyTex); UnloadTexture(flowerTex); UnloadTexture(decorTex);
    for (int i = 0; i < 4; i++) UnloadTexture(animalTex[i]);
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

    // pășunea = interiorul țarcului de animale (gardul îi ține înăuntru)
    pasture = { (TileMap::PenX0 + 1.0f)*TS, (TileMap::PenY0 + 1.0f)*TS,
                (TileMap::PenX1 - TileMap::PenX0 - 1.0f)*TS, (TileMap::PenY1 - TileMap::PenY0 - 1.0f)*TS };
    beasts.clear();

    // decor pe iarbă, ÎN AFARA grădinii: tufe de iarbă (textură) + flori (culoare)
    decos.clear();
    auto onGrass = [&](int tx, int ty) {
        if (map.At(tx, ty) != Terrain::Grass) return false;
        bool inGarden = (tx >= TileMap::GX0 && tx <= TileMap::GX1 &&
                         ty >= TileMap::GY0 && ty <= TileMap::GY1);
        return !inGarden;
    };
    for (int n = 0; n < 240; n++) {                              // tufe de iarbă (textură subtilă)
        int tx = GetRandomValue(2, TileMap::Width - 2), ty = GetRandomValue(2, TileMap::Height - 2);
        if (!onGrass(tx, ty)) continue;
        int col = GetRandomValue(0, 5), rw = GetRandomValue(0, 2);   // rândurile de tufe verzi
        Deco d; d.sheet = 1; d.src = { col * 16.0f, rw * 16.0f, 16, 16 };
        d.pos = { tx*(float)TS + TS/2.0f, ty*(float)TS + TS/2.0f };
        decos.push_back(d);
    }
    for (int n = 0; n < 70; n++) {                              // flori colorate (mai rare)
        int tx = GetRandomValue(2, TileMap::Width - 2), ty = GetRandomValue(2, TileMap::Height - 2);
        if (!onGrass(tx, ty)) continue;
        int col = GetRandomValue(0, 4), rw = GetRandomValue(0, 7);
        Deco d; d.sheet = 0; d.src = { col * 16.0f, rw * 16.0f, 16, 16 };
        d.pos = { tx*(float)TS + TS/2.0f, ty*(float)TS + TS/2.0f };
        decos.push_back(d);
    }
    // chenar de flori în jurul grădinii — arată frumos de la început
    for (int tx = TileMap::GX0 - 1; tx <= TileMap::GX1 + 1; tx++)
        for (int ty = TileMap::GY0 - 1; ty <= TileMap::GY1 + 1; ty++) {
            bool ring = (tx == TileMap::GX0 - 1 || tx == TileMap::GX1 + 1 ||
                         ty == TileMap::GY0 - 1 || ty == TileMap::GY1 + 1);
            if (!ring || map.At(tx, ty) != Terrain::Grass) continue;
            if (GetRandomValue(0, 2) != 0) continue;            // nu pe fiecare tile
            Deco d; d.sheet = 0;
            d.src = { GetRandomValue(0, 4) * 16.0f, GetRandomValue(0, 5) * 16.0f, 16, 16 };
            d.pos = { tx*(float)TS + TS/2.0f, ty*(float)TS + TS/2.0f };
            decos.push_back(d);
        }
}

void Ambient::SyncAnimals(const int counts[4]) {
    beasts.clear();
    for (int type = 0; type < 4; type++) {
        int show = counts[type]; if (show > 6) show = 6;        // max 6 vizibile/tip
        for (int k = 0; k < show; k++) {
            Beast b; b.type = type;
            b.pos = { Rnd(pasture.x, pasture.x + pasture.width), Rnd(pasture.y, pasture.y + pasture.height) };
            b.target = b.pos; b.t = Rnd(0, 3);
            beasts.push_back(b);
        }
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
    // animale: se plimbă lent prin pășune
    for (auto& b : beasts) {
        b.t -= dt;
        if (b.t <= 0) {
            b.t = Rnd(2.5f, 6.0f);
            b.target = { Rnd(pasture.x, pasture.x + pasture.width), Rnd(pasture.y, pasture.y + pasture.height) };
        }
        Vector2 d = { b.target.x - b.pos.x, b.target.y - b.pos.y };
        float len = sqrtf(d.x*d.x + d.y*d.y);
        if (len > 3.0f) { b.pos.x += d.x/len * 20.0f * dt; b.pos.y += d.y/len * 20.0f * dt; }
        b.ft += dt;
        if (b.ft >= 0.4f) { b.ft -= 0.4f; b.frame = (b.frame + 1) % 2; }
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
        const Texture2D& tx = (d.sheet == 1) ? decorTex : flowerTex;
        float w = (d.sheet == 1) ? 24 : 22, h = (d.sheet == 1) ? 24 : 24;
        DrawTexturePro(tx, d.src,
            Rectangle{ d.pos.x - w/2, d.pos.y - h + 6, w, h }, { 0, 0 }, 0, WHITE);
    }
}

void Ambient::DrawAnimals(float playerFeetY, bool front) const {
    for (const auto& b : beasts) {
        if ((b.pos.y > playerFeetY) != front) continue;
        DrawTexturePro(animalTex[b.type], Rectangle{ b.frame * 32.0f, 0, 32, 32 },
            Rectangle{ b.pos.x, b.pos.y, 36, 36 }, { 18, 30 }, 0, WHITE);
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
