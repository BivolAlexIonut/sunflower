// Noduri de resurse din lume: copaci (topor → lemn) și cristale (târnăcop → cristale).
#pragma once
#include "raylib.h"
#include <vector>

class Inventory;
class Player;
class TileMap;

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
    void Generate(const class TileMap& map);   // copaci în pădure, cristale în dungeon

    void Update(float dt);
    bool Blocks(Vector2 feet) const;
    bool HasNode(int tx, int ty) const;   // există un copac/cristal activ în tile

    // Încearcă să lucreze nodul din tile-ul (tx,ty). Returnează:
    //  0 = niciun nod acolo (lasă farmatul să încerce)
    //  1 = nod prezent și acțiune făcută
    //  2 = nod prezent dar n-ai unealta (blochează farmatul, dar nu face nimic)
    int Interact(int tx, int ty, Inventory& inv, Player& player);

    void DrawBehind(float playerFeetY) const;
    void DrawFront(float playerFeetY) const;

    void PopulatePlot(int pc, int pr, int theme, const TileMap& map);   // resurse pe o parcelă cumpărată
    void PopulateOwnedPlots(const TileMap& map);         // după load: repopulează parcelele noi deținute

private:
    std::vector<Node> nodes;
    Texture2D treeTex[2]{};       // Stejar, Brad (fiecare 192x80 = ciot / copac / copac)
    Texture2D crystalTex[4]{};   // Blue, Gold, Green, Red
    float animTime = 0.0f;

    void DrawNode(const Node& n) const;

    // clădiri decorative (case, hambar, moară etc.) — Y-sortate, cu coliziune
    static constexpr int PropTexCount = 11;
    Texture2D propTex[PropTexCount]{};
    struct Prop { int tex; Rectangle src; Vector2 pos; float w, h; Rectangle col; };
    std::vector<Prop> props;
    void PlaceProps();
    void DrawProp(const Prop& p) const;
};
