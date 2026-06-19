#include "farm.h"
#include "tilemap.h"
#include "inventory.h"
#include "player.h"
#include <cmath>
#include <ostream>
#include <istream>

// Tile interior de sol arat din FarmLand (16x16, plin edge-to-edge).
static const Rectangle kSoil = { 32, 16, 16, 16 };

// shader care desenează textura în alb-negru (plante moarte)
static const char* kGrayFrag =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    "void main(){\n"
    "  vec4 c = texture(texture0, fragTexCoord)*colDiffuse*fragColor;\n"
    "  float g = dot(c.rgb, vec3(0.299,0.587,0.114));\n"
    "  finalColor = vec4(vec3(g), c.a);\n"
    "}\n";

Farm::Farm() { cells.resize(TileMap::Width * TileMap::Height); }

void Farm::Load() {
    soilAtlas = LoadTexture("sprites/NewAssets/Tiles/FarmLand/FarmLand_Tile.png");
    soilWet   = LoadTexture("sprites/NewAssets/Tiles/FarmLand/FarmLand_Wet_Tile.png");
    ftex[0] = LoadTexture("sprites/Objects/FG_Grass_Summer.png");
    ftex[1] = LoadTexture("sprites/Objects/FG_Grass_Winter.png");
    ftex[2] = LoadTexture("sprites/Objects/FG_Grass_Fall.png");
    ftex[3] = LoadTexture("sprites/Plants&supplies/Plants.png");
    grayShader = LoadShaderFromMemory(nullptr, kGrayFrag);
}
void Farm::Unload() {
    UnloadTexture(soilAtlas);
    UnloadTexture(soilWet);
    for (int i = 0; i < 4; i++) UnloadTexture(ftex[i]);
    UnloadShader(grayShader);
}

int Farm::CropCount() const {
    int n = 0;
    for (const auto& c : cells)
        if (c.plot == Plot::Crop && c.big != 2 && !c.dead) n++;  // copacul = 1 plantă; moartele nu se numără
    return n;
}

bool Farm::Diggable(int tx, int ty) const {
    return InBounds(tx, ty) && cells[Idx(tx, ty)].plot == Plot::Grass;
}
void Farm::Till(int tx, int ty) {
    if (InBounds(tx, ty) && cells[Idx(tx, ty)].plot == Plot::Grass)
        cells[Idx(tx, ty)].plot = Plot::Soil;
}

int Farm::Idx(int tx, int ty) const { return ty * TileMap::Width + tx; }
bool Farm::InBounds(int tx, int ty) const {
    return tx >= 0 && ty >= 0 && tx < TileMap::Width && ty < TileMap::Height;
}

void Farm::Reset() {
    for (auto& c : cells) c = Cell{};
}

// c.growth = secunde reale acumulate în stadiul curent; prag = growTime * growMul.
int Farm::GardenRating() const {
    double rating = 0.0;
    for (const auto& c : cells) {
        if (c.plot != Plot::Crop || c.big == 2 || c.dead) continue;
        int rarity = c.flower + 1;                                  // index mai mare = mai rar
        float stageTime = FLOWERS[c.flower].growTime * growMul;     // timp/stadiu
        double invested = c.stage * (double)stageTime + c.growth;   // secunde investite
        rating += rarity * invested / 60.0;                         // în minute
    }
    return (int)rating;
}

void Farm::Update(float dt, bool autoWater, bool fastGrow) {
    float rate = fastGrow ? 2.0f : 1.0f;   // power-up creștere rapidă
    for (auto& c : cells) {
        if (c.big == 2) continue;   // celulele acoperite de un copac 2x2 nu cresc singure
        if (c.plot == Plot::Crop && c.stage < MatureStage && !c.dead) {
            if (autoWater) { c.watered = true; c.dryTime = 0.0f; }   // power-up auto-udare
            if (c.watered) {
                c.growth += dt * rate;
                if (c.growth >= FLOWERS[c.flower].growTime * growMul) {
                    c.growth = 0.0f;
                    c.stage++;
                    c.watered = false;
                    c.dryTime = 0.0f;   // noul stadiu are nevoie de apă: pornește răbdarea
                }
            } else {
                // are nevoie de apă: dacă o neglijezi prea mult, moare
                c.dryTime += dt;
                if (c.dryTime >= DryDeathSeconds) c.dead = true;
            }
        }
    }
}

