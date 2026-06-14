// Sistemul de farmat — parcele care pot fi săpate, plantate și recoltate.
#pragma once
#include "raylib.h"
#include <vector>

class Inventory;

enum class Plot { Grass, Soil, Crop };

struct Cell {
    Plot plot = Plot::Grass;
    int stage = 0;        // 0..MaxStage pentru o cultură
    float growth = 0.0f;  // timer spre stadiul următor
};

class Farm {
public:
    Farm();

    void Update(float dt);
    void DrawGround(const Camera2D& cam, const Texture2D& plants) const;
    void DrawTargetHighlight(int tx, int ty) const;

    // Acțiunea contextuală pe parcela țintă (sapă / plantează / recoltează).
    void Interact(int tx, int ty, Inventory& inv);

    static constexpr int MaxStage = 3;      // 3 = matură, gata de recoltat
    static constexpr float StageTime = 3.0f; // secunde per stadiu (rapid pt. test)

private:
    std::vector<Cell> cells;
    int Idx(int tx, int ty) const;
    bool InBounds(int tx, int ty) const;
};
