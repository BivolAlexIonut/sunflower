// Noduri de resurse din lume: copaci (topor → lemn) și cristale (târnăcop → cristale).
#pragma once
#include "raylib.h"
#include <vector>

class Inventory;
class Player;

enum class NodeType { Tree, Crystal };

struct Node {
    NodeType type;
    Vector2 pos;        // ancoră jos-centru, în coordonate de lume
    int variant = 0;
    int hp = 3;
    float respawn = 0;  // > 0 = tăiat/minat, numără invers spre reapariție
};

class World {
public:
    void Load();
    void Unload();
    void Generate(int seed, int cx0, int cy0, int cx1, int cy1);  // în afara zonei de fermă

    void Update(float dt);
    bool Blocks(Vector2 feet) const;

    // Încearcă să lucreze nodul din tile-ul (tx,ty). Returnează:
    //  0 = niciun nod acolo (lasă farmatul să încerce)
    //  1 = nod prezent și acțiune făcută
    //  2 = nod prezent dar n-ai unealta (blochează farmatul, dar nu face nimic)
    int Interact(int tx, int ty, Inventory& inv, Player& player);

    void DrawBehind(float playerFeetY) const;
    void DrawFront(float playerFeetY) const;

private:
    std::vector<Node> nodes;
    Texture2D treeTex{};
    Texture2D crystalTex{};
    float animTime = 0.0f;

    void DrawNode(const Node& n) const;
};
