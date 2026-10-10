// neomogen.cpp
//
// Algoritmul (cap. 7.8.2.2):
//  1. Calculeaza descompunerea celulara pana la o anumita rezolutie;
//  2. Identifica celulele care contin configuratiile initiala si finala;
//  3. Cauta o secventa de celule Libere/Mixte intre cele doua celule;
//  4. Daca nu exista nicio secventa -> iesire, nu exista drum;
//  5. Daca exista o secventa de celule Libere -> iesire, drum gasit;
//  6. Daca s-a atins rezolutia limita -> iesire, esec;
//  7. Descompune in continuare celulele Mixte;
//  8. Salt la 2.
#include "neomogen.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <queue>

void QuadPlanner::reset(const Scene& s, int maxDepth) {
    scene_ = s;
    maxDepth_ = maxDepth;
    nodes.clear();
    channel.clear();
    channelLM.clear();
    toSplit.clear();
    path.clear();
    log.clear();
    iteration = 0;
    done = success = false;
    message = "Apasă «Pas» pentru a rula algoritmul iterație cu iterație.";
    pathLen = ms = 0;

    QNode root;
    root.r = {0, 0, s.W, s.H};
    root.st = classifyCell(root.r, s);
    root.label = "0";
    nodes.push_back(root);
    collectLeaves();
    startLeaf = locate(s.start);
    goalLeaf = locate(s.goal);
    log.push_back(std::string("Pasul 1: celula rădăcină 0 = tot spațiul de lucru (") + cellStateName(root.st) + ").");
}

void QuadPlanner::split(int id) {
    const RectD r = nodes[id].r;
    const std::string lbl = nodes[id].label;
    const int depth = nodes[id].depth;
    const double mx = (r.x0 + r.x1) * 0.5, my = (r.y0 + r.y1) * 0.5;
    const RectD sub[4] = {{r.x0, r.y0, mx, my}, {mx, r.y0, r.x1, my}, {r.x0, my, mx, r.y1}, {mx, my, r.x1, r.y1}};
    for (int q = 0; q < 4; q++) {
        QNode c;
        c.r = sub[q];
        c.st = classifyCell(c.r, scene_);
        c.depth = depth + 1;
        c.label = (id == 0 ? std::string() : lbl) + char('1' + q);
        nodes[id].child[q] = (int)nodes.size();
        nodes.push_back(c);
    }
}

void QuadPlanner::collectLeaves() {
    leaves.clear();
    nFree = nOcc = nMixed = 0;
    maxDepthReached = 0;
    for (int i = 0; i < (int)nodes.size(); i++) {
        if (!nodes[i].isLeaf()) continue;
        leaves.push_back(i);
        maxDepthReached = std::max(maxDepthReached, nodes[i].depth);
        switch (nodes[i].st) {
            case CellState::Free: nFree++; break;
            case CellState::Occupied: nOcc++; break;
            default: nMixed++; break;
        }
    }
}

int QuadPlanner::locate(Vec2 p) const {
    if (nodes.empty() || p.x < 0 || p.y < 0 || p.x > scene_.W || p.y > scene_.H) return -1;
    int id = 0;
    while (!nodes[id].isLeaf()) {
        const RectD& r = nodes[id].r;
        int q = (p.x >= (r.x0 + r.x1) * 0.5 ? 1 : 0) + (p.y >= (r.y0 + r.y1) * 0.5 ? 2 : 0);
        id = nodes[id].child[q];
    }
    return id;
}

// Doua frunze sunt adiacente daca au o portiune de latura comuna.
// Toate laturile frunzelor stau pe grila celei mai fine rezolutii, deci e suficient
// sa esantionam fiecare latura cu pasul celulei celei mai mici.
std::vector<std::vector<int>> QuadPlanner::buildAdjacency() const {
    std::vector<std::vector<int>> adj(nodes.size());
    double minW = 1e300, minH = 1e300;
    for (int l : leaves) {
        minW = std::min(minW, nodes[l].r.w());
        minH = std::min(minH, nodes[l].r.h());
    }
    const double ex = minW * 0.25, ey = minH * 0.25;
    for (int L : leaves) {
        if (nodes[L].st == CellState::Occupied) continue;
        const RectD& r = nodes[L].r;
        auto& out = adj[L];
        auto add = [&](int n) {
            if (n < 0 || n == L || nodes[n].st == CellState::Occupied) return;
            if (std::find(out.begin(), out.end(), n) == out.end()) out.push_back(n);
        };
        for (double y = r.y0 + minH * 0.5; y < r.y1; y += minH) {
            if (r.x1 < scene_.W - 1e-9) add(locate({r.x1 + ex, y}));
            if (r.x0 > 1e-9) add(locate({r.x0 - ex, y}));
        }
        for (double x = r.x0 + minW * 0.5; x < r.x1; x += minW) {
            if (r.y1 < scene_.H - 1e-9) add(locate({x, r.y1 + ey}));
            if (r.y0 > 1e-9) add(locate({x, r.y0 - ey}));
        }
    }
    return adj;
}

