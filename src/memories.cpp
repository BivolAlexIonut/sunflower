#include "memories.h"
#include "tilemap.h"
#include "particles.h"
#include <ostream>
#include <istream>
#include <cmath>
#include <cstring>

// Cele 5 momente. Bilețelele le poți edita oricând aici.
static const Milestone kMs[Memories::Count] = {
    { "Acolo ne-am cunoscut", "Sala de forta", 2,  6,  9,
      "Aici te-am vazut prima data, la sala. Printre greutati si oglinzi, eu te priveam doar pe tine. "
      "Cine si-ar fi imaginat ca acolo o sa-mi gasesc cea mai frumoasa floare? De atunci a inceput totul.",
      Color{ 235, 90, 110, 255 } },

    { "Primul sarut", "Banca din parc", 4,  15, 25,
      "Pe banca asta, in parc, te-am sarutat prima oara. Mi s-a oprit inima o secunda si lumea a tacut. "
      "As ramane in clipa aceea la nesfarsit.",
      Color{ 255, 140, 190, 255 } },

    { "Negresa", "Prima noastra iesire", 6,  32, 22,
      "Prima data cand am iesit impreuna, mi-ai adus o negresa. Un gest mic care mi-a spus totul: "
      "tu esti omul care are mereu grija de mine. Iar eu, de tine, mereu.",
      Color{ 150, 95, 60, 255 } },

    { "Florile tale", "Gradina ta", 8,  24, 9,
      "Iti plac florile la nebunie - de-aia gradina asta intreaga e a ta. Iar floarea-soarelui mai presus "
      "de toate, pentru ca tu esti soarele dupa care ma intorc mereu.",
      Color{ 255, 205, 90, 255 } },

    { "Prima vacanta", "Iasi", 10,  8, 24,
      "Prima noastra vacanta, la Iasi. Primul drum impreuna, dintr-o viata intreaga de drumuri care vor urma. "
      "Oriunde, oricand, atata timp cat esti tu langa mine.",
      Color{ 110, 210, 200, 255 } },
};

const Milestone& Memories::Get(int i) const { return kMs[i]; }

bool Memories::Unlocked(int i, int level) const {
    return i >= 0 && i < Count && level >= kMs[i].level;
}

Vector2 Memories::WorldPos(int i) const {
    const float TS = (float)TileMap::TileSize;
    return { kMs[i].tileX * TS + TS / 2.0f, kMs[i].tileY * TS + TS / 2.0f };
}

// inimă desenată din primitive (centru cx,cy; r = rază)
static void DrawHeart(float cx, float cy, float r, Color c) {
    DrawCircle((int)(cx - r * 0.5f), (int)(cy - r * 0.3f), r * 0.62f, c);
    DrawCircle((int)(cx + r * 0.5f), (int)(cy - r * 0.3f), r * 0.62f, c);
    DrawTriangle({ cx - r, cy - r * 0.05f }, { cx + r, cy - r * 0.05f },
                 { cx, cy + r * 1.05f }, c);
}

void Memories::Update(float dt, int level, Particles& fx) {
    (void)dt;
    for (int i = 0; i < Count; i++) {
        bool now = Unlocked(i, level);
        if (now && !shown[i]) {                 // tocmai s-a deblocat: sărbătoare
            shown[i] = true;
            Vector2 p = WorldPos(i);
            fx.Hearts(p, 14);
            fx.Stars(p);
        }
        if (now) shown[i] = true;
    }
}

int Memories::NearUnlocked(Vector2 pp, int level) const {
    for (int i = 0; i < Count; i++) {
        if (!Unlocked(i, level)) continue;
        Vector2 p = WorldPos(i);
        if (fabsf(pp.x - p.x) < 44 && fabsf(pp.y - p.y) < 52) return i;
    }
    return -1;
}

void Memories::OpenNote(int i) {
    if (i < 0 || i >= Count) return;
    noteOpen = true; noteIdx = i; seen[i] = true;
}

void Memories::Draw(Vector2 playerPos, int level) const {
    float t = (float)GetTime();
    for (int i = 0; i < Count; i++) {
        Vector2 p = WorldPos(i);
        bool unlocked = Unlocked(i, level);
        // stâlp de lemn
        DrawRectangle((int)p.x - 3, (int)p.y - 6, 6, 22, Color{ 110, 75, 45, 255 });
        DrawRectangle((int)p.x - 3, (int)p.y - 6, 6, 4, Color{ 80, 55, 33, 255 });

        float bob = sinf(t * 2.0f + i) * 3.0f;
        float hy = p.y - 22 + bob;
        if (unlocked) {
            const Milestone& m = kMs[i];
            // halou
            Color glow = m.color; glow.a = 60;
            DrawCircle((int)p.x, (int)hy, 16, glow);
            DrawHeart(p.x, hy, 10, m.color);
            DrawHeart(p.x - 2, hy - 2, 4, Color{ 255, 255, 255, 180 });
            // dacă jucătorul e aproape, arată titlul + invitația
            bool near = fabsf(playerPos.x - p.x) < 44 && fabsf(playerPos.y - p.y) < 52;
            if (near) {
                int tw = MeasureText(m.title, 11);
                DrawRectangle((int)p.x - tw/2 - 4, (int)hy - 36, tw + 8, 16, Color{ 0,0,0,170 });
                DrawText(m.title, (int)p.x - tw/2, (int)hy - 34, 11, m.color);
                const char* e = "[E]";
                int ew = MeasureText(e, 11);
                DrawText(e, (int)p.x - ew/2, (int)p.y + 16, 11, Color{ 255, 240, 160, 255 });
            }
            if (!seen[i]) {        // bulină "nou"
                DrawCircle((int)p.x + 9, (int)hy - 9, 4, Color{ 255, 80, 80, 255 });
            }
        } else {
            // blocat: inimă gri + nivelul necesar
            DrawHeart(p.x, hy, 8, Color{ 90, 90, 100, 220 });
            const char* lv = TextFormat("Nivel %d", kMs[i].level);
            int tw = MeasureText(lv, 10);
            DrawText(lv, (int)p.x - tw/2, (int)hy - 26, 10, Color{ 160, 160, 170, 230 });
        }
    }
}

