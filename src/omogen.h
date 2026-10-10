// omogen.h - 7.8.2.1 Metoda descompunerii omogene (metoda grilei omogene)
#pragma once
#include <string>
#include <vector>

#include "scene.h"

struct GridResult {
    int cols = 0, rows = 0;
    double cw = 0, ch = 0;              // dimensiunile celulelor
    std::vector<CellState> cells;       // rows*cols, ordonate pe linii
    std::vector<int> channel;           // canalul: secventa de celule adiacente libere
    std::vector<Vec2> path;             // drumul robotului in interiorul canalului
    int nFree = 0, nOcc = 0, nMixed = 0;
    bool found = false;
    double pathLen = 0, ms = 0;
    std::string message;

    int index(int i, int j) const { return j * cols + i; }
    RectD cellRect(int i, int j, const Scene& s) const;
    int cellAt(Vec2 p) const;
};

// cellW x cellH: dimensiunea celulelor (patrate daca sunt egale, dreptunghiulare altfel)
// Doua celule sunt adiacente daca au o latura comuna (4 vecini: stanga, dreapta, sus, jos).
GridResult runGrid(const Scene& s, double cellW, double cellH);
