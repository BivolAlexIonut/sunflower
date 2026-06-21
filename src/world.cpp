#include "world.h"
#include "tilemap.h"
#include "inventory.h"
#include "player.h"
#include <cmath>

static unsigned int Hash(int a, int b) {
    unsigned int h = (unsigned int)(a*73856093) ^ (unsigned int)(b*19349663);
    h ^= h>>13; h *= 0x5bd1e995; h ^= h>>15; return h;
}

void World::Load() {
    treeTex[0] = LoadTexture("sprites/NewAssets/Trees/Big_Oak_Tree.png");
    treeTex[1] = LoadTexture("sprites/NewAssets/Trees/Big_Spruce_tree.png");
    crystalTex[0] = LoadTexture("sprites/Objects/FG_Crystal_Blue_1.png");
    crystalTex[1] = LoadTexture("sprites/Objects/FG_Crystal_Gold_1.png");
    crystalTex[2] = LoadTexture("sprites/Objects/FG_Crystal_Green_1.png");
    crystalTex[3] = LoadTexture("sprites/Objects/FG_Crystal_Red_1.png");
    const char* B = "sprites/NewAssets/Buildings/Buildings/Unique_Buildings/";
    propTex[0] = LoadTexture(TextFormat("%sBarn/Barn_Base_Red.png", B));
    propTex[1] = LoadTexture(TextFormat("%sCoop/Coop_Base_Red.png", B));
    propTex[2] = LoadTexture(TextFormat("%sWindmill/Windmill.png", B));
    propTex[3] = LoadTexture(TextFormat("%sSilo/Silo.png", B));
    propTex[4] = LoadTexture("sprites/NewAssets/Outdoor decoration/Well.png");
    propTex[5] = LoadTexture("sprites/NewAssets/Buildings/Buildings/Houses/Wood/House_2_Wood_Base_Blue.png");
    propTex[6] = LoadTexture("sprites/NewAssets/Trees/Big_Birch_Tree.png");   // mesteacăn decorativ (galben)
    propTex[7] = LoadTexture(TextFormat("%sInn/Inn_Red.png", B));                       // han (oraș)
    propTex[8] = LoadTexture(TextFormat("%sBlacksmith_House/Blacksmith_House_Red.png", B));  // fierărie
    propTex[9] = LoadTexture(TextFormat("%sStalls/Market_Stalls.png", B));              // tarabe
    propTex[10] = LoadTexture("sprites/NewAssets/Buildings/Buildings/Houses/Wood/House_3_Wood_Base_Red.png");  // căsuță (oraș)
}
void World::Unload() {
    for (int i = 0; i < 2; i++) UnloadTexture(treeTex[i]);
    for (int i = 0; i < 4; i++) UnloadTexture(crystalTex[i]);
    for (int i = 0; i < PropTexCount; i++) UnloadTexture(propTex[i]);
}

// clădirile decorative — bottom-anchored, cu o cutie de coliziune la bază
void World::PlaceProps() {
    const float TS = (float)TileMap::TileSize;
    auto add = [&](int tex, Rectangle src, int tx, int ty, float w, float h, float colW) {
        Prop p; p.tex = tex; p.src = src;
        p.pos = { tx * TS + TS/2, ty * TS + TS };     // bottom-center la baza tile-ului
        p.w = w; p.h = h;
        p.col = { p.pos.x - colW/2, p.pos.y - 18, colW, 20 };   // baza blochează mersul
        props.push_back(p);
    };
    props.clear();
    // --- zona casei (vizibilă de la start, pe iarbă) ---
    add(2, { 0,0,80,112 }, 3,  9, 70, 98, 44);       // Moară (corp, fără lame) — stânga
    add(4, { 0,0,32,48 }, 17, 12, 38, 57, 28);       // Fântână (lângă grădină)
    add(5, { 0,0,96,128 }, 4, 18, 84, 112, 50);      // Căsuță (luminiș stânga, NU pe lac)

    // --- lângă țarcul de animale (clădirile stau AFARĂ din țarc, nu peste animale) ---
    add(0, { 0,0,128,144 }, 13, 37, 100, 112, 64);   // Hambar (stânga țarcului)
    add(3, { 0,0,48,80 },  10, 37, 44, 74, 32);      // Siloz
    add(1, { 0,0,80,128 }, 32, 37, 72, 115, 44);     // Coteț (dreapta țarcului, afară)

    // --- ORAȘUL (dreapta-jos, lunca de flori — fără cristale; aici vor fi questuri) ---
    // fiecare clădire încape într-o parcelă (col6=60-69, col7=70-79, col8=80-89, pr5=40-47)
    add(7, { 0,0,240,192 }, 65, 43, 150, 120, 84);   // Han (col6) — aici intri la SUDOKU
    add(5, { 0,0,96,128 }, 66, 47, 84, 112, 50);     // Căsuță (col6 jos)
    add(8, { 0,0,160,128 }, 74, 43, 110, 88, 64);    // Fierărie (col7)
    add(9, { 0,0,48,48 },  73, 47, 42, 42, 0);       // Tarabă
    add(9, { 48,0,48,48 }, 75, 47, 42, 42, 0);       // Tarabă
    add(9, { 96,0,48,48 }, 77, 47, 42, 42, 0);       // Tarabă
    add(10, { 0,0,96,128 }, 84, 42, 84, 112, 50);    // Căsuță (col8)
    add(4, { 0,0,32,48 },  85, 46, 38, 57, 28);      // Fântână (col8 jos)

    // --- mesteceni decorativi (galben) — varietate, fără coliziune, pe iarbă curată ---
    int bx[] = { 2, 11, 17, 24, 31, 61, 80, 70 }, by[] = { 7, 4, 9, 6, 5, 41, 45, 41 };
    for (int i = 0; i < 8; i++) add(6, { 32,0,32,80 }, bx[i], by[i], 52, 82, 0);
}

