// geometry.cpp
#include "geometry.h"

#include <algorithm>

static constexpr double kPi = 3.14159265358979323846;

Polygon makeRect(double x0, double y0, double x1, double y1) {
    return {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}};
}

Polygon makeRotatedSquare(Vec2 c, double side, double angleDeg) {
    double a = angleDeg * kPi / 180.0, h = side / 2;
    Polygon p;
    const double xs[4] = {-h, h, h, -h}, ys[4] = {-h, -h, h, h};
    for (int i = 0; i < 4; i++)
        p.push_back({c.x + xs[i] * std::cos(a) - ys[i] * std::sin(a),
                     c.y + xs[i] * std::sin(a) + ys[i] * std::cos(a)});
    return p;
}

bool pointInPolygon(Vec2 p, const Polygon& poly) {
    bool in = false;
    size_t n = poly.size();
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        const Vec2& a = poly[i];
        const Vec2& b = poly[j];
        if ((a.y > p.y) != (b.y > p.y)) {
            double x = (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x;
            if (p.x < x) in = !in;
        }
    }
    return in;
}

static int orient(Vec2 a, Vec2 b, Vec2 c) {
    double v = cross(b - a, c - a);
    if (v > 1e-12) return 1;
    if (v < -1e-12) return -1;
    return 0;
}
static bool onSeg(Vec2 a, Vec2 b, Vec2 p) {
    return std::min(a.x, b.x) - 1e-12 <= p.x && p.x <= std::max(a.x, b.x) + 1e-12 &&
           std::min(a.y, b.y) - 1e-12 <= p.y && p.y <= std::max(a.y, b.y) + 1e-12;
}

bool segmentsIntersect(Vec2 a, Vec2 b, Vec2 c, Vec2 d) {
    int o1 = orient(a, b, c), o2 = orient(a, b, d), o3 = orient(c, d, a), o4 = orient(c, d, b);
    if (o1 != o2 && o3 != o4) return true;
    if (o1 == 0 && onSeg(a, b, c)) return true;
    if (o2 == 0 && onSeg(a, b, d)) return true;
    if (o3 == 0 && onSeg(c, d, a)) return true;
    if (o4 == 0 && onSeg(c, d, b)) return true;
    return false;
}

// Liang-Barsky: decupam segmentul la dreptunghi si verificam daca portiunea
// ramasa trece prin interior (nu doar de-a lungul unei laturi).
bool segmentCrossesRectInterior(Vec2 a, Vec2 b, const RectD& r) {
    double t0 = 0, t1 = 1;
    double dx = b.x - a.x, dy = b.y - a.y;
    const double p[4] = {-dx, dx, -dy, dy};
    const double q[4] = {a.x - r.x0, r.x1 - a.x, a.y - r.y0, r.y1 - a.y};
    for (int i = 0; i < 4; i++) {
        if (std::fabs(p[i]) < 1e-15) {
            if (q[i] < 0) return false;
        } else {
            double t = q[i] / p[i];
            if (p[i] < 0) t0 = std::max(t0, t);
            else t1 = std::min(t1, t);
            if (t0 > t1) return false;
        }
    }
    Vec2 m = a + (b - a) * ((t0 + t1) * 0.5);
    const double e = 1e-9 * std::max(1.0, std::max(r.w(), r.h()));
    return m.x > r.x0 + e && m.x < r.x1 - e && m.y > r.y0 + e && m.y < r.y1 - e;
}

bool rectInsidePolygon(const RectD& r, const Polygon& poly) {
    const Vec2 c[4] = {{r.x0, r.y0}, {r.x1, r.y0}, {r.x1, r.y1}, {r.x0, r.y1}};
    for (auto& v : c)
        if (!pointInPolygon(v, poly)) return false;
    for (size_t i = 0, n = poly.size(); i < n; i++)
        if (segmentCrossesRectInterior(poly[i], poly[(i + 1) % n], r)) return false;
    return true;
}

