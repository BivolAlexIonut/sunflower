// Personajul jucabil — pachetul FG. Mers + toate abilitățile (unelte).
// Proiectat ca "skin": schimbi numele caracterului și se încarcă alt set de sprite-uri.
#pragma once
#include "raylib.h"
#include <string>

enum class Dir { Down, Up, Left, Right };

// Ordinea TREBUIE să corespundă cu kActionNames din player.cpp.
enum class Action { Walk, Hoe, Axe, Pickaxe, Sword, Watercan, Dead, COUNT };

class Player {
public:
    void Load(const std::string& character = "Character01");
    void SetSkin(const std::string& character);   // schimbă caracterul (skin)
    void Unload();
    void Update(float dt);
    void UpdateSide(float dt, float minX, float maxX);   // mers doar stânga/dreapta (scenă side-view)
    void Draw() const;
    void DrawSide(float scale, float groundY) const;     // mărit, picioarele pe sol (side-view)

    void StartAction(Action a);     // pornește o animație de unealtă (one-shot)
    bool IsBusy() const { return acting; }
    Action CurrentAction() const { return action; }

    Vector2 position{ 0, 0 };       // centrul personajului, coordonate de lume
    float speed = 110.0f;
    Dir facing() const { return dir; }
    void FaceTo(Vector2 worldTarget);   // orientează personajul spre o țintă

private:
    std::string name;
    Texture2D tex[(int)Action::COUNT][4]{};   // [acțiune][direcție]

    Dir dir = Dir::Down;
    Action action = Action::Walk;
    bool moving = false;
    bool acting = false;            // execută o animație de unealtă
    int frame = 0;
    float frameTimer = 0.0f;

    static constexpr int FrameW = 32;
    static constexpr int FrameH = 32;
    static constexpr int FrameCount = 4;
    static constexpr float Scale = 2.0f;
    static constexpr float WalkFrameTime = 0.12f;
    static constexpr float ActionFrameTime = 0.08f;
};
