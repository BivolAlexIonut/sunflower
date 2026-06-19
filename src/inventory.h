// Inventar + economie: bani, semințe și flori recoltate pe tipuri (rarități).
#pragma once
#include "raylib.h"
#include <iosfwd>
#include <string>

// Plante de plantat (flori, copaci etc.). Indexate doar prin poziție; COUNT = nr. total.
enum class Flower { COUNT = 15 };

// Texturile de plante (indexate de FlowerInfo.tex):
//  0 = FG_Grass_Summer, 1 = FG_Grass_Winter, 2 = FG_Grass_Fall, 3 = Plants&supplies/Plants
static constexpr int FlowerTexCount = 4;

struct FlowerInfo {
    const char* name;
    int   seedCost;     // preț sămânță la magazin
    int   sellPrice;    // preț de vânzare la recoltare
    float growTime;     // secunde per stadiu de creștere
    int   unlockCost;   // cât costă deblocarea (0 = de la start)
    int   tex;          // ce textură (vezi mai sus)
    Rectangle r1, r2;   // dreptunghi sursă pt. stadiul 1 (tânăr) și 2 (matur)
    float scale;        // factor de desenare
    bool  isTree;       // true = se recoltează cu toporul (dă și lemn)
};

extern const FlowerInfo FLOWERS[(int)Flower::COUNT];

class Inventory {
public:
    int money = 50;
    int seeds[(int)Flower::COUNT]     = { 3 };       // start: 3 semințe albe (restul 0)
    int harvested[(int)Flower::COUNT] = { 0 };
    bool unlocked[(int)Flower::COUNT] = { true };    // doar prima deblocată la start
    int selectedSeed = 0;

    // resurse din lume
    int wood = 0;
    int crystals = 0;

    // unelte deblocabile (la start doar sapă/plantează/udă)
    bool hasAxe = false;
    bool hasPickaxe = false;

    // materiale de construcție (drum / piatră) + ce construiești acum (runtime)
    int roadCount = 0;
    int stoneCount = 0;
    int buildSel = 0;        // 0 = nimic, 1 = drum, 2 = piatră (nu se salvează)
    static constexpr int RoadCost = 12;
    static constexpr int StoneCost = 25;

    // piață: prețurile fluctuează în fiecare "zi"
    int day = 0;
    float dayTimer = 0.0f;
    static constexpr float DaySeconds = 90.0f;
    static constexpr int WoodPrice = 5;
    static constexpr int CrystalPrice = 35;

    void TickTime(float dt);              // avansează ziua
    float PriceFactor(int flower) const;  // 0.6..1.6 în funcție de zi
    int   CurrentSell(int flower) const;  // preț de vânzare curent (cu fluctuație)

    // dificultate aleasă la "Joc nou": 0 = Gradinar (blând), 1 = Fermier (normal), 2 = Supravietuitor (realist)
    int difficulty = 1;
    static const char* DifficultyName(int d);
    static const char* DifficultyDesc(int d);
    static int StartMoneyFor(int d);
    const char* DifficultyName() const { return DifficultyName(difficulty); }
    float GrowMul() const;     // cât de repede cresc plantele (timp * mul)
    float SellMul() const;     // cât de mult valorează florile la vânzare
    float SeedMul() const;     // cât costă semințele
    int   StartMoney() const;  // bani la început

    // timp total petrecut în joc (secunde) — pentru statistici
    double playSeconds = 0.0;

    // progresie XP / nivel
    int xp = 0;
    int level = 1;
    int levelUpTimer = 0;          // runtime: cât mai afișăm popup-ul de level-up
    bool levelUpPending = false;   // runtime: trebuie afișat modalul de level-up
    std::string levelUpMsg;        // runtime
    int XPForNext() const { return 80 + level * 70; }   // XP necesar pentru nivelul următor
    void AddXP(int amount);               // adaugă XP, urcă în nivel, deblochează flori
    void LoseXP(int amount) { xp -= amount; if (xp < 0) xp = 0; }   // penalizare (nu coboară nivelul)
    void EnsureValidSeed();               // selectează o sămânță pe care o ai (dacă cea curentă e 0)
    void DrawLevel(int plantedCount) const;

    // Power-up-uri (buff-uri temporare): 0 viteză, 1 auto-udare, 2 creștere rapidă,
    //                                    3 bonus bani, 4 bonus XP
    static constexpr int BuffCount = 5;
    float buff[BuffCount] = { 0, 0, 0, 0, 0 };   // secunde rămase
    void AddBuff(int t, float sec) { if (t >= 0 && t < BuffCount) buff[t] = sec; }
    bool BuffActive(int t) const { return t >= 0 && t < BuffCount && buff[t] > 0.0f; }
    void UpdateBuffs(float dt) { for (int i = 0; i < BuffCount; i++) if (buff[i] > 0) buff[i] -= dt; }

    void CycleSeed();   // Q: trece la următoarea floare deblocată
    void Draw(const Texture2D* ftex, const Texture2D& icons) const;   // ftex = array[FlowerTexCount]

    void Serialize(std::ostream&) const;
    void Deserialize(std::istream&);
};