// Celula si obstacolul se ating/suprapun?
bool rectTouchesPolygon(const RectD& r, const Polygon& poly) {
    for (auto& v : poly)
        if (r.contains(v)) return true;                    // un varf al obstacolului e in celula
    if (pointInPolygon(r.center(), poly)) return true;     // celula e in interiorul obstacolului
    const Vec2 c[4] = {{r.x0, r.y0}, {r.x1, r.y0}, {r.x1, r.y1}, {r.x0, r.y1}};
    for (size_t i = 0, n = poly.size(); i < n; i++)        // laturile se intersecteaza
        for (int k = 0; k < 4; k++)
            if (segmentsIntersect(poly[i], poly[(i + 1) % n], c[k], c[(k + 1) % 4])) return true;
    return false;
}

RectD boundingBox(const Polygon& poly) {
    RectD r{1e300, 1e300, -1e300, -1e300};
    for (auto& v : poly) {
        r.x0 = std::min(r.x0, v.x);
        r.y0 = std::min(r.y0, v.y);
        r.x1 = std::max(r.x1, v.x);
        r.y1 = std::max(r.y1, v.y);
    }
    return r;
}

double signedArea(const Polygon& poly) {
    double s = 0;
    for (size_t i = 0, n = poly.size(); i < n; i++) s += cross(poly[i], poly[(i + 1) % n]);
    return s * 0.5;
}

bool isSimplePolygon(const Polygon& poly) {
    size_t n = poly.size();
    if (n < 3 || std::fabs(signedArea(poly)) < 1e-6) return false;
    for (size_t i = 0; i < n; i++) {
        Vec2 a = poly[i], b = poly[(i + 1) % n];
        for (size_t j = i + 1; j < n; j++) {
            if (j == i || (j + 1) % n == i || (i + 1) % n == j) continue;  // laturi vecine
            if (segmentsIntersect(a, b, poly[j], poly[(j + 1) % n])) return false;
        }
    }
    return true;
}

// ---- triangulare prin "ear clipping" (pentru desenarea poligoanelor neconvexe)
static bool insideTri(Vec2 p, Vec2 a, Vec2 b, Vec2 c) {
    return cross(b - a, p - a) >= 0 && cross(c - b, p - b) >= 0 && cross(a - c, p - c) >= 0;
}

std::vector<int> triangulate(const Polygon& P) {
    std::vector<int> out;
    int n = (int)P.size();
    if (n < 3) return out;
    std::vector<int> V(n);
    if (signedArea(P) > 0)
        for (int i = 0; i < n; i++) V[i] = i;
    else
        for (int i = 0; i < n; i++) V[i] = n - 1 - i;

    int nv = n, guard = 2 * nv;
    for (int v = nv - 1; nv > 2;) {
        if (guard-- <= 0) break;  // poligon degenerat
        int u = v;
        if (nv <= u) u = 0;
        v = u + 1;
        if (nv <= v) v = 0;
        int w = v + 1;
        if (nv <= w) w = 0;
        Vec2 A = P[V[u]], B = P[V[v]], C = P[V[w]];
        bool ear = cross(B - A, C - A) > 1e-10;
        for (int k = 0; ear && k < nv; k++) {
            if (k == u || k == v || k == w) continue;
            if (insideTri(P[V[k]], A, B, C)) ear = false;
        }
        if (ear) {
            out.push_back(V[u]);
            out.push_back(V[v]);
            out.push_back(V[w]);
            V.erase(V.begin() + v);
            nv--;
            guard = 2 * nv;
        }
    }
    return out;
}

Vec2 sharedBoundaryMid(const RectD& a, const RectD& b) {
    double ox0 = std::max(a.x0, b.x0), ox1 = std::min(a.x1, b.x1);
    double oy0 = std::max(a.y0, b.y0), oy1 = std::min(a.y1, b.y1);
    return {(ox0 + ox1) * 0.5, (oy0 + oy1) * 0.5};
}
