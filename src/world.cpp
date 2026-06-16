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
    treeTex    = LoadTexture("sprites/Objects/FG_Tree_Summer.png");
    crystalTex = LoadTexture("sprites/Objects/FG_Crystal_Blue_1.png");
}
void World::Unload() {
    UnloadTexture(treeTex);
    UnloadTexture(crystalTex);
}

void World::Generate(const TileMap& map) {
    nodes.clear();
    const int TS = TileMap::TileSize;
    for (int ty = 0; ty < TileMap::Height; ty++) {
        for (int tx = 0; tx < TileMap::Width; tx++) {
            Terrain t = map.At(tx, ty);
            unsigned int h = Hash(tx * 31, ty * 17);
            bool path = (ty == 15 || ty == 16);

            if (t == Terrain::Stone) {               // cristale în dungeon
                if (!path && h % 8 == 0)
                    nodes.push_back({ NodeType::Crystal,
                        { tx*(float)TS + 16.0f, ty*(float)TS + 28.0f }, 0, 2, 0 });
            }
            else if (t == Terrain::Grass || t == Terrain::GrassDark) {
                // copaci în pădure (stânga) + pe marginile de sus/jos (cadru); nu în grădină
                bool border = (tx <= 15 || ty <= 2 || ty >= TileMap::Height - 3);
                if (border && !path && h % 3 == 0) {
                    float jx = (float)((h>>4)%12) - 6;
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
        DrawTexturePro(crystalTex, Rectangle{ frame*16.0f, 0, 16, 16 },
            Rectangle{ n.pos.x, n.pos.y, 32, 32 }, { 16, 32 }, 0, WHITE);
    }
}

void World::DrawBehind(float pFeetY) const {
    for (const Node& n : nodes) if (n.pos.y <= pFeetY) DrawNode(n);
}
void World::DrawFront(float pFeetY) const {
    for (const Node& n : nodes) if (n.pos.y > pFeetY) DrawNode(n);
}
