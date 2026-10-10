// omogen.cpp
#include "omogen.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <queue>

RectD GridResult::cellRect(int i, int j, const Scene& s) const {
    return {i * cw, j * ch, std::min((i + 1) * cw, s.W), std::min((j + 1) * ch, s.H)};
}

int GridResult::cellAt(Vec2 p) const {
    int i = (int)std::floor(p.x / cw), j = (int)std::floor(p.y / ch);
    if (i < 0 || j < 0 || i >= cols || j >= rows) return -1;
    return index(i, j);
}

GridResult runGrid(const Scene& s, double cellW, double cellH) {
    auto t0 = std::chrono::steady_clock::now();
    GridResult g;
    g.cw = cellW;
    g.ch = cellH;
    g.cols = (int)std::ceil(s.W / cellW - 1e-9);
    g.rows = (int)std::ceil(s.H / cellH - 1e-9);
    const int N = g.cols * g.rows;
    g.cells.resize(N);
    std::vector<char> closed(N, 0);  // celule deja expandate de A*

    // 1. Descompunerea intregului spatiu de lucru in celule identice + clasificare L/O/M
    for (int j = 0; j < g.rows; j++)
        for (int i = 0; i < g.cols; i++) {
            CellState st = classifyCell(g.cellRect(i, j, s), s);
            g.cells[g.index(i, j)] = st;
            if (st == CellState::Free) g.nFree++;
            else if (st == CellState::Occupied) g.nOcc++;
            else g.nMixed++;
        }

    auto finish = [&]() {
        g.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        return g;
    };

    // 2. Celulele care contin configuratia initiala si cea finala
    const int startCell = g.cellAt(s.start), goalCell = g.cellAt(s.goal);
    if (startCell < 0 || goalCell < 0) {
        g.message = "Start/țintă în afara spațiului de lucru.";
        return finish();
    }
    if (g.cells[startCell] != CellState::Free) {
        g.message = std::string("Configurația inițială se află într-o celulă ") +
                    cellStateName(g.cells[startCell]) + " (inaccesibilă).";
        return finish();
    }
    if (g.cells[goalCell] != CellState::Free) {
        g.message = std::string("Configurația finală se află într-o celulă ") +
                    cellStateName(g.cells[goalCell]) + " (inaccesibilă).";
        return finish();
    }

    // 3. Cautarea canalului (A*) prin celule libere, respectand conditia de adiacenta
    auto center = [&](int id) { return g.cellRect(id % g.cols, id / g.cols, s).center(); };
    auto isFree = [&](int i, int j) {
        return i >= 0 && j >= 0 && i < g.cols && j < g.rows && g.cells[g.index(i, j)] == CellState::Free;
    };
    std::vector<double> cost(N, 1e300);
    std::vector<int> parent(N, -1);
    using QE = std::pair<double, int>;
    std::priority_queue<QE, std::vector<QE>, std::greater<QE>> pq;
    Vec2 goalC = center(goalCell);
    cost[startCell] = 0;
    pq.push({dist(center(startCell), goalC), startCell});

    static const int D4[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!pq.empty()) {
        const int u = pq.top().second;
        pq.pop();
        if (closed[u]) continue;
        closed[u] = 1;
        if (u == goalCell) break;
        int ui = u % g.cols, uj = u / g.cols;
        Vec2 cu = center(u);
        auto relax = [&](int vi, int vj) {
            int v = g.index(vi, vj);
            if (closed[v]) return;
            double nc = cost[u] + dist(cu, center(v));
            if (nc < cost[v]) {
                cost[v] = nc;
                parent[v] = u;
                pq.push({nc + dist(center(v), goalC), v});
            }
        };
        for (auto& d : D4)
            if (isFree(ui + d[0], uj + d[1])) relax(ui + d[0], uj + d[1]);
    }

    if (parent[goalCell] < 0 && goalCell != startCell) {
        g.message = "Nu există un canal de celule libere între start și țintă. "
                    "Încearcă celule mai mici.";
        return finish();
    }
    for (int c = goalCell; c >= 0; c = parent[c]) g.channel.push_back(c);
    std::reverse(g.channel.begin(), g.channel.end());

    // 4. Drumul: start -> mijloacele frontierelor comune dintre celulele canalului -> tinta
    g.path.push_back(s.start);
    for (size_t k = 1; k < g.channel.size(); k++) {
        int a = g.channel[k - 1], b = g.channel[k];
        g.path.push_back(sharedBoundaryMid(g.cellRect(a % g.cols, a / g.cols, s),
                                           g.cellRect(b % g.cols, b / g.cols, s)));
    }
    g.path.push_back(s.goal);

    g.found = true;
    g.pathLen = pathLength(g.path);
    g.message = "Canal găsit: " + std::to_string(g.channel.size()) + " celule libere adiacente.";
    return finish();
}
