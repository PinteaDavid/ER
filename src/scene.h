// scene.h - spatiul de lucru, clasificarea celulelor si scene predefinite
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "geometry.h"

struct Scene {
    double W = 1000, H = 700;          // spatiul de lucru dreptunghiular
    std::vector<Polygon> obstacles;    // obstacole poligonale (convexe sau neconvexe)
    Vec2 start{60, 60}, goal{940, 640};  // robotul este considerat punctiform
};

// Starea unei celule (cap. 7.8.2): Libera / Ocupata / Mixta
enum class CellState : uint8_t { Free, Occupied, Mixed };

// Libera  <=> int(K) ∩ SCB = Ø
// Ocupata <=> K ⊆ SCB
// Mixta   <=> altfel
// SCB = regiunea ocupata de obstacole.
CellState classifyCell(const RectD& cell, const Scene& s);

const char* cellStateName(CellState st);

double pathLength(const std::vector<Vec2>& path);

struct Preset {
    std::string name;
    Scene scene;
};
std::vector<Preset> makePresets();
