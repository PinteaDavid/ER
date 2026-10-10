// scene.cpp
#include "scene.h"

#include <algorithm>
#include <cmath>

CellState classifyCell(const RectD& r, const Scene& s) {
    bool mixed = false;
    for (const Polygon& poly : s.obstacles) {
        if (poly.size() < 3) continue;
        RectD bb = boundingBox(poly);
        // respingere rapida: celula in afara cutiei obstacolului
        if (r.x1 < bb.x0 || r.x0 > bb.x1 || r.y1 < bb.y0 || r.y0 > bb.y1) continue;

        if (!rectTouchesPolygon(r, poly)) continue;              // nu se ating -> nu conteaza
        if (rectInsidePolygon(r, poly)) return CellState::Occupied;  // complet in obstacol
        mixed = true;                                             // partial in obstacol
    }
    return mixed ? CellState::Mixed : CellState::Free;
}

const char* cellStateName(CellState st) {
    switch (st) {
        case CellState::Free: return "Liberă";
        case CellState::Occupied: return "Ocupată";
        default: return "Mixtă";
    }
}

double pathLength(const std::vector<Vec2>& p) {
    double L = 0;
    for (size_t i = 1; i < p.size(); i++) L += dist(p[i - 1], p[i]);
    return L;
}

std::vector<Preset> makePresets() {
    std::vector<Preset> v;

    {  // Figura 7.74 - doua obstacole: unul neconvex si un patrat rotit
        Scene s;
        s.obstacles.push_back({{330, 140}, {480, 172}, {462, 262}, {385, 232}, {360, 292}, {262, 430}, {305, 292}});
        s.obstacles.push_back(makeRotatedSquare({690, 330}, 150, 20));
        s.start = {80, 110};
        s.goal = {930, 400};
        v.push_back({"Fig. 7.74", s});
    }
    {  // Figura 7.75 - obstacol in forma de T, palnie si varf ascutit
        Scene s;
        s.obstacles.push_back(makeRect(230, 100, 770, 180));
        s.obstacles.push_back(makeRect(450, 180, 530, 400));
        s.obstacles.push_back({{110, 370}, {560, 432}, {470, 545}, {300, 505}});
        s.obstacles.push_back({{650, 650}, {700, 250}, {750, 650}});
        s.start = {580, 240};
        s.goal = {890, 630};
        v.push_back({"Fig. 7.75", s});
    }
    {  // Figura 7.76 - pentagon si doua obstacole mici (metoda arborelui)
        Scene s;
        s.obstacles.push_back({{230, 120}, {350, 150}, {380, 280}, {270, 335}, {195, 250}});
        s.obstacles.push_back({{330, 520}, {395, 465}, {405, 565}});
        s.obstacles.push_back({{650, 500}, {730, 410}, {790, 540}, {705, 610}});
        s.start = {360, 70};
        s.goal = {520, 620};
        v.push_back({"Fig. 7.76", s});
    }
    return v;
}
