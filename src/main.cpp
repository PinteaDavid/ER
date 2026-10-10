// main.cpp - Metoda descompunerii celulare aproximative (Cap. 7.8.2)
//  - 7.8.2.1 Descompunere omogena  (grila omogena)
//  - 7.8.2.2 Descompunere neomogena (metoda arborelui)
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "neomogen.h"
#include "omogen.h"
#include "raylib.h"
#include "scene.h"
#include "ui.h"

using ui::T;

static const Color C_WORK = {30, 35, 46, 255};      // spatiul de lucru (celule libere)
static const Color C_GRID = {255, 255, 255, 30};
static const Color C_OCC = {239, 68, 68, 165};      // celula ocupata
static const Color C_MIX = {245, 158, 11, 150};     // celula mixta
static const Color C_CHAN = {45, 212, 191, 125};    // canal (celule libere)
static const Color C_CHAN_LM = {45, 212, 191, 55};  // canal L/M (in curs, pas cu pas)
static const Color C_SPLIT = {253, 224, 71, 255};   // celule ce vor fi descompuse
static const Color C_OBS = {59, 130, 246, 240};
static const Color C_START = {34, 197, 94, 255};
static const Color C_GOAL = {244, 63, 94, 255};

struct App {
    std::vector<Preset> presets = makePresets();
    Scene scene;
    int method = 0;                  // 0 = omogena, 1 = neomogena
    float cellW = 40, cellH = 40;    // omogena
    bool square = true;
    float maxDepth = 6;              // neomogena: inaltimea arborelui
    bool showLines = true;
    int tool = 0;                    // 0 muta, 1 deseneaza, 2 sterge

    GridResult grid;
    QuadPlanner quad;
    bool dirty = true, manual = false, playing = false;
    float playTimer = 0;

    Polygon drawing;                 // poligon in curs de desenare
    int drag = 0, dragObs = -1;      // 1 start, 2 tinta, 3 obstacol
    Vec2 dragLast;
    std::string toast;
    float toastT = 0;
};

// transformare coordonate spatiu de lucru <-> ecran
struct View {
    float ox = 0, oy = 0, sc = 1;
    Vector2 w2s(Vec2 p) const { return {(float)(ox + p.x * sc), (float)(oy + p.y * sc)}; }
    Vec2 s2w(Vector2 p) const { return {(p.x - ox) / sc, (p.y - oy) / sc}; }
    Rectangle rect(const RectD& r) const {
        return {(float)(ox + r.x0 * sc), (float)(oy + r.y0 * sc), (float)(r.w() * sc), (float)(r.h() * sc)};
    }
};

static void recompute(App& a) {
    if (a.square) a.cellH = a.cellW;
    if (a.method == 0) {
        a.grid = runGrid(a.scene, a.cellW, a.cellH);
    } else {
        a.quad.reset(a.scene, (int)a.maxDepth);
        if (!a.manual) a.quad.runToEnd();
    }
    a.dirty = false;
}

static void setMethod(App& a, int m) {
    a.method = m;
    a.manual = a.playing = false;
    a.dirty = true;
}

// Executie pas cu pas a metodei arborelui
static void quadStep(App& a) {
    if (!a.manual) {
        a.manual = true;
        recompute(a);
    }
    a.playing = false;
    a.quad.step();
}

// ------------------------------------------------------------------ desen
static void drawPolygon(const View& v, const Polygon& p, Color fill) {
    auto tri = triangulate(p);
    for (size_t k = 0; k + 2 < tri.size(); k += 3) {
        Vector2 A = v.w2s(p[tri[k]]), B = v.w2s(p[tri[k + 1]]), C = v.w2s(p[tri[k + 2]]);
        DrawTriangle(A, C, B, fill);  // una din cele doua orientari e desenata
        DrawTriangle(A, B, C, fill);
    }
    for (size_t i = 0; i < p.size(); i++) DrawLineEx(v.w2s(p[i]), v.w2s(p[(i + 1) % p.size()]), 1.6f, {147, 197, 253, 255});
}

