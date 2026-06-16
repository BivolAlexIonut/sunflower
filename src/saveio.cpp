#include "saveio.h"
#include "inventory.h"
#include "shop.h"
#include "farm.h"
#include <fstream>
#include <string>

static const int kSaveVersion = 2;

void SaveIO::Save(const char* path, const Inventory& inv, const Shop& shop,
                  const Farm& farm, Vector2 playerPos) {
    std::ofstream f(path);
    if (!f) return;
    f << "SUNFLOWER " << kSaveVersion << "\n";
    f << playerPos.x << " " << playerPos.y << "\n";
    inv.Serialize(f);
    shop.Serialize(f);
    farm.Serialize(f);
}

bool SaveIO::Load(const char* path, Inventory& inv, Shop& shop,
                  Farm& farm, Vector2& playerPos) {
    std::ifstream f(path);
    if (!f) return false;

    std::string magic; int ver = 0;
    f >> magic >> ver;
    if (magic != "SUNFLOWER" || ver != kSaveVersion) return false;

    f >> playerPos.x >> playerPos.y;
    inv.Deserialize(f);
    shop.Deserialize(f);
    farm.Deserialize(f);
    return f.good() || f.eof();
}
