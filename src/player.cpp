#include "player.h"
#include <cmath>

// Ordinea corespunde cu enum Action (fără Dead, care e tratat separat).
static const char* kActionNames[] = {
    "Walk", "Hoe", "Axe", "Pickaxe", "Sword", "Watercan"
};
static const char* kDirNames[] = { "Down", "Up", "Left", "Right" };

void Player::Load(const std::string& character) {
    name = character;
    std::string base = "sprites/Characters/" + name + "/" + name + "_";

    // Walk + unelte: câte un fișier per direcție (128x32 = 4 frame-uri).
    for (int a = 0; a <= (int)Action::Watercan; a++) {
        for (int d = 0; d < 4; d++) {
            std::string path = base + kActionNames[a] + "_" + kDirNames[d] + ".png";
            tex[a][d] = LoadTexture(path.c_str());
        }
    }
    // Dead: un singur fișier, fără direcție — îl punem pe toate sloturile.
    Texture2D dead = LoadTexture((base + "Dead.png").c_str());
    for (int d = 0; d < 4; d++) tex[(int)Action::Dead][d] = dead;
}

void Player::SetSkin(const std::string& character) {
    if (character == name) return;
    Unload();
    Load(character);
}

void Player::Unload() {
    for (int a = 0; a <= (int)Action::Watercan; a++)
        for (int d = 0; d < 4; d++)
            UnloadTexture(tex[a][d]);
    UnloadTexture(tex[(int)Action::Dead][0]);   // încărcat o singură dată
}

void Player::StartAction(Action a) {
    if (acting) return;             // nu întrerupem o animație în curs
    action = a;
    acting = true;
    frame = 0;
    frameTimer = 0.0f;
}

void Player::Update(float dt) {
    if (acting) {
        // derulează animația de unealtă o singură dată, apoi revine la mers
        frameTimer += dt;
        if (frameTimer >= ActionFrameTime) {
            frameTimer -= ActionFrameTime;
            frame++;
            if (frame >= FrameCount) {
                acting = false;
                action = Action::Walk;
                frame = 0;
            }
        }
        return;                     // mișcarea e blocată cât lucrează
    }

    // --- mișcare ---
    Vector2 move{ 0, 0 };
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    move.y -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  move.y += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  move.x -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) move.x += 1;

    moving = (move.x != 0 || move.y != 0);
    action = Action::Walk;

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
        if (frameTimer >= WalkFrameTime) {
            frameTimer -= WalkFrameTime;
            frame = (frame + 1) % FrameCount;
        }
    } else {
        frame = 0;
    }
}

void Player::Draw() const {
    const Texture2D& sheet = tex[(int)action][(int)dir];

    Rectangle src{ (float)(frame * FrameW), 0, (float)FrameW, (float)FrameH };
    Rectangle dst{ position.x, position.y, FrameW * Scale, FrameH * Scale };
    Vector2 origin{ FrameW * Scale / 2.0f, FrameH * Scale / 2.0f };

    DrawTexturePro(sheet, src, dst, origin, 0.0f, WHITE);
}