// desenează text încadrat pe lățime (max maxLines rânduri); întoarce y-ul final
static int WrapText(const char* text, int x, int y, int maxW, int fs, int lh, Color c,
                    int maxLines = 999) {
    char buf[1024]; strncpy(buf, text, sizeof(buf) - 1); buf[sizeof(buf) - 1] = 0;
    char line[1024] = { 0 };
    int lines = 0;
    char* tok = strtok(buf, " ");
    while (tok && lines < maxLines) {
        char test[1024];
        if (line[0]) { strcpy(test, line); strcat(test, " "); strcat(test, tok); }
        else strcpy(test, tok);
        if (MeasureText(test, fs) > maxW && line[0]) {
            if (lines == maxLines - 1) { strcat(line, "..."); break; }
            DrawText(line, x, y, fs, c);
            y += lh; lines++;
            strcpy(line, tok);
        } else {
            strcpy(line, test);
        }
        tok = strtok(nullptr, " ");
    }
    if (line[0] && lines < maxLines) { DrawText(line, x, y, fs, c); y += lh; }
    return y;
}

void Memories::DrawNote() const {
    if (!noteOpen || noteIdx < 0) return;
    const Milestone& m = kMs[noteIdx];
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, Color{ 0, 0, 0, 170 });

    int pw = 560, ph = 320;
    int px = sw/2 - pw/2, py = sh/2 - ph/2;
    DrawRectangle(px, py, pw, ph, Color{ 250, 244, 225, 252 });           // hârtie crem
    DrawRectangleLinesEx(Rectangle{ (float)px,(float)py,(float)pw,(float)ph }, 4, m.color);
    DrawRectangle(px, py, pw, 6, m.color);

    DrawHeart((float)px + 34, (float)py + 40, 16, m.color);
    DrawText(m.title, px + 64, py + 22, 26, Color{ 60, 45, 35, 255 });
    DrawText(m.place, px + 64, py + 52, 16, Color{ 150, 120, 90, 255 });
    DrawLine(px + 24, py + 80, px + pw - 24, py + 80, Color{ 200, 185, 150, 255 });

    WrapText(m.note, px + 30, py + 100, pw - 60, 20, 28, Color{ 70, 55, 45, 255 });

    const char* hint = "Cu drag, al tau.            [ESC] inchide";
    DrawText(hint, px + 30, py + ph - 34, 16, Color{ 150, 120, 90, 255 });
}

void Memories::DrawAlbum(int x, int y, int w, int h, int level) const {
    (void)h;
    DrawText("Locurile noastre. Le deblochezi crescand in nivel; mergi la inima de pe harta si apasa E.",
             x, y, 14, Color{ 190, 200, 190, 255 });
    int unlockedCnt = 0;
    for (int i = 0; i < Count; i++) if (Unlocked(i, level)) unlockedCnt++;
    DrawText(TextFormat("Deblocate: %d/%d", unlockedCnt, Count), x + w - 130, y, 14, Color{ 255,220,90,255 });

    for (int i = 0; i < Count; i++) {
        int ry = y + 30 + i * 78;
        const Milestone& m = kMs[i];
        bool unlocked = Unlocked(i, level);
        DrawRectangle(x - 6, ry - 6, w + 12, 72, Color{ 0, 0, 0, 60 });
        DrawHeart((float)x + 18, (float)ry + 24, 14,
                  unlocked ? m.color : Color{ 90, 90, 100, 255 });
        if (unlocked) {
            DrawText(m.title, x + 46, ry, 20, Color{ 255, 235, 200, 255 });
            DrawText(m.place, x + 46, ry + 22, 13, m.color);
            // primele rânduri din bilețel ca teaser (mergi la inima de pe harta pentru tot)
            WrapText(m.note, x + 46, ry + 40, w - 60, 13, 15, Color{ 200, 200, 190, 255 }, 2);
        } else {
            DrawText("? ? ?", x + 46, ry, 20, Color{ 150, 150, 160, 255 });
            DrawText(TextFormat("Se deblocheaza la nivelul %d", m.level),
                     x + 46, ry + 24, 14, Color{ 170, 150, 120, 255 });
        }
    }
}

void Memories::Serialize(std::ostream& o) const {
    for (int i = 0; i < Count; i++) o << (seen[i] ? 1 : 0) << " ";
    o << "\n";
}

void Memories::Deserialize(std::istream& in) {
    for (int i = 0; i < Count; i++) {
        int v = 0;
        if (in >> v) seen[i] = (v != 0);
    }
}
