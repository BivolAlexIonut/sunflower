// Salvare/încărcare progres într-un fișier text simplu (save.txt lângă executabil).
#pragma once
#include "raylib.h"

class Inventory;
class Farm;
class Shop;
class TileMap;
class Memories;

namespace SaveIO {
    void Save(const char* path, const Inventory& inv, const Shop& shop,
              const Farm& farm, const TileMap& map, const Memories& mem, Vector2 playerPos);
    // returnează true dacă a încărcat cu succes (fișier valid, versiune ok).
    // offlineSeconds = câte secunde reale au trecut de la ultima salvare (0 dacă necunoscut).
    bool Load(const char* path, Inventory& inv, Shop& shop,
              Farm& farm, TileMap& map, Memories& mem, Vector2& playerPos,
              double& offlineSeconds);

    // rezumat rapid al unui slot (pentru meniul "Incarca"); true dacă slotul e valid.
    bool ReadSummary(const char* path, int& level, int& money, int& difficulty, double& playSeconds);
}
