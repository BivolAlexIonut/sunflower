#include "particles.h"
#include <cmath>

static float Rnd(float a, float b) {
    return a + (b - a) * (GetRandomValue(0, 10000) / 10000.0f);
}

void Particles::Update(float dt) {
    for (auto& p : ps) {
        p.life -= dt;
        p.vel.y += p.gravity * dt;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
    }
    // compactare: scoate particulele moarte
    int w = 0;
    for (int i = 0; i < (int)ps.size(); i++)
        if (ps[i].life > 0.0f) ps[w++] = ps[i];
    ps.resize(w);
}

void Particles::Draw() const {
    for (const auto& p : ps) {
        float t = p.life / p.maxLife;          // 1 -> 0
        if (t < 0) t = 0; if (t > 1) t = 1;
        unsigned char a = (unsigned char)(255 * (t > 0.6f ? 1.0f : t / 0.6f));
        Color c = p.color; c.a = a;
        if (p.kind == 1) {
            // pătrățel rotit ușor (petală / așchie)
            float s = p.size * (0.6f + 0.4f * t);
            DrawRectanglePro(Rectangle{ p.pos.x, p.pos.y, s, s }, { s/2, s/2 },
                             p.life * 180.0f, c);
        } else if (p.kind == 2) {
            // stea (scânteie cu halou)
            Color glow = c; glow.a = (unsigned char)(a / 3);
            DrawCircleV(p.pos, p.size * 1.8f * t, glow);
            DrawPoly(p.pos, 5, p.size * (0.5f + t), p.life * 90.0f, c);
        } else {
            DrawCircleV(p.pos, p.size * (0.4f + 0.6f * t), c);
        }
    }
}

void Particles::Burst(Vector2 pos, int count, Color c, float speed, float life,
                      float gravity, float size, int kind) {
    ps.reserve(ps.size() + count);
    for (int i = 0; i < count; i++) {
        float ang = Rnd(0, 2 * PI);
        float sp = Rnd(speed * 0.3f, speed);
        Particle p;
        p.pos = pos;
        p.vel = { cosf(ang) * sp, sinf(ang) * sp };
        p.maxLife = p.life = Rnd(life * 0.6f, life);
        p.size = Rnd(size * 0.7f, size);
        p.gravity = gravity;
        p.color = c;
        p.kind = kind;
        ps.push_back(p);
    }
}

void Particles::Dirt(Vector2 pos) {
    Burst(pos, 10, Color{ 120, 85, 55, 255 }, 90, 0.5f, 320, 3.0f, 1);
}
void Particles::Water(Vector2 pos) {
    for (int i = 0; i < 12; i++) {
        Particle p;
        p.pos = { pos.x + Rnd(-6, 6), pos.y - 10 };
        p.vel = { Rnd(-40, 40), Rnd(-70, -20) };
        p.maxLife = p.life = Rnd(0.4f, 0.8f);
        p.size = Rnd(2.0f, 3.5f);
        p.gravity = 300;
        p.color = Color{ 90, 170, 245, 255 };
        p.kind = 0;
        ps.push_back(p);
    }
}
void Particles::Plant(Vector2 pos) {
    Burst(pos, 10, Color{ 110, 200, 110, 255 }, 70, 0.6f, 120, 3.0f, 1);
    Burst(pos, 4, Color{ 130, 90, 60, 255 }, 60, 0.5f, 300, 3.0f, 1);
}
void Particles::Petals(Vector2 pos, Color c) {
    Burst(pos, 16, c, 110, 1.0f, 90, 4.0f, 1);
    Burst(pos, 6, Color{ 255, 245, 200, 255 }, 80, 0.9f, 60, 3.0f, 2);
}
void Particles::Coins(Vector2 pos) {
    for (int i = 0; i < 8; i++) {
        Particle p;
        p.pos = pos;
        p.vel = { Rnd(-50, 50), Rnd(-160, -90) };
        p.maxLife = p.life = Rnd(0.6f, 1.0f);
        p.size = Rnd(3.0f, 4.5f);
        p.gravity = 380;
        p.color = Color{ 255, 215, 90, 255 };
        p.kind = 2;
        ps.push_back(p);
    }
}
void Particles::WoodChips(Vector2 pos) {
    Burst(pos, 12, Color{ 160, 110, 60, 255 }, 120, 0.7f, 360, 3.5f, 1);
}
void Particles::Stars(Vector2 pos) {
    Burst(pos, 22, Color{ 255, 230, 120, 255 }, 150, 1.1f, -20, 4.5f, 2);
    Burst(pos, 10, Color{ 255, 255, 255, 255 }, 100, 0.9f, -10, 3.0f, 2);
}
void Particles::Hearts(Vector2 pos, int count) {
    for (int i = 0; i < count; i++) {
        Particle p;
        p.pos = { pos.x + Rnd(-40, 40), pos.y + Rnd(-10, 30) };
        p.vel = { Rnd(-15, 15), Rnd(-50, -25) };
        p.maxLife = p.life = Rnd(1.4f, 2.4f);
        p.size = Rnd(4.0f, 7.0f);
        p.gravity = -8;
        p.color = Color{ 255, 120, 160, 255 };
        p.kind = 0;
        ps.push_back(p);
    }
}
