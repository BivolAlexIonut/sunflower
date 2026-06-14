#include "player.h"
#include <cmath>

void Player::Load() {
    walk = LoadTexture("sprites/Happy_Harvest_Free/Player/Player_Walk.png");
}

void Player::Unload() {
    UnloadTexture(walk);
}

void Player::Update(float dt) {
    Vector2 move{ 0, 0 };
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    move.y -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  move.y += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  move.x -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) move.x += 1;

    moving = (move.x != 0 || move.y != 0);

    if (moving) {
        float len = sqrtf(move.x * move.x + move.y * move.y);
        move.x /= len; move.y /= len;
        position.x += move.x * speed * dt;
        position.y += move.y * speed * dt;

        if (fabsf(move.x) > fabsf(move.y))
            dir = (move.x > 0) ? Dir::Right : Dir::Left;
        else
            dir = (move.y > 0) ? Dir::Down : Dir::Up;

        frameTimer += dt;
        if (frameTimer >= FrameTime) {
            frameTimer -= FrameTime;
            frame = (frame + 1) % FrameCount;
        }
    } else {
        frame = 0;   // stă pe primul frame când e oprit
    }
}

void Player::Draw() const {
    // rândul din spritesheet: jos=0, lateral=1, sus=2 (stânga = lateral oglindit)
    int row = (dir == Dir::Down) ? 0 : (dir == Dir::Up) ? 2 : 1;
    float fw = (dir == Dir::Left) ? -(float)FrameW : (float)FrameW;  // flip orizontal

    Rectangle src{ (float)(frame * FrameW), (float)(row * FrameH), fw, (float)FrameH };
    Rectangle dst{ position.x, position.y, FrameW * Scale, FrameH * Scale };
    Vector2 origin{ FrameW * Scale / 2.0f, FrameH * Scale / 2.0f };

    DrawTexturePro(walk, src, dst, origin, 0.0f, WHITE);
}