static void drawMarker(const View& v, Vec2 p, Color c, const char* label) {
    Vector2 s = v.w2s(p);
    DrawCircleV(s, 9, WHITE);
    DrawCircleV(s, 6.5f, c);
    Vector2 m = ui::Measure(label, 12, true);
    DrawRectangleRounded({s.x + 13, s.y - 10, m.x + 12, 20}, 1.0f, 8, c);
    ui::Text(label, s.x + 19, s.y - m.y / 2, 12, WHITE, true);
}

static void drawCanvas(const App& a, const View& v) {
    const Scene& s = a.scene;
    Rectangle ws = v.rect({0, 0, s.W, s.H});
    DrawRectangleRec(ws, C_WORK);
    BeginScissorMode((int)ws.x, (int)ws.y, (int)ws.width + 1, (int)ws.height + 1);

    if (a.method == 0) {
        const GridResult& g = a.grid;
        for (int j = 0; j < g.rows; j++)
            for (int i = 0; i < g.cols; i++) {
                CellState st = g.cells[g.index(i, j)];
                if (st != CellState::Free) DrawRectangleRec(v.rect(g.cellRect(i, j, s)), st == CellState::Occupied ? C_OCC : C_MIX);
            }
        for (int id : g.channel) DrawRectangleRec(v.rect(g.cellRect(id % g.cols, id / g.cols, s)), C_CHAN);
        if (a.showLines && g.cw * v.sc >= 4 && g.ch * v.sc >= 4) {
            for (int i = 1; i < g.cols; i++) DrawLineV(v.w2s({i * g.cw, 0}), v.w2s({i * g.cw, s.H}), C_GRID);
            for (int j = 1; j < g.rows; j++) DrawLineV(v.w2s({0, j * g.ch}), v.w2s({s.W, j * g.ch}), C_GRID);
        }
    } else {
        const QuadPlanner& q = a.quad;
        for (int id : q.leaves) {
            CellState st = q.nodes[id].st;
            if (st != CellState::Free) DrawRectangleRec(v.rect(q.nodes[id].r), st == CellState::Occupied ? C_OCC : C_MIX);
        }
        for (int id : q.channelLM) DrawRectangleRec(v.rect(q.nodes[id].r), C_CHAN_LM);
        for (int id : q.channel) DrawRectangleRec(v.rect(q.nodes[id].r), C_CHAN);
        if (a.showLines)
            for (int id : q.leaves) DrawRectangleLinesEx(v.rect(q.nodes[id].r), 1, C_GRID);
        for (int id : q.toSplit) DrawRectangleLinesEx(v.rect(q.nodes[id].r), 2, C_SPLIT);
    }

    Vec2 mw = v.s2w(GetMousePosition());
    for (const Polygon& p : s.obstacles)
        drawPolygon(v, p, a.tool == 2 && pointInPolygon(mw, p) ? C_GOAL : C_OBS);

    const auto& path = a.method == 0 ? a.grid.path : a.quad.path;
    for (size_t i = 1; i < path.size(); i++) DrawLineEx(v.w2s(path[i - 1]), v.w2s(path[i]), 2.6f, WHITE);
    for (size_t i = 1; i + 1 < path.size(); i++) DrawCircleV(v.w2s(path[i]), 3, WHITE);
    EndScissorMode();

    drawMarker(v, s.start, C_START, "Start");
    drawMarker(v, s.goal, C_GOAL, "Țintă");

    if (!a.drawing.empty()) {  // poligon in curs de desenare
        for (size_t i = 1; i < a.drawing.size(); i++) DrawLineEx(v.w2s(a.drawing[i - 1]), v.w2s(a.drawing[i]), 2, T.accentHover);
        DrawLineEx(v.w2s(a.drawing.back()), GetMousePosition(), 1.5f, T.accent);
        for (Vec2 p : a.drawing) DrawCircleV(v.w2s(p), 4, T.accentHover);
        DrawCircleV(v.w2s(a.drawing[0]), 6, T.accentHover);  // primul varf, mai mare
        if (a.drawing.size() >= 3 && CheckCollisionPointCircle(GetMousePosition(), v.w2s(a.drawing[0]), 12))
            DrawCircleLinesV(v.w2s(a.drawing[0]), 11, WHITE);
    }
}