bool QuadPlanner::search(bool allowMixed, const std::vector<std::vector<int>>& adj, std::vector<int>& out) const {
    out.clear();
    auto ok = [&](int id) {
        return nodes[id].st == CellState::Free || (allowMixed && nodes[id].st == CellState::Mixed);
    };
    if (startLeaf < 0 || goalLeaf < 0 || !ok(startLeaf) || !ok(goalLeaf)) return false;

    const size_t N = nodes.size();
    std::vector<double> cost(N, 1e300);
    std::vector<int> parent(N, -1);
    std::vector<char> closed(N, 0);
    using QE = std::pair<double, int>;
    std::priority_queue<QE, std::vector<QE>, std::greater<QE>> pq;
    const Vec2 gc = nodes[goalLeaf].r.center();
    cost[startLeaf] = 0;
    pq.push({dist(nodes[startLeaf].r.center(), gc), startLeaf});
    while (!pq.empty()) {
        int u = pq.top().second;
        pq.pop();
        if (closed[u]) continue;
        closed[u] = 1;
        if (u == goalLeaf) break;
        const Vec2 cu = nodes[u].r.center();
        for (int v : adj[u]) {
            if (closed[v] || !ok(v)) continue;
            Vec2 m = sharedBoundaryMid(nodes[u].r, nodes[v].r);
            Vec2 cv = nodes[v].r.center();
            double nc = cost[u] + dist(cu, m) + dist(m, cv);
            if (nc < cost[v]) {
                cost[v] = nc;
                parent[v] = u;
                pq.push({nc + dist(cv, gc), v});
            }
        }
    }
    if (!closed[goalLeaf]) return false;
    for (int c = goalLeaf; c >= 0; c = parent[c]) out.push_back(c);
    std::reverse(out.begin(), out.end());
    return true;
}

void QuadPlanner::finishFail(const std::string& msg) {
    done = true;
    success = false;
    message = msg;
    log.push_back(msg);
}

void QuadPlanner::step() {
    if (done) return;
    auto t0 = std::chrono::steady_clock::now();
    auto addTime = [&]() {
        ms += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    };

    // Pasul 7 (amanat din iteratia anterioara, ca sa se vada ce celule se rafineaza)
    if (!toSplit.empty()) {
        for (int id : toSplit) split(id);
        log.push_back("Pasul 7: am descompus " + std::to_string(toSplit.size()) + " celule Mixte în câte 4.");
        toSplit.clear();
    }
    channelLM.clear();
    channel.clear();
    iteration++;
    collectLeaves();
    log.push_back("— Iterația " + std::to_string(iteration) + " · adâncime " + std::to_string(maxDepthReached) +
                  " · " + std::to_string(leaves.size()) + " frunze (L " + std::to_string(nFree) + " / O " +
                  std::to_string(nOcc) + " / M " + std::to_string(nMixed) + ")");

    // Pasul 2
    startLeaf = locate(scene_.start);
    goalLeaf = locate(scene_.goal);
    if (startLeaf < 0 || goalLeaf < 0) {
        finishFail("Start/țintă în afara spațiului de lucru.");
        addTime();
        return;
    }
    if (nodes[startLeaf].st == CellState::Occupied) {
        finishFail("Pasul 2: configurația inițială este într-o celulă Ocupată → nu există drum.");
        addTime();
        return;
    }
    if (nodes[goalLeaf].st == CellState::Occupied) {
        finishFail("Pasul 2: configurația finală este într-o celulă Ocupată → nu există drum.");
        addTime();
        return;
    }
    log.push_back("Pasul 2: start în celula " + nodes[startLeaf].label + ", țintă în celula " +
                  nodes[goalLeaf].label + ".");

    auto adj = buildAdjacency();

    // Pasii 3-4
    if (!search(true, adj, channelLM)) {
        finishFail("Pasul 4: nu există nicio secvență de celule L/M → nu există drum.");
        addTime();
        return;
    }
    log.push_back("Pasul 3: secvență de celule L/M găsită (" + std::to_string(channelLM.size()) + " celule).");

    // Pasul 5
    if (search(false, adj, channel)) {
        path.clear();
        path.push_back(scene_.start);
        for (size_t k = 1; k < channel.size(); k++)
            path.push_back(sharedBoundaryMid(nodes[channel[k - 1]].r, nodes[channel[k]].r));
        path.push_back(scene_.goal);
        pathLen = pathLength(path);
        done = success = true;
        channelLM.clear();
        message = "Drum găsit: canal de " + std::to_string(channel.size()) + " celule Libere (iterația " +
                  std::to_string(iteration) + ").";
        log.push_back("Pasul 5: există o secvență de celule Libere → drum găsit.");
        addTime();
        return;
    }
    log.push_back("Pasul 5: încă nu există un canal doar din celule Libere.");

    // Pasul 6 / 7: toate celulele Mixte care nu au atins inca rezolutia limita
    std::vector<int> cand;
    for (int id : leaves)
        if (nodes[id].st == CellState::Mixed && nodes[id].depth < maxDepth_) cand.push_back(id);

    if (cand.empty()) {
        finishFail("Pasul 6: rezoluția limită atinsă (înălțimea arborelui = " + std::to_string(maxDepth_) +
                   ") → eșec. Celulele mixte rămase sunt considerate ocupate.");
        addTime();
        return;
    }
    toSplit = cand;
    message = "Iterația " + std::to_string(iteration) + ": " + std::to_string(cand.size()) +
              " celule Mixte vor fi descompuse.";
    log.push_back("Pasul 7: " + std::to_string(cand.size()) + " celule Mixte marcate pentru descompunere → pasul 2.");
    addTime();
}

void QuadPlanner::runToEnd() {
    int guard = 64;
    while (!done && guard-- > 0) step();
}
