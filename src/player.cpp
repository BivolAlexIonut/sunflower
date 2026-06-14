#include "player.h"
#include <cmath>

// Ordinea direcțiilor = ordinea din enum Dir (Down, Up, Left, Right).
static const char* kIdlePaths[4] = {
    "sprites/character/IDLE/idle_down.png",
    "sprites/character/IDLE/idle_up.png",
    "sprites/character/IDLE/idle_left.png",
    "sprites/character/IDLE/idle_right.png",
};
static const char* kRunPaths[4] = {
    "sprites/character/RUN/run_down.png",
    "sprites/character/RUN/run_up.png",
    "sprites/character/RUN/run_left.png",
    "sprites/character/RUN/run_right.png",
};

void Player::Load() {
    for (int d = 0; d < 4; d++) {
        textures[(int)AnimState::Idle][d] = LoadTexture(kIdlePaths[d]);
        textures[(int)AnimState::Run][d]  = LoadTexture(kRunPaths[d]);
    }
}

void Player::Unload() {
    for (int s = 0; s < 2; s++)
        for (int d = 0; d < 4; d++)
            UnloadTexture(textures[s][d]);
}

const Texture2D& Player::CurrentSheet() const {
    return textures[(int)state][(int)dir];
}

void Player::Update(float dt) {
    // --- input de mișcare ---
    Vector2 move{ 0, 0 };
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    move.y -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  move.y += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  move.x -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) move.x += 1;

    bool moving = (move.x != 0 || move.y != 0);

    if (moving) {
        // normalizăm ca diagonala să nu fie mai rapidă
        float len = sqrtf(move.x * move.x + move.y * move.y);
        move.x /= len;
        move.y /= len;

        position.x += move.x * speed * dt;
        position.y += move.y * speed * dt;

        // alegem direcția de privit — prioritizăm axa dominantă
        if (fabsf(move.x) > fabsf(move.y))
            dir = (move.x > 0) ? Dir::Right : Dir::Left;
        else
            dir = (move.y > 0) ? Dir::Down : Dir::Up;

        state = AnimState::Run;
    } else {
        state = AnimState::Idle;
    }

    // --- avans animație ---
    frameTimer += dt;
    if (frameTimer >= FrameTime) {
        frameTimer -= FrameTime;
        frame = (frame + 1) % FrameCount;
    }
}

void Player::Draw() const {
    const Texture2D& sheet = CurrentSheet();

    Rectangle src{ (float)(frame * FrameW), 0, (float)FrameW, (float)FrameH };
    Rectangle dst{
        position.x, position.y,
        FrameW * Scale, FrameH * Scale
    };
    Vector2 origin{ FrameW * Scale / 2.0f, FrameH * Scale / 2.0f };

    DrawTexturePro(sheet, src, dst, origin, 0.0f, WHITE);
}