bool Farm::TreeAnchor(int tx, int ty, int& ax, int& ay) const {
    if (!InBounds(tx, ty)) return false;
    const Cell& c = cells[Idx(tx, ty)];
    if (c.big == 1) { ax = tx; ay = ty; return true; }
    if (c.big == 2) {
        const int dx[3] = { -1, 0, -1 }, dy[3] = { 0, -1, -1 };
        for (int k = 0; k < 3; k++) {
            int bx = tx + dx[k], by = ty + dy[k];
            if (InBounds(bx, by) && cells[Idx(bx, by)].big == 1) { ax = bx; ay = by; return true; }
        }
    }
    return false;
}

bool Farm::Tree2x2Soil(int tx, int ty, int& ax, int& ay) const {
    const int cxs[4] = { tx, tx-1, tx, tx-1 }, cys[4] = { ty, ty, ty-1, ty-1 };
    for (int k = 0; k < 4; k++) {
        int bx = cxs[k], by = cys[k];
        bool ok = true;
        for (int oy = 0; oy < 2 && ok; oy++)
            for (int ox = 0; ox < 2 && ok; ox++)
                if (!InBounds(bx+ox, by+oy) || cells[Idx(bx+ox, by+oy)].plot != Plot::Soil)
                    ok = false;
        if (ok) { ax = bx; ay = by; return true; }
    }
    return false;
}

void Farm::TargetArea(int tx, int ty, const Inventory& inv, int& ax, int& ay, int& size) const {
    int bx, by;
    if (TreeAnchor(tx, ty, bx, by)) { ax = bx; ay = by; size = 2; return; }   // copac existent
    if (FLOWERS[inv.selectedSeed].isTree && Tree2x2Soil(tx, ty, bx, by)) {
        ax = bx; ay = by; size = 2; return;                                    // pregătit de plantat copac
    }
    ax = tx; ay = ty; size = 1;
}

int Farm::Interact(int tx, int ty, Inventory& inv, Player& player) {
    if (!InBounds(tx, ty)) return 0;

    // dacă am dat click pe o celulă acoperită de un copac 2x2, redirecționăm spre ancoră
    int rax, ray;
    if (TreeAnchor(tx, ty, rax, ray)) { tx = rax; ty = ray; }

    Cell& c = cells[Idx(tx, ty)];

    switch (c.plot) {
        case Plot::Grass:
            // NU se sapă din click instant — săpatul se face DOAR ținând apăsat (hold-to-dig)
            // și DOAR pe iarbă lucrabilă (gardul/piatra/drumul nu se sapă). Vezi main.cpp.
            return 0;
        case Plot::Soil: {
            int f = inv.selectedSeed;
            if (inv.seeds[f] <= 0) break;

            if (FLOWERS[f].isTree) {
                // copac → are nevoie de 2x2 săpat
                int ax, ay;
                if (!Tree2x2Soil(tx, ty, ax, ay)) break;   // nu sunt 4 pătrățele săpate

                inv.seeds[f]--;
                for (int oy = 0; oy < 2; oy++) for (int ox = 0; ox < 2; ox++) {
                    Cell& s = cells[Idx(ax+ox, ay+oy)];
                    s.plot = Plot::Crop; s.flower = f; s.stage = 0; s.growth = 0; s.watered = false;
                    s.big = (ox == 0 && oy == 0) ? 1 : 2;   // colțul stânga-sus = ancoră
                }
                inv.AddXP(5);
                inv.EnsureValidSeed();
            } else {
                // floare normală → un singur pătrat
                inv.seeds[f]--;
                c.plot = Plot::Crop; c.flower = f; c.stage = 0; c.growth = 0; c.watered = false; c.big = 0;
                inv.AddXP(3);
                inv.EnsureValidSeed();
            }
            return 1;   // plantat
        }
        case Plot::Crop:
            if (c.dead) {                            // planta moartă: o smulgi, pierzi XP, replantezi
                int pen = 8 + FLOWERS[c.flower].sellPrice / 50;
                inv.LoseXP(pen);
                player.StartAction(Action::Hoe);
                if (FLOWERS[c.flower].isTree && c.big == 1) {
                    int ax = tx, ay = ty;
                    for (int oy = 0; oy < 2; oy++) for (int ox = 0; ox < 2; ox++) {
                        cells[Idx(ax+ox, ay+oy)] = Cell{}; cells[Idx(ax+ox, ay+oy)].plot = Plot::Soil;
                    }
                } else {
                    c = Cell{}; c.plot = Plot::Soil;
                }
                return 6;   // recoltat moartă (praf, fără monede)
            }
            if (c.stage >= MatureStage) {            // matură
                bool tree = FLOWERS[c.flower].isTree;
                if (tree) {
                    if (!inv.hasAxe) break;           // fără topor nu poți tăia
                    player.StartAction(Action::Axe);
                    inv.harvested[c.flower]++;
                    inv.wood += 4;
                } else {
                    inv.harvested[c.flower]++;
                }
                inv.AddXP(10 + FLOWERS[c.flower].sellPrice / 20);

                if (tree && c.big == 1) {            // copac 2x2 → eliberează toate cele 4 pătrate
                    int ax = tx, ay = ty;
                    for (int oy = 0; oy < 2; oy++) for (int ox = 0; ox < 2; ox++) {
                        Cell& s = cells[Idx(ax+ox, ay+oy)];
                        s = Cell{}; s.plot = Plot::Soil;
                    }
                } else {
                    c.plot = Plot::Soil; c.stage = 0; c.growth = 0; c.watered = false; c.big = 0;
                }
                return tree ? 4 : 3;   // recoltat
            } else if (!c.watered) {                  // udă → pornește creșterea + oprește setea
                player.StartAction(Action::Watercan);
                c.watered = true; c.dryTime = 0.0f;
                if (c.big == 1)                       // copac 2x2 → udă toate cele 4 pătrate
                    for (int oy = 0; oy < 2; oy++) for (int ox = 0; ox < 2; ox++) {
                        cells[Idx(tx+ox, ty+oy)].watered = true;
                        cells[Idx(tx+ox, ty+oy)].dryTime = 0.0f;
                    }
                return 2;   // udat
            }
            break;
    }
    return 0;
}

