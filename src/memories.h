// Amintiri = momentele relației, deblocate pe nivel. Locuri speciale pe hartă
// (inimă pe un stâlp); mergi la ele, apeși E, apare un bilețel. Tabul "Amintiri" le adună.
#pragma once
#include "raylib.h"
#include <iosfwd>

class Particles;

struct Milestone {
    const char* title;     // titlul locului
    const char* place;     // unde (subtitlu)
    int   level;           // nivel de deblocare
    int   tileX, tileY;    // poziția pe hartă (tile)
    const char* note;      // bilețelul de la el
    Color color;           // culoarea inimii / temei
};

class Memories {
public:
    static constexpr int Count = 5;

    void Update(float dt, int level, Particles& fx);   // detectează deblocări noi (scântei + inimi)
    void Draw(Vector2 playerPos, int level) const;     // markere pe hartă (world-space)

    int  NearUnlocked(Vector2 playerPos, int level) const;  // index milestone aproape+deblocat, altfel -1
    void OpenNote(int i);
    bool NoteOpen() const { return noteOpen; }
    void CloseNote() { noteOpen = false; }
    void DrawNote() const;                              // overlay (screen-space)

    // tabul "Amintiri" din meniu
    void DrawAlbum(int x, int y, int w, int h, int level) const;

    Vector2 WorldPos(int i) const;
    const Milestone& Get(int i) const;
    bool Unlocked(int i, int level) const;

    void Serialize(std::ostream&) const;
    void Deserialize(std::istream&);

private:
    bool seen[Count]   = { false, false, false, false, false };
    bool shown[Count]  = { false, false, false, false, false };  // runtime: deja "deblocat" vizual
    bool noteOpen = false;
    int  noteIdx  = -1;
};
