// neomogen.h - 7.8.2.2 Metoda descompunerii neomogene (metoda arborelui 2^m, m = 2 -> quadtree)
#pragma once
#include <string>
#include <vector>

#include "scene.h"

struct QNode {
    RectD r;
    CellState st = CellState::Mixed;
    int depth = 0;
    int child[4] = {-1, -1, -1, -1};  // 1 = stanga-sus, 2 = dreapta-sus, 3 = stanga-jos, 4 = dreapta-jos
    std::string label;                // eticheta: "0" radacina, apoi "1".."4", "11", "114", ...
    bool isLeaf() const { return child[0] < 0; }
};

class QuadPlanner {
public:
    // maxDepth = "inaltimea" arborelui (rezolutia limita)
    void reset(const Scene& s, int maxDepth);
    void step();       // o iteratie a algoritmului (pasii 2..8)
    void runToEnd();

    int locate(Vec2 p) const;  // frunza care contine punctul

    // ---- stare / rezultate
    std::vector<QNode> nodes;
    std::vector<int> leaves;
    std::vector<int> channel;     // canal doar din celule Libere (solutia)
    std::vector<int> channelLM;   // canal din celule Libere/Mixte (ghideaza rafinarea)
    std::vector<int> toSplit;     // celule Mixte care vor fi descompuse la pasul urmator
    std::vector<Vec2> path;
    std::vector<std::string> log;
    int iteration = 0;
    int startLeaf = -1, goalLeaf = -1;
    bool done = false, success = false;
    std::string message;
    double pathLen = 0, ms = 0;
    int nFree = 0, nOcc = 0, nMixed = 0;
    int maxDepthReached = 0;

private:
    Scene scene_;
    int maxDepth_ = 6;

    void split(int id);
    void collectLeaves();
    std::vector<std::vector<int>> buildAdjacency() const;
    bool search(bool allowMixed, const std::vector<std::vector<int>>& adj, std::vector<int>& out) const;
    void finishFail(const std::string& msg);
};