// Informatii despre celula de sub mouse
static void drawHoverInfo(const App& a, const View& v, Rectangle canvas) {
    Vector2 m = GetMousePosition();
    Vec2 w = v.s2w(m);
    if (!CheckCollisionPointRec(m, canvas) || a.drag || a.tool != 0 || w.x < 0 || w.y < 0 || w.x > a.scene.W ||
        w.y > a.scene.H)
        return;
    char buf[128];
    std::vector<std::string> lines;
    RectD r;
    if (a.method == 0) {
        int id = a.grid.cellAt(w);
        if (id < 0) return;
        r = a.grid.cellRect(id % a.grid.cols, id / a.grid.cols, a.scene);
        snprintf(buf, sizeof buf, "Celula (%d, %d)", id % a.grid.cols, id / a.grid.cols);
        lines = {buf, std::string("Stare: ") + cellStateName(a.grid.cells[id])};
    } else {
        int id = a.quad.locate(w);
        if (id < 0) return;
        const QNode& n = a.quad.nodes[id];
        r = n.r;
        snprintf(buf, sizeof buf, "Adâncime: %d", n.depth);
        lines = {"Nodul " + n.label, std::string("Stare: ") + cellStateName(n.st), buf};
    }
    snprintf(buf, sizeof buf, "Dimensiuni: %.1f × %.1f", r.w(), r.h());
    lines.push_back(buf);
    DrawRectangleLinesEx(v.rect(r), 2, WHITE);
    ui::Tooltip(m, lines);
}

// ------------------------------------------------------------------ mouse pe canvas
static Vec2 clampW(const Scene& s, Vec2 p) { return {std::clamp(p.x, 0.0, s.W), std::clamp(p.y, 0.0, s.H)}; }

static int obstacleAt(const Scene& s, Vec2 w) {
    for (int i = (int)s.obstacles.size() - 1; i >= 0; i--)
        if (pointInPolygon(w, s.obstacles[i])) return i;
    return -1;
}

static void closeDrawing(App& a) {
    if (a.drawing.size() >= 3 && isSimplePolygon(a.drawing)) {
        a.scene.obstacles.push_back(a.drawing);
        a.dirty = true;
    } else {
        a.toast = "Poligon invalid (minim 3 vârfuri, fără laturi care se intersectează).";
        a.toastT = 2.5f;
    }
    a.drawing.clear();
}

static void handleCanvas(App& a, const View& v, Rectangle canvas) {
    Vector2 m = GetMousePosition();
    bool inside = CheckCollisionPointRec(m, canvas) && !ui::Captured();
    bool click = inside && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    Vec2 w = clampW(a.scene, v.s2w(m));

    if (a.tool == 0) {  // muta
        if (click) {
            if (CheckCollisionPointCircle(m, v.w2s(a.scene.start), 14)) a.drag = 1;
            else if (CheckCollisionPointCircle(m, v.w2s(a.scene.goal), 14)) a.drag = 2;
            else if ((a.dragObs = obstacleAt(a.scene, w)) >= 0) a.drag = 3;
            a.dragLast = w;
        }
        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) a.drag = 0;
        if (a.drag == 1) a.scene.start = w;
        if (a.drag == 2) a.scene.goal = w;
        if (a.drag == 3)
            for (auto& p : a.scene.obstacles[a.dragObs]) p = p + (w - a.dragLast);
        if (a.drag) a.dirty = true;
        a.dragLast = w;
    } else if (a.tool == 1) {  // deseneaza
        bool onFirst = a.drawing.size() >= 3 && CheckCollisionPointCircle(m, v.w2s(a.drawing[0]), 12);
        if (click && onFirst) closeDrawing(a);  // click pe primul varf = inchide poligonul
        else if (click) a.drawing.push_back(w);
        if (IsKeyPressed(KEY_BACKSPACE) && !a.drawing.empty()) a.drawing.pop_back();
        if ((inside && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) || IsKeyPressed(KEY_ENTER)) closeDrawing(a);
        if (IsKeyPressed(KEY_ESCAPE)) a.drawing.clear();
    } else if (click) {  // sterge
        int i = obstacleAt(a.scene, w);
        if (i >= 0) a.scene.obstacles.erase(a.scene.obstacles.begin() + i), a.dirty = true;
    }
}