void World::DrawProp(const Prop& p) const {
    DrawTexturePro(propTex[p.tex], p.src,
        Rectangle{ p.pos.x - p.w/2, p.pos.y - p.h, p.w, p.h }, { 0, 0 }, 0, WHITE);
}

void World::Generate(const TileMap& map) {
    nodes.clear();
    const int TS = TileMap::TileSize;
    for (int ty = 0; ty < TileMap::Height; ty++) {
        for (int tx = 0; tx < TileMap::Width; tx++) {
            Terrain t = map.At(tx, ty);
            unsigned int h = Hash(tx * 31, ty * 17);
            bool path = (ty == 15 || ty == 16);

            if (t == Terrain::Stone) {               // cristale colorate în dungeon
                if (!path && h % 7 == 0)
                    nodes.push_back({ NodeType::Crystal,
                        { tx*(float)TS + 16.0f, ty*(float)TS + 28.0f }, (int)(h % 4), 2, 0 });
            }
            else if ((t == Terrain::Grass || t == Terrain::GrassDark) && !path) {
                // copaci așezați UNIFORM (grilă din 3 în 3) în pădure
                bool forest = (tx <= 15);
                if (forest && tx % 3 == 1 && ty % 3 == 1)
                    nodes.push_back({ NodeType::Tree,
                        { tx*(float)TS + 16.0f, ty*(float)TS + 30.0f }, (int)(h % 4), 3, 0 });
            }
        }
    }
    PlaceProps();
}

void World::Update(float dt) {
    animTime += dt;
    for (auto& n : nodes)
        if (n.respawn > 0) {
            n.respawn -= dt;
            if (n.respawn <= 0) { n.respawn = 0; n.hp = (n.type == NodeType::Tree) ? 3 : 2; }
        }
}

bool World::Blocks(Vector2 feet) const {
    for (const Node& n : nodes) {
        if (n.respawn > 0) continue;
        if (n.type == NodeType::Tree) {
            Rectangle trunk{ n.pos.x - 12, n.pos.y - 14, 24, 14 };
            if (CheckCollisionPointRec(feet, trunk)) return true;
        } else {
            Rectangle r{ n.pos.x - 12, n.pos.y - 14, 24, 14 };
            if (CheckCollisionPointRec(feet, r)) return true;
        }
    }
    for (const Prop& p : props)               // clădirile blochează mersul la bază
        if (CheckCollisionPointRec(feet, p.col)) return true;
    return false;
}

bool World::HasNode(int tx, int ty) const {
    const int TS = TileMap::TileSize;
    for (const Node& n : nodes) {
        if (n.respawn > 0) continue;
        if ((int)(n.pos.x / TS) == tx && (int)((n.pos.y - 2) / TS) == ty) return true;
    }
    return false;
}