int Farm::ApplyOffline(double seconds, int& diedCount) {
    diedCount = 0;
    if (seconds <= 0) return 0;
    int stagesGrown = 0;
    for (auto& c : cells) {
        if (c.big == 2) continue;                 // celulele acoperite de copac nu cresc singure
        if (c.plot != Plot::Crop || c.dead) continue;
        double t = seconds;
        // avansează cât e udată: după ce termină stadiul, are nevoie de apă din nou
        while (c.stage < MatureStage && c.watered && t > 0) {
            float need = FLOWERS[c.flower].growTime * growMul - c.growth;
            if (t >= need) {
                t -= need;
                c.growth = 0.0f;
                c.stage++;
                c.watered = false;
                c.dryTime = 0.0f;
                stagesGrown++;
            } else {
                c.growth += (float)t;
                t = 0;
            }
        }
        // timpul rămas, cât e neudată (și încă nu e matură): acumulează sete; prea mult → moare
        if (c.stage < MatureStage && !c.watered && t > 0) {
            c.dryTime += (float)t;
            if (c.dryTime >= DryDeathSeconds) { c.dead = true; diedCount++; }
        }
    }
    return stagesGrown;
}

void Farm::Serialize(std::ostream& o) const {
    int n = 0;
    for (const auto& c : cells) if (c.plot != Plot::Grass) n++;
    o << n << "\n";
    for (int i = 0; i < (int)cells.size(); i++) {
        const Cell& c = cells[i];
        if (c.plot == Plot::Grass) continue;
        o << i << " " << (int)c.plot << " " << c.flower << " " << c.stage << " "
          << c.growth << " " << (c.watered ? 1 : 0) << " " << c.big << " "
          << c.dryTime << " " << (c.dead ? 1 : 0) << "\n";
    }
}

void Farm::Deserialize(std::istream& in) {
    for (auto& c : cells) c = Cell{};
    int n; in >> n;
    for (int k = 0; k < n; k++) {
        int i, p, fl, st, wt, bg, dd; float g, dry;
        in >> i >> p >> fl >> st >> g >> wt >> bg >> dry >> dd;
        if (i >= 0 && i < (int)cells.size())
            cells[i] = Cell{ (Plot)p, fl, st, g, wt != 0, bg, dry, dd != 0 };
    }
}

