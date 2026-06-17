// Salvare/încărcare progres într-un fișier text simplu (save.txt lângă executabil).
#pragma once
#include "raylib.h"

class Inventory;
class Farm;
class Shop;
class TileMap;

namespace SaveIO {
    void Save(const char* path, const Inventory& inv, const Shop& shop,
              const Farm& farm, const TileMap& map, Vector2 playerPos);
    // returnează true dacă a încărcat cu succes (fișier valid, versiune ok)
    bool Load(const char* path, Inventory& inv, Shop& shop,
              Farm& farm, TileMap& map, Vector2& playerPos);
}
