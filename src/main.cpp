// Grădina Florii-Soarelui — prototip 0.1
// O floare, un click, o petală. De aici crește totul.

#include "raylib.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

struct FloatingText {
    Vector2 pos;
    float life;     // 1.0 -> 0.0, apoi dispare
};

int main() {
    const int screenW = 960;
    const int screenH = 540;

    InitWindow(screenW, screenH, "Gradina Florii-Soarelui");
    SetTargetFPS(60);

    long long petale = 0;

    const Vector2 flowerCenter = { screenW / 2.0f, screenH / 2.0f + 40 };
    const float flowerRadius = 90.0f;   // zona de click
    float squash = 0.0f;                // animația de "apăsare" a florii

    std::vector<FloatingText> floaters;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // --- input ---
        Vector2 mouse = GetMousePosition();
        bool overFlower = CheckCollisionPointCircle(mouse, flowerCenter, flowerRadius);

        if (overFlower && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            petale++;
            squash = 1.0f;
            floaters.push_back({ { mouse.x, mouse.y - 10 }, 1.0f });
        }

        // --- update ---
        squash = fmaxf(0.0f, squash - dt * 5.0f);

        for (auto& f : floaters) {
            f.pos.y -= 60.0f * dt;
            f.life -= dt;
        }
        while (!floaters.empty() && floaters.front().life <= 0.0f)
            floaters.erase(floaters.begin());

        // --- draw ---
        BeginDrawing();
        ClearBackground(Color{ 170, 215, 130, 255 });   // verde de pajiște

        // tulpina
        DrawRectangle((int)flowerCenter.x - 6, (int)flowerCenter.y, 12, screenH, Color{ 80, 140, 60, 255 });

        // floarea pulsează ușor când e apăsată
        float scale = 1.0f + 0.12f * squash;
        float petalDist = 55.0f * scale;

        // petale galbene așezate în cerc
        for (int i = 0; i < 12; i++) {
            float angle = i * (2 * PI / 12);
            Vector2 p = {
                flowerCenter.x + cosf(angle) * petalDist,
                flowerCenter.y + sinf(angle) * petalDist
            };
            DrawCircleV(p, 26 * scale, GOLD);
        }
        // miezul maro
        DrawCircleV(flowerCenter, 42 * scale, Color{ 101, 67, 33, 255 });

        // cursor "mânuță" peste floare
        SetMouseCursor(overFlower ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

        // textele +1 care plutesc în sus
        for (const auto& f : floaters) {
            unsigned char alpha = (unsigned char)(255 * f.life);
            DrawText("+1", (int)f.pos.x, (int)f.pos.y, 24, Color{ 255, 255, 255, alpha });
        }

        // contorul de petale
        std::string counter = "Petale: " + std::to_string(petale);
        int tw = MeasureText(counter.c_str(), 36);
        DrawText(counter.c_str(), screenW / 2 - tw / 2, 30, 36, Color{ 60, 50, 20, 255 });

        DrawText("click pe floare!", 20, screenH - 34, 20, Color{ 60, 50, 20, 180 });

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
