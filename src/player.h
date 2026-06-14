// Personajul jucabil — mișcare pe 4 direcții, animat (idle / run).
#pragma once
#include "raylib.h"

enum class Dir { Down, Up, Left, Right };
enum class AnimState { Idle, Run };

class Player {
public:
    void Load();
    void Unload();
    void Update(float dt);
    void Draw() const;

    Vector2 position{ 0, 0 };   // centrul personajului, în coordonate de lume
    float speed = 140.0f;       // pixeli/secundă

private:
    // textures[stare][directie] — fiecare e un spritesheet de 8 frame-uri pe orizontală
    Texture2D textures[2][4]{};

    Dir dir = Dir::Down;
    AnimState state = AnimState::Idle;

    int frame = 0;
    float frameTimer = 0.0f;

    static constexpr int FrameW = 96;
    static constexpr int FrameH = 80;
    static constexpr int FrameCount = 8;
    static constexpr float Scale = 2.0f;        // mărim sprite-ul ca să fie vizibil
    static constexpr float FrameTime = 0.10f;   // secunde per frame de animație

    const Texture2D& CurrentSheet() const;
};
