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
    treeTex = LoadTexture("sprites/Objects/FG_Tree_Summer.png");
    crystalTex[0] = LoadTexture("sprites/Objects/FG_Crystal_Blue_1.png");
    crystalTex[1] = LoadTexture("sprites/Objects/FG_Crystal_Gold_1.png");
    crystalTex[2] = LoadTexture("sprites/Objects/FG_Crystal_Green_1.png");
    crystalTex[3] = LoadTexture("sprites/Objects/FG_Crystal_Red_1.png");
}
void World::Unload() {
    UnloadTexture(treeTex);
    for (int i = 0; i < 4; i++) UnloadTexture(crystalTex[i]);
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
                // copaci rari și DISTANȚAȚI: doar pe tile-uri pare (nu se ating) în pădure
                bool spaced = (tx % 2 == 0 && ty % 2 == 0);
                bool forest = (tx <= 15);
                if (spaced && forest && h % 2 == 0) {
                    float jx = (float)((h>>4)%8) - 4;
                    nodes.push_back({ NodeType::Tree,
                        { tx*(float)TS + 16 + jx, ty*(float)TS + 30 }, (int)(h%4), 3, 0 });
                }
            }
        }
    }
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
            if (--n.hp <= 0) { inv.wood += 3; n.respawn = 30.0f; }
            return 1;
        } else {
            if (!inv.hasPickaxe) return 2;           // n-ai târnăcop
            player.StartAction(Action::Pickaxe);
            if (--n.hp <= 0) { inv.crystals += 2; n.respawn = 45.0f; }
            return 1;
        }
    }
    return 0;
}

void World::DrawNode(const Node& n) const {
    if (n.respawn > 0) return;
    if (n.type == NodeType::Tree) {
        DrawTexturePro(treeTex, Rectangle{ n.variant*64.0f, 0, 64, 80 },
            Rectangle{ n.pos.x, n.pos.y, 64, 80 }, { 32, 80 }, 0, WHITE);
    } else {
        int frame = ((int)(animTime * 8)) % 8;       // licărire
        int col = (n.variant >= 0 && n.variant < 4) ? n.variant : 0;
        DrawTexturePro(crystalTex[col], Rectangle{ frame*16.0f, 0, 16, 16 },
            Rectangle{ n.pos.x, n.pos.y, 32, 32 }, { 16, 32 }, 0, WHITE);
    }
}

void World::PopulatePlot(int pc, int pr, int theme) {
    const int TS = TileMap::TileSize;
    int tx0 = pc * TileMap::PlotW, ty0 = pr * TileMap::PlotH;
    for (int ty = ty0; ty < ty0 + TileMap::PlotH; ty++) {
        for (int tx = tx0; tx < tx0 + TileMap::PlotW; tx++) {
            unsigned int h = Hash(tx * 5 + 1, ty * 9 + 2);
            if (theme == 1) {                          // mină de cristale
                if (h % 5 == 0)
                    nodes.push_back({ NodeType::Crystal,
                        { tx*(float)TS + 16.0f, ty*(float)TS + 28.0f }, (int)(h % 4), 2, 0 });
            } else {                                   // pădure / crâng (copaci → lemn)
                bool spaced = (tx % 2 == 0 && ty % 2 == 0);
                if (spaced && h % (theme == 2 ? 2 : 3) == 0)
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
                PopulatePlot(pc, pr, map.PlotTheme(pc, pr));
}

void World::DrawBehind(float pFeetY) const {
    for (const Node& n : nodes) if (n.pos.y <= pFeetY) DrawNode(n);
}
void World::DrawFront(float pFeetY) const {
    for (const Node& n : nodes) if (n.pos.y > pFeetY) DrawNode(n);
}