// ------------------------------------------------------------------ panou stanga
static void leftPanel(App& a, float H) {
    const float x = 20, w = 280;
    float y = 22;
    ui::Text("Descompunere celulară", x, y, 22, T.text, true);
    ui::Text("aproximativă  ·  Cap. 7.8.2", x, y + 28, 14, T.textDim);
    y += 62;

    ui::SectionTitle("METODA", x, y);
    int m = a.method;
    if (ui::Segmented({x, y + 20, w, 36}, {"Omogenă (grilă)", "Neomogenă (arbore)"}, m)) setMethod(a, m);
    y += 66;
    y += ui::TextWrapped(a.method == 0 ? "Spațiul de lucru e împărțit în celule identice. Celulele Libere formează "
                                         "canalul; celulele Ocupate și Mixte sunt interzise."
                                       : "Celulele Mixte se descompun recursiv în 4 celule egale (arbore 2^m, m = 2), "
                                         "până la găsirea unui canal de celule Libere.",
                         x, y, w, 13, T.textDim) + 14;

    ui::SectionTitle("PARAMETRI", x, y);
    y += 22;
    if (a.method == 0) {
        if (ui::Slider({x, y, w, 40}, a.square ? "Latura celulei" : "Lățimea celulei", a.cellW, 5, 120)) a.dirty = true;
        if (ui::Toggle({x, y + 48, w, 26}, "Celule pătrate", a.square)) a.dirty = true;
        if (ui::Slider({x, y + 82, w, 40}, "Înălțimea celulei", a.square ? a.cellW : a.cellH, 5, 120, !a.square))
            a.dirty = true;
        y += 130;
    } else {
        if (ui::Slider({x, y, w, 40}, "Înălțimea arborelui", a.maxDepth, 1, 9)) a.dirty = true;
        ui::Text("Execuție pas cu pas", x, y + 48, 13, T.textDim);
        float bw = (w - 18) / 4, by = y + 68;
        if (ui::Button({x, by, bw, 32}, "Reset")) a.manual = true, a.playing = false, a.dirty = true;
        if (ui::Button({x + bw + 6, by, bw, 32}, "Pas", true)) quadStep(a);
        if (ui::Button({x + 2 * (bw + 6), by, bw, 32}, a.playing ? "Pauză" : "Play")) {
            if (!a.manual || a.quad.done) a.manual = true, recompute(a);
            a.playing = !a.playing;
        }
        if (ui::Button({x + 3 * (bw + 6), by, bw, 32}, "Final")) setMethod(a, 1);
        y += 120;
    }

    ui::SectionTitle("AFIȘARE", x, y);
    ui::Toggle({x, y + 22, w, 26}, a.method == 0 ? "Linii grilă" : "Contur celule", a.showLines);
    y += 66;

    ui::SectionTitle("SCENĂ ȘI OBSTACOLE", x, y);
    if (ui::Segmented({x, y + 22, w, 34}, {"Mută", "Desenează", "Șterge"}, a.tool)) a.drawing.clear();
    y += 64;
    const char* hint[] = {"Trage Start / Țintă sau un obstacol.",
                          "Click = vârf nou · click pe primul vârf, click dreapta sau Enter = închide · Backspace = șterge ultimul vârf · Esc = anulează",
                          "Click pe un obstacol pentru a-l șterge."};
    y += ui::TextWrapped(hint[a.tool], x, y, w, 12, T.textDim) + 10;
    float pw = (w - 12) / 3;
    for (size_t i = 0; i <= a.presets.size(); i++) {
        Rectangle r = {x + (i % 3) * (pw + 6), y + (i / 3) * 38.f, pw, 32};
        if (i == a.presets.size()) {
            if (ui::Button(r, "Golește")) a.scene.obstacles.clear(), a.dirty = true;
        } else if (ui::Button(r, a.presets[i].name)) {
            a.scene = a.presets[i].scene;
            a.dirty = true;
        }
    }
    ui::TextWrapped("Taste: Spațiu = pas · 1/2 = metodă", x, H - 36, w, 12, T.textDim);
}

