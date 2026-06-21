// Minigame: Sudoku cu flori (la hanul din oraș). Scenă separată, cu dificultăți.
#pragma once
#include "raylib.h"

class Sudoku {
public:
    void Enter();                          // resetează la alegerea dificultății
    bool Update();                         // true = ieși înapoi în lume
    void Draw(const Texture2D* flowerTex) const;   // flowerTex = array[FlowerTexCount]
    bool TakeWinReward();                  // true o singură dată la câștig (pt. recompensă)
    void DebugStart(int diff) { Generate(diff); }   // doar pentru screenshot-uri

private:
    int  phase = 0;                        // 0 = alegi dificultatea, 1 = joc, 2 = câștigat
    int  difficulty = 1;
    int  solution[81] = { 0 };
    int  grid[81] = { 0 };                 // -1 = gol, 0..8 = floare
    bool given[81] = { false };            // celulă fixă (indiciu)
    int  selected = -1;
    int  pick = -1;                        // floarea aleasă din paletă
    float t = 0.0f;
    bool rewardTaken = false;

    void Generate(int diff);
    bool Conflict(int idx) const;
    bool Solved() const;
    void Place(int idx, int val);

    // layout (ecran 960x540)
    static constexpr int Cell = 46, GX = 250, GY = 60;
    int CellAt(Vector2 m) const;
    Rectangle PalRect(int d) const;
    Rectangle EraseRect() const;
    Rectangle DiffRect(int d) const;
};
