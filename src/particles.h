// Sistem simplu de particule: stropi de apă, petale, monede, scântei, stele.
// Particulele trăiesc în coordonate de lume (desenează în interiorul BeginMode2D).
#pragma once
#include "raylib.h"
#include <vector>

struct Particle {
    Vector2 pos{};
    Vector2 vel{};
    float life = 0.0f;
    float maxLife = 1.0f;
    float size = 3.0f;
    float gravity = 0.0f;
    Color color{ 255, 255, 255, 255 };
    int kind = 0;        // 0 = cerc, 1 = pătrățel (petală/așchie), 2 = stea
};

class Particles {
public:
    void Update(float dt);
    void Draw() const;        // în spațiul lumii

    void Burst(Vector2 pos, int count, Color c, float speed, float life,
               float gravity, float size, int kind = 0);

    // efecte specifice acțiunilor din joc
    void Dirt(Vector2 pos);
    void Water(Vector2 pos);
    void Plant(Vector2 pos);
    void Petals(Vector2 pos, Color c);
    void Coins(Vector2 pos);
    void WoodChips(Vector2 pos);
    void Stars(Vector2 pos);
    void Hearts(Vector2 pos, int count);

    int Count() const { return (int)ps.size(); }
    void Clear() { ps.clear(); }

private:
    std::vector<Particle> ps;
};