// ------------------------------------------------------------------ panou dreapta
static void statRow(float x, float& y, float w, Color sw, const std::string& k, const std::string& v) {
    if (sw.a) DrawRectangle((int)x, (int)y + 3, 11, 11, sw);
    ui::Text(k, x + (sw.a ? 19 : 0), y, 14, T.textDim);
    ui::Text(v, x + w - ui::Measure(v, 14, true).x, y, 14, T.text, true);
    y += 22;
}

static void rightPanel(const App& a, float X, float H) {
    const float x = X + 18, w = 264;
    float y = 22;
    char buf[64];
    const bool omog = a.method == 0;

    ui::SectionTitle("REZULTAT", x, y);
    y += 22;
    bool found = omog ? a.grid.found : a.quad.success, running = !omog && !a.quad.done;
    auto lines = ui::Wrap(omog ? a.grid.message : a.quad.message, w - 24, 13);
    float ch = 46 + lines.size() * 17.f;
    ui::Card({x, y, w, ch});
    Color badge = running ? T.warning : (found ? T.success : T.danger);
    ui::Text(running ? "ÎN CURS" : (found ? "DRUM GĂSIT" : "EȘEC"), x + 12, y + 14, 12, badge, true);
    for (size_t i = 0; i < lines.size(); i++) ui::Text(lines[i], x + 12, y + 40 + i * 17.f, 13, T.text);
    y += ch + 14;

    statRow(x, y, w, {148, 163, 184, 255}, "Celule libere", std::to_string(omog ? a.grid.nFree : a.quad.nFree));
    statRow(x, y, w, {239, 68, 68, 255}, "Celule ocupate", std::to_string(omog ? a.grid.nOcc : a.quad.nOcc));
    statRow(x, y, w, {245, 158, 11, 255}, "Celule mixte", std::to_string(omog ? a.grid.nMixed : a.quad.nMixed));
    statRow(x, y, w, {45, 212, 191, 255}, "Celule în canal", std::to_string(omog ? a.grid.channel.size() : a.quad.channel.size()));
    snprintf(buf, sizeof buf, "%.1f", omog ? a.grid.pathLen : a.quad.pathLen);
    statRow(x, y, w, {}, "Lungime drum", found ? buf : "–");
    if (omog) {
        snprintf(buf, sizeof buf, "%d × %d = %d", a.grid.cols, a.grid.rows, a.grid.cols * a.grid.rows);
        statRow(x, y, w, {}, "Grilă (col. × linii)", buf);
    } else {
        statRow(x, y, w, {}, "Noduri în arbore", std::to_string(a.quad.nodes.size()));
        snprintf(buf, sizeof buf, "%d / %d", a.quad.maxDepthReached, (int)a.maxDepth);
        statRow(x, y, w, {}, "Adâncime atinsă", buf);
        statRow(x, y, w, {}, "Iterații", std::to_string(a.quad.iteration));
    }
    snprintf(buf, sizeof buf, "%.2f ms", omog ? a.grid.ms : a.quad.ms);
    statRow(x, y, w, {}, "Timp de calcul", buf);
    y += 14;

    const float legendY = H - 100;
    if (!omog) {  // jurnalul pasilor algoritmului (ultimele randuri care incap)
        ui::SectionTitle("JURNAL ALGORITM", x, y);
        float top = y + 22, bottom = legendY - 16;
        ui::Card({x, top, w, bottom - top});
        std::vector<std::pair<std::string, bool>> all;
        for (auto& l : a.quad.log) {
            bool hdr = l.rfind("—", 0) == 0;  // antetul unei iteratii
            for (auto& wl : ui::Wrap(l, w - 22, 12, hdr)) all.push_back({wl, hdr});
        }
        int fit = (int)((bottom - top - 16) / 16), s0 = std::max(0, (int)all.size() - fit);
        for (int i = s0; i < (int)all.size(); i++)
            ui::Text(all[i].first, x + 11, top + 8 + (i - s0) * 16, 12, all[i].second ? T.accentHover : T.textDim, all[i].second);
    }

    ui::SectionTitle("LEGENDĂ", x, legendY);
    const std::pair<Color, const char*> items[] = {{C_WORK, "Liberă"}, {{239, 68, 68, 255}, "Ocupată"},
                                                   {{245, 158, 11, 255}, "Mixtă"}, {C_CHAN, "Canal"}, {C_OBS, "Obstacol"}};
    for (int i = 0; i < 5; i++) {
        float ix = x + (i % 2) * (w / 2), iy = legendY + 22 + (i / 2) * 22.f;
        DrawRectangleRec({ix, iy + 2, 14, 14}, items[i].first);
        DrawRectangleLinesEx({ix, iy + 2, 14, 14}, 1, T.border);
        ui::Text(items[i].second, ix + 22, iy + 1, 13, T.textDim);
    }
}

