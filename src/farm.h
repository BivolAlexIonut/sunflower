// Ferma — parcele lucrate cu uneltele personajului: sapă, plantează, udă, recoltează.
#pragma once
#include "raylib.h"
#include <vector>
#include <iosfwd>

class Inventory;
class Player;

enum class Plot { Grass, Soil, Crop };

struct Cell {
    Plot plot = Plot::Grass;
    int flower = 0;       // tip floare (index în FLOWERS)
    int stage = 0;        // 0 = plantat, 1 = răsărit, 2 = matură
    float growth = 0.0f;
    bool watered = false; // crește doar dacă e udată; se resetează la fiecare stadiu
    int big = 0;          // 0 = normal, 1 = ancoră 2x2 (copac), 2 = celulă acoperită de un copac 2x2
    float dryTime = 0.0f; // secunde de când are nevoie de apă (resetat la udare); prea mult → moare
    bool dead = false;    // a murit de sete: alb-negru, se recoltează cu penalizare XP
};

class Farm {
public:
    Farm();
    void Load();
    void Unload();

    void Update(float dt, bool autoWater, bool fastGrow);
    void DrawGround(const Camera2D& cam) const;
    void DrawTargetHighlight(int ax, int ay, int size, bool inRange) const;

    // zona țintă pentru highlight: 2x2 dacă e copac (existent sau 2x2 de pământ + sămânță copac)
    void TargetArea(int tx, int ty, const class Inventory& inv, int& ax, int& ay, int& size) const;

    // Acțiune contextuală pe parcela țintă; declanșează animația de unealtă potrivită.
    // Returnează ce s-a întâmplat (pentru particule): 0 nimic, 1 plantat, 2 udat,
    // 3 recoltat floare, 4 recoltat copac.
    int Interact(int tx, int ty, Inventory& inv, Player& player);

    // Progres offline: avansează plantele UDATE pentru `seconds` secunde reale
    // (nu udă singur — planta se opreste cand stadiul udat s-a terminat). Plantele neudate
    // prea mult mor. Returnează stadiile avansate; `diedCount` = câte au murit.
    int ApplyOffline(double seconds, int& diedCount);

    // O plantă care are nevoie de apă moare dacă nu e udată atâtea secunde reale (~2 zile).
    static constexpr float DryDeathSeconds = 172800.0f;

    void Serialize(std::ostream&) const;
    void Deserialize(std::istream&);

    int CropCount() const;   // câte plante sunt în pământ (pentru XP/level)
    bool Diggable(int tx, int ty) const;   // celulă de iarbă (se poate săpa)
    void Till(int tx, int ty);             // sapă: iarbă -> pământ

    void SetGrowMul(float m) { growMul = (m > 0.01f) ? m : 1.0f; }   // dificultate
    void Reset();                          // joc nou: golește toate celulele
    int  GardenRating() const;             // nr.flori * raritate * timp investit

    static constexpr int MatureStage = 2;

private:
    std::vector<Cell> cells;
    float growMul = 1.0f;
    Shader grayShader{};     // pentru randarea alb-negru a plantelor moarte
    bool TreeAnchor(int tx, int ty, int& ax, int& ay) const;     // celula face parte dintr-un copac 2x2
    bool Tree2x2Soil(int tx, int ty, int& ax, int& ay) const;    // există un 2x2 de pământ care conține (tx,ty)
    Texture2D soilAtlas{};   // FarmLand_Tile (sol arat uscat)
    Texture2D soilWet{};     // FarmLand_Wet_Tile (sol arat udat)
    Texture2D ftex[4]{};     // 0 vară, 1 iarnă, 2 toamnă, 3 Plants

    int Idx(int tx, int ty) const;
    bool InBounds(int tx, int ty) const;
};