void Farm::DrawGround(const Camera2D& cam) const {
    const int TS = TileMap::TileSize;
    Vector2 tl = GetScreenToWorld2D({ 0, 0 }, cam);
    Vector2 br = GetScreenToWorld2D({ (float)GetScreenWidth(), (float)GetScreenHeight() }, cam);
    int x0 = (int)floorf(tl.x / TS) - 1, y0 = (int)floorf(tl.y / TS) - 1;
    int x1 = (int)ceilf(br.x / TS) + 1,  y1 = (int)ceilf(br.y / TS) + 1;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 > TileMap::Width)  x1 = TileMap::Width;
    if (y1 > TileMap::Height) y1 = TileMap::Height;

    // solul arat — varianta udă (mai închisă) când cultura e udată
    for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) {
        const Cell& c = cells[Idx(x, y)];
        if (c.plot == Plot::Soil || c.plot == Plot::Crop) {
            bool wet = (c.plot == Plot::Crop && c.watered);
            DrawTexturePro(wet ? soilWet : soilAtlas, kSoil,
                Rectangle{ (float)(x*TS), (float)(y*TS), (float)TS, (float)TS }, {0,0}, 0, WHITE);
        }
    }
    // plantele (bottom-anchored) — textură, rect și scală după tipul plantei
    for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) {
        const Cell& c = cells[Idx(x, y)];
        if (c.plot != Plot::Crop || c.big == 2) continue;   // sclavii 2x2 nu se desenează singuri
        // planta moartă apare chiar dacă murise din stadiul 0 (folosim măcar stadiul 1)
        int dispStage = (c.dead && c.stage < 1) ? 1 : c.stage;
        if (dispStage > 0) {
            const FlowerInfo& fi = FLOWERS[c.flower];
            Rectangle src = (dispStage == 1) ? fi.r1 : fi.r2;
            // copacul 2x2 = centrat în cele 4 pătrate, dimensiune potrivită (nu enorm)
            float extra = (c.big == 1) ? 1.3f : 1.0f;
            float cx = (c.big == 1) ? (x + 1.0f) * TS : x * TS + TS / 2.0f;
            float by = (c.big == 1) ? (y + 1.85f) * TS : (y + 1.0f) * TS;   // baza spre centrul 2x2
            float w = src.width * fi.scale * extra, h = src.height * fi.scale * extra;
            if (c.dead) BeginShaderMode(grayShader);          // alb-negru
            DrawTexturePro(ftex[fi.tex], src,
                Rectangle{ cx - w/2.0f, by - h, w, h }, {0,0}, 0, c.dead ? Color{ 200,200,200,255 } : WHITE);
            if (c.dead) EndShaderMode();
        }
        if (c.dead) {
            // marcaj roșu "moartă" deasupra
            float dx = x*TS + ((c.big == 1) ? TS : TS/2.0f);
            float dy = (float)(y*TS - 10);
            DrawLine((int)dx-5,(int)dy-5,(int)dx+5,(int)dy+5, Color{ 220,60,60,255 });
            DrawLine((int)dx-5,(int)dy+5,(int)dx+5,(int)dy-5, Color{ 220,60,60,255 });
            continue;
        }
        if (c.stage < MatureStage) {
            float cxOff = (c.big == 1) ? TS : TS/2.0f;          // centrat peste 2x2 dacă e copac
            float dx = x*TS + cxOff;
            if (!c.watered) {
                // are nevoie de apă; picătura devine roșie/portocalie când planta e pe moarte
                float frac = c.dryTime / DryDeathSeconds;       // 0..1 spre moarte
                Color drop = (frac > 0.5f) ? Color{ 240, 90, 60, 255 } : Color{ 70, 150, 240, 255 };
                float bob = sinf(GetTime() * 3.0f + x + y) * 2.0f;
                float dy = y*TS - 6 + bob;
                DrawCircle((int)dx, (int)dy, 5, drop);
                DrawTriangle({ dx-5, dy-1 }, { dx+5, dy-1 }, { dx, dy-9 }, drop);
                DrawCircle((int)dx-1, (int)dy-1, 2, Color{ 255, 245, 230, 255 });
                if (frac > 0.5f) {   // avertizare: cât mai are până moare (ore)
                    int hrs = (int)ceilf((DryDeathSeconds - c.dryTime) / 3600.0f);
                    const char* w = TextFormat("moare in %dh", hrs);
                    int tw = MeasureText(w, 10);
                    DrawText(w, (int)dx - tw/2, y*TS - 24, 10, Color{ 250, 120, 90, 255 });
                }
            } else {
                // timer: cât mai durează până trebuie udată din nou (mm:ss)
                int rem = (int)ceilf(FLOWERS[c.flower].growTime * growMul - c.growth);
                if (rem < 0) rem = 0;
                const char* t = TextFormat("%d:%02d", rem / 60, rem % 60);
                int tw = MeasureText(t, 10);
                int bx = (int)dx - tw/2 - 3, byy = y*TS - 16;
                DrawRectangle(bx, byy, tw + 6, 13, Color{ 20, 40, 25, 200 });
                DrawText(t, bx + 3, byy + 1, 10, Color{ 160, 240, 160, 255 });
            }
        }
    }
}

void Farm::DrawTargetHighlight(int ax, int ay, int size, bool inRange) const {
    if (!InBounds(ax, ay)) return;
    const int TS = TileMap::TileSize;
    Color c = inRange ? Color{ 255, 255, 255, 200 } : Color{ 230, 70, 70, 160 };
    Rectangle r{ (float)(ax*TS), (float)(ay*TS), (float)(size*TS), (float)(size*TS) };
    if (inRange) DrawRectangleRec(r, Color{ 255, 255, 255, 35 });
    DrawRectangleLinesEx(r, 2.0f, c);
}