// ------------------------------------------------------------------ main
int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(1400, 820, "Descompunere celulară aproximativă – omogenă și neomogenă");
    SetWindowMinSize(1100, 700);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    ui::LoadFonts();

    App a;
    a.scene = a.presets[0].scene;

    while (!WindowShouldClose()) {
        const float SW = (float)GetScreenWidth(), SH = (float)GetScreenHeight(), LP = 320, RP = 300;
        Rectangle canvas = {LP + 18, 18, SW - LP - RP - 36, SH - 36};
        View v;
        v.sc = (float)std::min((canvas.width - 24) / a.scene.W, (canvas.height - 24) / a.scene.H);
        v.ox = canvas.x + (canvas.width - (float)(a.scene.W * v.sc)) / 2;
        v.oy = canvas.y + (canvas.height - (float)(a.scene.H * v.sc)) / 2;

        if (a.drawing.empty() && IsKeyPressed(KEY_ONE)) setMethod(a, 0);
        if (a.drawing.empty() && IsKeyPressed(KEY_TWO)) setMethod(a, 1);
        if (a.method == 1 && IsKeyPressed(KEY_SPACE)) quadStep(a);
        if (a.playing && (a.playTimer += GetFrameTime()) > 0.7f) {  // rulare automata
            a.playTimer = 0;
            a.quad.step();
            a.playing = !a.quad.done;
        }

        ui::BeginFrame();
        handleCanvas(a, v, canvas);

        BeginDrawing();
        ClearBackground(T.bg);
        drawCanvas(a, v);
        drawHoverInfo(a, v, canvas);
        if (a.toastT > 0) {
            a.toastT -= GetFrameTime();
            Vector2 m = ui::Measure(a.toast, 14, true);
            DrawRectangleRounded({canvas.x + (canvas.width - m.x) / 2 - 16, canvas.y, m.x + 32, 36}, 1.0f, 8, C_GOAL);
            ui::Text(a.toast, canvas.x + (canvas.width - m.x) / 2, canvas.y + (36 - m.y) / 2, 14, WHITE, true);
        }
        DrawRectangleRec({0, 0, LP, SH}, T.panel);
        DrawRectangleRec({SW - RP, 0, RP, SH}, T.panel);
        leftPanel(a, SH);
        rightPanel(a, SW - RP, SH);
        EndDrawing();

        if (a.dirty) recompute(a);
    }
    ui::UnloadFonts();
    CloseWindow();
    return 0;
}
