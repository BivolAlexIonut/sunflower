// Personajul jucabil (pachetul Happy Harvest) — mers animat pe 4 direcții.
#pragma once
#include "raylib.h"

enum class Dir { Down, Up, Left, Right };

class Player {
public:
    void Load();
    void Unload();
    void Update(float dt);
    void Draw() const;

    Vector2 position{ 0, 0 };   // centrul personajului, în coordonate de lume
    float speed = 130.0f;

    Dir facing() const { return dir; }

private:
    Texture2D walk{};           // Player_Walk.png: 4 frame-uri x 3 rânduri (jos/lateral/sus)

    Dir dir = Dir::Down;
    bool moving = false;
    int frame = 0;
    float frameTimer = 0.0f;

    static constexpr int FrameW = 32;
    static constexpr int FrameH = 32;
    static constexpr int FrameCount = 4;
    static constexpr float Scale = 1.6f;
    static constexpr float FrameTime = 0.12f;
};