int World::Interact(int tx, int ty, Inventory& inv, Player& player) {
    const int TS = TileMap::TileSize;
    for (Node& n : nodes) {
        if (n.respawn > 0) continue;
        int ntx = (int)(n.pos.x / TS), nty = (int)((n.pos.y - 2) / TS);
        if (ntx != tx || nty != ty) continue;

        if (n.type == NodeType::Tree) {
            if (!inv.hasAxe) return 2;               // n-ai topor
            player.StartAction(Action::Axe);
            n.hp -= (1 + inv.AxeBonus());            // upgrade: tai mai repede
            if (n.hp <= 0) { inv.wood += 3 + inv.AxeBonus(); n.respawn = 30.0f; }
            return 1;
        } else {
            if (!inv.hasPickaxe) return 2;           // n-ai târnăcop
            player.StartAction(Action::Pickaxe);
            n.hp -= (1 + inv.PickBonus());           // upgrade: spargi mai repede
            if (n.hp <= 0) { inv.crystals += 2 + inv.PickBonus(); n.respawn = 45.0f; }
            return 1;
        }
    }
    return 0;
}

void World::DrawNode(const Node& n) const {
    if (n.type == NodeType::Tree) {
        // copacul are 3 cadre de 64x80: 0 = ciot, 1/2 = copac. Tăiat → arată ciotul.
        int ti = ((n.variant % 2) + 2) % 2;
        int frame = (n.respawn > 0) ? 0 : 1 + (n.variant & 1);
        DrawTexturePro(treeTex[ti], Rectangle{ frame*64.0f, 0, 64, 80 },
            Rectangle{ n.pos.x, n.pos.y, 72, 90 }, { 36, 84 }, 0, WHITE);
    } else {
        if (n.respawn > 0) return;                   // cristalele dispar până reapar
        int frame = ((int)(animTime * 8)) % 8;       // licărire
        int col = (n.variant >= 0 && n.variant < 4) ? n.variant : 0;
        DrawTexturePro(crystalTex[col], Rectangle{ frame*16.0f, 0, 16, 16 },
            Rectangle{ n.pos.x, n.pos.y, 32, 32 }, { 16, 32 }, 0, WHITE);
    }
}

void World::PopulatePlot(int pc, int pr, int theme, const TileMap& map) {
    const int TS = TileMap::TileSize;
    int tx0 = pc * TileMap::PlotW, ty0 = pr * TileMap::PlotH;
    bool crystals = (theme == 1 || theme == 4);
    bool trees    = (theme == 0 || theme == 2 || theme == 3);   // pădure/crâng/livadă
    // râu/plajă/luncă (5/6/7) → fără resurse de tăiat/minat
    for (int ty = ty0; ty < ty0 + TileMap::PlotH; ty++) {
        for (int tx = tx0; tx < tx0 + TileMap::PlotW; tx++) {
            if (map.At(tx, ty) != Terrain::Grass && map.At(tx, ty) != Terrain::GrassDark)
                continue;                              // doar pe iarbă (nu pe apă/nisip/cărare)
            unsigned int h = Hash(tx * 5 + 1, ty * 9 + 2);
            if (crystals) {
                bool place = (theme == 4) ? ((tx + ty) % 2 == 0) : (tx % 2 == 0 && ty % 2 == 0);
                if (place)
                    nodes.push_back({ NodeType::Crystal,
                        { tx*(float)TS + 16.0f, ty*(float)TS + 28.0f }, (int)(h % 4), 2, 0 });
            } else if (trees) {
                int step = (theme == 2) ? 2 : 3;       // crâng des = mai dens
                if (tx % step == 1 && ty % step == 1)
                    nodes.push_back({ NodeType::Tree,
                        { tx*(float)TS + 16.0f, ty*(float)TS + 30.0f }, (int)(h % 4), 3, 0 });
            }
        }
    }
}

void World::PopulateOwnedPlots(const TileMap& map) {
    for (int pr = 0; pr < TileMap::PlotRows; pr++)
        for (int pc = 0; pc < TileMap::PlotCols; pc++)
            if (map.PlotOwned(pc, pr) && !(pc <= 4 && pr <= 3))   // doar parcele NOI deținute
                PopulatePlot(pc, pr, map.PlotTheme(pc, pr), map);
}

void World::DrawBehind(float pFeetY) const {
    for (const Prop& p : props) if (p.pos.y <= pFeetY) DrawProp(p);
    for (const Node& n : nodes) if (n.pos.y <= pFeetY) DrawNode(n);
}
void World::DrawFront(float pFeetY) const {
    for (const Node& n : nodes) if (n.pos.y > pFeetY) DrawNode(n);
    for (const Prop& p : props) if (p.pos.y > pFeetY) DrawProp(p);
}
