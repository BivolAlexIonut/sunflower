#include "saveio.h"
#include "inventory.h"
#include "shop.h"
#include "farm.h"
#include "tilemap.h"
#include "memories.h"
#include <fstream>
#include <string>
#include <ctime>

// v19: hotbar (6 sloturi) + inventar; momente noi; țarc de animale.
static const int kSaveVersion = 19;

void SaveIO::Save(const char* path, const Inventory& inv, const Shop& shop,
                  const Farm& farm, const TileMap& map, const Memories& mem, Vector2 playerPos) {
    std::ofstream f(path);
    if (!f) return;
    f << "SUNFLOWER " << kSaveVersion << "\n";
    f << playerPos.x << " " << playerPos.y << "\n";
    inv.Serialize(f);
    shop.Serialize(f);
    farm.Serialize(f);
    map.Serialize(f);
    mem.Serialize(f);
    f << (long long)time(nullptr) << "\n";   // momentul salvării (pentru idle)
}

bool SaveIO::Load(const char* path, Inventory& inv, Shop& shop,
                  Farm& farm, TileMap& map, Memories& mem, Vector2& playerPos,
                  double& offlineSeconds) {
    offlineSeconds = 0.0;
    std::ifstream f(path);
    if (!f) return false;

    std::string magic; int ver = 0;
    f >> magic >> ver;
    if (magic != "SUNFLOWER" || ver != kSaveVersion) return false;

    f >> playerPos.x >> playerPos.y;
    inv.Deserialize(f);
    shop.Deserialize(f);
    farm.Deserialize(f);
    map.Deserialize(f);
    mem.Deserialize(f);

    long long savedTime = 0;
    if (f >> savedTime && savedTime > 0) {
        double el = (double)((long long)time(nullptr) - savedTime);
        offlineSeconds = (el > 0) ? el : 0.0;
    }
    return true;
}

bool SaveIO::ReadSummary(const char* path, int& level, int& money, int& difficulty, double& playSeconds) {
    std::ifstream f(path);
    if (!f) return false;
    std::string magic; int ver = 0;
    f >> magic >> ver;
    if (magic != "SUNFLOWER" || ver != kSaveVersion) return false;
    Vector2 pos; f >> pos.x >> pos.y;
    Inventory inv;
    inv.Deserialize(f);
    if (!(f.good() || f.eof())) return false;
    level = inv.level; money = inv.money; difficulty = inv.difficulty; playSeconds = inv.playSeconds;
    return true;
}
